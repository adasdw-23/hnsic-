/*
 * HNS Match Admin Skins - 管理员专属皮肤系统
 * 
 * 功能:
 * - 密码验证 (/linna -> 890514)
 * - 管理员专属 T/CT 皮肤
 * - 管理员专属刀皮
 * - PDS 存档记住选择
 * 
 * 命令:
 * /linna, /adminskins, /as - 打开管理员皮肤菜单（需密码验证）
 */

#include <amxmodx>
#include <amxmisc>
#include <reapi>
#define m_szViewModel (m_szModel + 128)
#include <PersistentDataStorage>

#define PLUGIN_NAME "HNS Admin Skins"
#define PLUGIN_VERSION "4.1.5"
#define PLUGIN_AUTHOR "HNS Match System"

#define MAX_AUTHID_LENGTH 64
#define MAX_MODEL_NAME 64
#define MAX_MODELS_PER_TEAM 32
#define MAX_KNIFE_MODELS 16
#define ADMIN_PASSWORD "890514"

#define Invalid_Array -1

// 验证状态
new bool:g_bAdminVerified[MAX_PLAYERS + 1];
new g_iVerifyStep[MAX_PLAYERS + 1]; // 0=未开始, 1=等待密码

// 管理员皮肤数组
new Array:g_aAdminTModels;
new Array:g_aAdminTModelNames;
new Array:g_aAdminCTModels;
new Array:g_aAdminCTModelNames;
new Array:g_aAdminKnifeModels;
new Array:g_aAdminKnifeModelNames;

// 玩家选择
new g_iAdminTModel[MAX_PLAYERS + 1] = {-1, ...};
new g_iAdminCTModel[MAX_PLAYERS + 1] = {-1, ...};
new g_iAdminKnifeModel[MAX_PLAYERS + 1] = {-1, ...};

// 临时变量
new g_iAdminSelectType[MAX_PLAYERS + 1]; // 0=T, 1=CT, 2=Knife
new g_iAdminSelectPage[MAX_PLAYERS + 1];

public plugin_precache() {
	admin_skins_init();
	admin_skins_precache();
}

public plugin_init() {
	register_plugin(PLUGIN_NAME, PLUGIN_VERSION, PLUGIN_AUTHOR);
	
	// 注册命令
	register_clcmd("say /linna", "cmdAdminSkinVerify");
	register_clcmd("say /adminskins", "cmdAdminSkinVerify");
	register_clcmd("say /as", "cmdAdminSkinVerify");
	register_clcmd("say_team /linna", "cmdAdminSkinVerify");
	register_clcmd("say_team /adminskins", "cmdAdminSkinVerify");
	register_clcmd("say_team /as", "cmdAdminSkinVerify");
	
	// 注册菜单
	register_menucmd(register_menuid("Admin Skin Menu"), 1023, "handleAdminSkinMenu");
	register_menucmd(register_menuid("Admin Select Model"), 1023, "handleAdminSelectMenu");
	
	// 注册事件
	RegisterHookChain(RG_CBasePlayer_Spawn, "OnPlayerSpawn", true);
}

public plugin_end() {
	admin_skins_cleanup();
}

public client_putinserver(id) {
	g_bAdminVerified[id] = false;
	g_iVerifyStep[id] = 0;
	g_iAdminTModel[id] = -1;
	g_iAdminCTModel[id] = -1;
	g_iAdminKnifeModel[id] = -1;
	
	admin_skins_load(id);
}

public client_disconnected(id) {
	admin_skins_save(id);
}

// ==================== 命令处理 ====================
public cmdAdminSkinVerify(id) {
	if (!is_user_connected(id)) {
		return PLUGIN_CONTINUE;
	}
	
	// 检查是否已验证
	if (g_bAdminVerified[id]) {
		showAdminSkinMenu(id);
		return PLUGIN_HANDLED;
	}
	
	// 开始验证流程
	g_iVerifyStep[id] = 1;
	client_print(id, print_chat, "[管理员皮肤] 请在聊天框输入密码解锁...");
	client_print(id, print_center, "请输入管理员密码");
	
	return PLUGIN_HANDLED;
}

