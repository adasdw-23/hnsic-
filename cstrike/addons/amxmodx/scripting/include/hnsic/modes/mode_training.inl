#if defined _hnsic_mode_training_included
    #endinput
#endif
#define _hnsic_mode_training_included

/* ============================================================
 *  HNSIC v1.0.0 - mode_training.inl
 *  Training mode (godmode + hook)
 * ============================================================ */

// ─── Training Init ───────────────────────────────────────────
stock ModeTraining_Init()
{
    // Set gameplay to training
    if (g_iCurrentGameplay != GAMEPLAY_TRAINING)
    {
        set_pcvar_num(g_pCvar[CVAR_GAMEPLAY], GAMEPLAY_TRAINING);
    }

    Hnsic_PrintChat(0, "训练模式已启用 - 无敌 + 钩子");
    Hnsic_PrintChat(0, "输入 /menu 打开菜单切换设置");
}

// ─── Training Cleanup ────────────────────────────────────────
stock ModeTraining_Cleanup()
{
    // Nothing special
}

// ─── Training On Spawn ──────────────────────────────────────
stock ModeTraining_OnSpawn(id)
{
    GameplayManager_OnSpawn(id);
    GameplayManager_ApplySpeed(id);

    // Give hook info
    if (GameplayTraining_IsHookEnabled())
    {
        GameplayTraining_GiveHook(id);
    }
}

// ─── Training On Kill ────────────────────────────────────────
stock ModeTraining_OnKill(victim, killer)
{
    // No scoring in training
}

// ─── Training On Round Start ─────────────────────────────────
stock ModeTraining_OnRoundStart()
{
    // Nothing special
}

// ─── Training On Round End ────────────────────────────────────
stock ModeTraining_OnRoundEnd(winner)
{
    // Nothing special
}
