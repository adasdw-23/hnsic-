#if defined _hnsic_gameplay_training_included
    #endinput
#endif
#define _hnsic_gameplay_training_included

/* ============================================================
 *  HNSIC v3.10.0 - gameplay_training.inc
 *  Training gameplay (godmode + hook)
 * ============================================================ */

// ─── Training Init ───────────────────────────────────────────
stock GameplayTraining_Init()
{
    // Enable godmode CVAR
    set_pcvar_num(g_pCvar[CVAR_TRAIN_GODMODE], 1);
}

// ─── Training Cleanup ────────────────────────────────────────
stock GameplayTraining_Cleanup()
{
    // Disable godmode
    set_pcvar_num(g_pCvar[CVAR_TRAIN_GODMODE], 0);
}

// ─── Training On Spawn ───────────────────────────────────────
stock GameplayTraining_OnSpawn(id)
{
    if (GetCvarInt(CVAR_TRAIN_GODMODE))
    {
        // Set godmode
        set_entvar(id, var_takedamage, DAMAGE_NO);
    }

    // Give knife
    Hnsic_StripAllWeapons(id);
}

// ─── Training Apply Speed ─────────────────────────────────────
stock GameplayTraining_ApplySpeed(id)
{
    // Normal speed in training
    Hnsic_SetSpeed(id, 250.0);
}

// ─── Training On Kill ─────────────────────────────────────────
stock GameplayTraining_OnKill(victim, killer)
{
    // No scoring in training
}

// ─── Training On Round Start ─────────────────────────────────
stock GameplayTraining_OnRoundStart()
{
    // Nothing special
}

// ─── Training On Round End ────────────────────────────────────
stock GameplayTraining_OnRoundEnd(winner)
{
    // Nothing special
}

// ─── Hook Functionality ──────────────────────────────────────
// Note: Hook requires a separate hook plugin or engine module
// This provides the interface for hook integration
stock GameplayTraining_GiveHook(id)
{
    if (!GetCvarInt(CVAR_TRAIN_HOOK))
        return;

    // Hook would be implemented via a separate module
    // Here we provide the CVAR interface
    Hnsic_PrintChat(id, "钩子功能已启用 (速度: %.0f)", GetCvarFloat(CVAR_TRAIN_HOOK_SPEED));
}

stock bool:GameplayTraining_IsHookEnabled()
{
    return (GetCvarInt(CVAR_TRAIN_HOOK) == 1);
}

stock Float:GameplayTraining_GetHookSpeed()
{
    return GetCvarFloat(CVAR_TRAIN_HOOK_SPEED);
}
