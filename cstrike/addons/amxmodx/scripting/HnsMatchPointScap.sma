// ============================================
// HnsMatchSystem - PointScap Extension
// Version: 3.4.0
// Description: PointScap zone editor for HNS Match System
// ============================================

#include <amxmodx>
#include <amxmisc>
#include <fakemeta>

#pragma semicolon 1

// ============================================
// Constants
// ============================================
#define MAX_ZONES 32
#define PLAYER_HEIGHT 72.0
#define PLUGIN_PREFIX "PointScap"

// Measurement states
enum (<<= 1) {
    MEASURE_NONE = 0,
    MEASURE_STARTED = 1,
    MEASURE_BOTTOM_SET = 2,
    MEASURE_TOP_SET = 4
};

// Zone data enum
enum _:ZONE_DATA {
    ZONE_ENABLED,
    ZONE_LABEL,
    ZONE_TYPE,
    ZONE_STATUS,
    Float:ZONE_MINS[3],
    Float:ZONE_MAXS[3],
    Float:ZONE_CAPTURE_TIME,
    ZONE_OWNER
};

// ============================================
// Global Variables
// ============================================
new Float:g_flScoreA;
new Float:g_flScoreB;
new g_eZones[MAX_ZONES][ZONE_DATA];
new g_iZoneCount;

// Editor variables
new Float:g_fMeasureBottom[33][3];
new Float:g_fMeasureTop[33][3];
new g_iMeasureState[33];
new g_iSelectedZone[33];
new g_iSelectedType[33];

new g_szZoneLabels[][] = {"A", "B", "C", "D", "E", "F", "G", "H", "I", "J"};
new g_szPointTypeNames[][] = {"3-Man Point", "4-Man Point", "5-Man Point"};
new Float:g_fPointScores[] = {0.5, 1.0, 2.0};

new g_sprBeam;

// ============================================
// Plugin Init
// ============================================
public plugin_init() {
    register_plugin("HNS Match PointScap", "4.0.4", "OpenHNS");
    register_cvar("hns_pointscap_version", "4.0.4", FCVAR_SERVER | FCVAR_SPONLY);
    
    // Commands
    register_concmd("pointscap_editor", "cmdPointScapEditor", ADMIN_CFG, "Open PointScap zone editor");
    register_clcmd("say /muin", "cmdPointScapEditor");
    register_clcmd("say_team /muin", "cmdPointScapEditor");
    
    // Register menus
    register_menucmd(register_menuid("PointScap Main Menu"), MENU_KEY_1|MENU_KEY_2|MENU_KEY_3|MENU_KEY_4|MENU_KEY_0, "handleMainMenu");
    register_menucmd(register_menuid("Select Zone Label"), MENU_KEY_1|MENU_KEY_2|MENU_KEY_3|MENU_KEY_4|MENU_KEY_5|MENU_KEY_6|MENU_KEY_7|MENU_KEY_8|MENU_KEY_9|MENU_KEY_0, "handleZoneSelect");
    register_menucmd(register_menuid("Select Point Type"), MENU_KEY_1|MENU_KEY_2|MENU_KEY_3|MENU_KEY_0, "handlePointTypeSelect");
    register_menucmd(register_menuid("Measurement Menu"), MENU_KEY_1|MENU_KEY_2|MENU_KEY_0, "handleMeasurementMenu");
    register_menucmd(register_menuid("View Zones Menu"), MENU_KEY_1|MENU_KEY_2|MENU_KEY_3|MENU_KEY_4|MENU_KEY_5|MENU_KEY_6|MENU_KEY_7|MENU_KEY_8|MENU_KEY_9|MENU_KEY_0, "handleViewZonesMenu");
    register_menucmd(register_menuid("Delete Zone Menu"), MENU_KEY_1|MENU_KEY_2|MENU_KEY_3|MENU_KEY_4|MENU_KEY_5|MENU_KEY_6|MENU_KEY_7|MENU_KEY_8|MENU_KEY_9|MENU_KEY_0, "handleDeleteZoneMenu");
    
    // Events
    register_event("HLTV", "eventRoundStart", "a", "1=0", "2=0");
    register_logevent("logeventRoundEnd", 2, "1=Round_End");
    
    // Load config
    pointscap_load_config();
    
    log_amx("[PointScap] Plugin initialized successfully");
}