// 拦截聊天消息检查密码
public client_command(id) {
	if (!is_user_connected(id) || g_iVerifyStep[id] != 1) {
		return PLUGIN_CONTINUE;
	}
	
	new szArgs[32];
	read_args(szArgs, charsmax(szArgs));
	remove_quotes(szArgs);
	trim(szArgs);
	
	if (equal(szArgs, ADMIN_PASSWORD)) {
		g_bAdminVerified[id] = true;
		g_iVerifyStep[id] = 0;
		client_print(id, print_chat, "[管理员皮肤] 密码正确！正在打开菜单...");
		showAdminSkinMenu(id);
		return PLUGIN_HANDLED; // 阻止密码显示在聊天
	} else {
		g_iVerifyStep[id] = 0;
		client_print(id, print_chat, "[管理员皮肤] 密码错误！");
		return PLUGIN_HANDLED;
	}
}

// ==================== 初始化 ====================
stock admin_skins_init() {
	g_aAdminTModels = ArrayCreate(MAX_MODEL_NAME, 1);
	g_aAdminTModelNames = ArrayCreate(MAX_MODEL_NAME, 1);
	g_aAdminCTModels = ArrayCreate(MAX_MODEL_NAME, 1);
	g_aAdminCTModelNames = ArrayCreate(MAX_MODEL_NAME, 1);
	g_aAdminKnifeModels = ArrayCreate(MAX_MODEL_NAME, 1);
	g_aAdminKnifeModelNames = ArrayCreate(MAX_MODEL_NAME, 1);
	
	// 加载配置
	admin_skins_load_config();
}

stock admin_skins_load_config() {
	new szPath[256];
	get_localinfo("amxx_configsdir", szPath, charsmax(szPath));
	format(szPath, charsmax(szPath), "%s/mixsystem/admin_models.ini", szPath);
	
	new f = fopen(szPath, "rt");
	if (!f) {
		log_amx("[AdminSkins] 配置文件不存在: %s", szPath);
		return;
	}
	
	new szLine[256], szKey[64], szValue[64];
	new bool:bInT = false, bool:bInCT = false, bool:bInKnife = false;
	
	while (!feof(f)) {
		fgets(f, szLine, charsmax(szLine));
		trim(szLine);
		
		if (szLine[0] == ';' || szLine[0] == '/' && szLine[1] == '/' || szLine[0] == 0)
			continue;
		
		if (szLine[0] == '[') {
			new len = strlen(szLine);
			if (szLine[len-1] == ']')
				szLine[--len] = 0;
			if (szLine[0] == '[')
				copy(szLine, charsmax(szLine), szLine[1]);
			
			if (equali(szLine, "Terrorist") || equali(szLine, "T") || equali(szLine, "TT")) {
				bInT = true;
				bInCT = false;
				bInKnife = false;
			} else if (equali(szLine, "Counter-Terrorist") || equali(szLine, "CT") || equali(szLine, "CounterTerrorist")) {
				bInCT = true;
				bInT = false;
				bInKnife = false;
			} else if (equali(szLine, "Knife") || equali(szLine, "Knives")) {
				bInKnife = true;
				bInT = false;
				bInCT = false;
			} else {
				bInT = false;
				bInCT = false;
				bInKnife = false;
			}
			continue;
		}
		
		// 解析 "显示名称 models/player/xxx/xxx.mdl"
		// 用空格分隔：第一个空格前是名称，后面是完整路径
		new iSpacePos = contain(szLine, " ");
		if (iSpacePos <= 0) continue;
		
		copy(szKey, iSpacePos + 1, szLine);
		copy(szValue, charsmax(szValue), szLine[iSpacePos + 1]);
		trim(szKey);
		trim(szValue);
		
		if (szKey[0] == 0 || szValue[0] == 0) continue;
		
		// 从完整路径提取模型文件夹名
		// models/player/xxx/xxx.mdl -> xxx
		// models/xxx/v_knife.mdl -> xxx
		new szFolder[MAX_MODEL_NAME];
		new iLastSlash = 0;
		new i, len = strlen(szValue);
		for (i = 0; i < len; i++) {
			if (szValue[i] == '/' || szValue[i] == 92)
				iLastSlash = i;
		}
		
		if (iLastSlash > 0) {
			// 从最后一个斜杠后开始，去掉文件名部分
			new szTemp[MAX_MODEL_NAME];
			copy(szTemp, charsmax(szTemp), szValue[iLastSlash + 1]);
			
			// 检查是否是刀模型 (v_knife.mdl)
			if (contain(szTemp, "v_knife") >= 0) {
				// 刀模型：文件夹就是最后一个斜杠前的部分
				copy(szFolder, charsmax(szFolder), szValue);
				szFolder[iLastSlash] = 0;
				new iPrevSlash = 0;
				for (i = 0; i < iLastSlash; i++) {
					if (szFolder[i] == '/' || szFolder[i] == 92)
						iPrevSlash = i;
				}
				if (iPrevSlash > 0)
					copy(szFolder, charsmax(szFolder), szFolder[iPrevSlash + 1]);
			} else {
				// 身体模型：去掉 .mdl 后缀就是文件夹名
				new iExt = contain(szTemp, ".mdl");
				if (iExt > 0)
					szTemp[iExt] = 0;
				copy(szFolder, charsmax(szFolder), szTemp);
			}
		} else {
			copy(szFolder, charsmax(szFolder), szValue);
		}
		
		if (bInT) {
			ArrayPushString(g_aAdminTModels, szValue); // 存完整路径
			ArrayPushString(g_aAdminTModelNames, szKey);
		} else if (bInCT) {
			ArrayPushString(g_aAdminCTModels, szValue); // 存完整路径
			ArrayPushString(g_aAdminCTModelNames, szKey);
		} else if (bInKnife) {
			ArrayPushString(g_aAdminKnifeModels, szValue); // 存完整路径
			ArrayPushString(g_aAdminKnifeModelNames, szKey);
		}
	}
	fclose(f);
	
	log_amx("[AdminSkins] 加载了 %d 个T模型, %d 个CT模型, %d 个刀皮", 
		ArraySize(g_aAdminTModels), ArraySize(g_aAdminCTModels), ArraySize(g_aAdminKnifeModels));
}

