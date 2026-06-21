#if defined _hnsic_core_included
    #endinput
#endif
#define _hnsic_core_included

#include <amxmodx>
#include <amxmisc>
#include <fakemeta>
#include <fakemeta_util>
#include <reapi>
#include <json>
#include <PersistentDataStorage>

/* ============================
   ENUMS
   ============================ */

enum HNS_MODES
{
    MODE_TRAINING = 0,
    MODE_KNIFE,
    MODE_PUB,
    MODE_DM,
    MODE_ZM,
    MODE_MIX,
    MODE_ASCENSION,
    MODE_VAMP
};

enum MATCH_STATUS
{
    MATCH_NONE = 0,
    MATCH_CAPTAINPICK,
    MATCH_CAPTAINKNIFE,
    MATCH_TEAMPICK,
    MATCH_CUPKNIFE,
    MATCH_CUPPICK,
    MATCH_TEAMKNIFE,
    MATCH_MAPPICK,
    MATCH_WAITCONNECT,
    MATCH_STARTED
};

enum MODE_STATES
{
    STATE_DISABLED = 0,
    STATE_PREPARE,
    STATE_PAUSED,
    STATE_ENABLED
};

enum MATCH_RULES
{
    RULES_MR = 0,
    RULES_TIMER,
    RULES_DUEL,
    RULES_POINTSCAP,
    RULES_VAMP
};

enum HNS_TEAM
{
    HNS_TEAM_B = 0,
    HNS_TEAM_A = 1
};

enum GAMEPLAYS
{
    GAMEPLAY_HNS = 0,
    GAMEPLAY_KNIFE,
    GAMEPLAY_TRAINING
};

enum PLAYER_ROLES
{
    ROLE_SPEC = 0,
    ROLE_CAP_A,
    ROLE_TEAM_A,
    ROLE_CAP_B,
    ROLE_TEAM_B
};

enum _:MATCH_INFO
{
    m_iTeamSize,
    m_iTeamSizeTT,
    Float:m_flWintime,
    Float:m_flSidesTime[HNS_TEAM],
    m_iSidesRounds[HNS_TEAM],
    Trie:m_tLeaveData,
    bool:m_bLeaved
};

enum _:PLAYER_INFO
{
    PLAYER_ROLES:m_iRole,
    bool:m_bInMatch,
    bool:m_bPickMe,
    m_szTeam[16],
    m_iLeaveRound
};

enum _:CVARS
{
    CVAR_RULES,
    CVAR_WINTIME,
    CVAR_MAXROUNDS,
    CVAR_DUELROUNDS,
    CVAR_HNSBOOST,
    CVAR_ONEHPMODE,
    CVAR_FLASH,
    CVAR_SMOKE,
    CVAR_LASTGRENADE,
    Float:CVAR_DMRESPAWN,
    CVAR_SUTVOTETIME,
    CVAR_SURTIMEDELAY,
    CVAR_RANDOMPICK,
    Float:CVAR_TIME_RANDOMPICK,
    CVAR_KNIFEMAP[24],
    CVAR_PREFIX[24],
    CVAR_GAMENAME,
    CVAR_WATCHER_FLAG[4],
    CVAR_FULL_WATCHER_FLAG[4],
    CVAR_ADMIN_FLAG[4],
    CVAR_POINTS_DISTANCE_1,
    CVAR_POINTS_DISTANCE_2,
    CVAR_POINTS_DISTANCE_3,
    Float:CVAR_POINTS_ADD_1,
    Float:CVAR_POINTS_ADD_2,
    Float:CVAR_POINTS_ADD_3,
    CVAR_POINTS_B_DEBUG,
    CVAR_POINTS_D_DEBUG,
    CVAR_POINTSCAP_DETECT_TIME,
    CVAR_POINTSCAP_KNIFE_TIME,
    Float:CVAR_POINTSCAP_STAY_TIME,
    Float:CVAR_POINTSCAP_SCORE_3,
    Float:CVAR_POINTSCAP_SCORE_4,
    Float:CVAR_POINTSCAP_SCORE_5,
    CVAR_POINTSCAP_KNIFE_BONUS,
    CVAR_POINTSCAP_TARGET_SCORE,
    CVAR_POINTSCAP_VAMP_INIT,
    CVAR_POINTSCAP_SOUND_CAPTURE,
    CVAR_POINTSCAP_SOUND_KNIFE,
    CVAR_POINTSCAP_SOUND_SCORE,
    CVAR_KNIFE_DISTANCE,
    CVAR_KNIFE_VIEWMODEL_ENABLE
};

