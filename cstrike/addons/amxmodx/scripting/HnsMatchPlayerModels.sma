/*
 * HNS Match Player Models - Independent Skin System
 * 独立皮肤系统插件
 * 
 * 功能:
 * - 玩家可选择自定义 T/CT 模型
 * - 支持 INI 配置文件
 * - PDS 存档记住选择
 * - 独立运行，不依赖主比赛系统
 * 
 * 命令:
 * /skins, /models, /skin, /model - 打开皮肤菜单
 */

#include <amxmodx>
#include <amxmisc>
#include <reapi>
#include <PersistentDataStorage>

#define PLUGIN_NAME "HNS Player Models"
#define PLUGIN_VERSION "4.1.0"
#define PLUGIN_AUTHOR "HNS Match System"

#define MAX_AUTHID_LENGTH 64
#define MAX_MODEL_NAME 64
#define MAX_MODELS_PER_TEAM 32
#define MODEL_MENU_PAGE_SIZE 7
#define Invalid_Array -1

// Available models for T and CT teams
new Array:g_aTModels;
new Array:g_aCTModels;
new Array:g_aTModelNames;  // 显示名称
new Array:g_aCTModelNames; // 显示名称

// Player's selected model index (-1 = default)
new g_iPlayerTModel[MAX_PLAYERS + 1] = {-1, ...};
new g_iPlayerCTModel[MAX_PLAYERS + 1] = {-1, ...};

// Temp variable to track which team's model list is being shown
new g_iModelSelectTeam[MAX_PLAYERS + 1];
new g_iModelSelectPage[MAX_PLAYERS + 1];

public plugin_precache() {
	player_models_init();
	player_models_precache();
}

public plugin_init() {
	register_plugin(PLUGIN_NAME, PLUGIN_VERSION, PLUGIN_AUTHOR);
	
	// 注册命令
	register_clcmd("say /skins", "cmdSkinMenu");
	register_clcmd("say /models", "cmdSkinMenu");
	register_clcmd("say /skin", "cmdSkinMenu");
	register_clcmd("say /model", "cmdSkinMenu");
	register_clcmd("say_team /skins", "cmdSkinMenu");
	register_clcmd("say_team /models", "cmdSkinMenu");
	register_clcmd("say_team /skin", "cmdSkinMenu");
	register_clcmd("say_team /model", "cmdSkinMenu");
	
	// 注册菜单命令
	register_menucmd(register_menuid("Player Models"), 1023, "handlePlayerModelsMenu");
	register_menucmd(register_menuid("Select Model"), 1023, "handleModelSelectMenu");
	
	// 注册事件
	RegisterHookChain(RG_CBasePlayer_Spawn, "OnPlayerSpawn", true);
	
	// 加载已连接玩家的数据
	new iPlayers[MAX_PLAYERS], iNum;
	get_players(iPlayers, iNum, "c");
	for (new i = 0; i < iNum; i++) {
		player_models_load(iPlayers[i]);
	}
}

public plugin_end() {
	player_models_cleanup();
}

public client_putinserver(id) {
	// 重置玩家数据
	g_iPlayerTModel[id] = -1;
	g_iPlayerCTModel[id] = -1;
	g_iModelSelectTeam[id] = 0;
	
	// 加载存档
	player_models_load(id);
}

public client_disconnected(id) {
	// 保存玩家数据
	player_models_save(id);
}

// ==================== 命令 ====================
public cmdSkinMenu(id) {
	if (!is_user_connected(id)) {
		return PLUGIN_CONTINUE;
	}
	
	showPlayerModelsMenu(id);
	return PLUGIN_HANDLED;
}