// ==================== 预缓存 ====================
stock admin_skins_precache() {
	new szModel[MAX_MODEL_NAME * 2];
	new i, iSize;
	
	// T 模型（直接用完整路径）
	iSize = ArraySize(g_aAdminTModels);
	for (i = 0; i < iSize; i++) {
		ArrayGetString(g_aAdminTModels, i, szModel, charsmax(szModel));
		precache_model(szModel);
	}
	
	// CT 模型（直接用完整路径）
	iSize = ArraySize(g_aAdminCTModels);
	for (i = 0; i < iSize; i++) {
		ArrayGetString(g_aAdminCTModels, i, szModel, charsmax(szModel));
		precache_model(szModel);
	}
	
	// 刀模型（直接用完整路径）
	iSize = ArraySize(g_aAdminKnifeModels);
	for (i = 0; i < iSize; i++) {
		ArrayGetString(g_aAdminKnifeModels, i, szModel, charsmax(szModel));
		precache_model(szModel);
	}
}

// ==================== 存档/读档 ====================
stock admin_skins_save(id) {
	if (!is_user_connected(id)) return;
	
	new szAuth[MAX_AUTHID_LENGTH];
	get_user_authid(id, szAuth, charsmax(szAuth));
	
	new szKey[64];
	format(szKey, charsmax(szKey), "hns_admin_t_%s", szAuth);
	PDS_SetCell(szKey, g_iAdminTModel[id]);
	
	format(szKey, charsmax(szKey), "hns_admin_ct_%s", szAuth);
	PDS_SetCell(szKey, g_iAdminCTModel[id]);
	
	format(szKey, charsmax(szKey), "hns_admin_knife_%s", szAuth);
	PDS_SetCell(szKey, g_iAdminKnifeModel[id]);
}

stock admin_skins_load(id) {
	if (!is_user_connected(id)) return;
	
	new szAuth[MAX_AUTHID_LENGTH];
	get_user_authid(id, szAuth, charsmax(szAuth));
	
	new szKey[64], data;
	format(szKey, charsmax(szKey), "hns_admin_t_%s", szAuth);
	if (PDS_GetCell(szKey, data))
		g_iAdminTModel[id] = data;
	
	format(szKey, charsmax(szKey), "hns_admin_ct_%s", szAuth);
	if (PDS_GetCell(szKey, data))
		g_iAdminCTModel[id] = data;
	
	format(szKey, charsmax(szKey), "hns_admin_knife_%s", szAuth);
	if (PDS_GetCell(szKey, data))
		g_iAdminKnifeModel[id] = data;
}