enum _:ZONE_DATA
{
    Float:z_vOriginA[3],
    Float:z_vOriginB[3],
    Float:z_flRadius,
    Float:z_vCenter[3],
    z_szName[32],
    z_iTeam,
    z_bEnabled,
    z_bActive,
    z_iCaptures,
    z_iCaptureTeam,
    Float:z_flCaptureTime,
    z_iCaptureCount[HNS_TEAM],
    Float:z_flCaptureScore[HNS_TEAM],
    Float:z_flCaptureProgress[HNS_TEAM],
    z_iCapturePlayers[HNS_TEAM],
    z_szCaptureName[HNS_TEAM][32]
};

enum _:PLAYER_STATS
{
    stat_kills,
    stat_deaths,
    stat_assists,
    stat_stabs,
    stat_dmg_ct,
    stat_dmg_tt,
    stat_runned,
    Float:stat_runnedtime,
    Float:stat_flashtime,
    Float:stat_survtime,
    Float:stat_hidetime,
    Float:stat_playtime,
    stat_ownages,
    stat_stops,
    stat_bhop_count,
    Float:stat_bhop_percent_sum,
    stat_sgs_count,
    Float:stat_sgs_percent_sum,
    stat_ddrun_count,
    Float:stat_ddrun_percent_sum,
    stat_match,
    stat_team
};

enum _:SAVE_PLAYER_DATA
{
    szAuth[24],
    iTeam
};

/* ============================
   CONSTANTS
   ============================ */

// Task IDs
#define TASK_TIMER           54345
#define HUD_PAUSE            23931
#define TASK_WAIT            12319
#define TASK_WAIT_CUP        11331
#define TASK_STARTED         13339
#define TASK_WAITCAP         13131
#define TASK_WAITLEAVECAP     15151
#define TASK_SURRENDER       55566
#define TASK_POINTSCAP_DETECT 67890
#define TASK_POINTSCAP_KNIFE  67891
#define TASK_POINTSCAP_HUD    67892
#define TASK_POINTS          54445
#define TASK_HOOK            99912
#define TASK_AFK             77832

// Limits
#define MAX_ZONES            10
#define MAX_OWNED_SKINS      64

// Default entities to remove on spawn
new const g_szDefaultEntities[][] =
{
    "func_bomb_target",
    "info_bomb_target",
    "hostage_entity",
    "monster_scientist",
    "func_hostage_rescue",
    "info_hostage_rescue",
    "func_vip_safetyzone",
    "info_vip_safetyzone",
    "func_escapezone",
    "info_map_parameters",
    "env_fog_controller",
    "func_buyzone"
};

// Sound constants
new const g_szSoundButton[]      = "buttons/blip1.wav";
new const g_szSoundDeny[]        = "common/wpn_denyselect.wav";
new const g_szSoundCapture[]     = "hnsic/capture.wav";
new const g_szSoundKnifeKill[]   = "hnsic/knifekill.wav";
new const g_szSoundScore[]      = "hnsic/score.wav";
new const g_szLaserBeamSprite[]  = "sprites/laserbeam.spr";

// Team names
new const g_szTeamNameA[] = "A";
new const g_szTeamNameB[] = "B";

// Role names for display
new const g_szRoleName[PLAYER_ROLES][] =
{
    "SPEC",
    "CAP_A",
    "TEAM_A",
    "CAP_B",
    "TEAM_B"
};

/* ============================
   GLOBAL VARIABLES
   ============================ */

// --- Core ---
bool:g_bDebugMode = false;
g_iRegisterSpawn = 0;
g_szLogPath[256];
iBeam;
g_szMapName[64];
g_iHostageEnt = 0;
g_msgMoney = 0;
g_szBaseHostname[128];

// --- Mode System ---
g_iCurrentMode = MODE_PUB;
g_szCurrentPrefix[16];
g_iCurrentGameplay = GAMEPLAY_HNS;
g_iCurrentRules = RULES_MR;
g_iMatchStatus = MATCH_NONE;
g_eMatchState = STATE_DISABLED;
g_isTeamTT = false;

