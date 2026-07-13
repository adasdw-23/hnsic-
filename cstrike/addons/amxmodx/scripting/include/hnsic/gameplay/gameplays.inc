#if defined _hnsic_gameplays_included
    #endinput
#endif
#define _hnsic_gameplays_included

/* ============================================================
 *  HNSIC v3.10.0 - gameplays.inc
 *  Gameplay manager - switches between gameplay types
 * ============================================================ */

// ─── Gameplay Switch ─────────────────────────────────────────
stock GameplayManager_Switch(newGameplay)
{
    new oldGameplay = g_iCurrentGameplay;

    if (oldGameplay == newGameplay)
        return;

    // Cleanup old gameplay
    GameplayManager_Cleanup(oldGameplay);

    // Initialize new gameplay
    g_iCurrentGameplay = newGameplay;

    switch (newGameplay)
    {
        case GAMEPLAY_HNS:      GameplayHNS_Init();
        case GAMEPLAY_KNIFE:    GameplayKnife_Init();
        case GAMEPLAY_TRAINING: GameplayTraining_Init();
        default:                Hnsic_Log("未知Gameplay: %d", newGameplay);
    }

    new szOldName[32], szNewName[32];
    Hnsic_GetGameplayName(oldGameplay, szOldName, charsmax(szOldName));
    Hnsic_GetGameplayName(newGameplay, szNewName, charsmax(szNewName));

    Hnsic_PrintChat(0, "Gameplay已切换: %s -> %s", szOldName, szNewName);
    Hnsic_Log("Gameplay切换: %s -> %s", szOldName, szNewName);

    // Execute forward
    if (g_fwdGameplayChange)
        ExecuteForward(g_fwdGameplayChange, _, oldGameplay, newGameplay);
}

// ─── Gameplay Cleanup ─────────────────────────────────────────
stock GameplayManager_Cleanup(gameplay)
{
    switch (gameplay)
    {
        case GAMEPLAY_HNS:      GameplayHNS_Cleanup();
        case GAMEPLAY_KNIFE:    GameplayKnife_Cleanup();
        case GAMEPLAY_TRAINING: GameplayTraining_Cleanup();
    }
}

// ─── Apply Gameplay on Spawn ─────────────────────────────────
stock GameplayManager_OnSpawn(id)
{
    if (!Hnsic_IsPlayerAlive(id))
        return;

    switch (g_iCurrentGameplay)
    {
        case GAMEPLAY_HNS:      GameplayHNS_OnSpawn(id);
        case GAMEPLAY_KNIFE:    GameplayKnife_OnSpawn(id);
        case GAMEPLAY_TRAINING: GameplayTraining_OnSpawn(id);
    }
}

// ─── Apply Gameplay Speed ──────────────────────────────────────
stock GameplayManager_ApplySpeed(id)
{
    if (!Hnsic_IsPlayerAlive(id))
        return;

    switch (g_iCurrentGameplay)
    {
        case GAMEPLAY_HNS:      GameplayHNS_ApplySpeed(id);
        case GAMEPLAY_KNIFE:    GameplayKnife_ApplySpeed(id);
        case GAMEPLAY_TRAINING: GameplayTraining_ApplySpeed(id);
    }
}

// ─── Gameplay on Kill ─────────────────────────────────────────
stock GameplayManager_OnKill(victim, killer)
{
    switch (g_iCurrentGameplay)
    {
        case GAMEPLAY_HNS:      GameplayHNS_OnKill(victim, killer);
        case GAMEPLAY_KNIFE:    GameplayKnife_OnKill(victim, killer);
        case GAMEPLAY_TRAINING: GameplayTraining_OnKill(victim, killer);
    }
}

// ─── Gameplay on Round Start ──────────────────────────────────
stock GameplayManager_OnRoundStart()
{
    switch (g_iCurrentGameplay)
    {
        case GAMEPLAY_HNS:      GameplayHNS_OnRoundStart();
        case GAMEPLAY_KNIFE:    GameplayKnife_OnRoundStart();
        case GAMEPLAY_TRAINING: GameplayTraining_OnRoundStart();
    }
}

// ─── Gameplay on Round End ────────────────────────────────────
stock GameplayManager_OnRoundEnd(winner)
{
    switch (g_iCurrentGameplay)
    {
        case GAMEPLAY_HNS:      GameplayHNS_OnRoundEnd(winner);
        case GAMEPLAY_KNIFE:    GameplayKnife_OnRoundEnd(winner);
        case GAMEPLAY_TRAINING: GameplayTraining_OnRoundEnd(winner);
    }
}
