/*
 * HnsIC - Hide'n'Seek Integrated Competition
 * Version: 2.0.0
 * Author: OpenHNS
 *
 * A complete rewrite merging 20+ HNS plugins into a single unified system.
 * Uses direct function calls instead of forward-based mode dispatch.
 */

#include <amxmodx>
#include <amxmisc>
#include <fakemeta>
#include <fakemeta_util>
#include <reapi>
#include <json>
#include <PersistentDataStorage>

// Internal includes - correct dependency order
#include "hnsic/hnsic_core.inc"
#include "hnsic/hnsic_cvars.inc"
#include "hnsic/hnsic_gameplay.inc"
#include "hnsic/hnsic_modes.inc"
#include "hnsic/hnsic_commands.inc"
#include "hnsic/hnsic_pointscap_editor.inc"
#include "hnsic/hnsic_player_models.inc"
#include "hnsic/hnsic_semiclip.inc"
#include "hnsic/hnsic_skin_system.inc"
#include "hnsic/hnsic_training.inc"
#include "hnsic/hnsic_stats.inc"
#include "hnsic/hnsic_chatmanager.inc"
#include "hnsic/hnsic_maps.inc"
#include "hnsic/hnsic_flynade.inc"
#include "hnsic/hnsic_ownage.inc"
#include "hnsic/hnsic_jumpstats.inc"
#include "hnsic/hnsic_flash_notifier.inc"
#include "hnsic/hnsic_player_info.inc"
#include "hnsic/hnsic_spectator_info.inc"
#include "hnsic/hnsic_mode_hud.inc"
#include "hnsic/hnsic_perm_system.inc"
#include "hnsic/hnsic_watcher.inc"
#include "hnsic/hnsic_recontrol.inc"
#include "hnsic/hnsic_ai_teams.inc"
#include "hnsic/hnsic_map_rules.inc"

/* ============================
   PLUGIN INFO
   ============================ */
public plugin_init()
{
    register_plugin("HnsIC - Hide'n'Seek Integrated Competition", "2.0.0", "OpenHNS");

    // Get map name via ReAPI
    rh_get_mapname(g_szMapName, charsmax(g_szMapName));

    // Initialize all subsystems in correct dependency order
    cvars_init();
    gameplay_init();
    modes_init();
    commands_init();
    pointscap_editor_init();
    player_models_init();
    semiclip_init();
    skin_system_init();
    training_init();
    stats_init();
    chatmanager_init();
    maps_init();
    flynade_init();
    ownage_init();
    jumpstats_init();
    flash_notifier_init();
    player_info_init();
    spectator_info_init();
    mode_hud_init();
    perm_system_init();
    watcher_init();
    recontrol_init();
    ai_teams_init();
    map_rules_init();

    // Register FM forwards
    register_forward(FM_EmitSound, "fw_EmitSound");
    register_forward(FM_ClientKill, "fw_ClientKill");
    register_forward(FM_GetGameDescription, "fw_GetGameDescription");
    register_forward(FM_CmdStart, "fw_CmdStart");

    // Register ReAPI HookChains - Round management
    RegisterHookChain(RG_RoundEnd, "fw_RoundEnd_Post", true);
    RegisterHookChain(RG_CSGameRules_RestartRound, "fw_RestartRound_Pre", false);
    RegisterHookChain(RG_CSGameRules_OnRoundFreezeEnd, "fw_OnRoundFreezeEnd_Pre", false);

    // Register ReAPI HookChains - Player movement
    // 不注册 RG_CBasePlayer_ResetMaxSpeed（需要 ReGameDLL，兼容无 ReGameDLL 服务器）
    // RegisterHookChain(RG_CBasePlayer_ResetMaxSpeed, "fw_ResetMaxSpeed_Pre", false);
    RegisterHookChain(RG_CBasePlayer_PreThink, "fw_PreThink_Pre", false);
    RegisterHookChain(RG_CBasePlayer_PostThink, "fw_PostThink_Post", true);

    // Register ReAPI HookChains - Fall damage (PRE + POST)
    RegisterHookChain(RG_CSGameRules_FlPlayerFallDamage, "fw_FallDamage_Pre", false);
    RegisterHookChain(RG_CSGameRules_FlPlayerFallDamage, "fw_FallDamage_Post", true);

    // Register ReAPI HookChains - Player lifecycle
    RegisterHookChain(RG_CBasePlayer_Spawn, "fw_PlayerSpawn_Post", true);
    RegisterHookChain(RG_CBasePlayer_Killed, "fw_PlayerKilled_Pre", false);
    RegisterHookChain(RG_CBasePlayer_Killed, "fw_PlayerKilled_Post", true);

    // Register ReAPI HookChains - Effects
    RegisterHookChain(RG_PlayerBlind, "fw_PlayerBlind_Post", true);

    // Register ReAPI HookChains - Bomb
    RegisterHookChain(RG_CBasePlayer_MakeBomber, "fw_MakeBomber_Pre", false);

    // Register message hooks
    // Block hostage position messages
    register_message(get_user_msgid("HostagePos"), "msg_HostagePos");

    // Auto-join: ShowMenu
    register_message(get_user_msgid("ShowMenu"), "msg_ShowMenu");

    // Auto-join: VGUIMenu
    register_message(get_user_msgid("VGUIMenu"), "msg_VGUIMenu");

    // Hide money display
    register_message(get_user_msgid("HideWeapon"), "msg_HideWeapon");

    // Chat manager for SayText
    register_message(get_user_msgid("SayText"), "msg_SayText");

    // Block HudTextArgs message
    register_message(get_user_msgid("HudTextArgs"), "msg_HudTextArgs");

    // Block Money message
    g_msgMoney = get_user_msgid("Money");
    register_message(g_msgMoney, "msg_Money");

    // Register touch for ownage system
    register_touch("player", "player", "fw_PlayerTouch");

    // Set repeating task for ShowTimeAsMoney (0.1s loop)
    set_task(1.0, "task_ShowTimeAsMoney", _, _, _, "b");

    // Set repeating task for HudTask (1.0s loop)
    set_task(1.0, "task_HudTask", _, _, _, "b");

    // Create Trie for leave data
    g_eMatchInfo[m_tLeaveData] = TrieCreate();

    // Register dictionary for multilingual support
    register_dictionary("hnsic.txt");

    // Execute match system configuration
    exec_cfg();

    // Set debug mode flag
    g_bDebugMode = (get_cvar_num("hnsic_debug") > 0);

    // Unregister FM_Spawn forward from precache (no longer needed)
    unregister_forward(FM_Spawn, g_iRegisterSpawn);
}

