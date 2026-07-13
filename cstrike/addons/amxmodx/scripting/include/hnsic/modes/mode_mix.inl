#if defined _hnsic_mode_mix_included
    #endinput
#endif
#define _hnsic_mode_mix_included

/* ============================================================
 *  HNSIC v1.0.0 - mode_mix.inl
 *  Mix/Match mode (point cap scoring, timer, pause, AFK, surrender)
 *
 *  Match Flow:
 *  1. Admin triggers AI grouping (/aigroup)
 *  2. AI auto-groups by rating
 *  3. Random captain from each group
 *  4. Two captains knife duel (1v1)
 *  5. Winner picks side (CT or TT)
 *  6. Pick map type: random Boost or random Skill
 *  7. Server changes map
 *  8. Wait for all players to reconnect
 *  9. Match starts (Point Cap scoring)
 *
 *  Scoring (Point Cap):
 *  - TT reaches capture points to score
 *  - CT kills TT to prevent scoring
 *  - Cumulative score determines winner
 *  - Also support Point Deduct mode
 * ============================================================ */

// ─── Mix Pause Counters ───────────────────────────────────────
new g_iPauseCount[3]; // Per team

// ─── Mix Init ────────────────────────────────────────────────
stock ModeMix_Init()
{
    g_iMatchState = STATE_IDLE;
    g_iCurrentHalf = 0;
    g_iRoundNum = 0;
    g_iMatchScore[TEAM_TT] = 0;
    g_iMatchScore[TEAM_CT] = 0;
    g_iTTPoints = 0;
    g_iCTPoints = 0;
    g_bMatchPaused = false;
    g_iPauseCount[TEAM_TT] = 0;
    g_iPauseCount[TEAM_CT] = 0;

    // Set default gameplay to HNS for Mix
    if (g_iCurrentGameplay != GAMEPLAY_HNS)
    {
        set_pcvar_num(g_pCvar[CVAR_GAMEPLAY], GAMEPLAY_HNS);
    }

    // Start AFK check
    Afk_StartCheck();

    Hnsic_PrintChat(0, "比赛模式已启用");
    Hnsic_PrintChat(0, "管理员使用 /startmatch 或 /aigroup 开始比赛");
}

// ─── Mix Cleanup ─────────────────────────────────────────────
stock ModeMix_Cleanup()
{
    Afk_StopCheck();
    Surrender_Reset();

    remove_task(TASK_MATCH_TIMER);
    remove_task(TASK_SURRENDER);
    remove_task(TASK_FREEZE_TIME);

    g_bMatchPaused = false;
    g_iPauseCount[TEAM_TT] = 0;
    g_iPauseCount[TEAM_CT] = 0;
    User_ResetMatchData();
}

// ─── Mix On Spawn ────────────────────────────────────────────
stock ModeMix_OnSpawn(id)
{
    if (g_iMatchState < STATE_WARMUP)
        return;

    // Apply gameplay
    GameplayManager_OnSpawn(id);
    GameplayManager_ApplySpeed(id);

    // AFK reset on spawn
    Afk_OnSpawn(id);
}

// ─── Mix On Kill ─────────────────────────────────────────────
stock ModeMix_OnKill(victim, killer)
{
    if (g_iMatchState < STATE_FIRST_HALF || g_iMatchState > STATE_OVERTIME)
        return;

    if (!Hnsic_IsPlayerValid(killer))
        return;

    new killerTeam = get_member(killer, m_iTeam);
    new victimTeam = get_member(victim, m_iTeam);

    // Track kills
    User_AddKill(killer);
    User_AddDeath(victim);

    // Scoring
    if (g_iScoringMode == SCORING_POINT_CAP)
    {
        // Point Cap: CT kills TT to prevent TT scoring
        if (killerTeam == TEAM_CT && victimTeam == TEAM_TT)
        {
            // CT successfully killed a TT
            User_AddPts(killer, 3);
        }
        else if (killerTeam == TEAM_TT && victimTeam == TEAM_CT)
        {
            // TT killed a CT (minor bonus)
            User_AddPts(killer, 1);
        }
    }
    else if (g_iScoringMode == SCORING_POINT_DEDUCT)
    {
        // Point Deduct: TT kills deduct from CT score
        if (killerTeam == TEAM_TT && victimTeam == TEAM_CT)
        {
            g_iMatchScore[TEAM_CT] = max(0, g_iMatchScore[TEAM_CT] - 1);
            User_AddPts(killer, 2);
            Hnsic_PrintChat(0, "CT积分被扣除! 当前: TT %d - %d CT",
                g_iMatchScore[TEAM_TT], g_iMatchScore[TEAM_CT]);
        }
    }

    // Also trigger gameplay kill handler
    GameplayManager_OnKill(victim, killer);
}

