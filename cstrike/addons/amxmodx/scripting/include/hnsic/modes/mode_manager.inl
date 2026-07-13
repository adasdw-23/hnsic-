#if defined _hnsic_mode_manager_included
    #endinput
#endif
#define _hnsic_mode_manager_included

/* ============================================================
 *  HNSIC v1.0.0 - mode_manager.inl
 *  Mode manager - switches between game modes
 *  Inline file for mode layer coordination
 * ============================================================ */

// ─── Mode Switch ──────────────────────────────────────────────
stock ModeManager_SwitchMode(newMode)
{
    new oldMode = g_iCurrentMode;

    if (oldMode == newMode)
        return;

    // Cleanup old mode
    ModeManager_Cleanup(oldMode);

    // Initialize new mode
    g_iCurrentMode = newMode;

    switch (newMode)
    {
        case MODE_PUB:      ModePub_Init();
        case MODE_TRAINING: ModeTraining_Init();
        case MODE_KNIFE:    ModeKnife_Init();
        case MODE_DM:       ModeDM_Init();
        case MODE_ZOMBIE:   ModeZombie_Init();
        case MODE_MIX:      ModeMix_Init();
        default:
        {
            Hnsic_Log("未知模式: %d", newMode);
            return;
        }
    }

    new szOldName[32], szNewName[32];
    Hnsic_GetModeName(oldMode, szOldName, charsmax(szOldName));
    Hnsic_GetModeName(newMode, szNewName, charsmax(szNewName));

    Hnsic_PrintChat(0, "模式已切换: %s -> %s", szOldName, szNewName);
    Hnsic_Log("模式切换: %s -> %s", szOldName, szNewName);

    // Execute forward
    if (g_fwdModeChange)
        ExecuteForward(g_fwdModeChange, _, oldMode, newMode);
}

// ─── Mode Cleanup ────────────────────────────────────────────
stock ModeManager_Cleanup(mode)
{
    switch (mode)
    {
        case MODE_PUB:      ModePub_Cleanup();
        case MODE_TRAINING: ModeTraining_Cleanup();
        case MODE_KNIFE:    ModeKnife_Cleanup();
        case MODE_DM:       ModeDM_Cleanup();
        case MODE_ZOMBIE:   ModeZombie_Cleanup();
        case MODE_MIX:      ModeMix_Cleanup();
    }

    // Reset match state
    g_iMatchState = STATE_IDLE;
    g_bMatchPaused = false;
    g_bSurrenderActive = false;
    g_bCaptainDuelActive = false;
    g_bWaitingReconnect = false;

    // Remove all tasks
    remove_task(TASK_MATCH_TIMER);
    remove_task(TASK_SURRENDER);
    remove_task(TASK_RECONNECT_WAIT);
    remove_task(TASK_FREEZE_TIME);
    remove_task(TASK_AFK_CHECK);
}

// ─── Mode On Spawn ───────────────────────────────────────────
stock ModeManager_OnSpawn(id)
{
    switch (g_iCurrentMode)
    {
        case MODE_PUB:      ModePub_OnSpawn(id);
        case MODE_TRAINING: ModeTraining_OnSpawn(id);
        case MODE_KNIFE:    ModeKnife_OnSpawn(id);
        case MODE_DM:       ModeDM_OnSpawn(id);
        case MODE_ZOMBIE:   ModeZombie_OnSpawn(id);
        case MODE_MIX:      ModeMix_OnSpawn(id);
    }
}

// ─── Mode On Kill ────────────────────────────────────────────
stock ModeManager_OnKill(victim, killer)
{
    switch (g_iCurrentMode)
    {
        case MODE_PUB:      ModePub_OnKill(victim, killer);
        case MODE_TRAINING: ModeTraining_OnKill(victim, killer);
        case MODE_KNIFE:    ModeKnife_OnKill(victim, killer);
        case MODE_DM:       ModeDM_OnKill(victim, killer);
        case MODE_ZOMBIE:   ModeZombie_OnKill(victim, killer);
        case MODE_MIX:      ModeMix_OnKill(victim, killer);
    }
}

// ─── Mode On Round Start ─────────────────────────────────────
stock ModeManager_OnRoundStart()
{
    switch (g_iCurrentMode)
    {
        case MODE_PUB:      ModePub_OnRoundStart();
        case MODE_TRAINING: ModeTraining_OnRoundStart();
        case MODE_KNIFE:    ModeKnife_OnRoundStart();
        case MODE_DM:       ModeDM_OnRoundStart();
        case MODE_ZOMBIE:   ModeZombie_OnRoundStart();
        case MODE_MIX:      ModeMix_OnRoundStart();
    }

    // Also trigger gameplay round start
    GameplayManager_OnRoundStart();
}

// ─── Mode On Round End ───────────────────────────────────────
stock ModeManager_OnRoundEnd(winner)
{
    switch (g_iCurrentMode)
    {
        case MODE_PUB:      ModePub_OnRoundEnd(winner);
        case MODE_TRAINING: ModeTraining_OnRoundEnd(winner);
        case MODE_KNIFE:    ModeKnife_OnRoundEnd(winner);
        case MODE_DM:       ModeDM_OnRoundEnd(winner);
        case MODE_ZOMBIE:   ModeZombie_OnRoundEnd(winner);
        case MODE_MIX:      ModeMix_OnRoundEnd(winner);
    }

    // Also trigger gameplay round end
    GameplayManager_OnRoundEnd(winner);
}