/* ============================
   PLUGIN PRECACHE
   ============================ */
public plugin_precache()
{
    // Register FM_Spawn forward for entity manipulation during map load
    g_iRegisterSpawn = register_forward(FM_Spawn, "fw_Spawn");

    // Create buyzone entity (ensures buyzone exists for weapon restrictions)
    new iEnt = engfunc(EngFunc_CreateNamedEntity, engfunc(EngFunc_AllocString, "func_buyzone"));
    if (iEnt > 0)
    {
        dllfunc(DLLFunc_Spawn, iEnt);
        set_pev(iEnt, pev_solid, SOLID_NOT);
    }

    // Precache sounds
    precache_sound(g_szSoundButton);
    precache_sound(g_szSoundDeny);

    // Precache pointscap sounds (if they exist)
    if (file_exists("sound/hnsic/capture.wav"))
        precache_sound(g_szSoundCapture);
    if (file_exists("sound/hnsic/knifekill.wav"))
        precache_sound(g_szSoundKnifeKill);
    if (file_exists("sound/hnsic/score.wav"))
        precache_sound(g_szSoundScore);

    // Precache laserbeam sprite
    iBeam = precache_model(g_szLaserBeamSprite);
}

/* ============================
   PLUGIN CFG
   ============================ */
public plugin_cfg()
{
    // Get amxx logs path
    new szLogDir[256];
    get_localinfo("amxx_logs", szLogDir, charsmax(szLogDir));

    // Create hnsic log directory
    formatex(g_szLogPath, charsmax(g_szLogPath), "%s/hnsic", szLogDir);
    if (!dir_exists(g_szLogPath))
    {
        mkdir(g_szLogPath);
    }

    LogSendMessage("HnsIC v2.0.0 started on map: %s", g_szMapName);

    // Cache base hostname
    get_cvar_string("hostname", g_szBaseHostname, charsmax(g_szBaseHostname));
}

/* ============================
   PLUGIN END
   ============================ */
public plugin_end()
{
    // Clean up Trie
    if (g_eMatchInfo[m_tLeaveData])
    {
        TrieDestroy(g_eMatchInfo[m_tLeaveData]);
        g_eMatchInfo[m_tLeaveData] = Invalid_Trie;
    }

    // Clean up prefix Tries
    if (g_tSteamPrefix)
    {
        TrieDestroy(g_tSteamPrefix);
        g_tSteamPrefix = Invalid_Trie;
    }
    if (g_tNamePrefix)
    {
        TrieDestroy(g_tNamePrefix);
        g_tNamePrefix = Invalid_Trie;
    }

    // Clean up permission Trie
    if (g_tPermData)
    {
        TrieDestroy(g_tPermData);
        g_tPermData = Invalid_Trie;
    }

    // Clean up model arrays
    if (g_aTModels != Invalid_Array)
        ArrayDestroy(g_aTModels);
    if (g_aCTModels != Invalid_Array)
        ArrayDestroy(g_aCTModels);

    // Clean up skin arrays
    if (g_aSkinTModels != Invalid_Array)
        ArrayDestroy(g_aSkinTModels);
    if (g_aSkinCTModels != Invalid_Array)
        ArrayDestroy(g_aSkinCTModels);
    if (g_aSkinKnifeModels != Invalid_Array)
        ArrayDestroy(g_aSkinKnifeModels);
    if (g_aSkinTNames != Invalid_Array)
        ArrayDestroy(g_aSkinTNames);
    if (g_aSkinCTNames != Invalid_Array)
        ArrayDestroy(g_aSkinCTNames);
    if (g_aSkinKnifeNames != Invalid_Array)
        ArrayDestroy(g_aSkinKnifeNames);

    // Clean up map arrays
    if (g_aMapSections != Invalid_Array)
        ArrayDestroy(g_aMapSections);
    if (g_aMapSectionNames != Invalid_Array)
        ArrayDestroy(g_aMapSectionNames);

    // Remove all tasks
    remove_task(TASK_TIMER);
    remove_task(HUD_PAUSE);
    remove_task(TASK_WAIT);
    remove_task(TASK_WAIT_CUP);
    remove_task(TASK_STARTED);
    remove_task(TASK_WAITCAP);
    remove_task(TASK_WAITLEAVECAP);
    remove_task(TASK_SURRENDER);
    remove_task(TASK_POINTSCAP_DETECT);
    remove_task(TASK_POINTSCAP_KNIFE);
    remove_task(TASK_POINTSCAP_HUD);
    remove_task(TASK_POINTS);
    remove_task(TASK_HOOK);
    remove_task(TASK_AFK);

    LogSendMessage("HnsIC plugin ended");
}

