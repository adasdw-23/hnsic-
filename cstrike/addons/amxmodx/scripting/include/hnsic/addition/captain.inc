#if defined _hnsic_captain_included
    #endinput
#endif
#define _hnsic_captain_included

/* ============================================================
 *  HNSIC v3.10.0 - captain.inc
 *  Captain pick system
 *  AI grouping by rating (PTS 50% + winrate 30% + matches 20% + 5% random)
 * ============================================================ */

// ─── AI Grouping ─────────────────────────────────────────────
stock Captain_StartAiGrouping()
{
    if (g_iMatchState != STATE_IDLE)
    {
        Hnsic_PrintChat(0, "比赛已在进行中, 无法重新分组");
        return;
    }

    new playerCount = Hnsic_GetPlayerCount();
    if (playerCount < 2)
    {
        Hnsic_PrintChat(0, "玩家数量不足, 至少需要2名玩家");
        return;
    }

    g_iMatchState = STATE_AIGROUP;
    Hnsic_PrintChat(0, "=== AI自动分组开始 ===");
    Hnsic_Log("AI分组开始, 玩家数: %d", playerCount);

    // Collect all players on teams
    new players[MAX_PLAYERS];
    new Float:ratings[MAX_PLAYERS];
    new count = 0;

    for (new i = 1; i <= g_iMaxPlayers; i++)
    {
        if (Hnsic_IsPlayerOnTeam(i))
        {
            players[count] = i;
            ratings[count] = Hnsic_CalcRating(i);
            count++;
        }
    }

    // Sort by rating descending
    Hnsic_SortByRating(players, Float:ratings, count);

    // Snake draft: alternate assignment to balance teams
    // Highest rated goes to group 0, next to group 1, etc.
    g_iGroupCount[0] = 0;
    g_iGroupCount[1] = 0;

    for (new i = 0; i < count; i++)
    {
        new group = i % 2; // Alternate: 0, 1, 0, 1, ...
        new pid = players[i];

        g_iGroupPlayers[group][g_iGroupCount[group]] = pid;
        g_fGroupRating[group][g_iGroupCount[group]] = ratings[i];
        g_iGroupCount[group]++;

        User_SetGroup(pid, group);
    }

    // Log groups
    Hnsic_Log("分组结果:");
    for (new g = 0; g < MAX_GROUPS; g++)
    {
        Hnsic_Log("  组%d (%d人):", g + 1, g_iGroupCount[g]);
        for (new i = 0; i < g_iGroupCount[g]; i++)
        {
            new pid = g_iGroupPlayers[g][i];
            Hnsic_Log("    %s (评分: %.2f)", g_eUserData[pid][UD_NAME], g_fGroupRating[g][i]);
        }
    }

    Hnsic_PrintChat(0, "AI分组完成! 组1: %d人, 组2: %d人",
        g_iGroupCount[0], g_iGroupCount[1]);

    // Select captains
    Captain_SelectCaptains();
}

// ─── Captain Selection ──────────────────────────────────────
stock Captain_SelectCaptains()
{
    g_iMatchState = STATE_CAPTAIN_PICK;

    // Find highest PTS player in each group (or random)
    for (new g = 0; g < MAX_GROUPS; g++)
    {
        new bestId = 0;
        new bestPts = -1;

        for (new i = 0; i < g_iGroupCount[g]; i++)
        {
            new pid = g_iGroupPlayers[g][i];
            if (g_eUserData[pid][UD_PTS] > bestPts)
            {
                bestPts = g_eUserData[pid][UD_PTS];
                bestId = pid;
            }
        }

        // If tie or no PTS data, random from group
        if (bestPts <= 0 && g_iGroupCount[g] > 0)
        {
            bestId = g_iGroupPlayers[g][random(g_iGroupCount[g])];
        }

        if (bestId > 0)
        {
            g_iCaptain[g] = bestId;
            User_SetCaptain(bestId, true);
            Hnsic_PrintChat(0, "组%d队长: %s (PTS: %d)",
                g + 1, g_eUserData[bestId][UD_NAME], g_eUserData[bestId][UD_PTS]);
        }
    }

    // Start knife duel
    Captain_StartKnifeDuel();
}

