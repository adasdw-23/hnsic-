// ============================================
// HnsMatchSystem - Ascension Mode (PointScap)
// 点位积分制 - T进入区域直接得分
// 完全重写版 - 简洁可靠，不依赖freezeend事件链
// ============================================

public ascension_init() {
	g_ModFuncs[MODE_ASCENSION][MODEFUNC_START]		= CreateOneForward(g_PluginId, "ascension_start");
	g_ModFuncs[MODE_ASCENSION][MODEFUNC_END]		= CreateOneForward(g_PluginId, "ascension_stop");
	g_ModFuncs[MODE_ASCENSION][MODEFUNC_PAUSE]		= CreateOneForward(g_PluginId, "ascension_pause");
	g_ModFuncs[MODE_ASCENSION][MODEFUNC_UNPAUSE]	= CreateOneForward(g_PluginId, "ascension_unpause");
	g_ModFuncs[MODE_ASCENSION][MODEFUNC_ROUNDSTART]	= CreateOneForward(g_PluginId, "ascension_roundstart");
	g_ModFuncs[MODE_ASCENSION][MODEFUNC_ROUNDEND]	= CreateOneForward(g_PluginId, "ascension_roundend", FP_CELL);
	g_ModFuncs[MODE_ASCENSION][MODEFUNC_FREEZEEND]	= CreateOneForward(g_PluginId, "ascension_freezeend");
	g_ModFuncs[MODE_ASCENSION][MODEFUNC_RESTARTROUND]= CreateOneForward(g_PluginId, "ascension_restartround");
	g_ModFuncs[MODE_ASCENSION][MODEFUNC_SWAP]		= CreateOneForward(g_PluginId, "ascension_swap");
	g_ModFuncs[MODE_ASCENSION][MODEFUNC_PLAYER_JOIN]= CreateOneForward(g_PluginId, "ascension_player_join", FP_CELL);
	g_ModFuncs[MODE_ASCENSION][MODEFUNC_PLAYER_LEAVE]= CreateOneForward(g_PluginId, "ascension_player_leave", FP_CELL);
	g_ModFuncs[MODE_ASCENSION][MODEFUNC_KILL]		= CreateOneForward(g_PluginId, "ascension_killed", FP_CELL, FP_CELL);
	g_ModFuncs[MODE_ASCENSION][MODEFUNC_FALLDAMAGE]	= CreateOneForward(g_PluginId, "ascension_falldamage", FP_CELL, FP_FLOAT);
	
	// ★ 点位分数设置已移到主菜单
}

stock Float:pointscap_get_zone_score(iZoneType) {
	switch (iZoneType) {
		case 6, 5: return g_flPointScapScore5;
		case 4: return g_flPointScapScore4;
	}
	return g_flPointScapScore3;
}

stock pointscap_add_hns_team_score(HNS_TEAM:iTeam, Float:flScore) {
	if (flScore <= 0.0) return;
	if (iTeam == HNS_TEAM_A) g_flScoreA += flScore;
	else g_flScoreB += flScore;
}

stock HNS_TEAM:pointscap_get_hns_team_by_cs_team(TeamName:iTeam) {
	// g_isTeamTT 表示当前哪一边是 T
	if (iTeam == TEAM_TERRORIST) return g_isTeamTT;
	return HNS_TEAM:!g_isTeamTT;
}

stock pointscap_get_main_leader(&iSecond, &Float:flMax, &Float:flSecond) {
	new iLeader = -1;
	iSecond = -1;
	flMax = -1.0;
	flSecond = -1.0;

	for (new i = 0; i < g_iZoneCount; i++) {
		new Float:v = g_flPointScapMainTrend[i];
		if (v > flMax) {
			flSecond = flMax;
			iSecond = iLeader;
			flMax = v;
			iLeader = i;
		} else if (v > flSecond) {
			flSecond = v;
			iSecond = i;
		}
	}
	return iLeader;
}

stock ascension_abort_missing_zones() {
	chat_print(0, "[Ascension] 当前地图没有点位配置，已阻止点位积分模式启动.");
	setTaskHud(0, 0.5, 1, 255, 80, 80, 5.0, "[Ascension] 当前地图没有点位配置，无法开始点位积分模式");
	ExecuteForward(g_hForwards[MATCH_CANCEL], _);
	match_reset_data();
	training_start();
}

