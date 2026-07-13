#if defined _hnsic_utils_included
    #endinput
#endif
#define _hnsic_utils_included

/* ============================================================
 *  HNSIC v3.10.0 - utils.inc
 *  Utility functions
 * ============================================================ */

// ─── Chat Message ───────────────────────────────────────────
stock Hnsic_PrintChat(id, const msg[], any:...)
{
    new buffer[512];
    new prefix[32];
    GetCvarString(CVAR_PREFIX, prefix, charsmax(prefix));

    formatex(buffer, charsmax(buffer), "%s ", prefix);
    vformat(buffer[strlen(buffer)], charsmax(buffer) - strlen(buffer), msg, 3);

    new team;
    if (id == 0)
    {
        // Send to all
        for (new i = 1; i <= g_iMaxPlayers; i++)
        {
            if (is_user_connected(i))
            {
                emessage_begin(MSG_ONE_UNRELIABLE, g_iMsgSayText, _, i);
                ewrite_byte(i);
                ewrite_string(buffer);
                emessage_end();
            }
        }
    }
    else
    {
        team = get_member(id, m_iTeam);
        emessage_begin(MSG_ONE_UNRELIABLE, g_iMsgSayText, _, id);
        ewrite_byte(id);
        ewrite_string(buffer);
        emessage_end();
    }
}

// ─── HUD Message ───────────────────────────────────────────
stock Hnsic_PrintHud(id, x, y, const msg[], any:...)
{
    new buffer[512];
    vformat(buffer, charsmax(buffer), msg, 5);

    set_dhudmessage(255, 255, 255, Float:x, Float:y, 0, 6.0, 5.0, 0.0, 0.5);
    show_dhudmessage(id, buffer);
}

// ─── Center Message ─────────────────────────────────────────
stock Hnsic_PrintCenter(id, const msg[], any:...)
{
    new buffer[512];
    vformat(buffer, charsmax(buffer), msg, 3);
    client_print_center(id, print_center, buffer);
}

// ─── Team Check ──────────────────────────────────────────────
stock Hnsic_IsPlayerValid(id)
{
    return (1 <= id <= g_iMaxPlayers && is_user_connected(id));
}

stock Hnsic_IsPlayerAlive(id)
{
    return (Hnsic_IsPlayerValid(id) && is_user_alive(id));
}

stock Hnsic_IsPlayerOnTeam(id)
{
    if (!Hnsic_IsPlayerValid(id))
        return 0;
    new team = get_member(id, m_iTeam);
    return (team == TEAM_TT || team == TEAM_CT);
}

stock Hnsic_GetPlayerCount()
{
    new count = 0;
    for (new i = 1; i <= g_iMaxPlayers; i++)
    {
        if (Hnsic_IsPlayerOnTeam(i))
            count++;
    }
    return count;
}

stock Hnsic_GetTeamCount(team)
{
    new count = 0;
    for (new i = 1; i <= g_iMaxPlayers; i++)
    {
        if (Hnsic_IsPlayerValid(i) && get_member(i, m_iTeam) == team)
            count++;
    }
    return count;
}

// ─── Admin Check ─────────────────────────────────────────────
stock Hnsic_IsAdmin(id)
{
    return (get_user_flags(id) & ADMIN_KICK);
}

stock Hnsic_IsRootAdmin(id)
{
    return (get_user_flags(id) & ADMIN_RCON);
}

// ─── Team Transfer ─────────────────────────────────────────
stock Hnsic_SwitchTeam(id, team)
{
    if (!Hnsic_IsPlayerValid(id))
        return;

    rg_set_user_team(id, team, MODEL_AUTO);
}

// ─── Strip Weapons ─────────────────────────────────────────
stock Hnsic_StripAllWeapons(id)
{
    if (!Hnsic_IsPlayerAlive(id))
        return;

    rg_remove_all_items(id);
    rg_give_item(id, "weapon_knife");
}

// ─── Give Weapon ────────────────────────────────────────────
stock Hnsic_GiveWeapon(id, const weapon[])
{
    if (!Hnsic_IsPlayerAlive(id))
        return;

    rg_give_item(id, weapon);
}

// ─── Set Speed ─────────────────────────────────────────────
stock Hnsic_SetSpeed(id, Float:speed)
{
    if (!Hnsic_IsPlayerAlive(id))
        return;

    set_member(id, m_flMaxSpeed, speed);
    set_entvar(id, var_maxspeed, speed);
}

// ─── Score Display ──────────────────────────────────────────
stock Hnsic_ShowScore(id)
{
    new tt_name[16], ct_name[16];
    copy(tt_name, charsmax(tt_name), "TT");
    copy(ct_name, charsmax(ct_name), "CT");

    if (g_iCurrentMode == MODE_MIX)
    {
        new szMsg[128];
        formatex(szMsg, charsmax(szMsg),
            "TT %d - %d CT | %s %d",
            g_iMatchScore[TEAM_TT], g_iMatchScore[TEAM_CT],
            (g_iCurrentHalf == 0) ? "上半场" : "下半场",
            g_iRoundNum);

        Hnsic_PrintHud(id, -1.0, 0.05, szMsg);
    }
}