/* ============================
   FORWARD HOOKS - FM
   ============================ */

// FM_Spawn - Called during map load to remove default entities
public fw_Spawn(iEnt)
{
    if (!pev_valid(iEnt))
        return FMRES_IGNORED;

    static szClassname[32];
    pev(iEnt, pev_classname, szClassname, charsmax(szClassname));

    for (new i = 0; i < sizeof(g_szDefaultEntities); i++)
    {
        if (equal(szClassname, g_szDefaultEntities[i]))
        {
            engfunc(EngFunc_RemoveEntity, iEnt);
            return FMRES_SUPERCEDE;
        }
    }

    return FMRES_IGNORED;
}

// FM_EmitSound - Sound interception
public fw_EmitSound(id, channel, const szSample[], Float:flVolume, Float:flAttn, iFlags, iPitch)
{
    // Dispatch to relevant modules
    // Chat manager handles custom sounds
    if (chatmanager_emit_sound(id, channel, szSample, flVolume, flAttn, iFlags, iPitch))
        return FMRES_SUPERCEDE;

    // Flash notifier handles flash sound interception
    if (flash_notifier_emit_sound(id, channel, szSample, flVolume, flAttn, iFlags, iPitch))
        return FMRES_SUPERCEDE;

    return FMRES_IGNORED;
}

// FM_ClientKill - Block suicide in certain modes
public fw_ClientKill(id)
{
    if (!is_user_connected(id))
        return FMRES_IGNORED;

    // Dispatch to gameplay module
    if (gameplay_client_kill(id))
        return FMRES_SUPERCEDE;

    return FMRES_IGNORED;
}

// FM_GetGameDescription - Custom game name
public fw_GetGameDescription()
{
    static szGameName[32];
    get_pcvar_string(g_pCvars[CVAR_GAMENAME], szGameName, charsmax(szGameName));

    if (szGameName[0] != EOS)
    {
        forward_return(FMV_STRING, szGameName);
        return FMRES_SUPERCEDE;
    }

    return FMRES_IGNORED;
}

// FM_CmdStart - Command start for jumpstats and other tracking
public fw_CmdStart(id, uc_handle, seed)
{
    if (!is_user_alive(id))
        return FMRES_IGNORED;

    // Dispatch to jumpstats module
    jumpstats_cmd_start(id, uc_handle, seed);

    // Dispatch to training module for hook
    training_cmd_start(id, uc_handle, seed);

    return FMRES_IGNORED;
}

/* ============================
   FORWARD HOOKS - ReAPI
   ============================ */

// RG_RoundEnd (POST)
public fw_RoundEnd_Post(WinStatus:status, ScenarioEventEndRound:event, Float:flDelay)
{
    // Dispatch to gameplay module
    gameplay_round_end(status, event, flDelay);

    // Dispatch to stats module
    stats_round_end(status, event);

    // Dispatch to modes module
    modes_round_end(status, event);

    // Dispatch to pointscap if applicable
    if (isPointScapMode())
        modes_pointscap_round_end(status, event);

    // Dispatch to ownage module
    ownage_round_end();

    // Dispatch to jumpstats module
    jumpstats_round_end();

    // Dispatch to player_info module
    player_info_round_end();

    // Dispatch to spectator_info module
    spectator_info_round_end();

    // Dispatch to afk module
    afk_round_end();

    // Dispatch to flynade module
    flynade_round_end();

    // Dispatch to mode_hud module
    mode_hud_round_end();

    // Dispatch to map_rules module
    map_rules_round_end();

    // Dispatch to recontrol module
    recontrol_round_end();

    // Dispatch to ai_teams module
    ai_teams_round_end();
}