// ============================================
// 模式开始
// ============================================
public ascension_start() {
	// ★ 不调用 match_reset_data()，由 mix_start() 已调用
	ChangeGameplay(GAMEPLAY_HNS);
	
	g_iCurrentMode = MODE_ASCENSION;
	update_hostname_prefix("ASCENSION");
	g_iCurrentRules = RULES_POINTSCAP;
	g_iMatchStatus = MATCH_STARTED;
	g_eMatchState = STATE_PREPARE;
	g_isTeamTT = HNS_TEAM_A;
	
	g_flScoreA = 0.0;
	g_flScoreB = 0.0;
	g_iPointScapRound = 0;
	g_iZoneCount = 0;
	
	pointscap_load_zones();
	
	server_print("[Ascension] Zones loaded: %d", g_iZoneCount);
	if (g_iZoneCount <= 0) {
		ascension_abort_missing_zones();
		return;
	}
	
	set_cvars_mode(MODE_ASCENSION);
	loadMapCFG();
	
	new iPlayers[MAX_PLAYERS], iNum;
	get_players(iPlayers, iNum, "ce", "TERRORIST");
	g_eMatchInfo[e_mTeamSizeTT] = iNum;
	g_eMatchInfo[e_mTeamSize] = get_num_players_in_match();
	
	hns_restart_round(2.0);
	// ★ 不调用 MATCH_START forward，由 mix_start() 统一调用
}

// ============================================
// 模式停止
// ============================================
public ascension_stop() {
	remove_all_tasks();
	ExecuteForward(g_hForwards[MATCH_CANCEL], _);
	match_reset_data();
	training_start();
}

// ============================================
// 回合开始 ★ 核心：直接启动检测，不依赖任何事件链
// ============================================
public ascension_roundstart() {
	remove_all_tasks();
	
	// ★ 如果已暂停，不覆盖状态
	if (g_eMatchState == STATE_PAUSED) {
		server_print("[Ascension] roundstart skipped: game is paused");
		return;
	}
	
	g_eMatchState = STATE_ENABLED;
	pointscap_load_zones();
	
	server_print("[Ascension] roundstart loaded %d zones", g_iZoneCount);
	if (g_iZoneCount <= 0) {
		ascension_abort_missing_zones();
		return;
	}
	for (new i = 0; i < g_iZoneCount; i++) {
		server_print("[Ascension] Zone %d: label=%c type=%d enabled=%d mins=(%.0f,%.0f,%.0f) maxs=(%.0f,%.0f,%.0f)",
			i, 'A' + g_eZones[i][ZONE_LABEL], g_eZones[i][ZONE_TYPE], g_eZones[i][ZONE_ENABLED],
			g_eZones[i][ZONE_MINS][0], g_eZones[i][ZONE_MINS][1], g_eZones[i][ZONE_MINS][2],
			g_eZones[i][ZONE_MAXS][0], g_eZones[i][ZONE_MAXS][1], g_eZones[i][ZONE_MAXS][2]);
	}
	
	g_bPointScapDetectFirstRun = true; // ★ 重置首次运行标记
	
	g_iPointScapRound++;
	g_iPointScapRoundCaptures = 0;
	g_flRoundTime = 0.0;
	g_iPointScapMainZone = -1;
	g_iPointScapMainState = 0;
	g_flPointScapMainWindowLeft = g_flPointScapMainWindow;
	g_bPointScapMainConflictMsg = false;
	g_bPointScapMainChosenMsg = false;
	for (new i = 0; i < MAX_ZONES; i++) {
		g_flPointScapMainTrend[i] = 0.0;
	}
	
	// 重置区域状态
	for (new i = 0; i < g_iZoneCount; i++) {
		g_eZones[i][ZONE_STATUS] = 0;
		g_eZones[i][ZONE_CAPTURED] = 0;
		g_eZones[i][ZONE_CAPTURE_TIME] = 0.0;
		g_eZones[i][ZONE_PLAYER_COUNT] = 0;
	}
	
	// 标记比赛玩家
	new iPlayers[MAX_PLAYERS], iNum;
	get_players(iPlayers, iNum, "ch");
	for (new i; i < iNum; i++) {
		new id = iPlayers[i];
		if (!is_user_connected(id)) continue;
		if (getUserTeam(id) == TEAM_TERRORIST || getUserTeam(id) == TEAM_CT) {
			g_ePlayerInfo[id][PLAYER_MATCH] = true;
			copy(g_ePlayerInfo[id][PLAYER_TEAM], charsmax(g_ePlayerInfo[][PLAYER_TEAM]), 
				fmt("%s", getUserTeam(id) == TEAM_TERRORIST ? "TERRORIST" : "CT"));
		} else {
			g_ePlayerInfo[id][PLAYER_MATCH] = false;
		}
	}
	
	// 初始化检测时间窗口
	g_flPointScapDetectTime = float(g_iPointScapDetectTime);
	if (g_flPointScapDetectTime < 10.0) g_flPointScapDetectTime = 10.0;
	
	// ★ 直接启动检测任务（每1.0秒，等冻结期过后开始计分）
	set_task(1.0, "taskAscensionDetect", TASK_POINTSCAP_DETECT, .flags = "b");
	
	// 启动HUD
	if (!task_exists(TASK_POINTSCAP_HUD))
		set_task(1.0, "taskAscensionHud", TASK_POINTSCAP_HUD, .flags = "b");
	
	// 刀杀计时
	g_flPointScapKnifeTime = float(g_iPointScapKnifeTime);
	set_task(3.0, "taskAscensionKnife", TASK_POINTSCAP_KNIFE, .flags = "b");
	
	server_print("[Ascension] Round %d started: zones=%d, detectTime=%.0f", 
		g_iPointScapRound, g_iZoneCount, g_flPointScapDetectTime);
	// ★ Restore scores from pre-round save (if this is a restart, not first round)
	if (g_flScorePreRound[0] > 0.0 || g_flScorePreRound[1] > 0.0) {
		g_flScoreA = g_flScorePreRound[0];
		g_flScoreB = g_flScorePreRound[1];
	}
	// ★ 保存回合开始时的分数（暂停恢复用）
	g_flScorePreRound[0] = g_flScoreA;
	g_flScorePreRound[1] = g_flScoreB;
	// ★ 用 HUD 中心消息，不会被 ChatManager 拦截
	set_hudmessage(0, 255, 0, -1.0, 0.3, 0, 6.0, 5.0);
	show_hudmessage(0, "[Ascension] 回合 %d 已启动!^n点位=%d 检测时间=%.0f秒^n3秒后开始计分",
		g_iPointScapRound, g_iZoneCount, g_flPointScapDetectTime);
	
	ExecuteForward(g_hForwards[HNS_ROUND_START], _);
}

