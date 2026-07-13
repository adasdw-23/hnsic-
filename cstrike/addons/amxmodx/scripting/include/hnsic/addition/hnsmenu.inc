#if defined _hnsic_hnsmenu_included
    #endinput
#endif
#define _hnsic_hnsmenu_included

/* ============================================================
 *  HNSIC v3.10.0 - hnsmenu.inc
 *  N-key main menu system
 *  Uses old-style show_menu for core menus (compatibility)
 *  Uses new-style menu_create for settings menus
 * ============================================================ */

// ─── Main Menu (Old-style show_menu) ─────────────────────────
stock HnsMenu_ShowMain(id)
{
    if (!Hnsic_IsPlayerValid(id))
        return;

    new szMenu[MAX_MENU_LEN];
    new len = 0;
    new szTitle[64];

    formatex(szTitle, charsmax(szTitle), "\y[HNSIC v3.10.0] 主菜单^n^n");

    len = copy(szMenu, charsmax(szMenu), szTitle);
    len += formatex(szMenu[len], charsmax(szMenu) - len, "\r1. \w模式信息^n");
    len += formatex(szMenu[len], charsmax(szMenu) - len, "\r2. \w查看比分^n");
    len += formatex(szMenu[len], charsmax(szMenu) - len, "\r3. \w个人统计^n");
    len += formatex(szMenu[len], charsmax(szMenu) - len, "\r4. \w排行榜^n");
    len += formatex(szMenu[len], charsmax(szMenu) - len, "\r5. \w返回游戏^n");

    // Admin options
    if (Hnsic_IsAdmin(id))
    {
        len += formatex(szMenu[len], charsmax(szMenu) - len, "^n\r6. \y[管理] 切换模式^n");
        len += formatex(szMenu[len], charsmax(szMenu) - len, "\r7. \y[管理] 切换Gameplay^n");
        len += formatex(szMenu[len], charsmax(szMenu) - len, "\r8. \y[管理] 比赛控制^n");
        len += formatex(szMenu[len], charsmax(szMenu) - len, "\r9. \y[管理] 设置菜单^n");
    }

    len += formatex(szMenu[len], charsmax(szMenu) - len, "^n\r0. \w关闭");

    show_menu(id, MENU_KEY_ALL, szMenu, -1, "HnsicMainMenu");
}

// ─── Main Menu Handler ───────────────────────────────────────
public HnsMenu_MainHandler(id, key)
{
    switch (key)
    {
        case 0: HnsMenu_ShowModeInfo(id);
        case 1: Cmd_Score(id);
        case 2: Cmd_Stats(id);
        case 3: Cmd_Top(id);
        case 4: Cmd_Back(id);
        case 5:
        {
            if (Hnsic_IsAdmin(id))
                HnsMenu_ShowModeSelect(id);
        }
        case 6:
        {
            if (Hnsic_IsAdmin(id))
                HnsMenu_ShowGameplaySelect(id);
        }
        case 7:
        {
            if (Hnsic_IsAdmin(id))
                HnsMenu_ShowMatchControl(id);
        }
        case 8:
        {
            if (Hnsic_IsAdmin(id))
                HnsMenu_ShowSettings(id);
        }
    }
    return PLUGIN_HANDLED;
}

// ─── Mode Info Menu ──────────────────────────────────────────
stock HnsMenu_ShowModeInfo(id)
{
    new szMenu[MAX_MENU_LEN];
    new len = 0;
    new szModeName[32], szGpName[32];

    Hnsic_GetModeName(g_iCurrentMode, szModeName, charsmax(szModeName));
    Hnsic_GetGameplayName(g_iCurrentGameplay, szGpName, charsmax(szGpName));

    new szStateName[32];
    Hnsic_GetStateName(g_iMatchState, szStateName, charsmax(szStateName));

    len += formatex(szMenu[len], charsmax(szMenu) - len, "\y[HNSIC] 模式信息^n^n");
    len += formatex(szMenu[len], charsmax(szMenu) - len, "\w当前模式: \r%s^n", szModeName);
    len += formatex(szMenu[len], charsmax(szMenu) - len, "\w当前Gameplay: \r%s^n", szGpName);
    len += formatex(szMenu[len], charsmax(szMenu) - len, "\w比赛状态: \r%s^n", szStateName);

    if (g_iCurrentMode == MODE_MIX)
    {
        len += formatex(szMenu[len], charsmax(szMenu) - len, "\w比分: \rTT %d - %d CT^n",
            g_iMatchScore[TEAM_TT], g_iMatchScore[TEAM_CT]);
        len += formatex(szMenu[len], charsmax(szMenu) - len, "\w当前: \r%s 第%d回合^n",
            (g_iCurrentHalf == 0) ? "上半场" : "下半场", g_iRoundNum);
    }

    len += formatex(szMenu[len], charsmax(szMenu) - len, "^n\r0. \w返回");

    show_menu(id, MENU_KEY_0, szMenu, -1, "HnsicModeInfo");
}