// RG_CBasePlayer_ResetMaxSpeed (PRE)
public fw_ResetMaxSpeed_Pre(id)
{
    if (!is_user_alive(id))
        return HC_CONTINUE;

    // Dispatch to gameplay module
    if (gameplay_reset_maxspeed(id))
        return HC_SUPERCEDE;

    // Dispatch to training module
    if (training_reset_maxspeed(id))
        return HC_SUPERCEDE;

    return HC_CONTINUE;
}

// RG_CSGameRules_RestartRound (PRE)
public fw_RestartRound_Pre()
{
    // Dispatch to gameplay module
    gameplay_restart_round();

    // Dispatch to stats module
    stats_restart_round();

    // Dispatch to modes module
    modes_restart_round();

    // Dispatch to pointscap if applicable
    if (isPointScapMode())
        modes_pointscap_restart_round();

    // Dispatch to ownage module
    ownage_restart_round();

    // Dispatch to jumpstats module
    jumpstats_restart_round();

    // Dispatch to player_info module
    player_info_restart_round();

    // Dispatch to spectator_info module
    spectator_info_restart_round();

    // Dispatch to flynade module
    flynade_restart_round();

    // Dispatch to mode_hud module
    mode_hud_restart_round();

    // Dispatch to map_rules module
    map_rules_restart_round();

    // Dispatch to recontrol module
    recontrol_restart_round();

    // Dispatch to ai_teams module
    ai_teams_restart_round();

    return HC_CONTINUE;
}

// RG_CSGameRules_OnRoundFreezeEnd (PRE)
public fw_OnRoundFreezeEnd_Pre()
{
    // Dispatch to gameplay module
    gameplay_freeze_end();

    // Dispatch to stats module
    stats_freeze_end();

    // Dispatch to modes module
    modes_freeze_end();

    // Dispatch to pointscap if applicable
    if (isPointScapMode())
        modes_pointscap_freeze_end();

    // Dispatch to ownage module
    ownage_freeze_end();

    // Dispatch to jumpstats module
    jumpstats_freeze_end();

    // Dispatch to player_info module
    player_info_freeze_end();

    // Dispatch to spectator_info module
    spectator_info_freeze_end();

    // Dispatch to flynade module
    flynade_freeze_end();

    // Dispatch to mode_hud module
    mode_hud_freeze_end();

    // Dispatch to map_rules module
    map_rules_freeze_end();

    // Dispatch to recontrol module
    recontrol_freeze_end();

    // Dispatch to ai_teams module
    ai_teams_freeze_end();

    return HC_CONTINUE;
}

// RG_CSGameRules_FlPlayerFallDamage (PRE) - Prevent fall damage
public fw_FallDamage_Pre(id, Float:flFallVelocity)
{
    if (!is_user_alive(id))
        return HC_CONTINUE;

    // Dispatch to gameplay module for fall damage control
    if (gameplay_fall_damage(id, flFallVelocity))
    {
        SetHookChainReturn(ATYPE_FLOAT, 0.0);
        return HC_SUPERCEDE;
    }

    // Dispatch to training module
    if (training_fall_damage(id, flFallVelocity))
    {
        SetHookChainReturn(ATYPE_FLOAT, 0.0);
        return HC_SUPERCEDE;
    }

    return HC_CONTINUE;
}

// RG_CSGameRules_FlPlayerFallDamage (POST) - Track fall damage
public fw_FallDamage_Post(id, Float:flFallVelocity)
{
    if (!is_user_alive(id))
        return;

    // Dispatch to jumpstats module for landing detection
    jumpstats_fall_damage(id, flFallVelocity);
}

// RG_CBasePlayer_Spawn (POST)
public fw_PlayerSpawn_Post(id)
{
    if (!is_user_alive(id))
        return;

    // Dispatch to gameplay module
    gameplay_player_spawn(id);

    // Dispatch to stats module
    stats_player_spawn(id);

    // Dispatch to modes module
    modes_player_spawn(id);

    // Dispatch to pointscap if applicable
    if (isPointScapMode())
        modes_pointscap_player_spawn(id);

    // Dispatch to ownage module
    ownage_player_spawn(id);

    // Dispatch to jumpstats module
    jumpstats_player_spawn(id);

    // Dispatch to player_info module
    player_info_player_spawn(id);

    // Dispatch to spectator_info module
    spectator_info_player_spawn(id);

    // Dispatch to flynade module
    flynade_player_spawn(id);

    // Dispatch to mode_hud module
    mode_hud_player_spawn(id);

    // Dispatch to map_rules module
    map_rules_player_spawn(id);

    // Dispatch to recontrol module
    recontrol_player_spawn(id);

    // Dispatch to ai_teams module
    ai_teams_player_spawn(id);

    // Dispatch to player_models module
    player_models_player_spawn(id);

    // Dispatch to skin_system module
    skin_system_player_spawn(id);

    // Dispatch to semiclip module
    semiclip_player_spawn(id);

    // Dispatch to flash_notifier module
    flash_notifier_player_spawn(id);

    // Dispatch to perm_system module
    perm_system_player_spawn(id);

    // Dispatch to watcher module
    watcher_player_spawn(id);

    // Dispatch to knife viewmodel
    g_iNextKnifeAttack[id] = 0;
}

