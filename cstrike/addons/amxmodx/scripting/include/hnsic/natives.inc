#if defined _hnsic_natives_included
    #endinput
#endif
#define _hnsic_natives_included

/* ============================================================
 *  HNSIC v3.10.0 - natives.inc
 *  Native API registration (16 natives)
 * ============================================================ */

// ─── Native Declarations ────────────────────────────────────

// 1. Get current mode
native Hnsic_GetMode();

// 2. Get current gameplay
native Hnsic_GetGameplay();

// 3. Get match state
native Hnsic_GetMatchState();

// 4. Get player PTS
native Hnsic_GetPlayerPts(id);

// 5. Set player PTS
native Hnsic_SetPlayerPts(id, pts);

// 6. Get player wins
native Hnsic_GetPlayerWins(id);

// 7. Get player matches
native Hnsic_GetPlayerMatches(id);

// 8. Get match score for team
native Hnsic_GetMatchScore(team);

// 9. Set match score for team
native Hnsic_SetMatchScore(team, score);

// 10. Force mode switch
native Hnsic_ForceMode(mode);

// 11. Force gameplay switch
native Hnsic_ForceGameplay(gameplay);

// 12. Get current half
native Hnsic_GetCurrentHalf();

// 13. Get round number
native Hnsic_GetRoundNum();

// 14. Check if match is active
native bool:Hnsic_IsMatchActive();

// 15. Get scoring mode
native Hnsic_GetScoringMode();

// 16. Get player winrate
native Float:Hnsic_GetPlayerWinrate(id);

// ─── Native Registration ──────────────────────────────────────
public plugin_natives()
{
    register_native("Hnsic_GetMode", "_native_get_mode");
    register_native("Hnsic_GetGameplay", "_native_get_gameplay");
    register_native("Hnsic_GetMatchState", "_native_get_match_state");
    register_native("Hnsic_GetPlayerPts", "_native_get_player_pts");
    register_native("Hnsic_SetPlayerPts", "_native_set_player_pts");
    register_native("Hnsic_GetPlayerWins", "_native_get_player_wins");
    register_native("Hnsic_GetPlayerMatches", "_native_get_player_matches");
    register_native("Hnsic_GetMatchScore", "_native_get_match_score");
    register_native("Hnsic_SetMatchScore", "_native_set_match_score");
    register_native("Hnsic_ForceMode", "_native_force_mode");
    register_native("Hnsic_ForceGameplay", "_native_force_gameplay");
    register_native("Hnsic_GetCurrentHalf", "_native_get_current_half");
    register_native("Hnsic_GetRoundNum", "_native_get_round_num");
    register_native("Hnsic_IsMatchActive", "_native_is_match_active");
    register_native("Hnsic_GetScoringMode", "_native_get_scoring_mode");
    register_native("Hnsic_GetPlayerWinrate", "_native_get_player_winrate");
}

// ─── Native Implementations ─────────────────────────────────

public _native_get_mode(plugin, params)
{
    return g_iCurrentMode;
}

public _native_get_gameplay(plugin, params)
{
    return g_iCurrentGameplay;
}

public _native_get_match_state(plugin, params)
{
    return g_iMatchState;
}

public _native_get_player_pts(plugin, params)
{
    new id = get_param(1);
    if (!Hnsic_IsPlayerValid(id))
        return 0;
    return g_eUserData[id][UD_PTS];
}

public _native_set_player_pts(plugin, params)
{
    new id = get_param(1);
    new pts = get_param(2);
    if (!Hnsic_IsPlayerValid(id))
        return 0;
    g_eUserData[id][UD_PTS] = pts;
    return 1;
}

public _native_get_player_wins(plugin, params)
{
    new id = get_param(1);
    if (!Hnsic_IsPlayerValid(id))
        return 0;
    return g_eUserData[id][UD_WINS];
}

public _native_get_player_matches(plugin, params)
{
    new id = get_param(1);
    if (!Hnsic_IsPlayerValid(id))
        return 0;
    return g_eUserData[id][UD_MATCHES];
}

public _native_get_match_score(plugin, params)
{
    new team = get_param(1);
    if (team < TEAM_TT || team > TEAM_CT)
        return 0;
    return g_iMatchScore[team];
}

public _native_set_match_score(plugin, params)
{
    new team = get_param(1);
    new score = get_param(2);
    if (team < TEAM_TT || team > TEAM_CT)
        return 0;
    g_iMatchScore[team] = score;
    return 1;
}

public _native_force_mode(plugin, params)
{
    new mode = get_param(1);
    if (mode < MODE_NONE || mode > MODE_MIX)
        return 0;
    ModeManager_SwitchMode(mode);
    return 1;
}

public _native_force_gameplay(plugin, params)
{
    new gp = get_param(1);
    if (gp < GAMEPLAY_NONE || gp > GAMEPLAY_TRAINING)
        return 0;
    GameplayManager_Switch(gp);
    return 1;
}

public _native_get_current_half(plugin, params)
{
    return g_iCurrentHalf;
}

public _native_get_round_num(plugin, params)
{
    return g_iRoundNum;
}

public _native_is_match_active(plugin, params)
{
    return (g_iMatchState >= STATE_WARMUP && g_iMatchState <= STATE_OVERTIME);
}

public _native_get_scoring_mode(plugin, params)
{
    return g_iScoringMode;
}

public _native_get_player_winrate(plugin, params)
{
    new id = get_param(1);
    if (!Hnsic_IsPlayerValid(id))
        return 0;
    return _:Hnsic_GetWinrate(id);
}