public HnsMenu_ModeInfoHandler(id, key)
{
    if (key == 9)
        HnsMenu_ShowMain(id);
}

// ─── Mode Select Menu (Old-style) ────────────────────────────
stock HnsMenu_ShowModeSelect(id)
{
    new szMenu[MAX_MENU_LEN];
    new len = 0;

    len += formatex(szMenu[len], charsmax(szMenu) - len, "\y[HNSIC] 选择模式^n^n");
    len += formatex(szMenu[len], charsmax(szMenu) - len, "\r1. \w公共模式 (HNS+自动换边)^n");
    len += formatex(szMenu[len], charsmax(szMenu) - len, "\r2. \w训练模式 (无敌+钩子)^n");
    len += formatex(szMenu[len], charsmax(szMenu) - len, "\r3. \w刀战模式^n");
    len += formatex(szMenu[len], charsmax(szMenu) - len, "\r4. \w混战模式 (重生+换边)^n");
    len += formatex(szMenu[len], charsmax(szMenu) - len, "\r5. \w僵尸模式 (感染)^n");
    len += formatex(szMenu[len], charsmax(szMenu) - len, "\r6. \w比赛模式 (积分赛)^n");
    len += formatex(szMenu[len], charsmax(szMenu) - len, "^n\r0. \w返回");

    show_menu(id, MENU_KEY_ALL, szMenu, -1, "HnsicModeSelect");
}

public HnsMenu_ModeSelectHandler(id, key)
{
    new mode = key + 1; // 1-6
    if (mode >= MODE_PUB && mode <= MODE_MIX)
    {
        set_pcvar_num(g_pCvar[CVAR_MODE], mode);
        new szName[32];
        Hnsic_GetModeName(mode, szName, charsmax(szName));
        Hnsic_PrintChat(id, "模式已切换为: %s", szName);
    }
}

// ─── Gameplay Select Menu (Old-style) ───────────────────────
stock HnsMenu_ShowGameplaySelect(id)
{
    new szMenu[MAX_MENU_LEN];
    new len = 0;

    len += formatex(szMenu[len], charsmax(szMenu) - len, "\y[HNSIC] 选择Gameplay^n^n");
    len += formatex(szMenu[len], charsmax(szMenu) - len, "\r1. \w躲猫猫 (TT:刀+闪光+烟雾+隐身)^n");
    len += formatex(szMenu[len], charsmax(szMenu) - len, "\r2. \w刀战 (仅刀具)^n");
    len += formatex(szMenu[len], charsmax(szMenu) - len, "\r3. \w训练 (无敌+钩子)^n");
    len += formatex(szMenu[len], charsmax(szMenu) - len, "^n\r0. \w返回");

    show_menu(id, MENU_KEY_ALL, szMenu, -1, "HnsicGpSelect");
}

public HnsMenu_GpSelectHandler(id, key)
{
    new gp = key + 1; // 1-3
    if (gp >= GAMEPLAY_HNS && gp <= GAMEPLAY_TRAINING)
    {
        set_pcvar_num(g_pCvar[CVAR_GAMEPLAY], gp);
        new szName[32];
        Hnsic_GetGameplayName(gp, szName, charsmax(szName));
        Hnsic_PrintChat(id, "Gameplay已切换为: %s", szName);
    }
}

// ─── Match Control Menu (Old-style) ──────────────────────────
stock HnsMenu_ShowMatchControl(id)
{
    new szMenu[MAX_MENU_LEN];
    new len = 0;

    len += formatex(szMenu[len], charsmax(szMenu) - len, "\y[HNSIC] 比赛控制^n^n");
    len += formatex(szMenu[len], charsmax(szMenu) - len, "\r1. \w开始AI分组^n");
    len += formatex(szMenu[len], charsmax(szMenu) - len, "\r2. \w停止比赛^n");
    len += formatex(szMenu[len], charsmax(szMenu) - len, "\r3. \w暂停/继续^n");
    len += formatex(szMenu[len], charsmax(szMenu) - len, "\r4. \w投降^n");
    len += formatex(szMenu[len], charsmax(szMenu) - len, "^n\r0. \w返回");

    show_menu(id, MENU_KEY_ALL, szMenu, -1, "HnsicMatchCtrl");
}