// RG_CBasePlayer_Killed (PRE)
public fw_PlayerKilled_Pre(id, attacker, iGib)
{
    if (!is_user_connected(id))
        return HC_CONTINUE;

    // Dispatch to gameplay module
    gameplay_player_killed_pre(id, attacker, iGib);

    // Dispatch to stats module
    stats_player_killed_pre(id, attacker);

    // Dispatch to modes module
    modes_player_killed_pre(id, attacker);

    // Dispatch to pointscap if applicable
    if (isPointScapMode())
        modes_pointscap_player_killed_pre(id, attacker);

    // Dispatch to ownage module
    ownage_player_killed_pre(id, attacker);

    // Dispatch to jumpstats module
    jumpstats_player_killed_pre(id, attacker);

    // Dispatch to player_info module
    player_info_player_killed_pre(id, attacker);

    // Dispatch to flynade module
    flynade_player_killed_pre(id, attacker);

    // Dispatch to map_rules module
    map_rules_player_killed_pre(id, attacker);

    // Dispatch to recontrol module
    recontrol_player_killed_pre(id, attacker);

    // Dispatch to ai_teams module
    ai_teams_player_killed_pre(id, attacker);

    return HC_CONTINUE;
}

// RG_CBasePlayer_Killed (POST)
public fw_PlayerKilled_Post(id, attacker, iGib)
{
    if (!is_user_connected(id))
        return;

    // Dispatch to gameplay module
    gameplay_player_killed_post(id, attacker, iGib);

    // Dispatch to stats module
    stats_player_killed_post(id, attacker);

    // Dispatch to modes module
    modes_player_killed_post(id, attacker);

    // Dispatch to pointscap if applicable
    if (isPointScapMode())
        modes_pointscap_player_killed_post(id, attacker);

    // Dispatch to ownage module
    ownage_player_killed_post(id, attacker);

    // Dispatch to jumpstats module
    jumpstats_player_killed_post(id, attacker);

    // Dispatch to player_info module
    player_info_player_killed_post(id, attacker);

    // Dispatch to flynade module
    flynade_player_killed_post(id, attacker);

    // Dispatch to mode_hud module
    mode_hud_player_killed_post(id, attacker);

    // Dispatch to map_rules module
    map_rules_player_killed_post(id, attacker);

    // Dispatch to recontrol module
    recontrol_player_killed_post(id, attacker);

    // Dispatch to ai_teams module
    ai_teams_player_killed_post(id, attacker);

    // Dispatch to semiclip module
    semiclip_player_killed_post(id, attacker);
}

// RG_PlayerBlind (POST)
public fw_PlayerBlind_Post(id, attacker, Float:flFadeTime, Float:flHoldTime, Float:flAlpha, iFlags)
{
    if (!is_user_connected(id))
        return;

    // Dispatch to flash_notifier module
    flash_notifier_player_blind(id, attacker, flFadeTime, flHoldTime, flAlpha, iFlags);

    // Dispatch to stats module
    stats_player_blind(id, attacker, flFadeTime, flHoldTime);
}

// RG_CBasePlayer_MakeBomber (PRE) - Block bomb assignment
public fw_MakeBomber_Pre(id, bool:bIsC4, bool:bForceGameMode)
{
    // Always block bomb assignment in HNS
    SetHookChainReturn(ATYPE_BOOL, false);
    return HC_SUPERCEDE;
}

// RG_CBasePlayer_PreThink (PRE)
public fw_PreThink_Pre(id)
{
    if (!is_user_alive(id))
        return;

    // Dispatch to semiclip module
    semiclip_prethink(id);

    // Dispatch to training module
    training_prethink(id);

    // Dispatch to flynade module
    flynade_prethink(id);

    // Dispatch to stats module (running/hiding tracking)
    stats_prethink(id);

    // Dispatch to jumpstats module
    jumpstats_prethink(id);

    // Dispatch to ownage module
    ownage_prethink(id);

    // Dispatch to afk module
    afk_prethink(id);

    // Dispatch to map_rules module
    map_rules_prethink(id);

    // Dispatch to recontrol module
    recontrol_prethink(id);

    // Dispatch to ai_teams module
    ai_teams_prethink(id);
}

// RG_CBasePlayer_PostThink (POST)
public fw_PostThink_Post(id)
{
    if (!is_user_alive(id))
        return;

    // Dispatch to semiclip module
    semiclip_postthink(id);

    // Dispatch to training module
    training_postthink(id);

    // Dispatch to flynade module
    flynade_postthink(id);

    // Dispatch to stats module
    stats_postthink(id);

    // Dispatch to jumpstats module
    jumpstats_postthink(id);

    // Dispatch to ownage module
    ownage_postthink(id);

    // Dispatch to map_rules module
    map_rules_postthink(id);

    // Dispatch to recontrol module
    recontrol_postthink(id);

    // Dispatch to ai_teams module
    ai_teams_postthink(id);
}