// ==================== 初始化 ====================
stock player_models_init() {
	g_aTModels = ArrayCreate(MAX_MODEL_NAME, 1);
	g_aCTModels = ArrayCreate(MAX_MODEL_NAME, 1);
	g_aTModelNames = ArrayCreate(MAX_MODEL_NAME, 1);
	g_aCTModelNames = ArrayCreate(MAX_MODEL_NAME, 1);

	// 加载默认 T 模型
	add_default_model(g_aTModels, g_aTModelNames, "arctic", "Arctic");
	add_default_model(g_aTModels, g_aTModelNames, "guerilla", "Guerilla");
	add_default_model(g_aTModels, g_aTModelNames, "leet", "Leet");
	add_default_model(g_aTModels, g_aTModelNames, "terror", "Terror");

	// 加载默认 CT 模型
	add_default_model(g_aCTModels, g_aCTModelNames, "gign", "GIGN");
	add_default_model(g_aCTModels, g_aCTModelNames, "gsg9", "GSG9");
	add_default_model(g_aCTModels, g_aCTModelNames, "sas", "SAS");
	add_default_model(g_aCTModels, g_aCTModelNames, "urban", "Urban");
	// add_default_model(g_aCTModels, g_aCTModelNames, "spetsnaz", "Spetsnaz"); // CS:CZ only, not available in CS 1.6
	add_default_model(g_aCTModels, g_aCTModelNames, "vip", "VIP");

	// 从配置文件加载自定义模型
	player_models_load_config();
}

// ==================== 预缓存模型 ====================
stock player_models_precache() {
	new szModel[MAX_MODEL_NAME];
	new szPrecache[128];
	new i, iSize;
	
	// 预缓存 T 模型
	iSize = ArraySize(g_aTModels);
	for (i = 0; i < iSize; i++) {
		ArrayGetString(g_aTModels, i, szModel, charsmax(szModel));
		formatex(szPrecache, charsmax(szPrecache), "models/player/%s/%s.mdl", szModel, szModel);
		precache_model(szPrecache);
		log_amx("[PlayerModels] Precache: %s", szPrecache);
	}
	
	// 预缓存 CT 模型
	iSize = ArraySize(g_aCTModels);
	for (i = 0; i < iSize; i++) {
		ArrayGetString(g_aCTModels, i, szModel, charsmax(szModel));
		formatex(szPrecache, charsmax(szPrecache), "models/player/%s/%s.mdl", szModel, szModel);
		precache_model(szPrecache);
		log_amx("[PlayerModels] Precache: %s", szPrecache);
	}
}

stock add_default_model(Array:aModels, Array:aNames, const szPath[], const szName[]) {
	ArrayPushString(aModels, szPath);
	ArrayPushString(aNames, szName);
}

stock player_models_load_config() {
	new szPath[256];
	get_localinfo("amxx_configsdir", szPath, charsmax(szPath));
	format(szPath, charsmax(szPath), "%s/mixsystem/player_models.ini", szPath);

	new f = fopen(szPath, "rt");
	if (!f) {
		log_amx("[PlayerModels] 配置文件不存在: %s", szPath);
		return;
	}

	new szLine[256], szKey[64], szValue[64];
	new bool:bInT = false, bool:bInCT = false;

	while (!feof(f)) {
		fgets(f, szLine, charsmax(szLine));
		trim(szLine);

		if (szLine[0] == ';' || szLine[0] == '/' && szLine[1] == '/' || szLine[0] == 0)
			continue;

		if (szLine[0] == '[') {
			// 段头
			new len = strlen(szLine);
			if (szLine[len-1] == ']')
				szLine[--len] = 0;
			if (szLine[0] == '[')
				copy(szLine, charsmax(szLine), szLine[1]);
			
			if (equali(szLine, "Terrorist") || equali(szLine, "T") || equali(szLine, "TT")) {
				bInT = true;
				bInCT = false;
			} else if (equali(szLine, "Counter-Terrorist") || equali(szLine, "CT")) {
				bInCT = true;
				bInT = false;
			} else {
				bInT = false;
				bInCT = false;
			}
			continue;
		}

		// 解析 "显示名称 = 模型文件夹名"
		strtok(szLine, szKey, charsmax(szKey), szValue, charsmax(szValue), '=');
		trim(szKey);
		trim(szValue);

		if (szKey[0] == 0) continue;
		if (szValue[0] == 0) continue; // 没有路径则跳过

		// szKey = 显示名称, szValue = 模型文件夹名
		if (bInT) {
			ArrayPushString(g_aTModels, szValue);
			ArrayPushString(g_aTModelNames, szKey);
		} else if (bInCT) {
			ArrayPushString(g_aCTModels, szValue);
			ArrayPushString(g_aCTModelNames, szKey);
		}
	}
	fclose(f);
	
	log_amx("[PlayerModels] 加载了 %d 个T模型和 %d 个CT模型", 
		ArraySize(g_aTModels), ArraySize(g_aCTModels));
}