// ─── Mix On Round Start ──────────────────────────────────────
stock ModeMix_OnRoundStart()
{
    if (g_iMatchState < STATE_FIRST_HALF || g_iMatchState > STATE_OVERTIME)
        return;

    if (g_bMatchPaused)
        return;

    // Reset round-specific scoring
    g_iTTPoints = 0;

    // Show score HUD
    for (new i = 1; i <= g_iMaxPlayers; i++)
    {
        if (Hnsic_IsPlayerValid(i))
            Hnsic_ShowScore(i);
    }

    Hnsic_Log("回合 %d 开始 (半场: %d)", g_iRoundNum, g_iCurrentHalf);
}

// ─── Mix On Round End ────────────────────────────────────────
stock ModeMix_OnRoundEnd(winner)
{
    if (g_iMatchState < STATE_FIRST_HALF || g_iMatchState > STATE_OVERTIME)
        return;

    if (g_bMatchPaused)
        return;

    // Process scoring
    Mix_ProcessRoundEnd(winner);

    // Save round stats periodically
    Save_RoundStats();

    // Check match end conditions
    Mix_CheckMatchEnd();

    // Show score
    for (new i = 1; i <= g_iMaxPlayers; i++)
    {
        if (Hnsic_IsPlayerValid(i))
            Hnsic_ShowScore(i);
    }
}

// ─── Process Round End Scoring ────────────────────────────────
stock Mix_ProcessRoundEnd(winner)
{
    if (g_iScoringMode == SCORING_POINT_CAP)
    {
        // In Point Cap mode:
        // - If TT wins the round (survived/escaped), they score points
        // - If CT wins (killed all TT), CT gets a point
        if (winner == TEAM_TT)
        {
            g_iMatchScore[TEAM_TT]++;
            Hnsic_PrintChat(0, "TT得分! TT %d - %d CT",
                g_iMatchScore[TEAM_TT], g_iMatchScore[TEAM_CT]);
        }
        else if (winner == TEAM_CT)
        {
            g_iMatchScore[TEAM_CT]++;
            Hnsic_PrintChat(0, "CT得分! TT %d - %d CT",
                g_iMatchScore[TEAM_TT], g_iMatchScore[TEAM_CT]);
        }
    }
    else if (g_iScoringMode == SCORING_POINT_DEDUCT)
    {
        // In Point Deduct mode:
        // - If TT wins, CT loses points
        // - If CT wins, CT gains a point
        if (winner == TEAM_TT)
        {
            g_iMatchScore[TEAM_CT] = max(0, g_iMatchScore[TEAM_CT] - 1);
            Hnsic_PrintChat(0, "CT积分扣除! TT %d - %d CT",
                g_iMatchScore[TEAM_TT], g_iMatchScore[TEAM_CT]);
        }
        else if (winner == TEAM_CT)
        {
            g_iMatchScore[TEAM_CT]++;
            Hnsic_PrintChat(0, "CT得分! TT %d - %d CT",
                g_iMatchScore[TEAM_TT], g_iMatchScore[TEAM_CT]);
        }
    }

    // Store half score
    g_iHalfScore[winner][g_iCurrentHalf]++;

    // Execute forward
    if (g_fwdPlayerScored)
        ExecuteForward(g_fwdPlayerScored, _, winner, g_iMatchScore[winner]);
}