// ============================================
// 冻结结束 - 仅作为兼容保留，实际检测已在roundstart启动
// ============================================
public ascension_freezeend() {
	if (g_eMatchState != STATE_ENABLED)
		return PLUGIN_HANDLED;
	
	// 如果检测还没启动（极端情况），现在启动
	if (!task_exists(TASK_POINTSCAP_DETECT)) {
		set_task(1.0, "taskAscensionDetect", TASK_POINTSCAP_DETECT, .flags = "b");
	}
	if (!task_exists(TASK_POINTSCAP_KNIFE)) {
		g_flPointScapKnifeTime = float(g_iPointScapKnifeTime);
		set_task(1.0, "taskAscensionKnife", TASK_POINTSCAP_KNIFE, .flags = "b");
	}
	
	set_task(5.0, "taskCheckAfk");
	
	if (g_bHnsBannedInit) {
		if (checkUserBan()) return PLUGIN_HANDLED;
	}
	
	ExecuteForward(g_hForwards[HNS_ROUND_FREEZEEND], _);
	return PLUGIN_HANDLED;
}

// ============================================
// ★ 核心：点位检测任务（每1.0秒）
// 使用玩家 origin 做点检测，简单可靠
// ============================================
public taskAscensionDetect() {
	if (g_eMatchState != STATE_ENABLED) {
		if (task_exists(TASK_POINTSCAP_DETECT)) remove_task(TASK_POINTSCAP_DETECT);
		g_bPointScapDetectFirstRun = true;
		return;
	}
	
	g_flPointScapDetectTime -= 1.0;
	if (g_flPointScapMainWindowLeft > 0.0) {
		g_flPointScapMainWindowLeft -= 1.0;
	}
	
	if (g_flPointScapDetectTime <= 0.0) {
		if (g_iPointScapMainState == 0 && g_iPointScapMainZone >= 0) {
			g_iPointScapMainState = 1;
		}
		server_print("[Ascension] 检测时间结束! A=%.1f B=%.1f", g_flScoreA, g_flScoreB);
		if (task_exists(TASK_POINTSCAP_DETECT)) remove_task(TASK_POINTSCAP_DETECT);
		g_bPointScapDetectFirstRun = true;
		return;
	}
	
	// 获取所有存活T
	new iTPlayers[MAX_PLAYERS], iTNum;
	get_players(iTPlayers, iTNum, "ae", "TERRORIST");
	
	if (g_bPointScapDetectFirstRun) {
		g_bPointScapDetectFirstRun = false;
		server_print("[Ascension] 检测启动: zones=%d T=%d time=%.0f", g_iZoneCount, iTNum, g_flPointScapDetectTime);
	}
	
	if (iTNum == 0) return;
	new Float:flNow = get_gametime();

	// ★ 每个 zone 独立检测：只要有人进入，就按该 zone 的 type 给分
	for (new zoneId = 0; zoneId < g_iZoneCount; zoneId++) {
		if (!g_eZones[zoneId][ZONE_ENABLED]) continue;
		if (g_eZones[zoneId][ZONE_CAPTURED]) continue; // 已占领，跳过

		new Float:zMin[3], Float:zMax[3];
		zMin[0] = g_eZones[zoneId][ZONE_MINS][0];
		zMin[1] = g_eZones[zoneId][ZONE_MINS][1];
		zMin[2] = g_eZones[zoneId][ZONE_MINS][2];
		zMax[0] = g_eZones[zoneId][ZONE_MAXS][0];
		zMax[1] = g_eZones[zoneId][ZONE_MAXS][1];
		zMax[2] = g_eZones[zoneId][ZONE_MAXS][2];

		new iCount = 0;
		for (new i = 0; i < iTNum; i++) {
			new id = iTPlayers[i];
			if (!is_user_alive(id)) continue;

			new Float:fOrigin[3];
			pev(id, pev_origin, fOrigin);

			if (fOrigin[0] >= zMin[0] && fOrigin[0] <= zMax[0] &&
			    fOrigin[1] >= zMin[1] && fOrigin[1] <= zMax[1] &&
			    fOrigin[2] >= zMin[2] && fOrigin[2] <= zMax[2]) {
				iCount++;
			}
		}

		if (iCount >= 1) {
			// ★ 动态主点趋势：在观察窗口内累计“这个点被主攻的强度”
			if (g_flPointScapMainWindowLeft > 0.0 && g_iPointScapMainState == 0 && g_iPointScapMainState != -1) {
				g_flPointScapMainTrend[zoneId] += float(iCount);
			}

			if (g_eZones[zoneId][ZONE_STATUS] != 1) {
				g_eZones[zoneId][ZONE_STATUS] = 1;
				g_eZones[zoneId][ZONE_CAPTURE_TIME] = flNow;
			}
			g_eZones[zoneId][ZONE_PLAYER_COUNT] = iCount;

			if ((flNow - g_eZones[zoneId][ZONE_CAPTURE_TIME]) < g_flPointScapStayTime) {
				continue;
			}

			new Float:pointScore = pointscap_get_zone_score(g_eZones[zoneId][ZONE_TYPE]);
			new bool:bMainBonus = false;
			if (g_iPointScapMainState != -1 && g_iPointScapMainZone == zoneId) {
				bMainBonus = true;
				g_iPointScapMainState = 1;
				pointScore += g_flPointScapMainBonus;
			}

			new HNS_TEAM:iTeam = g_isTeamTT;
			pointscap_add_hns_team_score(iTeam, pointScore);

			// ★ 开局观察窗口内，如果第一次出现有效占点，直接锁主点（更贴合“开局先打的点就是主点”）
			if (!g_bPointScapMainChosenMsg && g_flPointScapMainWindowLeft > 0.0 && g_iPointScapMainState == 0) {
				g_iPointScapMainZone = zoneId;
				g_iPointScapMainState = 1;
				g_bPointScapMainChosenMsg = true;
				chat_print(0, "[Ascension] 本回合主点锁定为 %c，额外加分 %.1f.",
					'A' + g_eZones[g_iPointScapMainZone][ZONE_LABEL], g_flPointScapMainBonus);
			}

			g_iPointScapRoundCaptures++;
			g_eZones[zoneId][ZONE_CAPTURED] = 1;
			g_eZones[zoneId][ZONE_STATUS] = 2;
			g_eZones[zoneId][ZONE_CAPTURE_TIME] = flNow;

			if (g_iPointScapSoundCapture) {
				client_cmd(0, "spk buttons/blip2.wav");
			}

			new szTeamName[8], szBonusTag[16];
			if (iTeam == HNS_TEAM_A) copy(szTeamName, charsmax(szTeamName), "Team A");
			else copy(szTeamName, charsmax(szTeamName), "Team B");
			if (bMainBonus) copy(szBonusTag, charsmax(szBonusTag), " [主点]");
			else szBonusTag[0] = 0;
			client_print(0, print_chat, "[Ascension] %s 占领了点位 %c (%d人点)%s! 得分 +%.1f | A %.1f - B %.1f",
				szTeamName, 'A' + g_eZones[zoneId][ZONE_LABEL], g_eZones[zoneId][ZONE_TYPE],
				szBonusTag, pointScore, g_flScoreA, g_flScoreB);

			server_print("[Asc-SCORE] Zone%c: 占领! %dT +%.1f -> A=%.1f B=%.1f",
				'A' + g_eZones[zoneId][ZONE_LABEL], iCount, pointScore, g_flScoreA, g_flScoreB);
		} else {
			g_eZones[zoneId][ZONE_STATUS] = 0;
			g_eZones[zoneId][ZONE_CAPTURE_TIME] = 0.0;
			g_eZones[zoneId][ZONE_PLAYER_COUNT] = 0;
		}
	}

	// ★ 观察窗口结束后，按趋势选择主点（如果已经锁定则跳过）
	if (!g_bPointScapMainChosenMsg && g_flPointScapMainWindowLeft <= 0.0 && g_iPointScapMainState == 0) {
		new iSecond, iLeader;
		new Float:flMax, Float:flSecond;
		iLeader = pointscap_get_main_leader(iSecond, flMax, flSecond);

		if (iLeader < 0 || flMax <= 0.0) {
			g_iPointScapMainState = -1;
			if (!g_bPointScapMainConflictMsg) {
				g_bPointScapMainConflictMsg = true;
				chat_print(0, "[Ascension] 本回合未能判定主点（无明显主攻点），取消主点额外加分.");
			}
		} else if (flSecond >= flMax) {
			g_iPointScapMainState = -1;
			if (!g_bPointScapMainConflictMsg) {
				g_bPointScapMainConflictMsg = true;
				chat_print(0, "[Ascension] 本回合主点冲突（多个点位强度相近），取消主点额外加分.");
			}
		} else {
			g_iPointScapMainZone = iLeader;
			g_iPointScapMainState = 1;
			g_bPointScapMainChosenMsg = true;
			chat_print(0, "[Ascension] 本回合主点确定为 %c，额外加分 %.1f.",
				'A' + g_eZones[g_iPointScapMainZone][ZONE_LABEL], g_flPointScapMainBonus);
		}
	}
	
	if (g_flScoreA >= float(g_iPointScapTargetScore)) {
		remove_all_tasks();
		ascensionFinished(1);
	} else if (g_flScoreB >= float(g_iPointScapTargetScore)) {
		remove_all_tasks();
		ascensionFinished(2);
	}
}

