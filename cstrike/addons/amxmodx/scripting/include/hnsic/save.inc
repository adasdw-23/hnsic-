#if defined _hnsic_save_included
    #endinput
#endif
#define _hnsic_save_included

/* ============================================================
 *  HNSIC v3.10.0 - save.inc
 *  PDS (PersistentDataStorage) save/load system
 * ============================================================ */

// ─── Save All Online Players ────────────────────────────────
stock Save_SaveAll()
{
    new count = 0;
    for (new i = 1; i <= g_iMaxPlayers; i++)
    {
        if (Hnsic_IsPlayerValid(i) && g_eUserData[i][UD_IS_CONNECTED])
        {
            User_SaveData(i);
            count++;
        }
    }
    Hnsic_Log("已保存 %d 名玩家数据", count);
}

// ─── Load All Online Players ────────────────────────────────
stock Save_LoadAll()
{
    new count = 0;
    for (new i = 1; i <= g_iMaxPlayers; i++)
    {
        if (Hnsic_IsPlayerValid(i) && g_eUserData[i][UD_IS_CONNECTED])
        {
            User_LoadData(i);
            count++;
        }
    }
    Hnsic_Log("已加载 %d 名玩家数据", count);
}

// ─── Save Match Results ──────────────────────────────────────
stock Save_MatchResults()
{
    // Determine winner
    new winner_team = TEAM_NONE;
    if (g_iMatchScore[TEAM_TT] > g_iMatchScore[TEAM_CT])
        winner_team = TEAM_TT;
    else if (g_iMatchScore[TEAM_CT] > g_iMatchScore[TEAM_TT])
        winner_team = TEAM_CT;

    // Update all players
    for (new i = 1; i <= g_iMaxPlayers; i++)
    {
        if (!Hnsic_IsPlayerValid(i) || !g_eUserData[i][UD_IS_CONNECTED])
            continue;

        new team = get_member(i, m_iTeam);
        if (team == TEAM_TT || team == TEAM_CT)
        {
            User_AddMatchResult(i, bool:(team == winner_team));
        }
    }

    Save_SaveAll();
    Hnsic_Log("比赛结果已保存: TT %d - %d CT, 获胜方: %s",
        g_iMatchScore[TEAM_TT], g_iMatchScore[TEAM_CT],
        winner_team == TEAM_TT ? "TT" : (winner_team == TEAM_CT ? "CT" : "平局"));
}

// ─── Save Round Stats ────────────────────────────────────────
stock Save_RoundStats()
{
    // Auto-save every 5 rounds in Mix mode
    if (g_iCurrentMode == MODE_MIX && g_iRoundNum % 5 == 0)
    {
        Save_SaveAll();
    }
}

// ─── Save on Map End ─────────────────────────────────────────
stock Save_OnMapEnd()
{
    Save_SaveAll();
}

// ─── Save on Shutdown ───────────────────────────────────────
stock Save_OnShutdown()
{
    Save_SaveAll();
    Hnsic_Log("服务器关闭, 所有数据已保存");
}
