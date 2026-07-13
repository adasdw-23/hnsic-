#if defined _hnsic_mode_knife_included
    #endinput
#endif
#define _hnsic_mode_knife_included

/* ============================================================
 *  HNSIC v1.0.0 - mode_knife.inl
 *  Knife mode (knife only combat)
 * ============================================================ */

// ─── Knife Mode Init ─────────────────────────────────────────
stock ModeKnife_Init()
{
    // Set gameplay to knife
    if (g_iCurrentGameplay != GAMEPLAY_KNIFE)
    {
        set_pcvar_num(g_pCvar[CVAR_GAMEPLAY], GAMEPLAY_KNIFE);
    }

    Hnsic_PrintChat(0, "刀战模式已启用 - 仅刀具战斗");
}

// ─── Knife Mode Cleanup ──────────────────────────────────────
stock ModeKnife_Cleanup()
{
    // Nothing special
}

// ─── Knife Mode On Spawn ─────────────────────────────────────
stock ModeKnife_OnSpawn(id)
{
    GameplayManager_OnSpawn(id);
    GameplayManager_ApplySpeed(id);
}

// ─── Knife Mode On Kill ─────────────────────────────────────
stock ModeKnife_OnKill(victim, killer)
{
    GameplayManager_OnKill(victim, killer);
}

// ─── Knife Mode On Round Start ──────────────────────────────
stock ModeKnife_OnRoundStart()
{
    // Nothing special
}

// ─── Knife Mode On Round End ────────────────────────────────
stock ModeKnife_OnRoundEnd(winner)
{
    // Nothing special
}