// ============================================
// 刀杀计时
// ============================================
public taskAscensionKnife() {
	if (g_eMatchState != STATE_ENABLED) {
		if (task_exists(TASK_POINTSCAP_KNIFE)) remove_task(TASK_POINTSCAP_KNIFE);
		return;
	}
	
	g_flPointScapKnifeTime -= 1.0;
	
	if (g_flPointScapKnifeTime <= 0.0) {
		if (task_exists(TASK_POINTSCAP_KNIFE)) remove_task(TASK_POINTSCAP_KNIFE);
	}
}

// ============================================
// HUD显示（每1秒）
// ============================================
public taskAscensionHud() {
	if (g_eMatchState != STATE_ENABLED) {
		if (task_exists(TASK_POINTSCAP_HUD)) remove_task(TASK_POINTSCAP_HUD);
		return;
	}
	
	draw_zone_boxes();
	
	// 构建区域状态
	new szZones[192] = "";
	new iZoneCount_show = g_iZoneCount;
	if (iZoneCount_show > 5) iZoneCount_show = 5;
	
	for (new i = 0; i < iZoneCount_show; i++) {
		new szZone[32];
		new cLabel = 'A' + g_eZones[i][ZONE_LABEL];
		
		if (g_eZones[i][ZONE_CAPTURED]) {
			// ★ 已占领，显示 ✓
			format(szZone, charsmax(szZone), "%c:✓", cLabel);
		} else if (g_eZones[i][ZONE_STATUS] >= 1) {
			new iType = g_eZones[i][ZONE_TYPE];
			new Float:fScore = (iType >= 5) ? g_flPointScapScore5 :
				((iType == 4) ? g_flPointScapScore4 : g_flPointScapScore3);
			format(szZone, charsmax(szZone), "%c:+%.1f", cLabel, fScore);
		} else {
			format(szZone, charsmax(szZone), "%c:--", cLabel);
		}
		
		if (i > 0) add(szZones, charsmax(szZones), "  ");
		add(szZones, charsmax(szZones), szZone);
	}
	
	// 分数显示
	new szScore[64];
	format(szScore, charsmax(szScore), "A: %.1f  |  B: %.1f  |  目标: %d", 
		g_flScoreA, g_flScoreB, g_iPointScapTargetScore);
	
	// 合并显示
	new szFullHUD[256];
	new szMain[64];
	if (g_iPointScapMainState == -1) {
		formatex(szMain, charsmax(szMain), "主点: 冲突/无加成");
	} else if (g_iPointScapMainZone >= 0) {
		formatex(szMain, charsmax(szMain), "主点: %c +%.1f", 'A' + g_eZones[g_iPointScapMainZone][ZONE_LABEL], g_flPointScapMainBonus);
	} else {
		new iSecond, iLeader;
		new Float:flMax, Float:flSecond;
		iLeader = pointscap_get_main_leader(iSecond, flMax, flSecond);
		if (iLeader >= 0 && flMax > 0.0 && g_flPointScapMainWindowLeft > 0.0) {
			formatex(szMain, charsmax(szMain), "主点候选: %c", 'A' + g_eZones[iLeader][ZONE_LABEL]);
		} else {
			formatex(szMain, charsmax(szMain), "主点: 等待判定");
		}
	}
	if (g_iZoneCount == 0) {
		format(szFullHUD, charsmax(szFullHUD), "%s^n^n[!] 无点位! 用 /creatzone 创建", szScore);
	} else {
		format(szFullHUD, charsmax(szFullHUD), "%s^n%s^n%s^n主点窗口: %.0f秒  回合占点: %d  剩余: %.0f秒", 
			szScore, szZones, szMain, g_flPointScapMainWindowLeft, g_iPointScapRoundCaptures, g_flPointScapDetectTime);
	}
	
	set_hudmessage(0, 200, 220, -1.0, 0.06, 0, 0.0, 1.5, 0.1, 0.0, -1);
	show_hudmessage(0, szFullHUD);
}

