#if defined _hnsic_mode_pub_included
    #endinput
#endif
#define _hnsic_mode_pub_included

/* ============================================================
 *  HNSIC v1.0.0 - mode_pub.inl
 *  Public mode (HNS + autoswap + /back)
 * ============================================================ */

// ─── Pub Init ────────────────────────────────────────────────
stock ModePub_Init()
{
    // Set default gameplay to HNS
    if (g_iCurrentGameplay != GAMEPLAY_HNS)
    {
        set_pcvar_num(g_pCvar[CVAR_GAMEPLAY], GAMEPLAY_HNS);
    }

    Hnsic_PrintChat(0, "公共模式已启用 - 躲猫猫 + 自动换边");
    Hnsic_PrintChat(0, "输入 /back 返回游戏, /menu 打开菜单");
}

// ─── Pub Cleanup ──────────────────────────────────────────────
stock ModePub_Cleanup()
{
    // Nothing special
}

// ─── Pub On Spawn ────────────────────────────────────────────
stock ModePub_OnSpawn(id)
{
    // Apply HNS gameplay
    GameplayManager_OnSpawn(id);
    GameplayManager_ApplySpeed(id);
}

// ─── Pub On Kill ─────────────────────────────────────────────
stock ModePub_OnKill(victim, killer)
{
    GameplayManager_OnKill(victim, killer);
}

// ─── Pub On Round Start ──────────────────────────────────────
stock ModePub_OnRoundStart()
{
    // Nothing special
}

// ─── Pub On Round End ────────────────────────────────────────
stock ModePub_OnRoundEnd(winner)
{
    if (!GetCvarInt(CVAR_PUB_AUTOSWAP))
        return;

    // Auto swap teams after each round
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

    Hnsic_PrintChat(0, "自动换边!");
}