// --- Match ---
g_eMatchInfo[MATCH_INFO];
g_eSurrenderData;
bool:g_bNoplay[MAX_PLAYERS + 1];
Float:g_flRoundTime = 0.0;
g_iPlayerDistance = 0;
g_iPointsDistance = 0;

// --- Player ---
g_ePlayerInfo[MAX_PLAYERS + 1][PLAYER_INFO];
g_eSpecBack[MAX_PLAYERS + 1];
g_iCaptainPick = 0;
g_iCaptainFirst = 0;
g_iCaptainSecond = 0;

// --- PointScap / Vampire ---
g_eZones[MAX_ZONES][ZONE_DATA];
g_iZoneCount = 0;
Float:g_flScoreA = 0.0;
Float:g_flScoreB = 0.0;
g_iKnifeKillCount = 0;
bool:g_bAllKnifeKills = false;
g_iPointScapRound = 0;
g_iPointScapDetectTime = 0;
g_iPointScapKnifeTime = 0;
Float:g_flPointScapStayTime = 0.0;
Float:g_flPointScapScore3 = 0.0;
Float:g_flPointScapScore4 = 0.0;
Float:g_flPointScapScore5 = 0.0;
g_iPointScapKnifeBonus = 0;
g_iPointScapTargetScore = 0;
g_iPointScapVampInit = 0;
bool:g_bPointScapSoundCapture = true;
bool:g_bPointScapSoundKnife = true;
bool:g_bPointScapSoundScore = true;

// --- Duel ---
Float:g_flPointsMatchTime = 0.0;
Float:g_flPointsMatchTimeSnap = 0.0;
Float:g_flPointsSnap[HNS_TEAM];
g_iPointsTeamSnap = 0;

// --- Deserter ---
g_iDesertCount[MAX_PLAYERS + 1];
g_iDeserterBanEnd[MAX_PLAYERS + 1];
bool:g_bDeserterActive[MAX_PLAYERS + 1];

// --- Knife ViewModel ---
bool:g_bKnifeViewModel[MAX_PLAYERS + 1];
g_iNextKnifeAttack[MAX_PLAYERS + 1];

// --- SemiClip ---
g_iSemiClipEnabled = 0;
g_iSemiClipTeam[MAX_PLAYERS + 1];
bool:g_bPlayerSemiClip[MAX_PLAYERS + 1];

// --- Training ---
Float:g_fCheckpoints[MAX_PLAYERS + 1][2][3];
bool:g_bCheckpointAlternate[MAX_PLAYERS + 1];
bool:g_bDamage[MAX_PLAYERS + 1];
bool:g_bSaveAngles[MAX_PLAYERS + 1];
bool:isHook[MAX_PLAYERS + 1];
Float:g_fHookOrigin[MAX_PLAYERS + 1][3];

// --- Stats ---
iStats[MAX_PLAYERS + 1][PLAYER_STATS];
g_StatsRound[MAX_PLAYERS + 1][PLAYER_STATS];
Float:g_flLastOrigin[MAX_PLAYERS + 1][3];
bool:g_bIsRunning[MAX_PLAYERS + 1];
bool:g_bIsHiding[MAX_PLAYERS + 1];

// --- Player Models ---
Array:g_aTModels;
Array:g_aCTModels;
g_iPlayerTModel[MAX_PLAYERS + 1];
g_iPlayerCTModel[MAX_PLAYERS + 1];
g_iModelSelectTeam[MAX_PLAYERS + 1];

// --- Skin System ---
Array:g_aSkinTModels;
Array:g_aSkinCTModels;
Array:g_aSkinKnifeModels;
Array:g_aSkinTNames;
Array:g_aSkinCTNames;
Array:g_aSkinKnifeNames;
bool:g_bOwnsSkin[MAX_PLAYERS + 1][MAX_OWNED_SKINS];
g_iActiveSkinT[MAX_PLAYERS + 1];
g_iActiveSkinCT[MAX_PLAYERS + 1];
g_iActiveSkinKnife[MAX_PLAYERS + 1];
g_iSkinAdminVerify[MAX_PLAYERS + 1];

// --- FlyNade ---
g_iFlyEnt[MAX_PLAYERS + 1];
bool:g_bHasFlyNade[MAX_PLAYERS + 1];
bool:g_bGrenadeThrown[MAX_PLAYERS + 1];
Float:g_fGrenadeOrigin[MAX_PLAYERS + 1][3];
Float:g_fGrenadeVelocity[MAX_PLAYERS + 1][3];
bool:g_bFlyActive[MAX_PLAYERS + 1];

