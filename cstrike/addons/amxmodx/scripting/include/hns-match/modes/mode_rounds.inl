// ============================================
// HnsMatchSystem - Rounds Mode (回合制)
// 先赢N局的队伍获胜
// ============================================
// Team A vs Team B，每回合攻守互换
// 先赢 g_iRoundsWinRounds 局的队伍获胜
// 总回合数上限 g_iRoundsMaxRounds = 2N-1
// 换边在 g_iRoundsMaxRounds/2 处触发
// 变量定义在 globals.inc 中
// ============================================

// 动态调整: 索引=每队人数, [0]=胜局数, [1]=最大局数
// 2v2→3/5, 3v3→4/7, 4v4→5/9, 5v5→6/10
new g_iRoundsTable[6][2] = {
	{0, 0},   // 0人
	{0, 0},   // 1v1 不用
	{3, 5},   // 2v2
	{4, 7},   // 3v3
	{5, 9},   // 4v4
	{6, 10}   // 5v5
};

public rounds_init() {
	g_ModFuncs[MODE_ROUNDS][MODEFUNC_START]		= CreateOneForward(g_PluginId, "rounds_start");
	g_ModFuncs[MODE_ROUNDS][MODEFUNC_END]		= CreateOneForward(g_PluginId, "rounds_stop");
	g_ModFuncs[MODE_ROUNDS][MODEFUNC_PAUSE]		= CreateOneForward(g_PluginId, "rounds_pause");
	g_ModFuncs[MODE_ROUNDS][MODEFUNC_UNPAUSE]	= CreateOneForward(g_PluginId, "rounds_unpause");
	g_ModFuncs[MODE_ROUNDS][MODEFUNC_ROUNDSTART]	= CreateOneForward(g_PluginId, "rounds_roundstart");
	g_ModFuncs[MODE_ROUNDS][MODEFUNC_ROUNDEND]	= CreateOneForward(g_PluginId, "rounds_roundend", FP_CELL);
	g_ModFuncs[MODE_ROUNDS][MODEFUNC_FREEZEEND]	= CreateOneForward(g_PluginId, "rounds_freezeend");
	g_ModFuncs[MODE_ROUNDS][MODEFUNC_RESTARTROUND]	= CreateOneForward(g_PluginId, "rounds_restartround");
	g_ModFuncs[MODE_ROUNDS][MODEFUNC_SWAP]		= CreateOneForward(g_PluginId, "rounds_swap");
	g_ModFuncs[MODE_ROUNDS][MODEFUNC_PLAYER_JOIN]	= CreateOneForward(g_PluginId, "rounds_player_join", FP_CELL);
	g_ModFuncs[MODE_ROUNDS][MODEFUNC_PLAYER_LEAVE]	= CreateOneForward(g_PluginId, "rounds_player_leave", FP_CELL);
	g_ModFuncs[MODE_ROUNDS][MODEFUNC_KILL]		= CreateOneForward(g_PluginId, "rounds_killed", FP_CELL, FP_CELL);
	g_ModFuncs[MODE_ROUNDS][MODEFUNC_FALLDAMAGE]	= CreateOneForward(g_PluginId, "rounds_falldamage", FP_CELL, FP_FLOAT);

	register_clcmd("say /rounds", "cmdRoundsConfig");
	register_clcmd("say_team /rounds", "cmdRoundsConfig");
}

public rounds_start() {
	match_reset_data();

	ChangeGameplay(GAMEPLAY_HNS);

	g_iCurrentMode = MODE_ROUNDS;
	update_hostname_prefix("ROUNDS");
	g_iMatchStatus = MATCH_STARTED;
	g_eMatchState = STATE_PREPARE;

	// Record match start for deserter penalty
	deserter_match_start();

	g_isTeamTT = HNS_TEAM_A;

	set_cvars_mode(MODE_ROUNDS);

	// Force spectator settings during match
	set_cvar_num("mp_forcecamera", 2);
	set_cvar_num("mp_limitteams", 0);

	loadMapCFG();

	// Reset rounds scores
	g_iRoundsScoreT = 0;
	g_iRoundsScoreCT = 0;
	g_iRoundsTotalPlayed = 0;

	new iPlayers[MAX_PLAYERS], iNum;
	get_players(iPlayers, iNum, "ce", "TERRORIST");
	g_eMatchInfo[e_mTeamSizeTT] = iNum;
	g_eMatchInfo[e_mTeamSize] = get_num_players_in_match();

	// 动态调整回合数: 根据每队人数 (手动设置后跳过)
	new iTeamSize = g_eMatchInfo[e_mTeamSizeTT];
	if (iTeamSize > 5) iTeamSize = 5;
	if (iTeamSize < 2) iTeamSize = 2;
	if (!g_bRoundsManual) {
		g_iRoundsWinRounds = g_iRoundsTable[iTeamSize][0];
		g_iRoundsMaxRounds = g_iRoundsTable[iTeamSize][1];
	}

	hns_restart_round(2.0);

	client_cmd(0, "spk plats/elevbell1.wav");
	setTaskHud(0, 0.0, 1, 255, 255, 255, 3.0, "Going Live in 3 second!");
	setTaskHud(0, 3.1, 1, 255, 255, 255, 3.0, "Live! Live! Live!^nGood Luck & Have Fun!");

	client_print(0, print_chat, "[Rounds] %dv%d | First to %d wins (max %d rounds)", iTeamSize, iTeamSize, g_iRoundsWinRounds, g_iRoundsMaxRounds);

	ExecuteForward(g_hForwards[MATCH_START], _);
}