public plugin_precache() {
    g_sprBeam = precache_model("sprites/zbeam4.spr");
    precache_sound("buttons/bell1.wav");
    precache_sound("buttons/blip1.wav");
}

// ============================================
// Event Handlers
// ============================================
public eventRoundStart() {
    // Reset scores and zone status
    g_flScoreA = 0.0;
    g_flScoreB = 0.0;
    for (new i = 0; i < g_iZoneCount; i++) {
        g_eZones[i][ZONE_STATUS] = 0;
        g_eZones[i][ZONE_CAPTURE_TIME] = 0.0;
    }
    return PLUGIN_CONTINUE;
}

public logeventRoundEnd() {
    return PLUGIN_CONTINUE;
}

// ============================================
// Main Editor Command
// ============================================
public cmdPointScapEditor(id) {
    if (!is_user_admin(id)) {
        client_print(id, print_chat, "[%s] 没有权限。", PLUGIN_PREFIX);
        return PLUGIN_HANDLED;
    }
    showMainMenu(id);
    return PLUGIN_HANDLED;
}

// ============================================
// Main Menu
// ============================================
showMainMenu(id) {
    new menu[512];
    new len = formatex(menu, charsmax(menu), "\r点位编辑器 (PointScap)^n^n");
    len += formatex(menu[len], charsmax(menu) - len, "\r1.\w 测量新区域^n");
    len += formatex(menu[len], charsmax(menu) - len, "\r2.\w 查看区域^n");
    len += formatex(menu[len], charsmax(menu) - len, "\r3.\w 删除区域^n");
    len += formatex(menu[len], charsmax(menu) - len, "\r4.\w 保存配置^n^n");
    len += formatex(menu[len], charsmax(menu) - len, "\r0.\w 退出");
    show_menu(id, MENU_KEY_1|MENU_KEY_2|MENU_KEY_3|MENU_KEY_4|MENU_KEY_0, menu, -1, "PointScap Main Menu");
}

public handleMainMenu(id, key) {
    if (key == 0) {
        menuSelectZone(id);
    } else if (key == 1) {
        showViewZonesMenu(id);
    } else if (key == 2) {
        showDeleteZoneMenu(id);
    } else if (key == 3) {
        pointscap_save_config();
        client_print(id, print_chat, "[%s] 配置已保存！", PLUGIN_PREFIX);
        showMainMenu(id);
    }
    return PLUGIN_HANDLED;
}

// ============================================
// Zone Selection
// ============================================
menuSelectZone(id) {
    new menu[512];
    new len = formatex(menu, charsmax(menu), "\r选择区域标签^n^n");
    new keys = MENU_KEY_0;
    
    for (new i = 0; i < sizeof(g_szZoneLabels); i++) {
        new bool:exists = false;
        new typeMask = 0;
        for (new j = 0; j < g_iZoneCount; j++) {
            if (g_eZones[j][ZONE_LABEL] == i) {
                exists = true;
                if (g_eZones[j][ZONE_TYPE] >= 3 && g_eZones[j][ZONE_TYPE] <= 5) {
                    typeMask |= (1 << (g_eZones[j][ZONE_TYPE] - 3));
                }
            }
        }
        if (exists) {
            len += formatex(menu[len], charsmax(menu) - len, "\r%d.\w %s区 \y[已有:%s%s%s]^n",
                i + 1,
                g_szZoneLabels[i],
                (typeMask & (1 << 0)) ? "3" : "",
                (typeMask & (1 << 1)) ? "4" : "",
                (typeMask & (1 << 2)) ? "5" : "");
        } else {
            len += formatex(menu[len], charsmax(menu) - len, "\r%d.\w %s区^n", i + 1, g_szZoneLabels[i]);
        }
        keys |= (1 << i);
    }
    len += formatex(menu[len], charsmax(menu) - len, "^n\r0.\w 返回");
    show_menu(id, keys, menu, -1, "Select Zone Label");
}

public handleZoneSelect(id, key) {
    if (key == 9) { showMainMenu(id); return PLUGIN_HANDLED; }
    if (key >= 0 && key < 10) {
        g_iSelectedZone[id] = key;
        g_iMeasureState[id] = MEASURE_STARTED;
        g_fMeasureBottom[id] = Float:{0.0, 0.0, 0.0};
        g_fMeasureTop[id] = Float:{0.0, 0.0, 0.0};
        showMeasurementMenu(id);
    }
    return PLUGIN_HANDLED;
}