// ==================== 存档/读档 ====================
stock player_models_save(id) {
	if (!is_user_connected(id)) return;
	
	new szAuth[MAX_AUTHID_LENGTH];
	get_user_authid(id, szAuth, charsmax(szAuth));

	new szKey[64];
	format(szKey, charsmax(szKey), "hns_model_%s_t", szAuth);
	PDS_SetCell(szKey, g_iPlayerTModel[id]);

	format(szKey, charsmax(szKey), "hns_model_%s_ct", szAuth);
	PDS_SetCell(szKey, g_iPlayerCTModel[id]);
}

stock player_models_load(id) {
	if (!is_user_connected(id)) return;
	
	new szAuth[MAX_AUTHID_LENGTH];
	get_user_authid(id, szAuth, charsmax(szAuth));

	new szKey[64], data;
	format(szKey, charsmax(szKey), "hns_model_%s_t", szAuth);
	if (PDS_GetCell(szKey, data))
		g_iPlayerTModel[id] = data;

	format(szKey, charsmax(szKey), "hns_model_%s_ct", szAuth);
	if (PDS_GetCell(szKey, data))
		g_iPlayerCTModel[id] = data;
}

// ==================== 应用模型 ====================
public OnPlayerSpawn(const id) {
	if (!is_user_alive(id)) return;
	
	// 延迟一帧应用模型，确保出生完成
	set_task(0.1, "apply_model_task", id);
}

public apply_model_task(id) {
	if (!is_user_alive(id)) return;
	player_models_apply(id);
}

stock player_models_apply(id) {
	if (!is_user_alive(id)) return;

	new TeamName:iTeam = get_member(id, m_iTeam);

	if (iTeam == TEAM_TERRORIST && g_iPlayerTModel[id] >= 0) {
		new iSize = ArraySize(g_aTModels);
		if (g_iPlayerTModel[id] < iSize) {
			new szModel[MAX_MODEL_NAME];
			ArrayGetString(g_aTModels, g_iPlayerTModel[id], szModel, charsmax(szModel));
			rg_set_user_model(id, szModel);
		}
	} else if (iTeam == TEAM_CT && g_iPlayerCTModel[id] >= 0) {
		new iSize = ArraySize(g_aCTModels);
		if (g_iPlayerCTModel[id] < iSize) {
			new szModel[MAX_MODEL_NAME];
			ArrayGetString(g_aCTModels, g_iPlayerCTModel[id], szModel, charsmax(szModel));
			rg_set_user_model(id, szModel);
		}
	}
}

stock player_models_reset(id) {
	rg_set_user_model(id, "");
}