// --- Chat ---
bool:g_bCustomPrefix[MAX_PLAYERS + 1];
g_sPlayerPrefix[MAX_PLAYERS + 1][32];
Trie:g_tSteamPrefix;
Trie:g_tNamePrefix;

// --- Maps ---
Array:g_aMapSections;
Array:g_aMapSectionNames;
g_iCurrentMapSection = 0;
bool:g_bMapLoaded = false;

// --- Permission ---
g_iPermLevel[MAX_PLAYERS + 1];
bool:g_bVerified[MAX_PLAYERS + 1];
Trie:g_tPermData;

// --- Watcher ---
g_iWatcherVote[MAX_PLAYERS + 1];
Float:g_flWatcherVoteTime[MAX_PLAYERS + 1];
bool:g_bWatcherVoteActive = false;
g_iWatcherVoteTarget = 0;
g_iWatcherVoteCaller = 0;
g_iWatcherVoteCount = 0;
g_iWatcherVoteNeeded = 0;

// --- Ownage ---
Float:g_flLastHeadTouch[MAX_PLAYERS + 1];

// --- JumpStats ---
bool:g_bJumpTracking[MAX_PLAYERS + 1];
Float:g_flJumpOrigin[MAX_PLAYERS + 1][3];
Float:g_flJumpStartZ[MAX_PLAYERS + 1];
bool:g_bJumpInAir[MAX_PLAYERS + 1];
Float:g_flJumpDistance[MAX_PLAYERS + 1];
Float:g_flJumpMaxHeight[MAX_PLAYERS + 1];
Float:g_flJumpHeight[MAX_PLAYERS + 1];
g_iJumpType[MAX_PLAYERS + 1];
Float:g_flJumpPreSpeed[MAX_PLAYERS + 1];
Float:g_flJumpMaxSpeed[MAX_PLAYERS + 1];
g_iJumpStrafes[MAX_PLAYERS + 1];
g_iJumpSync[MAX_PLAYERS + 1];
bool:g_bJumpValid[MAX_PLAYERS + 1];
Float:g_flJumpLastZVel[MAX_PLAYERS + 1];
g_iJumpFrames[MAX_PLAYERS + 1];
g_iJumpDuckCount[MAX_PLAYERS + 1];
bool:g_bJumpWeird[MAX_PLAYERS + 1];
Float:g_flJumpTurnVel[MAX_PLAYERS + 1];
Float:g_flJumpLastAngle[MAX_PLAYERS + 1];

// --- Flash Notifier ---
bool:g_isSpecFlashed[MAX_PLAYERS + 1];

// --- Player Info ---
g_iPlayerSpecTarget[MAX_PLAYERS + 1];
Float:g_flRoundBestTime = 0.0;
g_iRoundBestPlayer = 0;
Float:g_flRoundBestHideTime = 0.0;
g_iRoundBestHidePlayer = 0;

// --- AFK ---
enum _:AFK_DATA
{
    Float:afk_last_move,
    afk_spawn_count,
    Float:afk_last_origin[3],
    bool:afk_is_afk
};
new eAfkData[MAX_PLAYERS + 1][AFK_DATA];
Float:flAfkOrigin[MAX_PLAYERS + 1][3];

// --- Captain ---
Float:g_flRandomPickTime = 0.0;
Float:g_flCaptainLeaveTime = 0.0;

// --- Pointscap Editor ---
enum _:EDITOR_STATE
{
    editor_measuring,
    Float:editor_originA[3],
    Float:editor_originB[3],
    editor_selectedZone,
    bool:editor_active
};
new g_eEditor[EDITOR_STATE];
g_iEditorBeam = 0;

// --- Cvar pointers ---
new g_pCvars[CVARS];

/* ============================
   STOCK FUNCTIONS
   ============================ */

stock RegisterSayCmd(const szCommand[], const szFunction[], const szFlags[] = "-1", const szInfo[] = "")
{
    new szTemp[64];
    // say command
    formatex(szTemp, charsmax(szTemp), "say %s", szCommand);
    register_clcmd(szTemp, szFunction, szFlags, szInfo);
    // . command
    formatex(szTemp, charsmax(szTemp), "say .%s", szCommand);
    register_clcmd(szTemp, szFunction, szFlags, szInfo);
    // say / command
    formatex(szTemp, charsmax(szTemp), "say /%s", szCommand);
    register_clcmd(szTemp, szFunction, szFlags, szInfo);
    // say_team command
    formatex(szTemp, charsmax(szTemp), "say_team %s", szCommand);
    register_clcmd(szTemp, szFunction, szFlags, szInfo);
}