// ============================================
// Measurement Menu
// ============================================
showMeasurementMenu(id) {
    new menu[512];
    new len = formatex(menu, charsmax(menu), "\r测量 - %s区^n^n", g_szZoneLabels[g_iSelectedZone[id]]);
    
    if (!(g_iMeasureState[id] & MEASURE_BOTTOM_SET)) {
        len += formatex(menu[len], charsmax(menu) - len, "\r1.\w 设置底部位置^n");
        len += formatex(menu[len], charsmax(menu) - len, "\d2. 设置顶部位置^n");
        len += formatex(menu[len], charsmax(menu) - len, "^n走到区域底部角落按1。");
        show_menu(id, MENU_KEY_1|MENU_KEY_0, menu, -1, "Measurement Menu");
    } else if (!(g_iMeasureState[id] & MEASURE_TOP_SET)) {
        len += formatex(menu[len], charsmax(menu) - len, "\y1.\w 底部 [已设置]^n");
        len += formatex(menu[len], charsmax(menu) - len, "\r2.\w 设置顶部位置^n");
        len += formatex(menu[len], charsmax(menu) - len, "^n走到区域顶部角落按2。");
        show_menu(id, MENU_KEY_2|MENU_KEY_0, menu, -1, "Measurement Menu");
    } else {
        menuSelectPointType(id);
        return;
    }
    len += formatex(menu[len], charsmax(menu) - len, "^n\r0.\w 取消");
}

public handleMeasurementMenu(id, key) {
    switch (key) {
        case 0: {
            if (!(g_iMeasureState[id] & MEASURE_BOTTOM_SET)) {
                if (!is_user_alive(id)) {
                    client_print(id, print_chat, "[%s] 必须活着才能操作！", PLUGIN_PREFIX);
                    showMeasurementMenu(id);
                    return PLUGIN_HANDLED;
                }
                pev(id, pev_origin, g_fMeasureBottom[id]);
                g_iMeasureState[id] |= MEASURE_BOTTOM_SET;
                client_print(id, print_chat, "[%s] Bottom: %.0f %.0f %.0f", PLUGIN_PREFIX,
                    g_fMeasureBottom[id][0], g_fMeasureBottom[id][1], g_fMeasureBottom[id][2]);
                showMeasurementMenu(id);
            }
        }
        case 1: {
            if ((g_iMeasureState[id] & MEASURE_BOTTOM_SET) && !(g_iMeasureState[id] & MEASURE_TOP_SET)) {
                if (!is_user_alive(id)) {
                    client_print(id, print_chat, "[%s] 必须活着才能操作！", PLUGIN_PREFIX);
                    showMeasurementMenu(id);
                    return PLUGIN_HANDLED;
                }
                pev(id, pev_origin, g_fMeasureTop[id]);
                g_iMeasureState[id] |= MEASURE_TOP_SET;
                client_print(id, print_chat, "[%s] Top: %.0f %.0f %.0f", PLUGIN_PREFIX,
                    g_fMeasureTop[id][0], g_fMeasureTop[id][1], g_fMeasureTop[id][2]);
                menuSelectPointType(id);
            }
        }
        case 9: {
            g_iMeasureState[id] = MEASURE_NONE;
            menuSelectZone(id);
        }
    }
    return PLUGIN_HANDLED;
}

// ============================================
// Point Type Selection
// ============================================
menuSelectPointType(id) {
    new menu[256];
    new len = formatex(menu, charsmax(menu), "\r选择类型 - %s区^n^n", g_szZoneLabels[g_iSelectedZone[id]]);
    len += formatex(menu[len], charsmax(menu) - len, "\r1.\w 三人点 (0.5分)^n");
    len += formatex(menu[len], charsmax(menu) - len, "\r2.\w 四人点 (1.0分)^n");
    len += formatex(menu[len], charsmax(menu) - len, "\r3.\w 五人点 (2.0分)^n^n");
    len += formatex(menu[len], charsmax(menu) - len, "\r0.\w 返回");
    show_menu(id, MENU_KEY_1|MENU_KEY_2|MENU_KEY_3|MENU_KEY_0, menu, -1, "Select Point Type");
}

