#if defined _hnsic_mode_zombie_included
    #endinput
#endif
#define _hnsic_mode_zombie_included

/* ============================================================
 *  HNSIC v1.0.0 - mode_zombie.inl
 *  Zombie mode (infection)
 * ============================================================ */

// ─── Zombie Per-Player Data ──────────────────────────────────
new bool:g_bZombie[MAX_PLAYERS + 1];
new g_iFirstZombie;

// ─── Zombie Init ──────────────────────────────────────────────
stock ModeZombie_Init()
{
    // Reset all zombie flags
    for (new i = 1; i <= g_iMaxPlayers; i++)
    {
        g_bZombie[i] = false;
    }
    g_iFirstZombie = 0;

    Hnsic_PrintChat(0, "僵尸模式已启用 - 感染机制");
}

// ─── Zombie Cleanup ───────────────────────────────────────────
stock ModeZombie_Cleanup()
{
    for (new i = 1; i <= g_iMaxPlayers; i++)
    {
        g_bZombie[i] = false;
    }
    g_iFirstZombie = 0;
}

// ─── Zombie On Spawn ─────────────────────────────────────────
stock ModeZombie_OnSpawn(id)
{
    // Give knife
    Hnsic_StripAllWeapons(id);

    if (g_bZombie[id])
    {
        // Zombie: faster speed, blue glow
        Hnsic_SetSpeed(id, 300.0);
        set_entvar(id, var_rendercolor, 0, 150, 0); // Green tint
    }
    else
    {
        // Human: normal speed
        Hnsic_SetSpeed(id, 250.0);
        set_entvar(id, var_rendercolor, 255, 255, 255);
    }
}

// ─── Zombie On Kill ───────────────────────────────────────────
stock ModeZombie_OnKill(victim, killer)
{
    if (!Hnsic_IsPlayerValid(killer))
        return;

    // Zombie killing human: infect the human
    if (g_bZombie[killer] && !g_bZombie[victim])
    {
        g_bZombie[victim] = true;
        Hnsic_PrintChat(0, "%s 被感染了!", g_eUserData[victim][UD_NAME]);

        // Check if all humans infected
        ModeZombie_CheckWin();
    }
    else if (!g_bZombie[killer] && g_bZombie[victim])
    {
        // Human killing zombie: zombie respawns
        User_AddPts(killer, 2);
        User_AddKill(killer);
    }
}

// ─── Zombie On Round Start ────────────────────────────────────
stock ModeZombie_OnRoundStart()
{
    // Reset zombie flags
    for (new i = 1; i <= g_iMaxPlayers; i++)
    {
        g_bZombie[i] = false;
    }

    // Select first zombie(s)
    new Float:ratio = GetCvarFloat(CVAR_ZOMBIE_FIRST_RATIO);
    new totalPlayers = Hnsic_GetPlayerCount();
    new zombieCount = max(1, floatround(totalPlayers * ratio));

    // Collect CT players (zombies start from CT)
    new candidates[MAX_PLAYERS];
    new count = 0;
    for (new i = 1; i <= g_iMaxPlayers; i++)
    {
        if (Hnsic_IsPlayerOnTeam(i))
        {
            candidates[count] = i;
            count++;
        }
    }

    // Random selection
    Hnsic_ShuffleArray(candidates, count);

    for (new i = 0; i < min(zombieCount, count); i++)
    {
        g_bZombie[candidates[i]] = true;
        Hnsic_SwitchTeam(candidates[i], TEAM_TT); // Zombies go TT
        if (i == 0)
            g_iFirstZombie = candidates[i];
    }

    // Remaining players go CT (humans)
    for (new i = 1; i <= g_iMaxPlayers; i++)
    {
        if (Hnsic_IsPlayerOnTeam(i) && !g_bZombie[i])
        {
            Hnsic_SwitchTeam(i, TEAM_CT);
        }
    }

    Hnsic_PrintChat(0, "僵尸模式开始! %d名僵尸 vs %d名人类",
        zombieCount, totalPlayers - zombieCount);
}

// ─── Zombie On Round End ──────────────────────────────────────
stock ModeZombie_OnRoundEnd(winner)
{
    // Nothing special
}

// ─── Check Zombie Win Condition ─────────────────────────────
stock ModeZombie_CheckWin()
{
    new humanCount = 0;
    new zombieCount = 0;

    for (new i = 1; i <= g_iMaxPlayers; i++)
    {
        if (!Hnsic_IsPlayerAlive(i) || !Hnsic_IsPlayerOnTeam(i))
            continue;

        if (g_bZombie[i])
            zombieCount++;
        else
            humanCount++;
    }

    if (humanCount == 0)
    {
        // Zombies win
        rg_round_end(3.0, WINSTATUS_TERRORISTS, ROUND_TERRORISTS_WIN, 0.0, 0x01);
    }
    else if (zombieCount == 0)
    {
        // Humans win
        rg_round_end(3.0, WINSTATUS_CTS, ROUND_CTS_WIN, 0.0, 0x01);
    }
}

// ─── Zombie Infection on Touch ───────────────────────────────
stock ModeZombie_TouchInfector(attacker, victim)
{
    if (g_iCurrentMode != MODE_ZOMBIE)
        return;

    if (!Hnsic_IsPlayerAlive(attacker) || !Hnsic_IsPlayerAlive(victim))
        return;

    if (!g_bZombie[attacker] || g_bZombie[victim])
        return;

    new Float:infectTime = GetCvarFloat(CVAR_ZOMBIE_INFECTION_TIME);

    // Infect with delay
    set_task(infectTime, "ModeZombie_Infect", victim + TASK_MATCH_TIMER);
}

public ModeZombie_Infect(taskid)
{
    new victim = taskid - TASK_MATCH_TIMER;
    if (!Hnsic_IsPlayerAlive(victim))
        return;

    g_bZombie[victim] = true;
    Hnsic_SwitchTeam(victim, TEAM_TT);
    Hnsic_PrintChat(0, "%s 被感染了!", g_eUserData[victim][UD_NAME]);

    ModeZombie_CheckWin();
}
