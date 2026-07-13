#if defined _hnsic_mode_dm_included
    #endinput
#endif
#define _hnsic_mode_dm_included

/* ============================================================
 *  HNSIC v1.0.0 - mode_dm.inl
 *  DeathMatch mode (respawn + role swap)
 * ============================================================ */

// ─── DM Init ─────────────────────────────────────────────────
stock ModeDM_Init()
{
    // Set gameplay to knife for DM
    if (g_iCurrentGameplay != GAMEPLAY_KNIFE)
    {
        set_pcvar_num(g_pCvar[CVAR_GAMEPLAY], GAMEPLAY_KNIFE);
    }

    g_iRoundNum = 0;

    Hnsic_PrintChat(0, "混战模式已启用 - 自动重生 + 角色互换");
}

// ─── DM Cleanup ──────────────────────────────────────────────
stock ModeDM_Cleanup()
{
    // Nothing special
}

// ─── DM On Spawn ─────────────────────────────────────────────
stock ModeDM_OnSpawn(id)
{
    GameplayManager_OnSpawn(id);
    GameplayManager_ApplySpeed(id);
}

// ─── DM On Kill ──────────────────────────────────────────────
stock ModeDM_OnKill(victim, killer)
{
    GameplayManager_OnKill(victim, killer);

    // Auto respawn victim
    new Float:respawnTime = GetCvarFloat(CVAR_DM_RESPAWN_TIME);
    set_task(respawnTime, "ModeDM_Respawn", victim);
}

// ─── DM Respawn ──────────────────────────────────────────────
public ModeDM_Respawn(id)
{
    if (!Hnsic_IsPlayerValid(id))
        return;

    if (!Hnsic_IsPlayerOnTeam(id))
        return;

    rg_round_respawn(id);
    Hnsic_PrintChat(id, "你已重生!");
}

// ─── DM On Round Start ───────────────────────────────────────
stock ModeDM_OnRoundStart()
{
    g_iRoundNum++;

    // Role swap every N rounds
    new swapRounds = GetCvarInt(CVAR_DM_ROLE_SWAP);
    if (swapRounds > 0 && g_iRoundNum % swapRounds == 0)
    {
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
        Hnsic_PrintChat(0, "角色互换!");
    }
}

// ─── DM On Round End ─────────────────────────────────────────
stock ModeDM_OnRoundEnd(winner)
{
    // Nothing special, DM is continuous
}