stock getUserTeam(id)
{
    return get_member(id, m_iTeam);
}

stock getUserKey(id)
{
    new szAuth[24];
    get_user_authid(id, szAuth, charsmax(szAuth));
    return szAuth;
}

stock chat_print(id, const szMsg[], any:...)
{
    new szBuffer[192];
    vformat(szBuffer, charsmax(szBuffer), szMsg, 3);

    if (id == 0)
    {
        server_print("[%s] %s", g_szCurrentPrefix, szBuffer);
    }
    else
    {
        new szFinal[192];
        formatex(szFinal, charsmax(szFinal), "^4[%s] ^1%s", g_szCurrentPrefix, szBuffer);
        message_begin(MSG_ONE_UNRELIABLE, get_user_msgid("SayText"), _, id);
        write_byte(id);
        write_string(szFinal);
        message_end();
    }
}

stock chat_print_color(id, const szMsg[], any:...)
{
    new szBuffer[192];
    vformat(szBuffer, charsmax(szBuffer), szMsg, 3);

    if (id == 0)
    {
        server_print("[%s] %s", g_szCurrentPrefix, szBuffer);
    }
    else
    {
        message_begin(MSG_ONE_UNRELIABLE, get_user_msgid("SayText"), _, id);
        write_byte(id);
        write_string(szBuffer);
        message_end();
    }
}

stock fnConvertTime(Float:flTime, szOutput[], iLen)
{
    new iMinutes = floatround(flTime, floatround_floor) / 60;
    new iSeconds = floatround(flTime, floatround_floor) % 60;
    formatex(szOutput, iLen, "%d:%02d", iMinutes, iSeconds);
}

stock fnConvertTimeDetailed(Float:flTime, szOutput[], iLen)
{
    new iTotalSec = floatround(flTime, floatround_floor);
    new iHours = iTotalSec / 3600;
    new iMinutes = (iTotalSec % 3600) / 60;
    new iSeconds = iTotalSec % 60;

    if (iHours > 0)
        formatex(szOutput, iLen, "%d:%02d:%02d", iHours, iMinutes, iSeconds);
    else
        formatex(szOutput, iLen, "%d:%02d", iMinutes, iSeconds);
}

stock isUserWatcher(id)
{
    new szFlag[4];
    get_pcvar_string(g_pCvars[CVAR_WATCHER_FLAG], szFlag, charsmax(szFlag));
    if (szFlag[0] == EOS)
        return false;
    return (get_user_flags(id) & read_flags(szFlag)) != 0;
}

stock isUserFullWatcher(id)
{
    new szFlag[4];
    get_pcvar_string(g_pCvars[CVAR_FULL_WATCHER_FLAG], szFlag, charsmax(szFlag));
    if (szFlag[0] == EOS)
        return false;
    return (get_user_flags(id) & read_flags(szFlag)) != 0;
}

stock isUserAdmin(id)
{
    new szFlag[4];
    get_pcvar_string(g_pCvars[CVAR_ADMIN_FLAG], szFlag, charsmax(szFlag));
    if (szFlag[0] == EOS)
        return false;
    return (get_user_flags(id) & read_flags(szFlag)) != 0;
}

stock setUserGodmode(id, bool:bGodmode)
{
    if (bGodmode)
    {
        set_pev(id, pev_takedamage, DAMAGE_NO);
        set_member(id, m_bHasC4, 0);
    }
    else
    {
        set_pev(id, pev_takedamage, DAMAGE_AIM);
    }
}

stock transferUserToSpec(id)
{
    if (getUserTeam(id) == TEAM_SPECTATOR)
        return;

    new szName[32];
    get_user_name(id, szName, charsmax(szName));

    set_member(id, m_iTeam, TEAM_SPECTATOR);
    rg_set_user_team(id, TEAM_SPECTATOR, MODEL_UNASSIGNED);

    chat_print(0, "%L", LANG_PLAYER, "TRANSFER_SPEC", szName);
}

