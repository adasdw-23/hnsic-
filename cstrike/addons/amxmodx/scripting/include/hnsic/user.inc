#if defined _hnsic_user_included
    #endinput
#endif
#define _hnsic_user_included

/* ============================================================
 *  HNSIC v3.10.0 - user.inc
 *  User role management
 * ============================================================ */

// ─── Initialize User ────────────────────────────────────────
stock User_Init(id)
{
    if (!Hnsic_IsPlayerValid(id))
        return;

    // Reset user data
    new authid[MAX_AUTHID_LEN];
    new name[MAX_NAME_LEN];
    get_user_authid(id, authid, charsmax(authid));
    get_user_name(id, name, charsmax(name));

    copy(g_eUserData[id][UD_AUTHID], charsmax(g_eUserData[][UD_AUTHID]), authid);
    copy(g_eUserData[id][UD_NAME], charsmax(g_eUserData[][UD_NAME]), name);

    g_eUserData[id][UD_PTS] = 0;
    g_eUserData[id][UD_WINS] = 0;
    g_eUserData[id][UD_LOSSES] = 0;
    g_eUserData[id][UD_MATCHES] = 0;
    g_eUserData[id][UD_KILLS] = 0;
    g_eUserData[id][UD_DEATHS] = 0;
    g_eUserData[id][UD_IS_CAPTAIN] = 0;
    g_eUserData[id][UD_GROUP] = -1;
    g_eUserData[id][UD_IS_CONNECTED] = 1;
    g_eUserData[id][UD_LAST_MOVE] = get_gametime();
    g_eUserData[id][UD_IS_AFK] = 0;
    g_eUserData[id][UD_SURRENDER_VOTE] = 0;
    g_eUserData[id][UD_PICKED] = 0;
}

// ─── Cleanup User ───────────────────────────────────────────
stock User_Cleanup(id)
{
    if (!Hnsic_IsPlayerValid(id))
        return;

    g_eUserData[id][UD_IS_CONNECTED] = 0;
    g_eUserData[id][UD_IS_CAPTAIN] = 0;
    g_eUserData[id][UD_IS_AFK] = 0;
    g_eUserData[id][UD_SURRENDER_VOTE] = 0;
    g_eUserData[id][UD_PICKED] = 0;
    g_eUserData[id][UD_GROUP] = -1;
}

// ─── Load User Data from PDS ────────────────────────────────
stock User_LoadData(id)
{
    if (!Hnsic_IsPlayerValid(id))
        return;

    new authid[MAX_AUTHID_LEN];
    get_user_authid(id, authid, charsmax(authid));

    // Load from PDS
    pds_get(authid, g_szPDS_PTS, g_eUserData[id][UD_PTS]);
    pds_get(authid, g_szPDS_WINS, g_eUserData[id][UD_WINS]);
    pds_get(authid, g_szPDS_LOSSES, g_eUserData[id][UD_LOSSES]);
    pds_get(authid, g_szPDS_MATCHES, g_eUserData[id][UD_MATCHES]);
    pds_get(authid, g_szPDS_KILLS, g_eUserData[id][UD_KILLS]);
    pds_get(authid, g_szPDS_DEATHS, g_eUserData[id][UD_DEATHS]);

    Hnsic_Log("用户数据已加载: %s (PTS:%d W:%d L:%d)", authid,
        g_eUserData[id][UD_PTS], g_eUserData[id][UD_WINS], g_eUserData[id][UD_LOSSES]);
}

// ─── Save User Data to PDS ──────────────────────────────────
stock User_SaveData(id)
{
    if (!Hnsic_IsPlayerValid(id))
        return;

    new authid[MAX_AUTHID_LEN];
    get_user_authid(id, authid, charsmax(authid));

    pds_set(authid, g_szPDS_PTS, g_eUserData[id][UD_PTS]);
    pds_set(authid, g_szPDS_WINS, g_eUserData[id][UD_WINS]);
    pds_set(authid, g_szPDS_LOSSES, g_eUserData[id][UD_LOSSES]);
    pds_set(authid, g_szPDS_MATCHES, g_eUserData[id][UD_MATCHES]);
    pds_set(authid, g_szPDS_KILLS, g_eUserData[id][UD_KILLS]);
    pds_set(authid, g_szPDS_DEATHS, g_eUserData[id][UD_DEATHS]);

    Hnsic_Log("用户数据已保存: %s (PTS:%d)", authid, g_eUserData[id][UD_PTS]);
}

// ─── Update User Name ────────────────────────────────────────
stock User_UpdateName(id)
{
    if (!Hnsic_IsPlayerValid(id))
        return;

    get_user_name(id, g_eUserData[id][UD_NAME], charsmax(g_eUserData[][UD_NAME]));
}

// ─── Set Captain ────────────────────────────────────────────
stock User_SetCaptain(id, bool:set)
{
    if (!Hnsic_IsPlayerValid(id))
        return;

    g_eUserData[id][UD_IS_CAPTAIN] = set ? 1 : 0;
}

// ─── Set Group ───────────────────────────────────────────────
stock User_SetGroup(id, group)
{
    if (!Hnsic_IsPlayerValid(id))
        return;

    g_eUserData[id][UD_GROUP] = group;
}

// ─── Update Movement ────────────────────────────────────────
stock User_UpdateMovement(id)
{
    if (!Hnsic_IsPlayerValid(id))
        return;

    g_eUserData[id][UD_LAST_MOVE] = get_gametime();
    g_eUserData[id][UD_IS_AFK] = 0;
}

// ─── Add PTS ───────────────────────────────────────────────
stock User_AddPts(id, amount)
{
    if (!Hnsic_IsPlayerValid(id))
        return;

    g_eUserData[id][UD_PTS] += amount;
    if (g_eUserData[id][UD_PTS] < 0)
        g_eUserData[id][UD_PTS] = 0;
}

// ─── Add Kill/Death ──────────────────────────────────────────
stock User_AddKill(id)
{
    if (!Hnsic_IsPlayerValid(id))
        return;

    g_eUserData[id][UD_KILLS]++;
}

stock User_AddDeath(id)
{
    if (!Hnsic_IsPlayerValid(id))
        return;

    g_eUserData[id][UD_DEATHS]++;
}

// ─── Add Match Result ────────────────────────────────────────
stock User_AddMatchResult(id, bool:win)
{
    if (!Hnsic_IsPlayerValid(id))
        return;

    g_eUserData[id][UD_MATCHES]++;
    if (win)
    {
        g_eUserData[id][UD_WINS]++;
        User_AddPts(id, 25); // Win bonus
    }
    else
    {
        g_eUserData[id][UD_LOSSES]++;
        User_AddPts(id, 10); // Loss participation
    }
}

// ─── Get Captain from Team ───────────────────────────────────
stock User_GetCaptainByGroup(group)
{
    for (new i = 1; i <= g_iMaxPlayers; i++)
    {
        if (Hnsic_IsPlayerValid(i) && g_eUserData[i][UD_IS_CAPTAIN] && g_eUserData[i][UD_GROUP] == group)
            return i;
    }
    return 0;
}

// ─── Reset Match Data ───────────────────────────────────────
stock User_ResetMatchData()
{
    for (new i = 1; i <= g_iMaxPlayers; i++)
    {
        g_eUserData[i][UD_IS_CAPTAIN] = 0;
        g_eUserData[i][UD_GROUP] = -1;
        g_eUserData[i][UD_SURRENDER_VOTE] = 0;
        g_eUserData[i][UD_PICKED] = 0;
        g_eUserData[i][UD_IS_AFK] = 0;
    }
}