// ─── Captain Knife Duel ──────────────────────────────────────
stock Captain_StartKnifeDuel()
{
    if (g_iCaptain[0] == 0 || g_iCaptain[1] == 0)
    {
        Hnsic_PrintChat(0, "队长选择失败, 比赛取消");
        g_iMatchState = STATE_IDLE;
        return;
    }

    g_iMatchState = STATE_KNIFE_DUEL;
    g_bCaptainDuelActive = true;
    g_iCaptainDuelWinner = 0;

    new duel_time = GetCvarInt(CVAR_CAPTAIN_DUEL_TIME);

    // Move captains to TT and CT for duel
    Hnsic_SwitchTeam(g_iCaptain[0], TEAM_TT);
    Hnsic_SwitchTeam(g_iCaptain[1], TEAM_CT);

    // Move other players to spec
    for (new i = 1; i <= g_iMaxPlayers; i++)
    {
        if (Hnsic_IsPlayerOnTeam(i) && i != g_iCaptain[0] && i != g_iCaptain[1])
        {
            Hnsic_SwitchTeam(i, TEAM_SPEC);
        }
    }

    // Restart round for duel
    rg_round_restart();

    Hnsic_PrintChat(0, "=== 队长刀战开始 ===");
    Hnsic_PrintChat(0, "%s (TT) VS %s (CT)", g_eUserData[g_iCaptain[0]][UD_NAME],
        g_eUserData[g_iCaptain[1]][UD_Name]);
    Hnsic_PrintChat(0, "胜者选择阵营和地图类型! 限时%d秒", duel_time);

    // Set timer for duel
    remove_task(TASK_MATCH_TIMER);
    set_task(float(duel_time), "Captain_DuelTimeout", TASK_MATCH_TIMER);
}

// ─── Duel Timeout ────────────────────────────────────────────
public Captain_DuelTimeout()
{
    if (g_iMatchState != STATE_KNIFE_DUEL)
        return;

    // Random winner if duel timeout
    g_iCaptainDuelWinner = g_iCaptain[random(2)];
    Hnsic_PrintChat(0, "刀战超时! 随机选择: %s",
        g_eUserData[g_iCaptainDuelWinner][UD_NAME]);

    Captain_FinishDuel();
}

// ─── Finish Duel ─────────────────────────────────────────────
stock Captain_FinishDuel()
{
    g_bCaptainDuelActive = false;
    remove_task(TASK_MATCH_TIMER);

    g_iMatchState = STATE_SIDE_PICK;
    Hnsic_PrintChat(0, "刀战获胜者: %s", g_eUserData[g_iCaptainDuelWinner][UD_NAME]);
    Hnsic_PrintChat(0, "请使用 /pickside 选择阵营");

    Captain_ShowSidePickMenu(g_iCaptainDuelWinner);
}

// ─── Side Pick Menu ───────────────────────────────────────────
stock Captain_ShowSidePickMenu(id)
{
    new szMenu[MAX_MENU_LEN];
    new len = 0;

    len += formatex(szMenu[len], charsmax(szMenu) - len, "\y[HNSIC] 选择阵营^n^n");
    len += formatex(szMenu[len], charsmax(szMenu) - len, "\r1. \wTT (恐怖分子)^n");
    len += formatex(szMenu[len], charsmax(szMenu) - len, "\r2. \wCT (反恐精英)^n");
    len += formatex(szMenu[len], charsmax(szMenu) - len, "^n\r0. \w关闭");

    show_menu(id, MENU_KEY_1 | MENU_KEY_2 | MENU_KEY_0, szMenu, -1, "HnsicSidePick");
}

public HnsMenu_SidePickHandler(id, key)
{
    if (g_iMatchState != STATE_SIDE_PICK || id != g_iCaptainDuelWinner)
        return PLUGIN_HANDLED;

    new winnerGroup = g_eUserData[id][UD_GROUP];
    new loserGroup = 1 - winnerGroup;

    switch (key)
    {
        case 0: // TT
        {
            g_iCaptainTeam[winnerGroup] = TEAM_TT;
            g_iCaptainTeam[loserGroup] = TEAM_CT;
            Hnsic_PrintChat(0, "%s 选择了 TT", g_eUserData[id][UD_NAME]);
        }
        case 1: // CT
        {
            g_iCaptainTeam[winnerGroup] = TEAM_CT;
            g_iCaptainTeam[loserGroup] = TEAM_TT;
            Hnsic_PrintChat(0, "%s 选择了 CT", g_eUserData[id][UD_NAME]);
        }
        default: return PLUGIN_HANDLED;
    }

    // Move to map pick
    g_iMatchState = STATE_MAP_PICK;
    Hnsic_PrintChat(0, "请使用 /pickmap 选择地图类型");
    Captain_ShowMapPickMenu(id);
    return PLUGIN_HANDLED;
}