// ============================================
// 回合结束
// ============================================
public ascension_roundend(bool:win_ct) {
	if (g_eMatchState != STATE_ENABLED) return;
	
	g_eMatchState = STATE_PREPARE;
	remove_all_tasks();

	// ★ 综合积分：生存加分（回合结算一次，不刷屏）
	if (g_flPointScapSurviveScore > 0.0) {
		new iPlayers[MAX_PLAYERS], iNum;

		get_players(iPlayers, iNum, "ae", "TERRORIST");
		new iAliveT = iNum;
		get_players(iPlayers, iNum, "ae", "CT");
		new iAliveCT = iNum;

		new HNS_TEAM:iTeamT = g_isTeamTT;
		new HNS_TEAM:iTeamCT = HNS_TEAM:!g_isTeamTT;

		new Float:flAddT = g_flPointScapSurviveScore * float(iAliveT);
		new Float:flAddCT = g_flPointScapSurviveScore * float(iAliveCT);

		pointscap_add_hns_team_score(iTeamT, flAddT);
		pointscap_add_hns_team_score(iTeamCT, flAddCT);

		if (flAddT > 0.0 || flAddCT > 0.0) {
			chat_print(0, "[Ascension] 生存加分: T侧 +%.2f | CT侧 +%.2f", flAddT, flAddCT);
		}
	}

	// 检查目标分（包含生存/击杀/占点的综合得分）
	if (g_flScoreA >= float(g_iPointScapTargetScore)) {
		ascensionFinished(1);
		return;
	} else if (g_flScoreB >= float(g_iPointScapTargetScore)) {
		ascensionFinished(2);
		return;
	}

	if (g_iPointScapRoundCaptures <= 0 && g_flScorePreRound[0] == g_flScoreA && g_flScorePreRound[1] == g_flScoreB) {
		chat_print(0, "[Ascension] 本回合无人完成有效占点，判定为平回合.");
	}

	g_iPointScapMainZone = -1;
	g_iPointScapMainState = 0;
	g_flPointScapMainWindowLeft = 0.0;
	
	hns_swap_teams();
	ExecuteForward(g_hForwards[HNS_ROUND_END], _);
}