/* ============================
   MESSAGE HOOKS
   ============================ */

// Block HostagePos messages
public msg_HostagePos()
{
    return PLUGIN_HANDLED;
}

// Auto-join: ShowMenu
public msg_ShowMenu(iMsgId, iMsgDest, id)
{
    if (get_msg_arg_int(1) == 510)
    {
        // This is the auto-join menu, force join
        new szTeam[2];
        get_msg_arg_string(4, szTeam, charsmax(szTeam));

        if (szTeam[0] != EOS)
        {
            // Force auto-join via gameplay module
            gameplay_auto_join(id, szTeam[0]);
            return PLUGIN_HANDLED;
        }
    }

    return PLUGIN_CONTINUE;
}

// Auto-join: VGUIMenu
public msg_VGUIMenu(iMsgId, iMsgDest, id)
{
    if (get_msg_arg_int(1) == 2)
    {
        // This is the team select VGUI menu, force auto-join
        gameplay_auto_join(id, 0);
        return PLUGIN_HANDLED;
    }

    return PLUGIN_CONTINUE;
}

// HideWeapon - Hide money display
public msg_HideWeapon(iMsgId, iMsgDest, id)
{
    // Add HIDEHUD_MONEY flag to hide money display
    new iFlags = get_msg_arg_int(1);
    set_msg_arg_int(1, ARG_BYTE, iFlags | (1 << 5)); // HIDEHUD_MONEY = (1 << 5)
    return PLUGIN_CONTINUE;
}

// SayText - Chat manager
public msg_SayText(iMsgId, iMsgDest, id)
{
    if (chatmanager_saytext(iMsgId, iMsgDest, id))
        return PLUGIN_HANDLED;

    return PLUGIN_CONTINUE;
}

// Block HudTextArgs
public msg_HudTextArgs()
{
    return PLUGIN_HANDLED;
}

// Block Money message
public msg_Money(iMsgId, iMsgDest, id)
{
    return PLUGIN_HANDLED;
}

/* ============================
   TOUCH HOOKS
   ============================ */

// Player to Player touch - Ownage system
public fw_PlayerTouch(iTouched, iToucher)
{
    if (!is_user_alive(iTouched) || !is_user_alive(iToucher))
        return;

    // Dispatch to ownage module
    ownage_player_touch(iTouched, iToucher);
}

/* ============================
   CLIENT CONNECTION
   ============================ */