// ─── Map Pick Menu ────────────────────────────────────────────
stock Captain_ShowMapPickMenu(id)
{
    new szMenu[MAX_MENU_LEN];
    new len = 0;

    len += formatex(szMenu[len], charsmax(szMenu) - len, "\y[HNSIC] 选择地图类型^n^n");
    len += formatex(szMenu[len], charsmax(szMenu) - len, "\r1. \w随机Boost地图^n");
    len += formatex(szMenu[len], charsmax(szMenu) - len, "\r2. \w随机Skill地图^n");
    len += formatex(szMenu[len], charsmax(szMenu) - len, "^n\r0. \w关闭");

    show_menu(id, MENU_KEY_1 | MENU_KEY_2 | MENU_KEY_0, szMenu, -1, "HnsicMapPick");
}

public HnsMenu_MapPickHandler(id, key)
{
    if (g_iMatchState != STATE_MAP_PICK || id != g_iCaptainDuelWinner)
        return PLUGIN_HANDLED;

    new mapPool[6][MAX_MAPNAME_LEN];
    new poolSize;

    switch (key)
    {
        case 0: // Boost
        {
            g_iMapType = MAPTYPE_BOOST;
            poolSize = sizeof(g_szBoostMaps);
            for (new i = 0; i < poolSize; i++)
                copy(mapPool[i], charsmax(mapPool), g_szBoostMaps[i]);
            Hnsic_PrintChat(0, "选择了 Boost 地图类型");
        }
        case 1: // Skill
        {
            g_iMapType = MAPTYPE_SKILL;
            poolSize = sizeof(g_szSkillMaps);
            for (new i = 0; i < poolSize; i++)
                copy(mapPool[i], charsmax(mapPool), g_szSkillMaps[i]);
            Hnsic_PrintChat(0, "选择了 Skill 地图类型");
        }
        default: return PLUGIN_HANDLED;
    }

    // Pick random map from pool
    new chosen = random(poolSize);
    new szMap[MAX_MAPNAME_LEN];
    copy(szMap, charsmax(szMap), mapPool[chosen]);

    Hnsic_PrintChat(0, "随机地图: %s", szMap);
    Hnsic_Log("选图结果: %s (类型: %s)", szMap, (g_iMapType == MAPTYPE_BOOST) ? "Boost" : "Skill");

    // Change map
    Captain_ChangeMap(szMap);
    return PLUGIN_HANDLED;
}

// ─── Map Change ──────────────────────────────────────────────
stock Captain_ChangeMap(const map[])
{
    g_iMatchState = STATE_MAP_CHANGE;

    // Count expected players
    g_iExpectedPlayers = 0;
    for (new i = 1; i <= g_iMaxPlayers; i++)
    {
        if (Hnsic_IsPlayerValid(i))
            g_iExpectedPlayers++;
    }

    g_iReconnectedPlayers = 0;
    g_bWaitingReconnect = true;

    // Save current match data before map change
    // (In production, use nvault or similar for cross-map persistence)

    new szCmd[128];
    formatex(szCmd, charsmax(szCmd), "changelevel %s", map);
    server_cmd(szCmd);
}

// ─── Reconnect Check ─────────────────────────────────────────
stock Captain_CheckReconnect()
{
    if (!g_bWaitingReconnect)
        return;

    new currentPlayers = 0;
    for (new i = 1; i <= g_iMaxPlayers; i++)
    {
        if (Hnsic_IsPlayerValid(i))
            currentPlayers++;
    }

    if (currentPlayers >= g_iExpectedPlayers)
    {
        g_bWaitingReconnect = false;
        remove_task(TASK_RECONNECT_WAIT);
        Hnsic_PrintChat(0, "所有玩家已重新连接! 比赛即将开始");
        Captain_StartMatch();
    }
}

