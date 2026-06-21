new hook_cmd;
new bool:hook[MAX_PLAYERS + 1];

new HookChain:fwdPlayerMove;

public training_init() {
	g_GPFuncs[GAMEPLAY_TRAINING][GP_ENABLE]	  = CreateOneForward(g_PluginId, "training_enable_rules");
	g_GPFuncs[GAMEPLAY_TRAINING][GP_DISABLE]  = CreateOneForward(g_PluginId, "training_disable_rules");
	g_GPFuncs[GAMEPLAY_TRAINING][GP_SETROLE]  = CreateOneForward(g_PluginId, "training_setrole", FP_CELL);
}

public training_enable_rules() {
	// Semiclip is handled by HnsMatchSemiClip.amxx; do not force it here.

	if (!hook_cmd) {
		hook_cmd = register_clcmd("+hook", "hook_on");
		register_clcmd("-hook", "hook_off");
	}

	if (!fwdPlayerMove)
		fwdPlayerMove = RegisterHookChain(RG_PM_Move, "PM_PlayerMove", true);

	EnableHookChain(fwdPlayerMove);
}

public hook_on(id) {
	hook[id] = true;
}

public hook_off(id) {
	hook[id] = false;
}

public PM_PlayerMove(const PlayerMove:ppmove, const server) {
	static Float:velocity[3];
	new const id = get_pmove(pm_player_index) + 1;

	if (hook[id]) {
		velocity_by_aim(id, 750, velocity);
		set_pmove(pm_velocity, velocity);

		set_entvar(id, var_sequence, 8);
		set_entvar(id, var_gaitsequence, 1);
	}
}

public training_disable_rules() {
	if (fwdPlayerMove)
		DisableHookChain(fwdPlayerMove);

	for (new i = 1; i <= MAX_PLAYERS; i++) {
		hook[i] = false;
	}
}

public training_setrole(id) {
	rg_remove_all_items(id);
	setUserGodmode(id, true);
	set_pev(id, pev_health, 100.0);
	set_entvar(id, var_health, 100.0);
	rg_give_item(id, "weapon_knife");
	rg_set_user_footsteps(id, false);
}