public rounds_stop() {
	if (task_exists(7010)) remove_task(7010);
	ExecuteForward(g_hForwards[MATCH_CANCEL], _);

	// Restore spectator settings
	set_cvar_num("mp_forcecamera", 0);

	match_reset_data();
	training_start();
}

public rounds_freezeend() {
	if (g_eMatchState != STATE_ENABLED) {
		return PLUGIN_HANDLED;
	}

	set_task(5.0, "taskCheckAfk");

	if (g_bHnsBannedInit) {
		if (checkUserBan()) {
			return PLUGIN_HANDLED;
		}
	}

	if (g_eMatchInfo[e_mLeaved]) {
		set_task(1.0, "rounds_pause");
	}

	return PLUGIN_HANDLED;
}

public rounds_roundstart() {
	if (g_eMatchState == STATE_PREPARE) {
		g_eMatchState = STATE_ENABLED;
	}

	// ★ 启动回合制HUD
	if (!task_exists(7010)) {
		set_task(1.0, "taskRoundsHud", 7010, .flags = "b");
	}

	cmdShowTimers(0);

	ResetAfkData();

	if (g_bHnsBannedInit) {
		checkUserBan();
	}

	taskCheckLeave();

	new iPlayers[MAX_PLAYERS], iNum;
	get_players(iPlayers, iNum, "che", "TERRORIST");
	g_eMatchInfo[e_mTeamSizeTT] = iNum;

	get_players(iPlayers, iNum, "ch");
	for (new i; i < iNum; i++) {
		new id = iPlayers[i];

		if (!is_user_connected(id)) {
			continue;
		}

		if (getUserTeam(id) == TEAM_TERRORIST || getUserTeam(id) == TEAM_CT) {
			g_ePlayerInfo[id][PLAYER_MATCH] = true;
			copy(g_ePlayerInfo[id][PLAYER_TEAM], charsmax(g_ePlayerInfo[][PLAYER_TEAM]), getUserTeam(id) == TEAM_TERRORIST ? "TERRORIST" : "CT");
		} else {
			g_ePlayerInfo[id][PLAYER_MATCH] = false;
		}
	}

	set_task(0.3, "taskSaveAfk");
	set_task(3.0, "taskCheckAfk");
}

public rounds_roundend(bool:win_ct) {
	if (g_eMatchState != STATE_ENABLED) {
		return;
	}

	g_eMatchState = STATE_PREPARE;

	// Determine winner of this round
	new winner;
	if (win_ct) {
		winner = TEAM_CT;
	} else {
		winner = TEAM_TERRORIST;
	}

	// Count rounds won
	if (winner == TEAM_TERRORIST) g_iRoundsScoreT++;
	else if (winner == TEAM_CT) g_iRoundsScoreCT++;

	g_iRoundsTotalPlayed++;

	// Check if someone won
	if (g_iRoundsScoreT >= g_iRoundsWinRounds) {
		client_print(0, print_chat, "[Rounds] Team A wins the match! %d-%d", g_iRoundsScoreT, g_iRoundsScoreCT);
		rounds_finish(1);
		return;
	} else if (g_iRoundsScoreCT >= g_iRoundsWinRounds) {
		client_print(0, print_chat, "[Rounds] Team B wins the match! %d-%d", g_iRoundsScoreT, g_iRoundsScoreCT);
		rounds_finish(2);
		return;
	}

	// Swap at half (round max_rounds / 2)
	if (g_iRoundsTotalPlayed == g_iRoundsMaxRounds / 2) {
		rounds_swap();
	}

	// Display current score
	client_print(0, print_chat, "[Rounds] Score: A %d - %d B | First to %d wins", g_iRoundsScoreT, g_iRoundsScoreCT, g_iRoundsWinRounds);
}

public rounds_restartround() {
	if (g_eMatchState == STATE_ENABLED) {
		g_eMatchState = STATE_PREPARE;
	}

	ResetAfkData();
}