// ─── Check Match End ─────────────────────────────────────────
stock Mix_CheckMatchEnd()
{
    // Check point cap
    if (g_iMatchScore[TEAM_TT] >= g_iPointCapTarget || g_iMatchScore[TEAM_CT] >= g_iPointCapTarget)
    {
        new winner = (g_iMatchScore[TEAM_TT] >= g_iPointCapTarget) ? TEAM_TT : TEAM_CT;
        Mix_EndMatch(winner);
        return;
    }

    // Check half end
    if (g_iRoundNum >= g_iMaxRounds)
    {
        if (g_iCurrentHalf == 0)
        {
            // Halftime
            Mix_StartHalftime();
        }
        else
        {
            // Match end - determine winner by total score
            new winner;
            if (g_iMatchScore[TEAM_TT] > g_iMatchScore[TEAM_CT])
                winner = TEAM_TT;
            else if (g_iMatchScore[TEAM_CT] > g_iMatchScore[TEAM_TT])
                winner = TEAM_CT;
            else
                winner = TEAM_NONE; // Tie

            Mix_EndMatch(winner);
        }
        return;
    }

    // Continue to next round
    g_iRoundNum++;
}

// ─── Start Halftime ──────────────────────────────────────────
stock Mix_StartHalftime()
{
    g_iMatchState = STATE_HALFTIME;
    g_iCurrentHalf = 1;
    g_iRoundNum = 0;

    Hnsic_PrintChat(0, "=== 中场休息 ===");
    Hnsic_PrintChat(0, "上半场比分: TT %d - %d CT", g_iMatchScore[TEAM_TT], g_iMatchScore[TEAM_CT]);

    // Swap teams
    for (new i = 1; i <= g_iMaxPlayers; i++)
    {
        if (!Hnsic_IsPlayerValid(i))
            continue;

        new team = get_member(i, m_iTeam);
        if (team == TEAM_TT)
            Hnsic_SwitchTeam(i, TEAM_CT);
        else if (team == TEAM_CT)
            Hnsic_SwitchTeam(i, TEAM_TT);
    }

    Hnsic_PrintChat(0, "阵营已互换! 30秒后开始下半场");

    set_task(30.0, "Mix_StartSecondHalf", TASK_FREEZE_TIME);
}

// ─── Start Second Half ───────────────────────────────────────
public Mix_StartSecondHalf()
{
    g_iMatchState = STATE_SECOND_HALF;
    g_iRoundNum = 1;

    Hnsic_PrintChat(0, "=== 下半场开始! ===");
    rg_round_restart();
}

// ─── End Match ───────────────────────────────────────────────
stock Mix_EndMatch(winner)
{
    g_iMatchState = STATE_MATCH_END;

    remove_task(TASK_MATCH_TIMER);
    remove_task(TASK_FREEZE_TIME);

    Hnsic_PrintChat(0, "============================");
    Hnsic_PrintChat(0, "      比赛结束!");
    Hnsic_PrintChat(0, "============================");

    new szMsg[128];
    formatex(szMsg, charsmax(szMsg), "最终比分: TT %d - %d CT",
        g_iMatchScore[TEAM_TT], g_iMatchScore[TEAM_CT]);
    Hnsic_PrintChat(0, szMsg);

    if (winner == TEAM_TT)
    {
        Hnsic_PrintChat(0, "TT 获胜!");
    }
    else if (winner == TEAM_CT)
    {
        Hnsic_PrintChat(0, "CT 获胜!");
    }
    else
    {
        Hnsic_PrintChat(0, "平局!");
    }

    // Save match results
    Save_MatchResults();

    // Execute forward
    if (g_fwdMatchEnd)
        ExecuteForward(g_fwdMatchEnd, _, winner, g_iMatchScore[TEAM_TT], g_iMatchScore[TEAM_CT]);

    Hnsic_Log("比赛结束: TT %d - %d CT, 获胜: %s",
        g_iMatchScore[TEAM_TT], g_iMatchScore[TEAM_CT],
        winner == TEAM_TT ? "TT" : (winner == TEAM_CT ? "CT" : "平局"));
}

