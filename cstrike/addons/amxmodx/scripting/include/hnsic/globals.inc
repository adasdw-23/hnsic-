#if defined _hnsic_globals_included
    #endinput
#endif
#define _hnsic_globals_included

/* ============================================================
 *  HNSIC v3.10.0 - globals.inc
 *  Global variables and constants
 * ============================================================ */

// ─── Global State ───────────────────────────────────────────
new g_iCurrentMode = MODE_NONE;
new g_iCurrentGameplay = GAMEPLAY_NONE;
new g_iMatchState = STATE_IDLE;

// ─── User Data Arrays ───────────────────────────────────────
new g_eUserData[MAX_PLAYERS + 1][USER_DATA];

// ─── Match Variables ─────────────────────────────────────────
new g_iMatchScore[3];           // [TEAM_TT], [TEAM_CT], [TEAM_NONE=unused]
new g_iHalfScore[3][2];         // [team][half]
new g_iCurrentHalf;             // 0 = first, 1 = second
new g_iRoundNum;                // Current round number
new g_iMaxRounds;               // Max rounds per half
new Float:g_fMatchTimer;       // Match elapsed time
new g_iMatchTimerTask;          // Timer task ID

// ─── Captain System ──────────────────────────────────────────
new g_iCaptain[MAX_CAPTAINS];   // Captain player IDs
new g_iCaptainTeam[MAX_CAPTAINS]; // Captain team assignment
new g_iCaptainDuelWinner;       // Winner of knife duel
new bool:g_bCaptainDuelActive;

// ─── Group System ────────────────────────────────────────────
new g_iGroupPlayers[MAX_GROUPS][MAX_PLAYERS]; // Players in each group
new g_iGroupCount[MAX_GROUPS];
new Float:g_fGroupRating[MAX_GROUPS][MAX_PLAYERS]; // Player ratings for sorting

// ─── Map System ──────────────────────────────────────────────
new g_szCurrentMap[MAX_MAPNAME_LEN];
new g_iMapType = MAPTYPE_BOOST;
new g_szBoostMaps[][] = {
    "hns_crazy_jump", "hns_freddys", "hns_steam",
    "hns_ninja", "hns_electric", "hns_dark"
};
new g_szSkillMaps[][] = {
    "hns_alpine", "hns_sky", "hns_rooftops",
    "hns_bridge", "hns_tower", "hns_maze"
};

// ─── Scoring ────────────────────────────────────────────────
new g_iScoringMode = SCORING_POINT_CAP;
new g_iPointCapTarget;          // Points needed to win
new g_iTTPoints;                // TT capture points this round
new g_iCTPoints;                // CT score (for deduct mode)

// ─── AFK System ─────────────────────────────────────────────
new Float:g_fAfkCheckInterval = 30.0;
new g_iAfkMaxWarnings = 2;

// ─── Surrender System ───────────────────────────────────────
new g_iSurrenderVotes[3];       // Votes per team
new g_iSurrenderInitiator;
new Float:g_fSurrenderTime;
new bool:g_bSurrenderActive;

// ─── Pause System ──────────────────────────────────────────
new bool:g_bMatchPaused;
new g_iPauseRequester;

// ─── Reconnect System ──────────────────────────────────────
new g_iExpectedPlayers;
new g_iReconnectedPlayers;
new bool:g_bWaitingReconnect;
new g_iReconnectTask;

// ─── Misc ───────────────────────────────────────────────────
new g_iMaxPlayers;
new g_szLogFile[128];
new g_iMsgHideWeapon;
new g_iMsgShowMenu;
new g_iMsgVGUIMenu;
new g_iMsgHostagePos;
new g_iMsgHudTextArgs;
new g_iMsgMoney;
new g_iMsgSayText;
new g_iMsgTeamInfo;
new g_iMsgScoreInfo;

// ─── PDS Storage Keys ────────────────────────────────────────
new g_szPDS_PTS[]      = "hnsic_pts";
new g_szPDS_WINS[]     = "hnsic_wins";
new g_szPDS_LOSSES[]   = "hnsic_losses";
new g_szPDS_MATCHES[]  = "hnsic_matches";
new g_szPDS_KILLS[]    = "hnsic_kills";
new g_szPDS_DEATHS[]   = "hnsic_deaths";

// ─── Forward Handles ─────────────────────────────────────────
new g_fwdMatchStart;
new g_fwdMatchEnd;
new g_fwdModeChange;
new g_fwdGameplayChange;
new g_fwdRoundStart;
new g_fwdRoundEnd;
new g_fwdPlayerScored;

// ─── Knife Damage Multiplier ────────────────────────────────
new Float:g_fKnifeDmgMult = 1.0;