// ==================== 菜单 ====================
stock showPlayerModelsMenu(id) {
	if (!is_user_connected(id)) return;
	
	new szMenu[512];
	new iLen = 0;
	
	iLen += format(szMenu[iLen], charsmax(szMenu) - iLen, "\r[HNS] \wPlayer Models^n^n");
	
	// T 模型
	new szTModel[MAX_MODEL_NAME];
	if (g_iPlayerTModel[id] >= 0 && g_iPlayerTModel[id] < ArraySize(g_aTModelNames)) {
		ArrayGetString(g_aTModelNames, g_iPlayerTModel[id], szTModel, charsmax(szTModel));
	} else {
		copy(szTModel, charsmax(szTModel), "Default");
	}
	iLen += format(szMenu[iLen], charsmax(szMenu) - iLen, "\w1. T Model: \y%s^n", szTModel);
	
	// CT 模型
	new szCTModel[MAX_MODEL_NAME];
	if (g_iPlayerCTModel[id] >= 0 && g_iPlayerCTModel[id] < ArraySize(g_aCTModelNames)) {
		ArrayGetString(g_aCTModelNames, g_iPlayerCTModel[id], szCTModel, charsmax(szCTModel));
	} else {
		copy(szCTModel, charsmax(szCTModel), "Default");
	}
	iLen += format(szMenu[iLen], charsmax(szMenu) - iLen, "\w2. CT Model: \y%s^n", szCTModel);
	
	iLen += format(szMenu[iLen], charsmax(szMenu) - iLen, "^n\w3. Reset to Default^n");
	iLen += format(szMenu[iLen], charsmax(szMenu) - iLen, "^n\w0. Exit");
	
	show_menu(id, (1<<0)|(1<<1)|(1<<2)|(1<<9), szMenu, -1, "Player Models");
}

public handlePlayerModelsMenu(id, key) {
	switch (key) {
		case 0: {
			showModelSelectMenu(id, 0);  // T models
		}
		case 1: {
			showModelSelectMenu(id, 1);  // CT models
		}
		case 2: {
			// Reset
			g_iPlayerTModel[id] = -1;
			g_iPlayerCTModel[id] = -1;
			player_models_save(id);
			player_models_reset(id);
			client_print(id, print_chat, "[HNS] 皮肤已重置为默认");
			showPlayerModelsMenu(id);
		}
		case 9: { } // Exit
	}
	return PLUGIN_HANDLED;
}

stock showModelSelectMenu(id, team, page = 0) {
	if (!is_user_connected(id)) return;
	
	g_iModelSelectTeam[id] = team;
	
	new Array:aModels = team == 0 ? g_aTModels : g_aCTModels;
	new Array:aNames = team == 0 ? g_aTModelNames : g_aCTModelNames;
	new iSize = ArraySize(aModels);
	new iCurrent = team == 0 ? g_iPlayerTModel[id] : g_iPlayerCTModel[id];
	
	// 分页计算（每页7个模型 + 1个Default = 8项，但限制7项）
	new iPerPage = 7;
	new iTotalPages = (iSize + iPerPage - 1) / iPerPage;
	if (iTotalPages < 1) iTotalPages = 1;
	
	// 限制页码范围
	if (page < 0) page = 0;
	if (page >= iTotalPages) page = iTotalPages - 1;
	
	g_iModelSelectPage[id] = page;
	
	new iStart = page * iPerPage;
	new iEnd = min(iStart + iPerPage, iSize);
	
	new szMenu[1024];
	new iLen = 0;
	
	iLen += format(szMenu[iLen], charsmax(szMenu) - iLen, "\r[HNS] \wSelect %s Model \y%d/%d^n^n", team == 0 ? "T" : "CT", page + 1, iTotalPages);
	
	// 构建菜单按键位
	new iKeys = (1<<9); // 0 = Back
	new iItemNum = 1;
	
	// Model list
	for (new i = iStart; i < iEnd; i++) {
		new szName[MAX_MODEL_NAME];
		ArrayGetString(aNames, i, szName, charsmax(szName));
		
		if (i == iCurrent) {
			iLen += format(szMenu[iLen], charsmax(szMenu) - iLen, "\y%d. %s^n", iItemNum, szName);
		} else {
			iLen += format(szMenu[iLen], charsmax(szMenu) - iLen, "\w%d. %s^n", iItemNum, szName);
		}
		
		iKeys |= (1 << (iItemNum - 1));
		iItemNum++;
	}
	
	// 翻页按钮
	iLen += format(szMenu[iLen], charsmax(szMenu) - iLen, "^n");
	
	if (page > 0) {
		iLen += format(szMenu[iLen], charsmax(szMenu) - iLen, "\w8. << 上一页^n");
		iKeys |= (1<<7);
	} else {
		iLen += format(szMenu[iLen], charsmax(szMenu) - iLen, "\d8. << 上一页^n");
	}
	
	if (page < iTotalPages - 1) {
		iLen += format(szMenu[iLen], charsmax(szMenu) - iLen, "\w9. 下一页 >>^n");
		iKeys |= (1<<8);
	} else {
		iLen += format(szMenu[iLen], charsmax(szMenu) - iLen, "\d9. 下一页 >>^n");
	}
	
	iLen += format(szMenu[iLen], charsmax(szMenu) - iLen, "^n\w0. Back");
	
	show_menu(id, iKeys, szMenu, -1, "Select Model");
}