public handlePointTypeSelect(id, key) {
    if (key == 9) {
        g_iMeasureState[id] = MEASURE_STARTED;
        showMeasurementMenu(id);
        return PLUGIN_HANDLED;
    }
    if (key >= 0 && key < 3) {
        g_iSelectedType[id] = key + 3;
        pointscap_save_zone(id);
    }
    return PLUGIN_HANDLED;
}

// ============================================
// Save Zone
// ============================================
pointscap_save_zone(id) {
    new idx = -1;
    for (new i = 0; i < g_iZoneCount; i++) {
        if (g_eZones[i][ZONE_LABEL] == g_iSelectedZone[id] && g_eZones[i][ZONE_TYPE] == g_iSelectedType[id]) {
            idx = i;
            break;
        }
    }

    if (idx == -1 && g_iZoneCount >= MAX_ZONES) {
        client_print(id, print_chat, "[%s] 区域已满！", PLUGIN_PREFIX);
        return;
    }

    if (idx == -1) {
        idx = g_iZoneCount;
        g_iZoneCount++;
    }

    new Float:fMinX = floatmin(g_fMeasureBottom[id][0], g_fMeasureTop[id][0]) - 32.0;
    new Float:fMaxX = floatmax(g_fMeasureBottom[id][0], g_fMeasureTop[id][0]) + 32.0;
    new Float:fMinY = floatmin(g_fMeasureBottom[id][1], g_fMeasureTop[id][1]) - 32.0;
    new Float:fMaxY = floatmax(g_fMeasureBottom[id][1], g_fMeasureTop[id][1]) + 32.0;
    new Float:fMinZ = floatmin(g_fMeasureBottom[id][2], g_fMeasureTop[id][2]);
    new Float:fMaxZ = floatmax(g_fMeasureBottom[id][2], g_fMeasureTop[id][2]) + PLAYER_HEIGHT * 1.5;

    g_eZones[idx][ZONE_ENABLED] = 1;
    g_eZones[idx][ZONE_LABEL] = g_iSelectedZone[id];
    g_eZones[idx][ZONE_TYPE] = g_iSelectedType[id];
    g_eZones[idx][ZONE_STATUS] = 0;
    g_eZones[idx][ZONE_MINS][0] = fMinX;
    g_eZones[idx][ZONE_MINS][1] = fMinY;
    g_eZones[idx][ZONE_MINS][2] = fMinZ;
    g_eZones[idx][ZONE_MAXS][0] = fMaxX;
    g_eZones[idx][ZONE_MAXS][1] = fMaxY;
    g_eZones[idx][ZONE_MAXS][2] = fMaxZ;
    g_eZones[idx][ZONE_CAPTURE_TIME] = 0.0;
    g_eZones[idx][ZONE_OWNER] = 0;

    client_print(id, print_chat, "[%s] %s区已保存为%d人点 (%.1f分)！",
        PLUGIN_PREFIX, g_szZoneLabels[g_iSelectedZone[id]], g_iSelectedType[id], g_fPointScores[g_iSelectedType[id] - 3]);

    pointscap_save_config();
    client_print(id, print_chat, "[%s] 已自动保存到当前地图配置。", PLUGIN_PREFIX);
    
    g_iMeasureState[id] = MEASURE_NONE;
    showMainMenu(id);
}

// ============================================
// View Zones
// ============================================
showViewZonesMenu(id) {
    if (g_iZoneCount == 0) {
        client_print(id, print_chat, "[%s] 还没有配置任何区域！", PLUGIN_PREFIX);
        showMainMenu(id);
        return;
    }
    
    new menu[512];
    new len = formatex(menu, charsmax(menu), "\r查看区域^n^n");
    new keys = MENU_KEY_0;
    new maxItems = min(g_iZoneCount, 9);
    
    for (new i = 0; i < maxItems; i++) {
        len += formatex(menu[len], charsmax(menu) - len, "\r%d.\w %s区 - %d人 (%.1f)^n",
            i + 1, g_szZoneLabels[g_eZones[i][ZONE_LABEL]], g_eZones[i][ZONE_TYPE], g_fPointScores[g_eZones[i][ZONE_TYPE] - 3]);
        keys |= (1 << i);
    }
    len += formatex(menu[len], charsmax(menu) - len, "^n\r0.\w 返回");
    show_menu(id, keys, menu, -1, "View Zones Menu");
}