// ─── Start Match ──────────────────────────────────────────────
stock Captain_StartMatch()
{
    g_iMatchState = STATE_WARMUP;
    g_iCurrentHalf = 0;
    g_iRoundNum = 0;
    g_iMatchScore[TEAM_TT] = 0;
    g_iMatchScore[TEAM_CT] = 0;
    g_iTTPoints = 0;
    g_iCTPoints = 0;

    g_iScoringMode = GetCvarInt(CVAR_MIX_SCORING);
    g_iPointCapTarget = GetCvarInt(CVAR_MIX_POINT_CAP);
    g_iMaxRounds = GetCvarInt(CVAR_MIX_HALF_ROUNDS);

    // Assign teams based on captain picks
    for (new g = 0; g < MAX_GROUPS; g++)
    {
        new targetTeam = g_iCaptainTeam[g];
        for (new i = 0; i < g_iGroupCount[g]; i++)
        {
            new pid = g_iGroupPlayers[g][i];
            if (Hnsic_IsPlayerValid(pid))
            {
                Hnsic_SwitchTeam(pid, targetTeam);
            }
        }
    }

    new warmupTime = GetCvarInt(CVAR_MIX_WARMUP_TIME);
    Hnsic_PrintChat(0, "=== 比赛开始! ===");
    Hnsic_PrintChat(0, "热身时间: %d秒", warmupTime);
    Hnsic_PrintChat(0, "每半场: %d回合 | 积分上限: %d", g_iMaxRounds, g_iPointCapTarget);
    Hnsic_PrintChat(0, "计分模式: %s",
        (g_iScoringMode == SCORING_POINT_CAP) ? "积分制" : "扣分制");

    rg_round_restart();

    // Warmup timer
    set_task(float(warmupTime), "Captain_EndWarmup", TASK_FREEZE_TIME);
}

// ─── End Warmup ──────────────────────────────────────────────
public Captain_EndWarmup()
{
    if (g_iMatchState != STATE_WARMUP)
        return;

    g_iMatchState = STATE_FIRST_HALF;
    g_iRoundNum = 1;
    Hnsic_PrintChat(0, "=== 上半场开始! ===");
    rg_round_restart();
}

// ─── Register Captain Menus ──────────────────────────────────
stock Captain_RegisterMenus()
{
    register_menucmd(register_menuid("HnsicSidePick"), MENU_KEY_1 | MENU_KEY_2 | MENU_KEY_0, "HnsMenu_SidePickHandler");
    register_menucmd(register_menuid("HnsicMapPick"), MENU_KEY_1 | MENU_KEY_2 | MENU_KEY_0, "HnsMenu_MapPickHandler");
}

// ─── Pick Menu (for captain picking players) ────────────────
stock Captain_ShowPickMenu(id)
{
    new szMenu[MAX_MENU_LEN];
    new len = 0;
    new num = 1;

    len += formatex(szMenu[len], charsmax(szMenu) - len, "\y[HNSIC] 选人^n^n");

    // Show unpicked players
    for (new i = 1; i <= g_iMaxPlayers && num <= 9; i++)
    {
        if (Hnsic_IsPlayerValid(i) && !g_eUserData[i][UD_IS_CAPTAIN] && !g_eUserData[i][UD_PICKED])
        {
            len += formatex(szMenu[len], charsmax(szMenu) - len, "\r%d. \w%s (PTS: %d)^n",
                num, g_eUserData[i][UD_NAME], g_eUserData[i][UD_PTS]);
            num++;
        }
    }

    len += formatex(szMenu[len], charsmax(szMenu) - len, "^n\r0. \w跳过");

    show_menu(id, MENU_KEY_ALL, szMenu, -1, "HnsicCaptainPick");
}

public HnsMenu_CaptainPickHandler(id, key)
{
    if (g_iMatchState != STATE_CAPTAIN_PICK)
        return PLUGIN_HANDLED;

    if (key == 9) // Skip
    {
        // Check if picking is complete
        Captain_CheckPickComplete();
        return PLUGIN_HANDLED;
    }

    // Find the player at this key position
    new num = 0;
    for (new i = 1; i <= g_iMaxPlayers; i++)
    {
        if (Hnsic_IsPlayerValid(i) && !g_eUserData[i][UD_IS_CAPTAIN] && !g_eUserData[i][UD_PICKED])
        {
            if (num == key)
            {
                // Assign to captain's team
                new captainGroup = g_eUserData[id][UD_GROUP];
                new targetTeam = g_iCaptainTeam[captainGroup];
                Hnsic_SwitchTeam(i, targetTeam);
                g_eUserData[i][UD_PICKED] = 1;

                Hnsic_PrintChat(0, "%s 选择了 %s", g_eUserData[id][UD_NAME], g_eUserData[i][UD_Name]);
                break;
            }
            num++;
        }
    }

    Captain_CheckPickComplete();
    return PLUGIN_HANDLED;
}

// ─── Check if picking is done ───────────────────────────────
stock Captain_CheckPickComplete()
{
    // Check if all non-captain players are picked
    for (new i = 1; i <= g_iMaxPlayers; i++)
    {
        if (Hnsic_IsPlayerValid(i) && !g_eUserData[i][UD_IS_CAPTAIN] && !g_eUserData[i][UD_PICKED])
        {
            // Still players to pick, show menu for next captain
            return;
        }
    }

    // All picked, proceed to knife duel
    Captain_StartKnifeDuel();
}