// ============================================
// 暂停/恢复
// ============================================
public ascension_pause() {
	if (g_eMatchState == STATE_PAUSED) return;
	remove_all_tasks();
	g_eMatchState = STATE_PAUSED;
	ChangeGameplay(GAMEPLAY_TRAINING);
	set_pause_settings();
}

public ascension_unpause() {
	if (g_eMatchState != STATE_PAUSED) return;
	
	// ★ 恢复回合开始时的分数
	g_flScoreA = g_flScorePreRound[0];
	g_flScoreB = g_flScorePreRound[1];
	
	g_eMatchState = STATE_PREPARE;
	hns_restart_round(1.0);
	g_eMatchInfo[e_mTeamSize] = get_num_players_in_match();
	ChangeGameplay(GAMEPLAY_HNS);
	set_unpause_settings();
}

// ============================================
// 换边
// ============================================
public ascension_swap() {
	g_isTeamTT = HNS_TEAM:!g_isTeamTT;
	
	// ★ 交换双方分数（换边后分数跟队伍走，不跟角色走）
	new Float:flTmp = g_flScoreA;
	g_flScoreA = g_flScoreB;
	g_flScoreB = flTmp;
	
	for (new i = 0; i < g_iZoneCount; i++) {
		g_eZones[i][ZONE_STATUS] = 0;
		g_eZones[i][ZONE_CAPTURE_TIME] = 0.0;
		g_eZones[i][ZONE_PLAYER_COUNT] = 0;
	}
	ResetAfkData();
}

public ascension_restartround() {
	remove_all_tasks();
	if (g_eMatchState == STATE_ENABLED)
		g_eMatchState = STATE_PREPARE;
	
	// ★ Save current scores before restart so they persist
	g_flScorePreRound[0] = g_flScoreA;
	g_flScorePreRound[1] = g_flScoreB;
}