public handleModelSelectMenu(id, key) {
	if (key == 9) { // 按键0 = Back
		showPlayerModelsMenu(id);
		return PLUGIN_HANDLED;
	}
	
	new Array:aModels = g_iModelSelectTeam[id] == 0 ? g_aTModels : g_aCTModels;
	new Array:aNames = g_iModelSelectTeam[id] == 0 ? g_aTModelNames : g_aCTModelNames;
	new iSize = ArraySize(aModels);
	new iPerPage = 7;
	
	// 按键8 = 上一页 (key 7)
	if (key == 7) {
		showModelSelectMenu(id, g_iModelSelectTeam[id], g_iModelSelectPage[id] - 1);
		return PLUGIN_HANDLED;
	}
	
	// 按键9 = 下一页 (key 8)
	if (key == 8) {
		showModelSelectMenu(id, g_iModelSelectTeam[id], g_iModelSelectPage[id] + 1);
		return PLUGIN_HANDLED;
	}
	
	// key 0-6 = 选择模型
	new iModelIndex = g_iModelSelectPage[id] * iPerPage + key;
	
	// 检查索引是否有效
	if (iModelIndex >= iSize || iModelIndex < 0) {
		showModelSelectMenu(id, g_iModelSelectTeam[id], g_iModelSelectPage[id]);
		return PLUGIN_HANDLED;
	}
	
	// 保存选择
	if (g_iModelSelectTeam[id] == 0) {
		g_iPlayerTModel[id] = iModelIndex;
	} else {
		g_iPlayerCTModel[id] = iModelIndex;
	}
	
	player_models_save(id);
	
	// 获取模型名称
	new szName[MAX_MODEL_NAME];
	ArrayGetString(aNames, iModelIndex, szName, charsmax(szName));
	
	new szTeam[4];
	copy(szTeam, charsmax(szTeam), g_iModelSelectTeam[id] == 0 ? "T" : "CT");
	client_print(id, print_chat, "[HNS] %s 皮肤已设置为: %s", szTeam, szName);
	
	// 立即应用（如果在正确队伍）
	new TeamName:iTeam = get_member(id, m_iTeam);
	if ((g_iModelSelectTeam[id] == 0 && iTeam == TEAM_TERRORIST) ||
		(g_iModelSelectTeam[id] == 1 && iTeam == TEAM_CT)) {
		player_models_apply(id);
	}
	
	showPlayerModelsMenu(id);
	return PLUGIN_HANDLED;
}

// ==================== 清理 ====================
stock player_models_cleanup() {
	if (g_aTModels != Invalid_Array) {
		ArrayDestroy(g_aTModels);
		g_aTModels = Invalid_Array;
	}
	if (g_aCTModels != Invalid_Array) {
		ArrayDestroy(g_aCTModels);
		g_aCTModels = Invalid_Array;
	}
	if (g_aTModelNames != Invalid_Array) {
		ArrayDestroy(g_aTModelNames);
		g_aTModelNames = Invalid_Array;
	}
	if (g_aCTModelNames != Invalid_Array) {
		ArrayDestroy(g_aCTModelNames);
		g_aCTModelNames = Invalid_Array;
	}
}