stock hns_restart_round(Float:flDelay = 1.0)
{
    if (flDelay <= 0.0)
        flDelay = 1.0;

    // Stop any existing round timer
    remove_task(TASK_TIMER);

    // Reset round time
    rg_round_time(flDelay + get_pcvar_float(g_pCvars[CVAR_WINTIME]));

    // Restart the round
    set_member(0, m_bGameStarted, false);
    server_cmd("sv_restart 1");
}

stock hns_swap_teams()
{
    new iPlayers[MAX_PLAYERS], iNum;
    get_players(iPlayers, iNum, "h");

    for (new i = 0; i < iNum; i++)
    {
        new id = iPlayers[i];
        new iTeam = getUserTeam(id);

        if (iTeam == TEAM_TERRORIST)
        {
            rg_set_user_team(id, TEAM_CT, MODEL_AUTO);
        }
        else if (iTeam == TEAM_CT)
        {
            rg_set_user_team(id, TEAM_TERRORIST, MODEL_AUTO);
        }
    }

    g_isTeamTT = !g_isTeamTT;
}

stock setTaskHud(id, Float:flHoldTime, const szMsg[], any:...)
{
    new szBuffer[192];
    vformat(szBuffer, charsmax(szBuffer), szMsg, 4);

    set_hudmessage(0, 255, 0, -1.0, 0.35, 0, 0.0, flHoldTime, 0.1, 0.1, 3);
    show_hudmessage(id, szBuffer);
}

stock taskHudMessage(id, Float:flHoldTime, const szMsg[], any:...)
{
    new szBuffer[192];
    vformat(szBuffer, charsmax(szBuffer), szMsg, 4);

    set_hudmessage(0, 255, 0, -1.0, 0.35, 0, 0.0, flHoldTime, 0.1, 0.1, 3);
    show_hudmessage(id, szBuffer);
}

stock encodeText(const szInput[], szOutput[], iLen)
{
    new i, j;
    new iInLen = strlen(szInput);

    for (i = 0, j = 0; i < iInLen && j < iLen - 1; i++)
    {
        if (szInput[i] == '%' && i + 2 < iInLen)
        {
            new szHex[3];
            szHex[0] = szInput[i + 1];
            szHex[1] = szInput[i + 2];
            szHex[2] = EOS;

            new iVal = strtol(szHex, 0, 16);
            if (iVal > 0)
            {
                szOutput[j++] = iVal;
                i += 2;
                continue;
            }
        }
        szOutput[j++] = szInput[i];
    }
    szOutput[j] = EOS;
}

stock decodeText(const szInput[], szOutput[], iLen)
{
    new i, j;
    new iInLen = strlen(szInput);

    for (i = 0, j = 0; i < iInLen && j < iLen - 1; i++)
    {
        if (szInput[i] >= 0x20 && szInput[i] <= 0x7E)
        {
            szOutput[j++] = szInput[i];
        }
        else
        {
            j += formatex(szOutput[j], iLen - j, "%%%02x", szInput[i]);
        }
    }
    szOutput[j] = EOS;
}

stock LogSendMessage(const szFormat[], any:...)
{
    if (g_szLogPath[0] == EOS)
        return;

    new szDate[16], szTime[16];
    get_time("%Y%m%d", szDate, charsmax(szDate));
    get_time("%H:%M:%S", szTime, charsmax(szTime));

    new szBuffer[512];
    vformat(szBuffer, charsmax(szBuffer), szFormat, 2);

    new szLogFile[256];
    formatex(szLogFile, charsmax(szLogFile), "%s/hnsic_%s.log", g_szLogPath, szDate);

    new fp = fopen(szLogFile, "a");
    if (fp)
    {
        fprintf(fp, "[%s] %s^n", szTime, szBuffer);
        fclose(fp);
    }
}

stock hns_is_knife_map()
{
    new szKnifeMap[24];
    get_pcvar_string(g_pCvars[CVAR_KNIFEMAP], szKnifeMap, charsmax(szKnifeMap));

    if (szKnifeMap[0] == EOS)
        return 0;

    new szMap[24];
    copy(szMap, charsmax(szMap), g_szMapName);

    // Check if current map matches any entry in the knife map list
    new szPiece[24], iLen;
    while ((iLen = strtok(szKnifeMap, szPiece, charsmax(szPiece), szKnifeMap, charsmax(szKnifeMap), ',')) > 0)
    {
        trim(szPiece);
        if (equali(szMap, szPiece))
            return 1;
    }

    return 0;
}

