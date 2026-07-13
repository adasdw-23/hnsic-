#if defined _hnsic_afk_included
    #endinput
#endif
#define _hnsic_afk_included

/* ============================================================
 *  HNSIC v3.10.0 - afk.inc
 *  AFK detection system
 * ============================================================ */

// ─── AFK Check Task ─────────────────────────────────────────
stock Afk_StartCheck()
{
    remove_task(TASK_AFK_CHECK);
    set_task(g_fAfkCheckInterval, "Afk_CheckLoop", TASK_AFK_CHECK, _, _, "b");
}

stock Afk_StopCheck()
{
    remove_task(TASK_AFK_CHECK);
}

// ─── AFK Check Loop ─────────────────────────────────────────
public Afk_CheckLoop()
{
    // Only active in Mix mode during match
    if (g_iCurrentMode != MODE_MIX)
        return;

    if (g_iMatchState < STATE_FIRST_HALF || g_iMatchState > STATE_OVERTIME)
        return;

    if (g_bMatchPaused)
        return;

    new Float:afkTime = GetCvarFloat(CVAR_MIX_AFK_TIME);
    new Float:currentTime = get_gametime();

    for (new i = 1; i <= g_iMaxPlayers; i++)
    {
        if (!Hnsic_IsPlayerAlive(i))
            continue;

        if (!Hnsic_IsPlayerOnTeam(i))
            continue;

        new Float:elapsed = currentTime - g_eUserData[i][UD_LAST_MOVE];

        if (elapsed >= afkTime)
        {
            if (!g_eUserData[i][UD_IS_AFK])
            {
                g_eUserData[i][UD_IS_AFK] = 1;
                Hnsic_PrintChat(0, "%s 被标记为AFK", g_eUserData[i][UD_NAME]);

                // In Mix mode, move to spec after warning
                new szMsg[128];
                formatex(szMsg, charsmax(szMsg), "%s 已AFK超过%d秒, 将被移至旁观",
                    g_eUserData[i][UD_NAME], floatround(afkTime));
                Hnsic_PrintChat(0, szMsg);

                // Move to spec
                Hnsic_SwitchTeam(i, TEAM_SPEC);
            }
        }
    }
}

// ─── Update Movement (called from button press) ────────────
stock Afk_OnButtonPress(id)
{
    if (!Hnsic_IsPlayerValid(id))
        return;

    g_eUserData[id][UD_LAST_MOVE] = get_gametime();

    if (g_eUserData[id][UD_IS_AFK])
    {
        g_eUserData[id][UD_IS_AFK] = 0;
        Hnsic_Log("玩家 %s 不再AFK", g_eUserData[id][UD_NAME]);
    }
}

// ─── AFK Check on Spawn ─────────────────────────────────────
stock Afk_OnSpawn(id)
{
    g_eUserData[id][UD_LAST_MOVE] = get_gametime();
    g_eUserData[id][UD_IS_AFK] = 0;
}

// ─── Check if player is AFK ──────────────────────────────────
stock bool:Afk_IsPlayerAfk(id)
{
    if (!Hnsic_IsPlayerValid(id))
        return true;

    return (g_eUserData[id][UD_IS_AFK] == 1);
}

// ─── Get AFK count per team ─────────────────────────────────
stock Afk_GetTeamAfkCount(team)
{
    new count = 0;
    for (new i = 1; i <= g_iMaxPlayers; i++)
    {
        if (Hnsic_IsPlayerValid(i) && get_member(i, m_iTeam) == team && g_eUserData[i][UD_IS_AFK])
            count++;
    }
    return count;
}
