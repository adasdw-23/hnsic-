#if defined _hnsic_gameplay_knife_included
    #endinput
#endif
#define _hnsic_gameplay_knife_included

/* ============================================================
 *  HNSIC v3.10.0 - gameplay_knife.inc
 *  Knife gameplay - knife only combat
 * ============================================================ */

// ─── Knife Init ──────────────────────────────────────────────
stock GameplayKnife_Init()
{
    // Set gravity
    new Float:gravity = GetCvarFloat(CVAR_KNIFE_GRAVITY);
    new Float:worldGravity = get_entvar(0, var_gravity);
    set_entvar(0, var_gravity, worldGravity * gravity);
}

// ─── Knife Cleanup ────────────────────────────────────────────
stock GameplayKnife_Cleanup()
{
    // Restore default gravity
    set_entvar(0, var_gravity, 800.0);
}

// ─── Knife On Spawn ──────────────────────────────────────────
stock GameplayKnife_OnSpawn(id)
{
    // Strip all weapons, give knife only
    Hnsic_StripAllWeapons(id);
}

// ─── Knife Apply Speed ────────────────────────────────────────
stock GameplayKnife_ApplySpeed(id)
{
    new Float:speed = 250.0 * GetCvarFloat(CVAR_KNIFE_SPEED);
    Hnsic_SetSpeed(id, speed);
}

// ─── Knife On Kill ────────────────────────────────────────────
stock GameplayKnife_OnKill(victim, killer)
{
    if (!Hnsic_IsPlayerValid(killer))
        return;

    // Headshot only check
    if (GetCvarInt(CVAR_KNIFE_HS_ONLY))
    {
        // If not headshot, could be blocked by damage modifier
        // This is handled in the damage hook
    }

    User_AddPts(killer, 3);
    User_AddKill(killer);
    User_AddDeath(victim);
}

// ─── Knife On Round Start ────────────────────────────────────
stock GameplayKnife_OnRoundStart()
{
    // Nothing special needed
}

// ─── Knife On Round End ─────────────────────────────────────
stock GameplayKnife_OnRoundEnd(winner)
{
    // Nothing special needed
}

// ─── Knife Damage Modifier (called from Ham_Weapon_PrimaryAttack) ─
stock Float:GameplayKnife_GetDamageMultiplier()
{
    return GetCvarFloat(CVAR_KNIFE_DAMAGE);
}
