#if defined _hnsic_gameplay_hns_included
    #endinput
#endif
#define _hnsic_gameplay_hns_included

/* ============================================================
 *  HNSIC v3.10.0 - gameplay_hns.inc
 *  HNS (Hide and Seek) gameplay
 *  TT: knife + flash + smoke + silent feet + hidden model
 *  CT: knife + flashlight
 * ============================================================ */

// ─── HNS Per-Player Data ────────────────────────────────────
new Float:g_fHns_FlashTimer[MAX_PLAYERS + 1];
new Float:g_fHns_SmokeTimer[MAX_PLAYERS + 1];
new bool:g_bHns_ModelHidden[MAX_PLAYERS + 1];

// ─── HNS Init ────────────────────────────────────────────────
stock GameplayHNS_Init()
{
    for (new i = 1; i <= g_iMaxPlayers; i++)
    {
        g_fHns_FlashTimer[i] = 0.0;
        g_fHns_SmokeTimer[i] = 0.0;
        g_bHns_ModelHidden[i] = false;
    }
}

// ─── HNS Cleanup ─────────────────────────────────────────────
stock GameplayHNS_Cleanup()
{
    for (new i = 1; i <= g_iMaxPlayers; i++)
    {
        g_bHns_ModelHidden[i] = false;
        remove_task(i + TASK_HIDE_MODEL);
    }
}

// ─── HNS On Spawn ────────────────────────────────────────────
stock GameplayHNS_OnSpawn(id)
{
    new team = get_member(id, m_iTeam);

    // Strip all weapons
    Hnsic_StripAllWeapons(id);

    if (team == TEAM_TT)
    {
        // TT: knife + flash + smoke
        Hnsic_GiveWeapon(id, "weapon_flashbang");
        Hnsic_GiveWeapon(id, "weapon_smokegrenade");

        // Silent feet
        if (GetCvarInt(CVAR_HNS_SILENT_FEET))
        {
            set_member(id, m_flStepSoundTime, 9999.0);
        }

        // Hidden model (after freeze time)
        g_bHns_ModelHidden[id] = false;
        set_task(3.0, "GameplayHNS_HideModel", id + TASK_HIDE_MODEL);
    }
    else if (team == TEAM_CT)
    {
        // CT: knife + flashlight
        // Flashlight is default, just ensure battery
        set_member(id, m_iFlashBattery, 100);
    }

    // Reset timers
    g_fHns_FlashTimer[id] = 0.0;
    g_fHns_SmokeTimer[id] = 0.0;
}

// ─── HNS Hide Model ─────────────────────────────────────────
public GameplayHNS_HideModel(taskid)
{
    new id = taskid - TASK_HIDE_MODEL;
    if (!Hnsic_IsPlayerAlive(id) || get_member(id, m_iTeam) != TEAM_TT)
        return;

    // Make TT model invisible/hidden
    set_entvar(id, var_rendermode, kRenderTransTexture);
    set_entvar(id, var_renderamt, 15.0); // Nearly invisible
    g_bHns_ModelHidden[id] = true;
}

// ─── HNS Apply Speed ──────────────────────────────────────────
stock GameplayHNS_ApplySpeed(id)
{
    new team = get_member(id, m_iTeam);
    new Float:speed;

    if (team == TEAM_TT)
    {
        speed = 250.0 * GetCvarFloat(CVAR_HNS_TT_SPEED);
    }
    else if (team == TEAM_CT)
    {
        speed = 250.0 * GetCvarFloat(CVAR_HNS_CT_SPEED);
    }
    else
    {
        speed = 250.0;
    }

    Hnsic_SetSpeed(id, speed);
}

// ─── HNS On Kill ─────────────────────────────────────────────
stock GameplayHNS_OnKill(victim, killer)
{
    if (!Hnsic_IsPlayerValid(killer))
        return;

    new killerTeam = get_member(killer, m_iTeam);
    new victimTeam = get_member(victim, m_iTeam);

    // CT killing TT is expected in HNS
    if (killerTeam == TEAM_CT && victimTeam == TEAM_TT)
    {
        // Award points
        User_AddPts(killer, 5);
        User_AddKill(killer);
        User_AddDeath(victim);
    }
}

// ─── HNS On Round Start ───────────────────────────────────────
stock GameplayHNS_OnRoundStart()
{
    // Reset all timers
    for (new i = 1; i <= g_iMaxPlayers; i++)
    {
        g_fHns_FlashTimer[i] = 0.0;
        g_fHns_SmokeTimer[i] = 0.0;
        g_bHns_ModelHidden[i] = false;
    }
}

// ─── HNS On Round End ────────────────────────────────────────
stock GameplayHNS_OnRoundEnd(winner)
{
    // Restore all models
    for (new i = 1; i <= g_iMaxPlayers; i++)
    {
        if (Hnsic_IsPlayerAlive(i))
        {
            set_entvar(i, var_rendermode, kRenderNormal);
            set_entvar(i, var_renderamt, 255.0);
            g_bHns_ModelHidden[i] = false;
        }
    }
}

// ─── HNS Flash Charge (recharge over time) ───────────────────
stock GameplayHNS_GiveFlash(id)
{
    if (!Hnsic_IsPlayerAlive(id) || get_member(id, m_iTeam) != TEAM_TT)
        return;

    new Float:currentTime = get_gametime();
    new Float:chargeTime = GetCvarFloat(CVAR_HNS_FLASH_CHARGE);

    if (currentTime - g_fHns_FlashTimer[id] >= chargeTime)
    {
        rg_give_item(id, "weapon_flashbang");
        g_fHns_FlashTimer[id] = currentTime;
    }
}

// ─── HNS Smoke Charge ────────────────────────────────────────
stock GameplayHNS_GiveSmoke(id)
{
    if (!Hnsic_IsPlayerAlive(id) || get_member(id, m_iTeam) != TEAM_TT)
        return;

    new Float:currentTime = get_gametime();
    new Float:chargeTime = GetCvarFloat(CVAR_HNS_SMOKE_CHARGE);

    if (currentTime - g_fHns_SmokeTimer[id] >= chargeTime)
    {
        rg_give_item(id, "weapon_smokegrenade");
        g_fHns_SmokeTimer[id] = currentTime;
    }
}