public HnsMenu_MatchCtrlHandler(id, key)
{
    switch (key)
    {
        case 0: Cmd_AiGroup(id);
        case 1: Cmd_StopMatch(id);
        case 2:
        {
            if (g_bMatchPaused)
                Cmd_Unpause(id);
            else
                Cmd_Pause(id);
        }
        case 3: Cmd_Surrender(id);
    }
}

// ─── Settings Menu (New-style menu_create) ───────────────────
stock HnsMenu_ShowSettings(id)
{
    new menu = menu_create("\y[HNSIC] 设置菜单", "HnsMenu_SettingsHandler");

    menu_additem(menu, "HNS TT速度", "", 0);
    menu_additem(menu, "HNS CT速度", "", 0);
    menu_additem(menu, "刀战速度", "", 0);
    menu_additem(menu, "刀战伤害倍率", "", 0);
    menu_additem(menu, "比赛积分上限", "", 0);
    menu_additem(menu, "比赛半场回合数", "", 0);
    menu_additem(menu, "投降投票比例", "", 0);
    menu_additem(menu, "AFK超时时间", "", 0);

    menu_setprop(menu, MPROP_EXIT, MEXIT_ALL);
    menu_display(id, menu, 0);
}

public HnsMenu_SettingsHandler(id, menu, item)
{
    if (item == MENU_EXIT)
    {
        menu_destroy(menu);
        HnsMenu_ShowMain(id);
        return PLUGIN_HANDLED;
    }

    new szInfo[32], szName[64], access, callback;
    menu_item_getinfo(menu, item, access, szInfo, charsmax(szInfo), szName, charsmax(szName), callback);

    new Float:value;
    new cvar_index;

    switch (item)
    {
        case 0:
        {
            cvar_index = CVAR_HNS_TT_SPEED;
            value = GetCvarFloat(CVAR_HNS_TT_SPEED);
            value += 0.05;
            if (value > 2.0) value = 0.8;
        }
        case 1:
        {
            cvar_index = CVAR_HNS_CT_SPEED;
            value = GetCvarFloat(CVAR_HNS_CT_SPEED);
            value += 0.05;
            if (value > 2.0) value = 0.8;
        }
        case 2:
        {
            cvar_index = CVAR_KNIFE_SPEED;
            value = GetCvarFloat(CVAR_KNIFE_SPEED);
            value += 0.1;
            if (value > 3.0) value = 0.8;
        }
        case 3:
        {
            cvar_index = CVAR_KNIFE_DAMAGE;
            value = GetCvarFloat(CVAR_KNIFE_DAMAGE);
            value += 0.1;
            if (value > 5.0) value = 0.5;
        }
        case 4:
        {
            cvar_index = CVAR_MIX_POINT_CAP;
            value = float(GetCvarInt(CVAR_MIX_POINT_CAP));
            value += 1.0;
            if (value > 30.0) value = 5.0;
        }
        case 5:
        {
            cvar_index = CVAR_MIX_HALF_ROUNDS;
            value = float(GetCvarInt(CVAR_MIX_HALF_ROUNDS));
            value += 1.0;
            if (value > 30.0) value = 5.0;
        }
        case 6:
        {
            cvar_index = CVAR_MIX_SURRENDER_RATIO;
            value = GetCvarFloat(CVAR_MIX_SURRENDER_RATIO);
            value += 0.05;
            if (value > 1.0) value = 0.5;
        }
        case 7:
        {
            cvar_index = CVAR_MIX_AFK_TIME;
            value = float(GetCvarInt(CVAR_MIX_AFK_TIME));
            value += 5.0;
            if (value > 120.0) value = 15.0;
        }
    }

    set_pcvar_float(g_pCvar[cvar_index], value);
    Hnsic_PrintChat(id, "%s 已设置为: %.2f", szName, value);

    // Refresh menu
    menu_destroy(menu);
    HnsMenu_ShowSettings(id);
    return PLUGIN_HANDLED;
}

// ─── Menu Registration ───────────────────────────────────────
stock HnsMenu_RegisterMenus()
{
    register_menucmd(register_menuid("HnsicMainMenu"), MENU_KEY_ALL, "HnsMenu_MainHandler");
    register_menucmd(register_menuid("HnsicModeInfo"), MENU_KEY_0, "HnsMenu_ModeInfoHandler");
    register_menucmd(register_menuid("HnsicModeSelect"), MENU_KEY_ALL, "HnsMenu_ModeSelectHandler");
    register_menucmd(register_menuid("HnsicGpSelect"), MENU_KEY_ALL, "HnsMenu_GpSelectHandler");
    register_menucmd(register_menuid("HnsicMatchCtrl"), MENU_KEY_ALL, "HnsMenu_MatchCtrlHandler");
}