// ─── Request Pause ───────────────────────────────────────────
stock Mix_RequestPause(id)
{
    new team = get_member(id, m_iTeam);
    if (team != TEAM_TT && team != TEAM_CT)
        return;

    new maxPauses = GetCvarInt(CVAR_MIX_PAUSE_LIMIT);
    if (g_iPauseCount[team] >= maxPauses)
    {
        Hnsic_PrintChat(id, "你的队伍已用完所有暂停次数 (%d/%d)",
            g_iPauseCount[team], maxPauses);
        return;
    }

    g_bMatchPaused = true;
    g_iPauseCount[team]++;
    g_iPauseRequester = id;

    // Freeze all players
    for (new i = 1; i <= g_iMaxPlayers; i++)
    {
        if (Hnsic_IsPlayerAlive(i))
        {
            set_entvar(i, var_flags, get_entvar(i, var_flags) | FL_FROZEN);
        }
    }

    new szTeam[16];
    copy(szTeam, charsmax(szTeam), (team == TEAM_TT) ? "TT" : "CT");
    Hnsic_PrintChat(0, "比赛已暂停! (%s请求, 剩余%d次)", szTeam, maxPauses - g_iPauseCount[team]);
    Hnsic_PrintChat(0, "输入 /unpause 继续比赛");
}

// ─── Resume from Pause ───────────────────────────────────────
stock Mix_ResumeFromPause()
{
    if (!g_bMatchPaused)
        return;

    g_bMatchPaused = false;

    // Unfreeze all players
    for (new i = 1; i <= g_iMaxPlayers; i++)
    {
        if (Hnsic_IsPlayerAlive(i))
        {
            set_entvar(i, var_flags, get_entvar(i, var_flags) & ~FL_FROZEN);
        }
    }

    Hnsic_PrintChat(0, "比赛继续!");
}

// ─── Overtime ────────────────────────────────────────────────
stock Mix_StartOvertime()
{
    g_iMatchState = STATE_OVERTIME;
    g_iCurrentHalf = 2;
    g_iRoundNum = 0;

    Hnsic_PrintChat(0, "=== 加时赛! ===");
    Hnsic_PrintChat(0, "比分: TT %d - %d CT", g_iMatchScore[TEAM_TT], g_iMatchScore[TEAM_CT]);

    // Swap teams for overtime
    for (new i = 1; i <= g_iMaxPlayers; i++)
    {
        if (!Hnsic_IsPlayerValid(i))
            continue;

        new team = get_member(i, m_iTeam);
        if (team == TEAM_TT)
            Hnsic_SwitchTeam(i, TEAM_CT);
        else if (team == TEAM_CT)
            Hnsic_SwitchTeam(i, TEAM_TT);
    }

    // Overtime: 3 rounds per half
    g_iMaxRounds = 3;
    rg_round_restart();
}

// ─── Mix Timer Display ────────────────────────────────────────
public Mix_TimerDisplay()
{
    if (g_iMatchState < STATE_FIRST_HALF || g_iMatchState > STATE_OVERTIME)
        return;

    g_fMatchTimer += 1.0;

    // Show timer to all players
    new minutes = floatround(g_fMatchTimer) / 60;
    new seconds = floatround(g_fMatchTimer) % 60;

    for (new i = 1; i <= g_iMaxPlayers; i++)
    {
        if (Hnsic_IsPlayerValid(i))
        {
            new szMsg[128];
            formatex(szMsg, charsmax(szMsg), "%02d:%02d | TT %d - %d CT",
                minutes, seconds, g_iMatchScore[TEAM_TT], g_iMatchScore[TEAM_CT]);
            Hnsic_PrintHud(i, -1.0, 0.05, szMsg);
        }
    }
}