// ==================== 应用模型 ====================
// 从完整路径提取模型文件夹名
// models/player/xxx/xxx.mdl -> xxx
// models/xxx/v_knife.mdl -> xxx
stock extract_folder_from_path(const szPath[], szFolder[], iLen) {
	new iLastSlash = 0;
	new i, len = strlen(szPath);
	for (i = 0; i < len; i++) {
		if (szPath[i] == '/' || szPath[i] == 92)
			iLastSlash = i;
	}
	
	if (iLastSlash <= 0) {
		copy(szFolder, iLen, szPath);
		return;
	}
	
	new szTemp[MAX_MODEL_NAME * 2];
	copy(szTemp, charsmax(szTemp), szPath[iLastSlash + 1]);
	
	// 刀模型：文件夹是倒数第二个斜杠后的部分
	if (contain(szTemp, "v_knife") >= 0) {
		new szDir[MAX_MODEL_NAME * 2];
		copy(szDir, charsmax(szDir), szPath);
		szDir[iLastSlash] = 0;
		new iPrevSlash = 0;
		for (i = 0; i < iLastSlash; i++) {
			if (szDir[i] == '/' || szDir[i] == 92)
				iPrevSlash = i;
		}
		if (iPrevSlash > 0)
			copy(szFolder, iLen, szDir[iPrevSlash + 1]);
		else
			copy(szFolder, iLen, szDir);
	} else {
		// 身体模型：去掉 .mdl 后缀
		new iExt = contain(szTemp, ".mdl");
		if (iExt > 0)
			szTemp[iExt] = 0;
		copy(szFolder, iLen, szTemp);
	}
}

public OnPlayerSpawn(const id) {
	if (!is_user_alive(id)) return;
	set_task(0.1, "apply_admin_skin_task", id);
}

public apply_admin_skin_task(id) {
	if (!is_user_alive(id)) return;
	admin_skins_apply(id);
}

stock admin_skins_apply(id) {
	if (!is_user_alive(id)) return;
	
	new TeamName:iTeam = get_member(id, m_iTeam);
	
	// 应用身体模型
	if (iTeam == TEAM_TERRORIST && g_iAdminTModel[id] >= 0) {
		new iSize = ArraySize(g_aAdminTModels);
		if (g_iAdminTModel[id] < iSize) {
			new szPath[MAX_MODEL_NAME * 2];
			new szFolder[MAX_MODEL_NAME];
			ArrayGetString(g_aAdminTModels, g_iAdminTModel[id], szPath, charsmax(szPath));
			extract_folder_from_path(szPath, szFolder, charsmax(szFolder));
			rg_set_user_model(id, szFolder);
		}
	} else if (iTeam == TEAM_CT && g_iAdminCTModel[id] >= 0) {
		new iSize = ArraySize(g_aAdminCTModels);
		if (g_iAdminCTModel[id] < iSize) {
			new szPath[MAX_MODEL_NAME * 2];
			new szFolder[MAX_MODEL_NAME];
			ArrayGetString(g_aAdminCTModels, g_iAdminCTModel[id], szPath, charsmax(szPath));
			extract_folder_from_path(szPath, szFolder, charsmax(szFolder));
			rg_set_user_model(id, szFolder);
		}
	}
	
	// 应用刀模型
	if (g_iAdminKnifeModel[id] >= 0) {
		new iSize = ArraySize(g_aAdminKnifeModels);
		if (g_iAdminKnifeModel[id] < iSize) {
			new szPath[MAX_MODEL_NAME * 2];
			ArrayGetString(g_aAdminKnifeModels, g_iAdminKnifeModel[id], szPath, charsmax(szPath));
			set_member(id, m_szViewModel, szPath);
		}
	}
}