stock isPointScapMode()
{
    return (g_iCurrentMode == MODE_ASCENSION || g_iCurrentMode == MODE_VAMP);
}

stock pointscap_get_score_a()
{
    return floatround(g_flScoreA);
}

stock pointscap_get_score_b()
{
    return floatround(g_flScoreB);
}

stock loadMapCFG()
{
    new szPath[256];
    formatex(szPath, charsmax(szPath), "hnsic/maps/%s.cfg", g_szMapName);

    if (file_exists(szPath))
    {
        server_cmd("exec %s", szPath);
        server_exec();
        LogSendMessage("Loaded map config: %s", szPath);
        return true;
    }

    return false;
}

stock hns_set_user_role(id, PLAYER_ROLES:iRole)
{
    g_ePlayerInfo[id][m_iRole] = iRole;
}

stock PLAYER_ROLES:hns_is_user_role(id)
{
    return PLAYER_ROLES:g_ePlayerInfo[id][m_iRole];
}

stock hns_get_captain_role(PLAYER_ROLES:iRole)
{
    if (iRole == ROLE_CAP_A)
        return ROLE_CAP_A;
    if (iRole == ROLE_CAP_B)
        return ROLE_CAP_B;
    return ROLE_SPEC;
}

stock hns_get_role_num(PLAYER_ROLES:iRole)
{
    switch (iRole)
    {
        case ROLE_SPEC: return 0;
            break;
        case ROLE_CAP_A: return 1;
            break;
        case ROLE_TEAM_A: return 2;
            break;
        case ROLE_CAP_B: return 3;
            break;
        case ROLE_TEAM_B: return 4;
            break;
    }
    return 0;
}

stock PLAYER_ROLES:hns_get_random_role()
{
    new iRand = random_num(1, 4);
    return PLAYER_ROLES:iRand;
}

stock hns_distribute_roles_teams()
{
    new iPlayers[MAX_PLAYERS], iNum;
    get_players(iPlayers, iNum, "h");

    for (new i = 0; i < iNum; i++)
    {
        new id = iPlayers[i];
        new iRole = g_ePlayerInfo[id][m_iRole];

        switch (iRole)
        {
            case ROLE_CAP_A, ROLE_TEAM_A:
            {
                rg_set_user_team(id, TEAM_TERRORIST, MODEL_AUTO);
                copy(g_ePlayerInfo[id][m_szTeam], charsmax(g_ePlayerInfo[][m_szTeam]), g_szTeamNameA);
                break;
            }
            case ROLE_CAP_B, ROLE_TEAM_B:
            {
                rg_set_user_team(id, TEAM_CT, MODEL_AUTO);
                copy(g_ePlayerInfo[id][m_szTeam], charsmax(g_ePlayerInfo[][m_szTeam]), g_szTeamNameB);
                break;
            }
            case ROLE_SPEC:
            {
                rg_set_user_team(id, TEAM_SPECTATOR, MODEL_UNASSIGNED);
                break;
            }
        }
    }
}

stock hns_reset_users_roles()
{
    new iPlayers[MAX_PLAYERS], iNum;
    get_players(iPlayers, iNum, "h");

    for (new i = 0; i < iNum; i++)
    {
        new id = iPlayers[i];
        g_ePlayerInfo[id][m_iRole] = ROLE_SPEC;
        g_ePlayerInfo[id][m_bInMatch] = false;
        g_ePlayerInfo[id][m_bPickMe] = false;
        g_ePlayerInfo[id][m_szTeam][0] = EOS;
        g_ePlayerInfo[id][m_iLeaveRound] = 0;
    }
}

stock getUserInMatch(id)
{
    return g_ePlayerInfo[id][m_bInMatch];
}

stock get_num_players_in_match()
{
    new iCount = 0;
    for (new id = 1; id <= MAX_PLAYERS; id++)
    {
        if (g_ePlayerInfo[id][m_bInMatch])
            iCount++;
    }
    return iCount;
}

stock checkUserBan(id)
{
    new szAuth[24];
    get_user_authid(id, szAuth, charsmax(szAuth));

    if (g_iDeserterBanEnd[id] > get_systime())
    {
        return true;
    }

    return false;
}