// ─── Winrate Calculation ────────────────────────────────────
stock Float:Hnsic_GetWinrate(id)
{
    new matches = g_eUserData[id][UD_MATCHES];
    if (matches == 0)
        return 0.5; // Default 50% for new players

    return float(g_eUserData[id][UD_WINS]) / float(matches);
}

// ─── Rating Calculation ─────────────────────────────────────
stock Float:Hnsic_CalcRating(id)
{
    new pts_weight = GetCvarInt(CVAR_CAPTAIN_PTS_WEIGHT);
    new winrate_weight = 30;
    new matches_weight = 20;
    new random_weight = 5;
    // Normalize: total = 100
    new total_weight = pts_weight + winrate_weight + matches_weight + random_weight;

    new pts = g_eUserData[id][UD_PTS];
    new max_pts = 1000; // Max expected PTS
    Float:pts_score = (float(pts) / float(max_pts)) * float(pts_weight);

    Float:winrate = Hnsic_GetWinrate(id);
    Float:winrate_score = winrate * float(winrate_weight);

    new matches = g_eUserData[id][UD_MATCHES];
    new max_matches = 100; // Cap for normalization
    Float:matches_score = (float(min(matches, max_matches)) / float(max_matches)) * float(matches_weight);

    Float:random_score = random_float(0.0, 1.0) * float(random_weight);

    return (pts_score + winrate_score + matches_score + random_score) / float(total_weight);
}

// ─── Shuffle Array ──────────────────────────────────────────
stock Hnsic_ShuffleArray(array[], size)
{
    for (new i = size - 1; i > 0; i--)
    {
        new j = random(i + 1);
        new temp = array[i];
        array[i] = array[j];
        array[j] = temp;
    }
}

// ─── Sort by Rating (descending) ────────────────────────────
stock Hnsic_SortByRating(players[], ratings[], size)
{
    // Simple bubble sort for small arrays
    for (new i = 0; i < size - 1; i++)
    {
        for (new j = 0; j < size - i - 1; j++)
        {
            if (ratings[j] < ratings[j + 1])
            {
                // Swap players
                new temp_p = players[j];
                players[j] = players[j + 1];
                players[j + 1] = temp_p;

                // Swap ratings
                new Float:temp_r = ratings[j];
                ratings[j] = ratings[j + 1];
                ratings[j + 1] = temp_r;
            }
        }
    }
}

// ─── Log Message ─────────────────────────────────────────────
stock Hnsic_Log(const msg[], any:...)
{
    if (GetCvarInt(CVAR_DEBUG) == 0)
        return;

    new buffer[512];
    new timestamp[32];
    get_time("%Y-%m-%d %H:%M:%S", timestamp, charsmax(timestamp));

    formatex(buffer, charsmax(buffer), "[%s] [HNSIC] ", timestamp);
    vformat(buffer[strlen(buffer)], charsmax(buffer) - strlen(buffer), msg, 2);

    log_amx(buffer);
}

// ─── Get Mode Name ───────────────────────────────────────────
stock Hnsic_GetModeName(mode, name[], len)
{
    switch (mode)
    {
        case MODE_PUB:      copy(name, len, "公共模式");
        case MODE_TRAINING: copy(name, len, "训练模式");
        case MODE_KNIFE:    copy(name, len, "刀战模式");
        case MODE_DM:       copy(name, len, "混战模式");
        case MODE_ZOMBIE:   copy(name, len, "僵尸模式");
        case MODE_MIX:      copy(name, len, "比赛模式");
        default:            copy(name, len, "未知");
    }
}

// ─── Get Match State Name ───────────────────────────────────
stock Hnsic_GetStateName(state, name[], len)
{
    switch (state)
    {
        case STATE_IDLE:         copy(name, len, "空闲");
        case STATE_AIGROUP:      copy(name, len, "AI分组中");
        case STATE_CAPTAIN_PICK: copy(name, len, "队长选择");
        case STATE_KNIFE_DUEL:   copy(name, len, "队长刀战");
        case STATE_SIDE_PICK:    copy(name, len, "选择阵营");
        case STATE_MAP_PICK:     copy(name, len, "选择地图");
        case STATE_MAP_CHANGE:   copy(name, len, "换图中");
        case STATE_WARMUP:       copy(name, len, "热身中");
        case STATE_FIRST_HALF:   copy(name, len, "上半场");
        case STATE_HALFTIME:     copy(name, len, "中场休息");
        case STATE_SECOND_HALF:  copy(name, len, "下半场");
        case STATE_OVERTIME:     copy(name, len, "加时赛");
        case STATE_MATCH_END:    copy(name, len, "比赛结束");
        default:                 copy(name, len, "未知");
    }
}

// ─── Get Gameplay Name ──────────────────────────────────────
stock Hnsic_GetGameplayName(gp, name[], len)
{
    switch (gp)
    {
        case GAMEPLAY_HNS:      copy(name, len, "躲猫猫");
        case GAMEPLAY_KNIFE:    copy(name, len, "刀战");
        case GAMEPLAY_TRAINING: copy(name, len, "训练");
        default:                copy(name, len, "无");
    }
}