public rounds_pause() {
	if (g_eMatchState == STATE_PAUSED) {
		return;
	}

	if (task_exists(7010)) remove_task(7010);
	g_eMatchState = STATE_PAUSED;
	ChangeGameplay(GAMEPLAY_TRAINING);
	set_pause_settings();
}

public rounds_unpause() {
	if (g_eMatchState != STATE_PAUSED) {
		return;
	}

	g_eMatchState = STATE_PREPARE;

	hns_restart_round(1.0);

	g_eMatchInfo[e_mTeamSize] = get_num_players_in_match();

	ChangeGameplay(GAMEPLAY_HNS);

	set_unpause_settings();
}

public rounds_swap() {
	g_isTeamTT = HNS_TEAM:!g_isTeamTT;

	// ★ 交换双方分数（换边后分数跟队伍走，不跟角色走）
	new iTmp = g_iRoundsScoreT;
	g_iRoundsScoreT = g_iRoundsScoreCT;
	g_iRoundsScoreCT = iTmp;

	// ★ 实际交换玩家阵营
	rg_swap_all_players();

	ResetAfkData();
}

public rounds_killed(victim, killer) {
	// No special handling for rounds mode
}

public rounds_falldamage(id, Float:flDmg) {
	// No special handling for rounds mode
}

public rounds_player_join(id) {
	// Check deserter ban
	if (deserter_is_banned(id)) {
		new iRemaining = deserter_get_ban_remaining(id);
		new szTime[32];
		if (iRemaining >= 3600) {
			formatex(szTime, charsmax(szTime), "%dh %dmin", iRemaining / 3600, (iRemaining % 3600) / 60);
		} else {
			formatex(szTime, charsmax(szTime), "%dmin", iRemaining / 60);
		}
		chat_print(id, "[HNS] You are banned from matches for %s (%d desertions).", szTime, g_iDesertCount[id]);
		transferUserToSpec(id);
		return;
	}

	TrieGetArray(g_eMatchInfo[e_tLeaveData], getUserKey(id), g_ePlayerInfo[id], PLAYER_INFO);

	if (g_ePlayerInfo[id][PLAYER_MATCH]) {
		new iNum = get_num_players_in_match(id);

		new bool:bReplaced = iNum >= g_eMatchInfo[e_mTeamSize] ? true : false;

		ExecuteForward(g_hForwards[MATCH_JOIN_PLAYER], _, id, bReplaced);

		if (bReplaced) {
			transferUserToSpec(id);
			return;
		}

		new iMatchRounds = g_eMatchInfo[e_iSidesRounds][HNS_TEAM_A] + g_eMatchInfo[e_iSidesRounds][HNS_TEAM_B];

		if (iMatchRounds == g_ePlayerInfo[id][LEAVE_IN_ROUND]) {
			rg_set_user_team(id, g_ePlayerInfo[id][PLAYER_TEAM][0] == 'T' ? TEAM_TERRORIST : TEAM_CT);
		} else {
			rg_set_user_team(id, g_ePlayerInfo[id][PLAYER_TEAM][0] == 'T' ? TEAM_CT : TEAM_TERRORIST);
		}

		if (g_eMatchState == STATE_PAUSED)
			rg_round_respawn(id);
	} else {
		transferUserToSpec(id);
		return;
	}
}

public rounds_player_leave(id) {
	if (g_ePlayerInfo[id][PLAYER_MATCH]) {
		new iMatchRounds = g_eMatchInfo[e_iSidesRounds][HNS_TEAM_A] + g_eMatchInfo[e_iSidesRounds][HNS_TEAM_B];

		g_ePlayerInfo[id][LEAVE_IN_ROUND] = iMatchRounds;

		// Apply deserter penalty
		deserter_apply_penalty(id);
		deserter_save(id);
	}

	ExecuteForward(g_hForwards[MATCH_LEAVE_PLAYER], _, id);

	TrieSetArray(g_eMatchInfo[e_tLeaveData], getUserKey(id), g_ePlayerInfo[id], PLAYER_INFO);

	arrayset(g_ePlayerInfo[id], 0, PLAYER_INFO);
}

// ============================================
// ★ 回合制 HUD：显示回合数和比分
// ============================================
public taskRoundsHud() {
	if (g_eMatchState != STATE_ENABLED || g_iCurrentMode != MODE_ROUNDS) {
		if (task_exists(7010)) remove_task(7010);
		return;
	}
	
	new szHud[256];
	format(szHud, charsmax(szHud), "回合制 | 第 %d/%d 回合^nA %d  -  %d B | 先赢 %d 局",
		g_iRoundsTotalPlayed + 1, g_iRoundsMaxRounds,
		g_iRoundsScoreT, g_iRoundsScoreCT, g_iRoundsWinRounds);
	
	set_hudmessage(255, 255, 255, -1.0, 0.06, 0, 0.0, 1.5, 0.1, 0.0, -1);
	show_hudmessage(0, szHud);
}