public client_putinserver(id)
{
    // Initialize player data
    g_ePlayerInfo[id][m_iRole] = ROLE_SPEC;
    g_ePlayerInfo[id][m_bInMatch] = false;
    g_ePlayerInfo[id][m_bPickMe] = false;
    g_ePlayerInfo[id][m_szTeam][0] = EOS;
    g_ePlayerInfo[id][m_iLeaveRound] = 0;

    // Initialize stats
    for (new i = 0; i < PLAYER_STATS; i++)
    {
        iStats[id][i] = 0;
        g_StatsRound[id][i] = 0;
    }
    iStats[id][stat_playtime] = _:0.0;

    // Initialize other per-player data
    g_bKnifeViewModel[id] = false;
    g_iNextKnifeAttack[id] = 0;
    g_bPlayerSemiClip[id] = false;
    g_iSemiClipTeam[id] = 0;
    g_bCheckpointAlternate[id] = false;
    g_bDamage[id] = false;
    g_bSaveAngles[id] = false;
    isHook[id] = false;
    g_bIsRunning[id] = false;
    g_bIsHiding[id] = false;
    g_bCustomPrefix[id] = false;
    g_sPlayerPrefix[id][0] = EOS;
    g_bHasFlyNade[id] = false;
    g_bGrenadeThrown[id] = false;
    g_bFlyActive[id] = false;
    g_bJumpTracking[id] = false;
    g_bJumpInAir[id] = false;
    g_bJumpValid[id] = false;
    g_isSpecFlashed[id] = false;
    g_flLastHeadTouch[id] = 0.0;
    g_iPermLevel[id] = 0;
    g_bVerified[id] = false;
    g_bNoplay[id] = false;
    g_eSpecBack[id] = 0;
    g_iPlayerTModel[id] = 0;
    g_iPlayerCTModel[id] = 0;
    g_iModelSelectTeam[id] = 0;
    g_iActiveSkinT[id] = -1;
    g_iActiveSkinCT[id] = -1;
    g_iActiveSkinKnife[id] = -1;
    g_iSkinAdminVerify[id] = 0;
    g_iWatcherVote[id] = 0;
    g_flWatcherVoteTime[id] = 0.0;
    g_iPlayerSpecTarget[id] = 0;

    // Initialize AFK data
    eAfkData[id][afk_last_move] = _:get_gametime();
    eAfkData[id][afk_spawn_count] = 0;
    eAfkData[id][afk_is_afk] = false;
    vec_set(eAfkData[id][afk_last_origin], 0.0, 0.0, 0.0);

    // Initialize jumpstats data
    vec_set(g_flJumpOrigin[id], 0.0, 0.0, 0.0);
    g_flJumpStartZ[id] = 0.0;
    g_flJumpDistance[id] = 0.0;
    g_flJumpMaxHeight[id] = 0.0;
    g_flJumpHeight[id] = 0.0;
    g_iJumpType[id] = 0;
    g_flJumpPreSpeed[id] = 0.0;
    g_flJumpMaxSpeed[id] = 0.0;
    g_iJumpStrafes[id] = 0;
    g_iJumpSync[id] = 0;
    g_flJumpLastZVel[id] = 0.0;
    g_iJumpFrames[id] = 0;
    g_iJumpDuckCount[id] = 0;
    g_bJumpWeird[id] = false;
    g_flJumpTurnVel[id] = 0.0;
    g_flJumpLastAngle[id] = 0.0;

    // Initialize training checkpoints
    for (new i = 0; i < 2; i++)
        vec_set(g_fCheckpoints[id][i], 0.0, 0.0, 0.0);
    vec_set(g_fHookOrigin[id], 0.0, 0.0, 0.0);

    // Initialize flynade data
    vec_set(g_fGrenadeOrigin[id], 0.0, 0.0, 0.0);
    vec_set(g_fGrenadeVelocity[id], 0.0, 0.0, 0.0);

    // Initialize AFK origin
    vec_set(flAfkOrigin[id], 0.0, 0.0, 0.0);

    // Dispatch to modules
    stats_client_putinserver(id);
    chatmanager_client_putinserver(id);
    perm_system_client_putinserver(id);
    skin_system_client_putinserver(id);
    player_models_client_putinserver(id);
    flynade_client_putinserver(id);
    ownage_client_putinserver(id);
    jumpstats_client_putinserver(id);
    flash_notifier_client_putinserver(id);
    player_info_client_putinserver(id);
    spectator_info_client_putinserver(id);
    mode_hud_client_putinserver(id);
    watcher_client_putinserver(id);
    recontrol_client_putinserver(id);
    ai_teams_client_putinserver(id);
    map_rules_client_putinserver(id);
    training_client_putinserver(id);
    semiclip_client_putinserver(id);

    // Check deserter ban
    if (checkUserBan(id))
    {
        new szName[32];
        get_user_name(id, szName, charsmax(szName));
        chat_print(0, "%L", LANG_PLAYER, "DESERTER_BANNED", szName);
    }
}

public client_disconnected(id)
{
    // Remove all tasks for this player
    remove_task(id + TASK_TIMER);
    remove_task(id + HUD_PAUSE);
    remove_task(id + TASK_WAIT);
    remove_task(id + TASK_WAIT_CUP);
    remove_task(id + TASK_STARTED);
    remove_task(id + TASK_WAITCAP);
    remove_task(id + TASK_WAITLEAVECAP);
    remove_task(id + TASK_SURRENDER);
    remove_task(id + TASK_POINTSCAP_DETECT);
    remove_task(id + TASK_POINTSCAP_KNIFE);
    remove_task(id + TASK_POINTSCAP_HUD);
    remove_task(id + TASK_POINTS);
    remove_task(id + TASK_HOOK);
    remove_task(id + TASK_AFK);

    // Dispatch to modules
    stats_client_disconnected(id);
    chatmanager_client_disconnected(id);
    perm_system_client_disconnected(id);
    skin_system_client_disconnected(id);
    player_models_client_disconnected(id);
    flynade_client_disconnected(id);
    ownage_client_disconnected(id);
    jumpstats_client_disconnected(id);
    flash_notifier_client_disconnected(id);
    player_info_client_disconnected(id);
    spectator_info_client_disconnected(id);
    mode_hud_client_disconnected(id);
    watcher_client_disconnected(id);
    recontrol_client_disconnected(id);
    ai_teams_client_disconnected(id);
    map_rules_client_disconnected(id);
    training_client_disconnected(id);
    semiclip_client_disconnected(id);

    // Handle match leave
    if (g_ePlayerInfo[id][m_bInMatch])
    {
        modes_player_leave_match(id);
    }

    // Clean up flynade entity
    if (g_iFlyEnt[id] > 0 && pev_valid(g_iFlyEnt[id]))
    {
        engfunc(EngFunc_RemoveEntity, g_iFlyEnt[id]);
        g_iFlyEnt[id] = 0;
    }

    // Clean up hook entity
    if (isHook[id])
    {
        // Hook entity cleanup handled by training module
        training_client_disconnected(id);
    }

    // Log disconnect
    LogSendMessage("Player disconnected: %n (auth: %s)", id, getUserKey(id));
}

/* ============================
   REPEATING TASKS
   ============================ */