public handleViewZonesMenu(id, key) {
    if (key == 9) { showMainMenu(id); return PLUGIN_HANDLED; }
    if (key >= 0 && key < g_iZoneCount) {
        new Float:pos[3];
        pos[0] = (g_eZones[key][ZONE_MINS][0] + g_eZones[key][ZONE_MAXS][0]) / 2.0;
        pos[1] = (g_eZones[key][ZONE_MINS][1] + g_eZones[key][ZONE_MAXS][1]) / 2.0;
        pos[2] = g_eZones[key][ZONE_MINS][2] + 10.0;
        engfunc(EngFunc_SetOrigin, id, pos);
        client_print(id, print_chat, "[%s] Teleported to Zone %s", PLUGIN_PREFIX, g_szZoneLabels[g_eZones[key][ZONE_LABEL]]);
    }
    return PLUGIN_HANDLED;
}

// ============================================
// Delete Zone
// ============================================
showDeleteZoneMenu(id) {
    if (g_iZoneCount == 0) {
        client_print(id, print_chat, "[%s] 没有区域可以删除！", PLUGIN_PREFIX);
        showMainMenu(id);
        return;
    }
    
    new menu[512];
    new len = formatex(menu, charsmax(menu), "\r删除区域^n^n");
    new keys = MENU_KEY_0;
    new maxItems = min(g_iZoneCount, 9);
    
    for (new i = 0; i < maxItems; i++) {
        len += formatex(menu[len], charsmax(menu) - len, "\r%d.\w %s区 - %d人^n",
            i + 1, g_szZoneLabels[g_eZones[i][ZONE_LABEL]], g_eZones[i][ZONE_TYPE]);
        keys |= (1 << i);
    }
    len += formatex(menu[len], charsmax(menu) - len, "^n\r0.\w 返回");
    show_menu(id, keys, menu, -1, "Delete Zone Menu");
}

public handleDeleteZoneMenu(id, key) {
    if (key == 9) { showMainMenu(id); return PLUGIN_HANDLED; }
    if (key >= 0 && key < g_iZoneCount) {
        new deletedLabel = g_eZones[key][ZONE_LABEL];
        for (new i = key; i < g_iZoneCount - 1; i++) {
            g_eZones[i][ZONE_LABEL] = g_eZones[i + 1][ZONE_LABEL];
            g_eZones[i][ZONE_TYPE] = g_eZones[i + 1][ZONE_TYPE];
            g_eZones[i][ZONE_MINS] = g_eZones[i + 1][ZONE_MINS];
            g_eZones[i][ZONE_MAXS] = g_eZones[i + 1][ZONE_MAXS];
            g_eZones[i][ZONE_STATUS] = g_eZones[i + 1][ZONE_STATUS];
        }
        g_iZoneCount--;
        client_print(id, print_chat, "[%s] Zone %s deleted!", PLUGIN_PREFIX, g_szZoneLabels[deletedLabel]);
        pointscap_save_config();
        client_print(id, print_chat, "[%s] 已自动保存删除结果。", PLUGIN_PREFIX);
        showDeleteZoneMenu(id);
    }
    return PLUGIN_HANDLED;
}

// ============================================
// Save/Load Config
// ============================================
pointscap_save_config() {
    new szPath[128], szMapName[32];
    get_mapname(szMapName, charsmax(szMapName));
    get_configsdir(szPath, charsmax(szPath));
    
    new dirPath[128];
    formatex(dirPath, charsmax(dirPath), "%s/mixsystem/pointscap", szPath);
    if (!dir_exists(dirPath)) mkdir(dirPath);
    
    formatex(szPath, charsmax(szPath), "%s/%s.ini", dirPath, szMapName);
    
    new f = fopen(szPath, "w");
    if (!f) {
        log_amx("[PointScap] Failed to save: %s", szPath);
        return;
    }
    
    fprintf(f, "; PointScap Zone Config - %s^n; Auto-generated^n^n", szMapName);
    
    for (new i = 0; i < g_iZoneCount; i++) {
        fprintf(f, "[%s]^n", g_szZoneLabels[g_eZones[i][ZONE_LABEL]]);
        fprintf(f, "type %d^n", g_eZones[i][ZONE_TYPE]);
        fprintf(f, "mins %.0f %.0f %.0f^n", g_eZones[i][ZONE_MINS][0], g_eZones[i][ZONE_MINS][1], g_eZones[i][ZONE_MINS][2]);
        fprintf(f, "maxs %.0f %.0f %.0f^n^n", g_eZones[i][ZONE_MAXS][0], g_eZones[i][ZONE_MAXS][1], g_eZones[i][ZONE_MAXS][2]);
    }
    
    fclose(f);
    log_amx("[PointScap] Saved %d zones to %s", g_iZoneCount, szPath);
}