// ==================== 菜单 ====================
stock showAdminSkinMenu(id) {
	if (!is_user_connected(id)) return;
	
	new szMenu[512];
	new iLen = 0;
	
	iLen += format(szMenu[iLen], charsmax(szMenu) - iLen, "\r[管理员专属皮肤]^n^n");
	
	// T 皮肤
	new szTModel[MAX_MODEL_NAME];
	if (g_iAdminTModel[id] >= 0 && g_iAdminTModel[id] < ArraySize(g_aAdminTModelNames)) {
		ArrayGetString(g_aAdminTModelNames, g_iAdminTModel[id], szTModel, charsmax(szTModel));
	} else {
		copy(szTModel, charsmax(szTModel), "默认");
	}
	iLen += format(szMenu[iLen], charsmax(szMenu) - iLen, "\w1. T 皮肤: \y%s^n", szTModel);
	
	// CT 皮肤
	new szCTModel[MAX_MODEL_NAME];
	if (g_iAdminCTModel[id] >= 0 && g_iAdminCTModel[id] < ArraySize(g_aAdminCTModelNames)) {
		ArrayGetString(g_aAdminCTModelNames, g_iAdminCTModel[id], szCTModel, charsmax(szCTModel));
	} else {
		copy(szCTModel, charsmax(szCTModel), "默认");
	}
	iLen += format(szMenu[iLen], charsmax(szMenu) - iLen, "\w2. CT 皮肤: \y%s^n", szCTModel);
	
	// 刀皮
	new szKnifeModel[MAX_MODEL_NAME];
	if (g_iAdminKnifeModel[id] >= 0 && g_iAdminKnifeModel[id] < ArraySize(g_aAdminKnifeModelNames)) {
		ArrayGetString(g_aAdminKnifeModelNames, g_iAdminKnifeModel[id], szKnifeModel, charsmax(szKnifeModel));
	} else {
		copy(szKnifeModel, charsmax(szKnifeModel), "默认");
	}
	iLen += format(szMenu[iLen], charsmax(szMenu) - iLen, "\w3. 刀皮肤: \y%s^n", szKnifeModel);
	
	iLen += format(szMenu[iLen], charsmax(szMenu) - iLen, "^n\w4. 全部重置为默认^n");
	iLen += format(szMenu[iLen], charsmax(szMenu) - iLen, "^n\w0. 退出");
	
	show_menu(id, (1<<0)|(1<<1)|(1<<2)|(1<<3)|(1<<9), szMenu, -1, "Admin Skin Menu");
}

public handleAdminSkinMenu(id, key) {
	switch (key) {
		case 0: {
			showAdminSelectMenu(id, 0, 0);  // T
		}
		case 1: {
			showAdminSelectMenu(id, 1, 0);  // CT
		}
		case 2: {
			showAdminSelectMenu(id, 2, 0);  // Knife
		}
		case 3: {
			// 重置全部
			g_iAdminTModel[id] = -1;
			g_iAdminCTModel[id] = -1;
			g_iAdminKnifeModel[id] = -1;
			admin_skins_save(id);
			client_print(id, print_chat, "[管理员皮肤] 已重置为默认");
			showAdminSkinMenu(id);
		}
		case 9: {  } // 退出
	}
	return PLUGIN_HANDLED;
}

stock showAdminSelectMenu(id, type, page) {
	if (!is_user_connected(id)) return;
	
	g_iAdminSelectType[id] = type;
	
	new Array:aModels, Array:aNames, iSize;
	new szTypeName[16];
	
	switch (type) {
		case 0: {
			aModels = g_aAdminTModels;
			aNames = g_aAdminTModelNames;
			copy(szTypeName, charsmax(szTypeName), "T");
		}
		case 1: {
			aModels = g_aAdminCTModels;
			aNames = g_aAdminCTModelNames;
			copy(szTypeName, charsmax(szTypeName), "CT");
		}
		case 2: {
			aModels = g_aAdminKnifeModels;
			aNames = g_aAdminKnifeModelNames;
			copy(szTypeName, charsmax(szTypeName), "刀");
		}
		default: return;
	}
	
	iSize = ArraySize(aModels);
	new iCurrent;
	switch (type) {
		case 0: {
			iCurrent = g_iAdminTModel[id];
		}
		case 1: {
			iCurrent = g_iAdminCTModel[id];
		}
		case 2: {
			iCurrent = g_iAdminKnifeModel[id];
		}
	}
	
	// 分页
	new iPerPage = 7;
	new iTotalPages = (iSize + iPerPage - 1) / iPerPage;
	if (iTotalPages < 1) iTotalPages = 1;
	
	if (page < 0) page = 0;
	if (page >= iTotalPages) page = iTotalPages - 1;
	
	g_iAdminSelectPage[id] = page;
	
	new iStart = page * iPerPage;
	new iEnd = min(iStart + iPerPage, iSize);
	
	new szMenu[1024];
	new iLen = 0;
	
	iLen += format(szMenu[iLen], charsmax(szMenu) - iLen, "\r[管理员%s皮肤] \y%d/%d^n^n", szTypeName, page + 1, iTotalPages);
	
	new iKeys = (1<<9); // 0 = 返回
	new iItemNum = 1;
	
	// 模型列表
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
	
	iLen += format(szMenu[iLen], charsmax(szMenu) - iLen, "^n\w0. 返回");
	
	show_menu(id, iKeys, szMenu, -1, "Admin Select Model");
}

