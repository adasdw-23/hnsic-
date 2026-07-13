#if defined _hnsic_surrender_included
    #endinput
#endif
#define _hnsic_surrender_included

/* ============================================================
 *  HNSIC v3.10.0 - surrender.inc
 *  Surrender vote system
 * ============================================================ */

// ─── Start Surrender Vote ───────────────────────────────────
stock Surrender_StartVote(id)
{
    if (g_bSurrenderActive)
    {
        Hnsic_PrintChat(id, "已有一个投降投票进行中");
        return;
    }

    new team = get_member(id, m_iTeam);
    if (team != TEAM_TT && team != TEAM_CT)
    {
        Hnsic_PrintChat(id, "旁观者无法发起投降");
        return;
    }

    g_bSurrenderActive = true;
    g_iSurrenderInitiator = id;
    g_fSurrenderTime = get_gametime();

    // Reset votes
    g_iSurrenderVotes[TEAM_TT] = 0;
    g_iSurrenderVotes[TEAM_CT] = 0;

    // Auto-vote for initiator
    g_eUserData[id][UD_SURRENDER_VOTE] = 1;
    g_iSurrenderVotes[team]++;

    Hnsic_PrintChat(0, "%s 发起了投降投票!", g_eUserData[id][UD_NAME]);
    Hnsic_PrintChat(0, "同队玩家输入 /surrender 同意投降 (30秒内)");

    // Set timeout
    remove_task(TASK_SURRENDER);
    set_task(30.0, "Surrender_Timeout", TASK_SURRENDER);
}

// ─── Cast Surrender Vote ────────────────────────────────────
stock Surrender_CastVote(id)
{
    if (!g_bSurrenderActive)
    {
        Hnsic_PrintChat(id, "当前没有进行中的投降投票");
        return;
    }

    if (g_eUserData[id][UD_SURRENDER_VOTE] != 0)
    {
        Hnsic_PrintChat(id, "你已经投过票了");
        return;
    }

    new team = get_member(id, m_iTeam);
    if (team != TEAM_TT && team != TEAM_CT)
    {
        Hnsic_PrintChat(id, "旁观者无法投票");
        return;
    }

    g_eUserData[id][UD_SURRENDER_VOTE] = 1;
    g_iSurrenderVotes[team]++;

    Hnsic_PrintChat(id, "你已同意投降");

    // Check if threshold reached
    Surrender_CheckResult();
}

// ─── Check Surrender Result ───────────────────────────────────
stock Surrender_CheckResult()
{
    new Float:ratio = GetCvarFloat(CVAR_MIX_SURRENDER_RATIO);

    for (new team = TEAM_TT; team <= TEAM_CT; team++)
    {
        new teamPlayers = Hnsic_GetTeamCount(team);
        if (teamPlayers == 0)
            continue;

        new Float:voteRatio = float(g_iSurrenderVotes[team]) / float(teamPlayers);

        if (voteRatio >= ratio)
        {
            // Surrender accepted
            g_bSurrenderActive = false;
            remove_task(TASK_SURRENDER);

            new szTeam[16];
            copy(szTeam, charsmax(szTeam), (team == TEAM_TT) ? "TT" : "CT");
            Hnsic_PrintChat(0, "%s 投降! %s 获胜!", szTeam, (team == TEAM_TT) ? "CT" : "TT");

            // End match with opposing team as winner
            new winner = (team == TEAM_TT) ? TEAM_CT : TEAM_TT;
            Mix_EndMatch(winner);
            return;
        }
    }
}

// ─── Surrender Timeout ──────────────────────────────────────
public Surrender_Timeout()
{
    if (!g_bSurrenderActive)
        return;

    g_bSurrenderActive = false;
    Hnsic_PrintChat(0, "投降投票超时, 投票失败");

    // Reset all votes
    for (new i = 1; i <= g_iMaxPlayers; i++)
    {
        g_eUserData[i][UD_SURRENDER_VOTE] = 0;
    }
}

// ─── Reset Surrender ─────────────────────────────────────────
stock Surrender_Reset()
{
    g_bSurrenderActive = false;
    g_iSurrenderInitiator = 0;
    g_iSurrenderVotes[TEAM_TT] = 0;
    g_iSurrenderVotes[TEAM_CT] = 0;
    remove_task(TASK_SURRENDER);

    for (new i = 1; i <= g_iMaxPlayers; i++)
    {
        g_eUserData[i][UD_SURRENDER_VOTE] = 0;
    }
}