pointscap_load_config() {
    new szPath[128], szMapName[32];
    get_mapname(szMapName, charsmax(szMapName));
    get_configsdir(szPath, charsmax(szPath));
    formatex(szPath, charsmax(szPath), "%s/mixsystem/pointscap/%s.ini", szPath, szMapName);
    
    if (!file_exists(szPath)) {
        log_amx("[PointScap] No config for map: %s", szMapName);
        return;
    }
    
    new f = fopen(szPath, "r");
    if (!f) return;
    
    new line[256], szKey[32], szVal1[16], szVal2[16], szVal3[16];
    new currentZone = -1;
    
    g_iZoneCount = 0;
    
    while (!feof(f) && g_iZoneCount < MAX_ZONES) {
        fgets(f, line, charsmax(line));
        trim(line);
        
        if (line[0] == ';' || line[0] == '/' || !line[0]) continue;
        
        if (line[0] == '[') {
            // New zone section
            new labelIdx = line[1] - 'A';

            if (labelIdx >= 0 && labelIdx < 10) {
                currentZone = g_iZoneCount;
                g_eZones[currentZone][ZONE_ENABLED] = 1;
                g_eZones[currentZone][ZONE_LABEL] = labelIdx;
                g_eZones[currentZone][ZONE_STATUS] = 0;
                g_eZones[currentZone][ZONE_CAPTURE_TIME] = 0.0;
                g_eZones[currentZone][ZONE_OWNER] = 0;
                g_iZoneCount++;
            }
            continue;
        }
        
        if (currentZone < 0) continue;
        
        if (contain(line, "type") == 0) {
            parse(line, szKey, charsmax(szKey), szVal1, charsmax(szVal1));
            g_eZones[currentZone][ZONE_TYPE] = str_to_num(szVal1);
        }
        else if (contain(line, "mins") == 0) {
            parse(line, szKey, charsmax(szKey), szVal1, charsmax(szVal1), szVal2, charsmax(szVal2), szVal3, charsmax(szVal3));
            g_eZones[currentZone][ZONE_MINS][0] = str_to_float(szVal1);
            g_eZones[currentZone][ZONE_MINS][1] = str_to_float(szVal2);
            g_eZones[currentZone][ZONE_MINS][2] = str_to_float(szVal3);
        }
        else if (contain(line, "maxs") == 0) {
            parse(line, szKey, charsmax(szKey), szVal1, charsmax(szVal1), szVal2, charsmax(szVal2), szVal3, charsmax(szVal3));
            g_eZones[currentZone][ZONE_MAXS][0] = str_to_float(szVal1);
            g_eZones[currentZone][ZONE_MAXS][1] = str_to_float(szVal2);
            g_eZones[currentZone][ZONE_MAXS][2] = str_to_float(szVal3);
        }
    }
    
    fclose(f);
    log_amx("[PointScap] Loaded %d zones for map %s", g_iZoneCount, szMapName);
}

// ============================================
// Helper Functions
// ============================================
stock bool:is_player_in_zone(id, zone_id) {
    if (!is_user_alive(id)) return false;
    if (zone_id < 0 || zone_id >= g_iZoneCount) return false;
    if (!g_eZones[zone_id][ZONE_ENABLED]) return false;
    
    new Float:origin[3];
    pev(id, pev_origin, origin);
    
    return (
        origin[0] >= g_eZones[zone_id][ZONE_MINS][0] && origin[0] <= g_eZones[zone_id][ZONE_MAXS][0] &&
        origin[1] >= g_eZones[zone_id][ZONE_MINS][1] && origin[1] <= g_eZones[zone_id][ZONE_MAXS][1] &&
        origin[2] >= g_eZones[zone_id][ZONE_MINS][2] && origin[2] <= g_eZones[zone_id][ZONE_MAXS][2]
    );
}

public plugin_end() {
    log_amx("[PointScap] Plugin shutting down");
}