stock rounds_finish(iWinTeam) {
	if (task_exists(7010)) remove_task(7010);
	ExecuteForward(g_hForwards[MATCH_FINISH], _, iWinTeam);

	// Clear deserter active flags (match ended normally)
	deserter_clear_on_match_end();
	matchControl_reset();

	chat_print(0, "Team %s wins the match! (^3%d-%d^1)", iWinTeam == 1 ? "A" : "B", g_iRoundsScoreT, g_iRoundsScoreCT);

	setTaskHud(0, 1.0, 1, 255, 255, 255, 4.0, "Game Over");

	match_reset_data();

	training_start();

	ExecuteForward(g_hForwards[MATCH_FINISH_POST], _, iWinTeam);
}

// ============================================================
//  === 回合数动态调整菜单 (/rounds) ===
// ============================================================
public cmdRoundsConfig(id) {
	if (!isUserWatcher(id)) {
		client_print(id, print_chat, "[Rounds] 只有管理员可以调整回合设置");
		return PLUGIN_HANDLED;
	}
	showRoundsConfigMenu(id);
	return PLUGIN_HANDLED;
}

showRoundsConfigMenu(id) {
	new szMenu[512], iLen;
	iLen = formatex(szMenu, charsmax(szMenu), "\r回合比赛设置 \d- 动态调整^n^n");

	iLen += formatex(szMenu[iLen], charsmax(szMenu) - iLen, "\r1. \w获胜回合: \y%d (-1)^n", g_iRoundsWinRounds);
	iLen += formatex(szMenu[iLen], charsmax(szMenu) - iLen, "\r2. \w获胜回合: \y%d (+1)^n", g_iRoundsWinRounds);
	iLen += formatex(szMenu[iLen], charsmax(szMenu) - iLen, "\r3. \w最大回合: \y%d (-1)^n", g_iRoundsMaxRounds);
	iLen += formatex(szMenu[iLen], charsmax(szMenu) - iLen, "\r4. \w最大回合: \y%d (+1)^n", g_iRoundsMaxRounds);
	iLen += formatex(szMenu[iLen], charsmax(szMenu) - iLen, "^n\r5. \w根据人数自动设定回合数^n");
	iLen += formatex(szMenu[iLen], charsmax(szMenu) - iLen, "^n\r6. \w设置完成^n");
	iLen += formatex(szMenu[iLen], charsmax(szMenu) - iLen, "^n\r0. \w退出");

	show_menu(id, (1<<0)|(1<<1)|(1<<2)|(1<<3)|(1<<4)|(1<<5)|(1<<9), szMenu, -1, "HnsRoundsConfig");
}

public roundsConfigMenuHandler(id, key) {
	if (key == 9) return;

	if (key == 0) {
		// 获胜回合 -1
		if (g_iRoundsWinRounds > 2) {
			g_iRoundsWinRounds--;
			g_bRoundsManual = true;
		}
		showRoundsConfigMenu(id);
	} else if (key == 1) {
		// 获胜回合 +1
		if (g_iRoundsWinRounds < g_iRoundsMaxRounds) {
			g_iRoundsWinRounds++;
			g_bRoundsManual = true;
		}
		showRoundsConfigMenu(id);
	} else if (key == 2) {
		// 最大回合 -1
		if (g_iRoundsMaxRounds > g_iRoundsWinRounds + 1) {
			g_iRoundsMaxRounds--;
			if (g_iRoundsWinRounds > g_iRoundsMaxRounds)
				g_iRoundsWinRounds = g_iRoundsMaxRounds;
			g_bRoundsManual = true;
		}
		showRoundsConfigMenu(id);
	} else if (key == 3) {
		// 最大回合 +1
		if (g_iRoundsMaxRounds < 20) {
			g_iRoundsMaxRounds++;
			g_bRoundsManual = true;
		}
		showRoundsConfigMenu(id);
	} else if (key == 4) {
		// 根据人数自动设定 (取消手动模式)
		g_bRoundsManual = false;
		new iTeamSize = g_eMatchInfo[e_mTeamSizeTT];
		if (iTeamSize > 5) iTeamSize = 5;
		if (iTeamSize < 2) iTeamSize = 2;
		g_iRoundsWinRounds = g_iRoundsTable[iTeamSize][0];
		g_iRoundsMaxRounds = g_iRoundsTable[iTeamSize][1];
		client_print(id, print_chat, "[Rounds] 已根据 %dv%d 自动设定: 先赢%d局 / 最多%d局", iTeamSize, iTeamSize, g_iRoundsWinRounds, g_iRoundsMaxRounds);
		showRoundsConfigMenu(id);
	} else if (key == 5) {
		client_print(id, print_chat, "[Rounds] 回合设置: 先赢 %d 局 (最多 %d 局)", g_iRoundsWinRounds, g_iRoundsMaxRounds);
	}
}