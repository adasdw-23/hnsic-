#include <amxmodx>
#include <fakemeta>
#include <hamsandwich>
#include <reapi>
#include <hns_matchsystem_maps>

#pragma semicolon 1

#define PLUGIN_NAME "HNS: Team SemiClip"
#define PLUGIN_VERSION "1.0"
#define PLUGIN_AUTHOR "TRAE"

// Modes
#define SEMICLIP_OFF 0
#define SEMICLIP_TEAM_ONLY 1
#define SEMICLIP_EVERYONE 2

new g_pCvarEnabled;
new g_pCvarMode;
new g_pCvarAdminOverride;

new g_iEnabled;
new g_iMode;
new g_iAdminOverride;

new bool:g_bSolidNot[MAX_PLAYERS + 1];

public plugin_init() {
	register_plugin(PLUGIN_NAME, PLUGIN_VERSION, PLUGIN_AUTHOR);

	g_pCvarEnabled = register_cvar("hns_semiclip_enabled", "1");
	g_pCvarMode = register_cvar("hns_semiclip_mode", "1"); // 0=off,1=team only,2=everyone
	g_pCvarAdminOverride = register_cvar("hns_semiclip_override", "0");

	bind_pcvar_num(g_pCvarEnabled, g_iEnabled);
	bind_pcvar_num(g_pCvarMode, g_iMode);
	bind_pcvar_num(g_pCvarAdminOverride, g_iAdminOverride);

	RegisterHam(Ham_Player_PreThink, "player", "hamPreThink");
	RegisterHam(Ham_Player_PostThink, "player", "hamPostThink");

	register_clcmd("say /semiclip", "cmdToggleSemiClip");
	register_clcmd("say_team /semiclip", "cmdToggleSemiClip");
	register_clcmd("say /scm", "cmdToggleSemiClip");
	register_clcmd("say_team /scm", "cmdToggleSemiClip");

	server_print("[HNS-SemiClip] Initialized (mode=%d)", g_iMode);
}

public client_disconnected(id) {
	g_bSolidNot[id] = false;
}

public hamPreThink(id) {
	if (!g_iEnabled || !is_user_alive(id)) return HAM_IGNORED;

	if (ShouldPlayerNoclip(id)) {
		set_pev(id, pev_solid, SOLID_NOT);
		g_bSolidNot[id] = true;
	}

	return HAM_HANDLED;
}

public hamPostThink(id) {
	if (g_bSolidNot[id]) {
		if (is_user_alive(id)) {
			set_pev(id, pev_solid, SOLID_SLIDEBOX);
		}
		g_bSolidNot[id] = false;
	}
	return HAM_HANDLED;
}

bool:ShouldPlayerNoclip(id) {
	if (g_iMode == SEMICLIP_OFF) return false;

	// Boost maps disable semiclip unless admin override is active
	if (!g_iAdminOverride && hnsmatch_maps_is_boost()) return false;

	if (g_iMode == SEMICLIP_EVERYONE) return true;

	// Team only mode: only in non-spectator team
	return get_member(id, m_iTeam) != TEAM_SPECTATOR;
}

public cmdToggleSemiClip(id) {
	if (!is_user_connected(id)) return PLUGIN_CONTINUE;

	new bool:bBoost = hnsmatch_maps_is_boost();
	client_print(id, print_chat, "[HNS] SemiClip: %s | Mode: %s | Boost map: %s | Override: %s", 
		g_iEnabled ? "ON" : "OFF",
		g_iMode == SEMICLIP_OFF ? "OFF" : (g_iMode == SEMICLIP_TEAM_ONLY ? "Team Only" : "Everyone"),
		bBoost ? "YES" : "NO",
		g_iAdminOverride ? "ON" : "OFF");
	return PLUGIN_CONTINUE;
}