public handleAdminSelectMenu(id, key) {
	if (key == 9) { // 0 = 返回
		showAdminSkinMenu(id);
		return PLUGIN_HANDLED;
	}
	
	new Array:aModels;
	new Array:aNames;
	new iSize;
	
	switch (g_iAdminSelectType[id]) {
		case 0: {
			aModels = g_aAdminTModels;
			aNames = g_aAdminTModelNames;
		}
		case 1: {
			aModels = g_aAdminCTModels;
			aNames = g_aAdminCTModelNames;
		}
		case 2: {
			aModels = g_aAdminKnifeModels;
			aNames = g_aAdminKnifeModelNames;
		}
		default: return PLUGIN_HANDLED;
	}
	
	iSize = ArraySize(aModels);
	new iPerPage = 7;
	
	// 上一页
	if (key == 7) {
		showAdminSelectMenu(id, g_iAdminSelectType[id], g_iAdminSelectPage[id] - 1);
		return PLUGIN_HANDLED;
	}
	
	// 下一页
	if (key == 8) {
		showAdminSelectMenu(id, g_iAdminSelectType[id], g_iAdminSelectPage[id] + 1);
		return PLUGIN_HANDLED;
	}
	
	// 选择模型
	new iModelIndex = g_iAdminSelectPage[id] * iPerPage + key;
	
	if (iModelIndex >= iSize || iModelIndex < 0) {
		showAdminSelectMenu(id, g_iAdminSelectType[id], g_iAdminSelectPage[id]);
		return PLUGIN_HANDLED;
	}
	
	// 保存选择
	switch (g_iAdminSelectType[id]) {
		case 0: {
			g_iAdminTModel[id] = iModelIndex;
		}
		case 1: {
			g_iAdminCTModel[id] = iModelIndex;
		}
		case 2: {
			g_iAdminKnifeModel[id] = iModelIndex;
		}
	}
	
	admin_skins_save(id);
	
	// 获取名称
	new szName[MAX_MODEL_NAME];
	ArrayGetString(aNames, iModelIndex, szName, charsmax(szName));
	
	new szType[8];
	switch (g_iAdminSelectType[id]) {
		case 0: {
			copy(szType, charsmax(szType), "T");
		}
		case 1: {
			copy(szType, charsmax(szType), "CT");
		}
		case 2: {
			copy(szType, charsmax(szType), "刀");
		}
	}
	
	client_print(id, print_chat, "[管理员皮肤] %s 皮肤已设置为: %s", szType, szName);
	
	// 立即应用
	admin_skins_apply(id);
	
	showAdminSkinMenu(id);
	return PLUGIN_HANDLED;
}

// ==================== 清理 ====================
stock admin_skins_cleanup() {
	if (g_aAdminTModels != Invalid_Array) {
		ArrayDestroy(g_aAdminTModels);
		g_aAdminTModels = Invalid_Array;
	}
	if (g_aAdminTModelNames != Invalid_Array) {
		ArrayDestroy(g_aAdminTModelNames);
		g_aAdminTModelNames = Invalid_Array;
	}
	if (g_aAdminCTModels != Invalid_Array) {
		ArrayDestroy(g_aAdminCTModels);
		g_aAdminCTModels = Invalid_Array;
	}
	if (g_aAdminCTModelNames != Invalid_Array) {
		ArrayDestroy(g_aAdminCTModelNames);
		g_aAdminCTModelNames = Invalid_Array;
	}
	if (g_aAdminKnifeModels != Invalid_Array) {
		ArrayDestroy(g_aAdminKnifeModels);
		g_aAdminKnifeModels = Invalid_Array;
	}
	if (g_aAdminKnifeModelNames != Invalid_Array) {
		ArrayDestroy(g_aAdminKnifeModelNames);
		g_aAdminKnifeModelNames = Invalid_Array;
	}
}