// ============================================
// 击杀事件
// ============================================
public ascension_killed(victim, killer) {
	if (g_eMatchState != STATE_ENABLED) return;

	if (!is_user_connected(killer) || killer == victim) return;

	new TeamName:tVictim = getUserTeam(victim);
	new TeamName:tKiller = getUserTeam(killer);
	if ((tVictim != TEAM_TERRORIST && tVictim != TEAM_CT) || (tKiller != TEAM_TERRORIST && tKiller != TEAM_CT)) {
		return;
	}

	// ★ 综合积分：击杀加分（默认很小，避免击杀压过占点）
	if (g_flPointScapKillScore > 0.0) {
		new HNS_TEAM:iKillTeam = pointscap_get_hns_team_by_cs_team(tKiller);
		pointscap_add_hns_team_score(iKillTeam, g_flPointScapKillScore);
	}

	// ★ 击杀可能导致直接达标
	if (g_flScoreA >= float(g_iPointScapTargetScore)) {
		remove_all_tasks();
		ascensionFinished(1);
		return;
	} else if (g_flScoreB >= float(g_iPointScapTargetScore)) {
		remove_all_tasks();
		ascensionFinished(2);
		return;
	}
	
	new iTPlayers[MAX_PLAYERS], iTNum;
	get_players(iTPlayers, iTNum, "ae", "TERRORIST");
	
	if (iTNum == 0) {
		client_cmd(0, "spk ambience/thunder_clap.wav");
		ExecuteForward(g_hForwards[MATCH_RESET_ROUND], _);
	}
}

public ascension_falldamage(id, Float:flDmg) {
	return;
}

// ============================================
// 玩家进出
// ============================================
public ascension_player_join(id) {
	if (g_eMatchInfo[e_tLeaveData] != Invalid_Trie) {
		TrieGetArray(g_eMatchInfo[e_tLeaveData], getUserKey(id), g_ePlayerInfo[id], PLAYER_INFO);
	}
	
	if (g_ePlayerInfo[id][PLAYER_MATCH]) {
		new iNum = get_num_players_in_match(id);
		new bool:bReplaced = iNum >= g_eMatchInfo[e_mTeamSize] ? true : false;
		
		ExecuteForward(g_hForwards[MATCH_JOIN_PLAYER], _, id, bReplaced);
		
		if (bReplaced) {
			transferUserToSpec(id);
			return;
		}
		
		new iMatchRounds = g_eMatchInfo[e_iSidesRounds][HNS_TEAM_A] + g_eMatchInfo[e_iSidesRounds][HNS_TEAM_B];
		if (iMatchRounds == g_ePlayerInfo[id][LEAVE_IN_ROUND])
			rg_set_user_team(id, g_ePlayerInfo[id][PLAYER_TEAM][0] == 'T' ? TEAM_TERRORIST : TEAM_CT);
		else
			rg_set_user_team(id, g_ePlayerInfo[id][PLAYER_TEAM][0] == 'T' ? TEAM_CT : TEAM_TERRORIST);
		
		if (g_eMatchState == STATE_PAUSED)
			rg_round_respawn(id);
	} else {
		transferUserToSpec(id);
	}
}

public ascension_player_leave(id) {
	if (g_ePlayerInfo[id][PLAYER_MATCH]) {
		new iMatchRounds = g_eMatchInfo[e_iSidesRounds][HNS_TEAM_A] + g_eMatchInfo[e_iSidesRounds][HNS_TEAM_B];
		g_ePlayerInfo[id][LEAVE_IN_ROUND] = iMatchRounds;
	}
	ExecuteForward(g_hForwards[MATCH_LEAVE_PLAYER], _, id);
	TrieSetArray(g_eMatchInfo[e_tLeaveData], getUserKey(id), g_ePlayerInfo[id], PLAYER_INFO);
	arrayset(g_ePlayerInfo[id], 0, PLAYER_INFO);
}

// ============================================
// 比赛结束
// ============================================
stock ascensionFinished(iWinTeam) {
	ExecuteForward(g_hForwards[MATCH_FINISH], _, iWinTeam);
	
	new szWinner[16];
	format(szWinner, charsmax(szWinner), iWinTeam == 1 ? "Team A" : "Team B");
	
	setTaskHud(0, 0.5, 1, 255, 255, 255, 5.0, "[Ascension] %s 获胜! A: %.1f | B: %.1f", 
		szWinner, g_flScoreA, g_flScoreB);
	
	match_reset_data();
	training_start();
	ExecuteForward(g_hForwards[MATCH_FINISH_POST], _, iWinTeam);
}