stock check_captain_role(PLAYER_ROLES:iRole)
{
    return (iRole == ROLE_CAP_A || iRole == ROLE_CAP_B);
}

stock hns_get_team_from_role(PLAYER_ROLES:iRole)
{
    switch (iRole)
    {
        case ROLE_CAP_A, ROLE_TEAM_A: return HNS_TEAM_A;
            break;
        case ROLE_CAP_B, ROLE_TEAM_B: return HNS_TEAM_B;
            break;
    }
    return -1;
}

stock PLAYER_ROLES:hns_get_role_from_team(HNS_TEAM:iTeam, bool:bCaptain = false)
{
    if (bCaptain)
    {
        return (iTeam == HNS_TEAM_A) ? ROLE_CAP_A : ROLE_CAP_B;
    }
    return (iTeam == HNS_TEAM_A) ? ROLE_TEAM_A : ROLE_TEAM_B;
}

stock bool:is_user_alive_connected(id)
{
    return (id > 0 && id <= MAX_PLAYERS && is_user_connected(id) && is_user_alive(id));
}

stock bool:is_user_valid(id)
{
    return (id > 0 && id <= MAX_PLAYERS && is_user_connected(id));
}

stock Float:get_gametime_f()
{
    return get_gametime();
}

stock Float:get_round_time_remaining()
{
    new Float:flRoundTime = get_member(0, m_iRoundTimeSecs);
    new Float:flStartTime = get_member(0, m_fRoundStartTime);
    return flRoundTime - (get_gametime() - flStartTime);
}

stock get_alive_players()
{
    new iPlayers[MAX_PLAYERS], iNum;
    get_players(iPlayers, iNum, "ah");
    return iNum;
}

stock get_alive_players_team(iTeam)
{
    new iPlayers[MAX_PLAYERS], iNum;
    get_players(iPlayers, iNum, "ah");

    new iCount = 0;
    for (new i = 0; i < iNum; i++)
    {
        if (getUserTeam(iPlayers[i]) == iTeam)
            iCount++;
    }
    return iCount;
}

stock get_team_players(iTeam)
{
    new iPlayers[MAX_PLAYERS], iNum;
    get_players(iPlayers, iNum, "h");

    new iCount = 0;
    for (new i = 0; i < iNum; i++)
    {
        if (getUserTeam(iPlayers[i]) == iTeam)
            iCount++;
    }
    return iCount;
}

stock strip_user_weapons_full(id)
{
    new iWeapons[32], iNum;
    get_user_weapons(id, iWeapons, iNum);

    for (new i = 0; i < iNum; i++)
    {
        rg_remove_item(id, iWeapons[i]);
    }

    set_member(id, m_iPrimaryWeapon, 0);
    set_member(id, m_iSecondaryWeapon, 0);
}

stock give_item_safe(id, const szWeapon[])
{
    rg_give_item(id, szWeapon);
}

stock Float:entity_range(id1, id2)
{
    new Float:vOrigin1[3], Float:vOrigin2[3];
    pev(id1, pev_origin, vOrigin1);
    pev(id2, pev_origin, vOrigin2);

    new Float:flDist = get_distance_f(vOrigin1, vOrigin2);
    return flDist;
}

stock get_weapon_name(iWeaponId, szName[], iLen)
{
    new szWeapon[24];
    get_weaponname(iWeaponId, szWeapon, charsmax(szWeapon));
    copy(szName, iLen, szWeapon);
}

stock Float:vec_dist(const Float:v1[3], const Float:v2[3])
{
    return get_distance_f(v1, v2);
}

stock vec_copy(const Float:vIn[3], Float:vOut[3])
{
    vOut[0] = vIn[0];
    vOut[1] = vIn[1];
    vOut[2] = vIn[2];
}

stock vec_set(Float:v[3], Float:x, Float:y, Float:z)
{
    v[0] = x;
    v[1] = y;
    v[2] = z;
}

stock Float:vec_length(const Float:v[3])
{
    return xs_vec_len(v);
}

stock Float:vec_normalize(Float:v[3])
{
    new Float:flLen = xs_vec_len(v);
    if (flLen > 0.0)
    {
        v[0] /= flLen;
        v[1] /= flLen;
        v[2] /= flLen;
    }
    return flLen;
}

stock clamp(Float:value, Float:min, Float:max)
{
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

// PDS wrapper - use native pds_set directly