// Show time as money (replaces money display with round time)
public task_ShowTimeAsMoney()
{
    new iPlayers[MAX_PLAYERS], iNum;
    get_players(iPlayers, iNum, "ch");

    for (new i = 0; i < iNum; i++)
    {
        new id = iPlayers[i];

        if (getUserTeam(id) == TEAM_SPECTATOR)
            continue;

        new Float:flTimeLeft = get_round_time();

        if (flTimeLeft < 0.0)
            flTimeLeft = 0.0;

        // Send money message with round time encoded
        // We use the money message to display round time
        new iTime = floatround(flTimeLeft);

        // Show as money value: time in seconds (max 16000 fits in money range)
        engfunc(EngFunc_MessageBegin, MSG_ONE_UNRELIABLE, g_msgMoney, _, id);
        write_long(iTime);
        write_byte(0);
        message_end();
    }
}

// Main HUD task (1.0s loop)
public task_HudTask()
{
    // Dispatch to mode_hud module
    mode_hud_task();

    // Dispatch to spectator_info module
    spectator_info_task();

    // Dispatch to pointscap HUD if applicable
    if (isPointScapMode())
        modes_pointscap_hud_task();

    // Dispatch to afk module
    afk_task();

    // Dispatch to watcher module
    watcher_task();

    // Dispatch to recontrol module
    recontrol_task();

    // Dispatch to ai_teams module
    ai_teams_task();

    // Dispatch to map_rules module
    map_rules_task();
}

/* ============================
   HELPER FUNCTIONS
   ============================ */

// Execute match system configuration
stock exec_cfg()
{
    new szPath[256];
    get_localinfo("amxx_configsdir", szPath, charsmax(szPath));
    add(szPath, charsmax(szPath), "/plugins/hnsic/matchsystem.cfg");

    if (file_exists(szPath))
    {
        server_cmd("exec %s", szPath);
        server_exec();
        LogSendMessage("Executed matchsystem.cfg");
    }
    else
    {
        // Try alternate path
        formatex(szPath, charsmax(szPath), "hnsic/matchsystem.cfg");
        if (file_exists(szPath))
        {
            server_cmd("exec %s", szPath);
            server_exec();
            LogSendMessage("Executed matchsystem.cfg (alternate path)");
        }
    }
}

/* ============================
   NATIVES (for external plugins)
   ============================ */

// Register natives if needed by external plugins
public plugin_natives()
{
    register_native("hnsic_get_mode", "native_get_mode");
    register_native("hnsic_get_match_status", "native_get_match_status");
    register_native("hnsic_get_gameplay", "native_get_gameplay");
    register_native("hnsic_get_player_role", "native_get_player_role");
    register_native("hnsic_is_pointscap_mode", "native_is_pointscap_mode");
    register_native("hnsic_get_score_a", "native_get_score_a");
    register_native("hnsic_get_score_b", "native_get_score_b");
    register_native("hnsic_set_user_role", "native_set_user_role");
    register_native("hnsic_get_user_role", "native_get_user_role");
    register_native("hnsic_is_user_in_match", "native_is_user_in_match");
    register_native("hnsic_get_current_prefix", "native_get_current_prefix");
    register_native("hnsic_chat_print", "native_chat_print");
    register_native("hnsic_is_knife_map", "native_is_knife_map");
}

public native_get_mode(plugin, argc)
{
    return g_iCurrentMode;
}

public native_get_match_status(plugin, argc)
{
    return g_iMatchStatus;
}

public native_get_gameplay(plugin, argc)
{
    return g_iCurrentGameplay;
}

public native_get_player_role(plugin, argc)
{
    new id = get_param(1);
    return g_ePlayerInfo[id][m_iRole];
}

public native_is_pointscap_mode(plugin, argc)
{
    return isPointScapMode();
}

public native_get_score_a(plugin, argc)
{
    return pointscap_get_score_a();
}

public native_get_score_b(plugin, argc)
{
    return pointscap_get_score_b();
}

public native_set_user_role(plugin, argc)
{
    new id = get_param(1);
    new iRole = get_param(2);
    hns_set_user_role(id, PLAYER_ROLES:iRole);
    return 1;
}

public native_get_user_role(plugin, argc)
{
    new id = get_param(1);
    return hns_is_user_role(id);
}

public native_is_user_in_match(plugin, argc)
{
    new id = get_param(1);
    return getUserInMatch(id);
}

public native_get_current_prefix(plugin, argc)
{
    new szPrefix[16];
    copy(szPrefix, charsmax(szPrefix), g_szCurrentPrefix);
    set_string(1, szPrefix, get_param(2));
    return 1;
}

public native_chat_print(plugin, argc)
{
    new id = get_param(1);
    new szMsg[192];
    get_string(2, szMsg, charsmax(szMsg));
    // Format additional args if present
    if (argc > 2)
    {
        // For simplicity, just print the formatted string
        // A full implementation would use vdformat
        chat_print(id, szMsg);
    }
    else
    {
        chat_print(id, szMsg);
    }
    return 1;
}

public native_is_knife_map(plugin, argc)
{
    return hns_is_knife_map();
}