// ============================================
// 绘制区域框
// ============================================
stock draw_zone_boxes() {
	server_print("[Ascension] draw_zone_boxes: count=%d iBeam=%d g_sprBeam=%d", g_iZoneCount, iBeam, g_sprBeam);
	for (new i = 0; i < g_iZoneCount; i++) {
		if (!g_eZones[i][ZONE_ENABLED]) continue;
		
		new Float:fMins[3], Float:fMaxs[3];
		for (new k = 0; k < 3; k++) {
			fMins[k] = g_eZones[i][ZONE_MINS][k];
			fMaxs[k] = g_eZones[i][ZONE_MAXS][k];
		}
		
		new r, g, b;
		if (g_eZones[i][ZONE_CAPTURED]) {
			r = 255; g = 215; b = 0;   // ★ 金色 = 已占领
		} else if (g_eZones[i][ZONE_STATUS] >= 1) {
			r = 0; g = 255; b = 0;     // 绿色 = 有人在
		} else {
			r = 255; g = 50; b = 50;   // 红色 = 空
		}
		
		// 底面
		draw_beam_line(fMins[0], fMins[1], fMins[2], fMaxs[0], fMins[1], fMins[2], r, g, b);
		draw_beam_line(fMaxs[0], fMins[1], fMins[2], fMaxs[0], fMaxs[1], fMins[2], r, g, b);
		draw_beam_line(fMaxs[0], fMaxs[1], fMins[2], fMins[0], fMaxs[1], fMins[2], r, g, b);
		draw_beam_line(fMins[0], fMaxs[1], fMins[2], fMins[0], fMins[1], fMins[2], r, g, b);
		// 顶面
		draw_beam_line(fMins[0], fMins[1], fMaxs[2], fMaxs[0], fMins[1], fMaxs[2], r, g, b);
		draw_beam_line(fMaxs[0], fMins[1], fMaxs[2], fMaxs[0], fMaxs[1], fMaxs[2], r, g, b);
		draw_beam_line(fMaxs[0], fMaxs[1], fMaxs[2], fMins[0], fMaxs[1], fMaxs[2], r, g, b);
		draw_beam_line(fMins[0], fMaxs[1], fMaxs[2], fMins[0], fMins[1], fMaxs[2], r, g, b);
		// 竖直
		draw_beam_line(fMins[0], fMins[1], fMins[2], fMins[0], fMins[1], fMaxs[2], r, g, b);
		draw_beam_line(fMaxs[0], fMins[1], fMins[2], fMaxs[0], fMins[1], fMaxs[2], r, g, b);
		draw_beam_line(fMaxs[0], fMaxs[1], fMins[2], fMaxs[0], fMaxs[1], fMaxs[2], r, g, b);
		draw_beam_line(fMins[0], fMaxs[1], fMins[2], fMins[0], fMaxs[1], fMaxs[2], r, g, b);
	}
}

stock draw_beam_line(Float:x1, Float:y1, Float:z1, Float:x2, Float:y2, Float:z2, r, g, b) {
	new iSprite = (iBeam > 0) ? iBeam : g_sprBeam;
	if (iSprite <= 0) {
		server_print("[Ascension] Cannot draw zone line: no beam sprite available");
		return;
	}

	message_begin(MSG_ALL, SVC_TEMPENTITY);
	write_byte(TE_BEAMPOINTS);
	write_coord(floatround(x1));
	write_coord(floatround(y1));
	write_coord(floatround(z1));
	write_coord(floatround(x2));
	write_coord(floatround(y2));
	write_coord(floatround(z2));
	write_short(iSprite);
	write_byte(1);    // framestart
	write_byte(10);   // framerate
	write_byte(30);   // ★ life in 0.1s = 3.0秒（不闪烁）
	write_byte(5);    // width
	write_byte(0);    // noise
	write_byte(r);    // r
	write_byte(g);    // g
	write_byte(b);    // b
	write_byte(255);  // brightness
	write_byte(0);    // speed
	message_end();
}

// ============================================
// 清理所有任务
// ============================================
stock remove_all_tasks() {
	if (task_exists(TASK_POINTSCAP_DETECT)) remove_task(TASK_POINTSCAP_DETECT);
	if (task_exists(TASK_POINTSCAP_KNIFE)) remove_task(TASK_POINTSCAP_KNIFE);
	if (task_exists(TASK_POINTSCAP_HUD)) remove_task(TASK_POINTSCAP_HUD);
	if (task_exists(TASK_POINTSCAP_FALLBACK)) remove_task(TASK_POINTSCAP_FALLBACK);
	if (task_exists(TASK_POINTSCAP_FORCE)) remove_task(TASK_POINTSCAP_FORCE);
}

// ============================================
// ★ 点位分数设置已移到主菜单 → 比赛设置 → 点位分数配置
// ============================================
