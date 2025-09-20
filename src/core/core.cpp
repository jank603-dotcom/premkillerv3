#include <include/global.hpp>
#include <fstream>
#include <string>
#include <mmsystem.h>
#include <windows.h>
#pragma comment(lib, "winmm.lib")

static std::string pk_resolve_cfg_path(const std::string& filename)
{
    // If a path is provided, use it; otherwise save/load under bin/user
    if (filename.find('\\') != std::string::npos || filename.find('/') != std::string::npos)
        return filename;
    return std::string("bin/user/") + filename;
}

void core_c::settings_c::save_config(const std::string& filename)
{
	const std::string fullpath = pk_resolve_cfg_path(filename);
	// Ensure target directory
	CreateDirectoryA("bin", nullptr);
	CreateDirectoryA("bin\\user", nullptr);
	std::ofstream file(fullpath);
	if (!file.is_open()) return;

	// Config meta (versioning and preferences)
	file << "[CONFIG]\n";
	file << "config_version=" << 2 << "\n";
	file << "name=" << this->config.config_name << "\n";
	file << "auto_save=" << this->config.auto_save << "\n";

	// Aim settings
	file << "[AIM]\n";
	file << "enabled=" << aim.enabled << "\n";
	file << "rcs=" << aim.rcs << "\n";
	file << "rcs_strength=" << aim.rcs_strength << "\n";
	file << "key=" << aim.key << "\n";
	file << "type=" << aim.type << "\n";
	file << "smooth=" << aim.smooth << "\n";
	file << "fov=" << aim.fov << "\n";
	file << "hitbox_head=" << aim.hitbox_head << "\n";
	file << "hitbox_chest=" << aim.hitbox_chest << "\n";
	file << "hitbox_pelvis=" << aim.hitbox_pelvis << "\n";
	file << "hitbox_arms=" << aim.hitbox_arms << "\n";
	file << "hitbox_legs=" << aim.hitbox_legs << "\n";
	file << "vis_check=" << aim.vis_check << "\n";
	file << "flash_check=" << aim.flash_check << "\n";
	file << "smoke_check=" << aim.smoke_check << "\n";
	// Humanize/jitter
	file << "humanize=" << aim.humanize << "\n";
	file << "jitter_amount_px=" << aim.jitter_amount_px << "\n";
	file << "jitter_rate_hz=" << aim.jitter_rate_hz << "\n";
	// Targeting
	file << "targeting_mode=" << aim.targeting_mode << "\n";
	file << "target_debounce_ms=" << aim.target_debounce_ms << "\n";
	file << "viz_fov=" << aim.visualization.fov << "\n";
	file << "viz_line=" << aim.visualization.line << "\n";
	file << "viz_dot=" << aim.visualization.dot << "\n";
	// New visibility and preferred hitbox settings
	file << "visibility_mode=" << aim.visibility_mode << "\n";
	file << "anti_flicker=" << aim.anti_flicker << "\n";
	file << "use_preferred_hitbox=" << aim.use_preferred_hitbox << "\n";
	file << "preferred_hitbox=" << aim.preferred_hitbox << "\n";

	// Visuals settings
	file << "[VISUALS]\n";
	file << "box=" << visuals.player.box << "\n";
	file << "skeleton=" << visuals.player.skeleton << "\n";
	file << "health=" << visuals.player.health << "\n";
	file << "name=" << visuals.player.name << "\n";
	file << "weapon=" << visuals.player.weapon << "\n";
	file << "distance=" << visuals.player.distance << "\n";
	file << "oof_arrows=" << visuals.player.oof_arrows << "\n";
	file << "chams=" << visuals.player.chamies << "\n";
	file << "chams_health_based=" << visuals.player.chamies_health_based << "\n";
	file << "head_circle=" << visuals.player.head_circle << "\n";
    // Player flags persistence
    file << "player_flags=" << visuals.player.flags << "\n";
    file << "player_flags_color_r=" << visuals.player.flag_color[0] << "\n";
    file << "player_flags_color_g=" << visuals.player.flag_color[1] << "\n";
    file << "player_flags_color_b=" << visuals.player.flag_color[2] << "\n";
    file << "player_flags_mask=" << visuals.player.flags_mask << "\n";
    file << "player_flags_text_size=" << visuals.player.flag_text_size << "\n";
    file << "player_show_ping=" << visuals.player.show_ping << "\n";
    file << "player_show_network_info=" << visuals.player.show_network_info << "\n";
    file << "player_ping_warning_threshold=" << visuals.player.ping_warning_threshold << "\n";
    	file << "player_lag_spike_threshold=" << visuals.player.lag_spike_threshold << "\n";
    // ESP layout persistence
    file << "esp_layout_health_side=" << visuals.player.layout.health_side << "\n";
    file << "esp_layout_name_offset_x=" << visuals.player.layout.name_offset[0] << "\n";
    file << "esp_layout_name_offset_y=" << visuals.player.layout.name_offset[1] << "\n";
    file << "esp_layout_weapon_offset_x=" << visuals.player.layout.weapon_offset[0] << "\n";
    file << "esp_layout_weapon_offset_y=" << visuals.player.layout.weapon_offset[1] << "\n";
    file << "esp_layout_distance_offset_x=" << visuals.player.layout.distance_offset[0] << "\n";
    file << "esp_layout_distance_offset_y=" << visuals.player.layout.distance_offset[1] << "\n";
    file << "esp_layout_health_bar_width=" << visuals.player.layout.health_bar_width << "\n";
    file << "esp_layout_health_bar_height=" << visuals.player.layout.health_bar_height << "\n";
    file << "esp_layout_box_style=" << visuals.player.layout.box_style << "\n";
    file << "esp_layout_text_align=" << visuals.player.layout.text_align << "\n";
    file << "esp_layout_font_size=" << visuals.player.layout.font_size << "\n";
	file << "world_smoke=" << visuals.world.smoke << "\n";
	file << "world_smoke_dome=" << visuals.world.smoke_dome << "\n";
	file << "world_molotov=" << visuals.world.molotov << "\n";
	file << "world_molotov_bounds=" << visuals.world.molotov_bounds << "\n";
	file << "world_drops=" << visuals.world.drops << "\n";
	file << "world_drops_bounds=" << visuals.world.drops_bounds << "\n";
	file << "world_bomb=" << visuals.world.bomb << "\n";
	file << "world_bomb_timer_world=" << visuals.world.bomb_timer_world << "\n";
	file << "world_bomb_timer_hud=" << visuals.world.bomb_timer_hud << "\n";
	file << "world_bomb_timer_color_r=" << visuals.world.bomb_timer_color[0] << "\n";
	file << "world_bomb_timer_color_g=" << visuals.world.bomb_timer_color[1] << "\n";
	file << "world_bomb_timer_color_b=" << visuals.world.bomb_timer_color[2] << "\n";
	    file << "world_bomb_timer_duration=" << visuals.world.bomb_timer_duration << "\n";
    // Bomb HUD draggable window position
    file << "world_bomb_timer_window_pos_x=" << visuals.world.bomb_timer_window_pos[0] << "\n";
    file << "world_bomb_timer_window_pos_y=" << visuals.world.bomb_timer_window_pos[1] << "\n";
    // Viewmodel changer
    file << "world_viewmodel_enabled=" << visuals.world.viewmodel_changer.enabled << "\n";
    file << "world_viewmodel_x=" << visuals.world.viewmodel_changer.offset_x << "\n";
    file << "world_viewmodel_y=" << visuals.world.viewmodel_changer.offset_y << "\n";
    file << "world_viewmodel_z=" << visuals.world.viewmodel_changer.offset_z << "\n";
    file << "world_viewmodel_fov=" << visuals.world.viewmodel_changer.fov_offset << "\n";
	file << "movement_local_trail=" << visuals.movement_tracers.local_trail << "\n";
	file << "movement_enemy_trail=" << visuals.movement_tracers.enemy_trail << "\n";

    // Spectator settings (now separated)
    file << "[SPECTATOR]\n";
    file << "enabled=" << visuals.spectator.enabled << "\n";
    file << "name=" << visuals.spectator.name << "\n";
    file << "text_color_r=" << visuals.spectator.text_color[0] << "\n";
    file << "text_color_g=" << visuals.spectator.text_color[1] << "\n";
    file << "text_color_b=" << visuals.spectator.text_color[2] << "\n";
    file << "window_pos_x=" << visuals.spectator.window_pos[0] << "\n";
    file << "window_pos_y=" << visuals.spectator.window_pos[1] << "\n";

    // Exploits settings (empty now)
    file << "[EXPLOITS]\n";

	    // Misc settings
	file << "[MISC]\n";
	file << "vsync=" << misc.vsync << "\n";
	file << "no_wait=" << misc.do_no_wait << "\n";
	file << "watermark=" << misc.watermark << "\n";
	file << "no_flash=" << misc.no_flash << "\n";
	file << "no_flash_alpha=" << misc.no_flash_alpha << "\n";
	file << "bunnyhop=" << misc.bunnyhop << "\n";
	file << "bunnyhop_debug=" << misc.bunnyhop_debug << "\n";
	file << "crosshair_overlay=" << misc.crosshair_overlay << "\n";
	file << "crosshair_use_theme_color=" << misc.crosshair_use_theme_color << "\n";
	file << "crosshair_size=" << misc.crosshair_size << "\n";
	file << "crosshair_thickness=" << misc.crosshair_thickness << "\n";
	file << "crosshair_color_r=" << misc.crosshair_color[0] << "\n";
	file << "crosshair_color_g=" << misc.crosshair_color[1] << "\n";
	file << "crosshair_color_b=" << misc.crosshair_color[2] << "\n";
	file << "night_mode=" << misc.night_mode << "\n";
	file << "night_strength=" << misc.night_mode_strength << "\n";
	file << "third_person=" << misc.third_person << "\n";
	file << "third_person_key=" << misc.third_person_key << "\n";
	// Hitmarker
	file << "hitmarker=" << misc.hitmarker << "\n";
	file << "hitmarker_volume=" << misc.hitmarker_volume << "\n";
	file << "hitmarker_sound_path=" << misc.hitmarker_sound_path << "\n";

	// Triggerbot settings
	file << "[TRIGGERBOT]\n";
	file << "enabled=" << triggerbot.enabled << "\n";
	file << "hotkey=" << triggerbot.hotkey << "\n";
	// legacy keys for older configs
	file << "reaction_time=" << triggerbot.first_shot_delay_ms << "\n";
	file << "time_between_shots=" << triggerbot.delay_between_shots_ms << "\n";
	// new keys
	file << "first_shot_delay_ms=" << triggerbot.first_shot_delay_ms << "\n";
	file << "delay_between_shots_ms=" << triggerbot.delay_between_shots_ms << "\n";
	file << "hitgroup_filter=" << triggerbot.hitgroup_filter << "\n";
	file << "burst_mode=" << triggerbot.burst_mode << "\n";

	// Themes settings
	file << "[THEMES]\n";
	file << "current_theme=" << themes.current_theme << "\n";
	file << "esp_color_r=" << themes.esp_color[0] << "\n";
	file << "esp_color_g=" << themes.esp_color[1] << "\n";
	file << "esp_color_b=" << themes.esp_color[2] << "\n";
	file << "fov_color_r=" << themes.fov_color[0] << "\n";
	file << "fov_color_g=" << themes.fov_color[1] << "\n";
	file << "fov_color_b=" << themes.fov_color[2] << "\n";

	// Extras settings
	file << "[EXTRAS]\n";
	file << "fps_counter=" << extras.fps_counter << "\n";
	file << "ping_display=" << extras.ping_display << "\n";
	file << "stream_proof=" << extras.stream_proof << "\n";
	file << "fps_cap_enabled=" << extras.fps_cap_enabled << "\n";
	file << "fps_cap_limit=" << extras.fps_cap_limit << "\n";

	// Death Match settings
	file << "[DEATH_MATCH]\n";
	file << "enabled=" << death_match.enabled << "\n";
	file << "highlight_all=" << death_match.highlight_all << "\n";
	file << "ignore_team=" << death_match.ignore_team << "\n";
	file << "show_enemy_names=" << death_match.show_enemy_names << "\n";
	file << "debug=" << death_match.debug << "\n";

	// Hotkeys settings
	file << "[HOTKEYS]\n";
	file << "menu_toggle=" << hotkeys.menu_toggle << "\n";
	file << "aimbot_toggle=" << hotkeys.aimbot_toggle << "\n";
	file << "esp_toggle=" << hotkeys.esp_toggle << "\n";
	file << "third_person=" << hotkeys.third_person << "\n";
	file << "night_mode=" << hotkeys.night_mode << "\n";
	file << "stream_proof=" << hotkeys.stream_proof << "\n";
	file << "fps_counter=" << hotkeys.fps_counter << "\n";
	file << "ping_display=" << hotkeys.ping_display << "\n";
	file << "no_flash=" << hotkeys.no_flash << "\n";
	file << "triggerbot_toggle=" << hotkeys.triggerbot_toggle << "\n";
	file << "crosshair_overlay=" << hotkeys.crosshair_overlay << "\n";

	file.close();
}

// Play hitmarker sound according to settings (supports .wav and .mp3 via MCI)
void core_c::play_hitmarker_sound()
{
    auto& misc = this->get_settings().misc;
    if (!misc.hitmarker) return;
    if (misc.hitmarker_sound_path[0] == '\0') return;

    // Determine file extension (lowercase)
    std::string path = misc.hitmarker_sound_path;
    // Verify file exists before attempting to open
    {
        std::ifstream f(path.c_str(), std::ios::binary);
        if (!f.good()) return;
    }
    std::string ext;
    auto dot = path.find_last_of('.') ;
    if (dot != std::string::npos) {
        ext = path.substr(dot + 1);
        for (auto& c : ext) c = (char)::tolower((unsigned char)c);
    }

    // Stop any previous alias
    mciSendStringA("close prem_hitmarker", nullptr, 0, nullptr);

    // Open with appropriate type
    char cmd[1024] = {0};
    if (ext == "mp3") {
        snprintf(cmd, sizeof(cmd), "open \"%s\" type mpegvideo alias prem_hitmarker", path.c_str());
    } else {
        // default to waveaudio for wav or others
        snprintf(cmd, sizeof(cmd), "open \"%s\" type waveaudio alias prem_hitmarker", path.c_str());
    }
    if (mciSendStringA(cmd, nullptr, 0, nullptr) != 0) {
        return; // failed to open
    }

    // Set volume (0..100 -> 0..1000)
    int vol = misc.hitmarker_volume;
    if (vol < 0) vol = 0; if (vol > 100) vol = 100;
    snprintf(cmd, sizeof(cmd), "setaudio prem_hitmarker volume to %d", vol * 10);
    mciSendStringA(cmd, nullptr, 0, nullptr);

    // Play from start, no wait
    mciSendStringA("play prem_hitmarker from 0", nullptr, 0, nullptr);
}

void core_c::settings_c::load_config(const std::string& filename)
{
	const std::string fullpath = pk_resolve_cfg_path(filename);
	std::ifstream file(fullpath);
	if (!file.is_open()) return;

	std::string line, section;
	int cfg_version = 1; // default if not present
	while (std::getline(file, line)) {
		if (line.empty() || line[0] == '#') continue;
		
		if (line[0] == '[') {
			section = line;
			continue;
		}
		
		auto pos = line.find('=');
		if (pos == std::string::npos) continue;
		
		std::string key = line.substr(0, pos);
		std::string value = line.substr(pos + 1);
		
		if (section == "[CONFIG]") {
			if (key == "config_version") { try { cfg_version = std::stoi(value); } catch(...) { cfg_version = 1; } }
			else if (key == "name") { strncpy_s(this->config.config_name, sizeof(this->config.config_name), value.c_str(), _TRUNCATE); }
			else if (key == "auto_save") { this->config.auto_save = (value == "1"); }
		}
		else
		if (section == "[AIM]") {
			if (key == "enabled") aim.enabled = (value == "1");
			else if (key == "rcs") aim.rcs = (value == "1");
			else if (key == "rcs_strength") aim.rcs_strength = std::stof(value);
			else if (key == "key") aim.key = std::stoi(value);
			else if (key == "type") aim.type = std::stoi(value);
			else if (key == "smooth") aim.smooth = std::stof(value);
			else if (key == "fov") aim.fov = std::stof(value);
			else if (key == "hitbox_head") aim.hitbox_head = (value == "1");
			else if (key == "hitbox_chest") aim.hitbox_chest = (value == "1");
			else if (key == "hitbox_pelvis") aim.hitbox_pelvis = (value == "1");
			else if (key == "hitbox_arms") aim.hitbox_arms = (value == "1");
			else if (key == "hitbox_legs") aim.hitbox_legs = (value == "1");
			else if (key == "vis_check") aim.vis_check = (value == "1");
			else if (key == "flash_check") aim.flash_check = (value == "1");
			else if (key == "smoke_check") aim.smoke_check = (value == "1");
			else if (key == "humanize") aim.humanize = (value == "1");
			else if (key == "jitter_amount_px") aim.jitter_amount_px = std::stof(value);
			else if (key == "jitter_rate_hz") aim.jitter_rate_hz = std::stof(value);
			else if (key == "targeting_mode") aim.targeting_mode = std::stoi(value);
			else if (key == "target_debounce_ms") aim.target_debounce_ms = std::stoi(value);
			else if (key == "viz_fov") aim.visualization.fov = (value == "1");
			else if (key == "viz_line") aim.visualization.line = (value == "1");
			else if (key == "viz_dot") aim.visualization.dot = (value == "1");
			else if (key == "visibility_mode") aim.visibility_mode = std::stoi(value);
			else if (key == "anti_flicker") aim.anti_flicker = (value == "1");
			else if (key == "use_preferred_hitbox") aim.use_preferred_hitbox = (value == "1");
			else if (key == "preferred_hitbox") aim.preferred_hitbox = std::stoi(value);
		}
		else if (section == "[VISUALS]") {
			if (key == "box") visuals.player.box = (value == "1");
			else if (key == "skeleton") visuals.player.skeleton = (value == "1");
			else if (key == "health") visuals.player.health = (value == "1");
			else if (key == "name") visuals.player.name = (value == "1");
			else if (key == "weapon") visuals.player.weapon = (value == "1");
			else if (key == "distance") visuals.player.distance = (value == "1");
			else if (key == "oof_arrows") visuals.player.oof_arrows = (value == "1");
			else if (key == "chams") visuals.player.chamies = (value == "1");
			else if (key == "chams_health_based") visuals.player.chamies_health_based = (value == "1");
			else if (key == "head_circle") visuals.player.head_circle = (value == "1");
            // Player flags settings
            else if (key == "player_flags") visuals.player.flags = (value == "1");
            else if (key == "player_flags_color_r") visuals.player.flag_color[0] = std::stof(value);
            else if (key == "player_flags_color_g") visuals.player.flag_color[1] = std::stof(value);
            else if (key == "player_flags_color_b") visuals.player.flag_color[2] = std::stof(value);
            else if (key == "player_flags_mask") visuals.player.flags_mask = std::stoi(value);
            else if (key == "player_flags_text_size") visuals.player.flag_text_size = std::stof(value);
            else if (key == "player_show_ping") visuals.player.show_ping = (value == "1");
            else if (key == "player_show_network_info") visuals.player.show_network_info = (value == "1");
            else if (key == "player_ping_warning_threshold") visuals.player.ping_warning_threshold = std::stof(value);
            			else if (key == "player_lag_spike_threshold") visuals.player.lag_spike_threshold = std::stof(value);
            // ESP layout
            else if (key == "esp_layout_health_side") visuals.player.layout.health_side = std::stoi(value);
            else if (key == "esp_layout_name_offset_x") visuals.player.layout.name_offset[0] = std::stof(value);
            else if (key == "esp_layout_name_offset_y") visuals.player.layout.name_offset[1] = std::stof(value);
            else if (key == "esp_layout_weapon_offset_x") visuals.player.layout.weapon_offset[0] = std::stof(value);
            else if (key == "esp_layout_weapon_offset_y") visuals.player.layout.weapon_offset[1] = std::stof(value);
            else if (key == "esp_layout_distance_offset_x") visuals.player.layout.distance_offset[0] = std::stof(value);
            else if (key == "esp_layout_distance_offset_y") visuals.player.layout.distance_offset[1] = std::stof(value);
            else if (key == "esp_layout_health_bar_width") visuals.player.layout.health_bar_width = std::stof(value);
            else if (key == "esp_layout_health_bar_height") visuals.player.layout.health_bar_height = std::stof(value);
            else if (key == "esp_layout_box_style") visuals.player.layout.box_style = std::stoi(value);
            else if (key == "esp_layout_text_align") visuals.player.layout.text_align = std::stoi(value);
            else if (key == "esp_layout_font_size") visuals.player.layout.font_size = std::stof(value);
			else if (key == "world_smoke") visuals.world.smoke = (value == "1");
			else if (key == "world_smoke_dome") visuals.world.smoke_dome = (value == "1");
			else if (key == "world_molotov") visuals.world.molotov = (value == "1");
			else if (key == "world_molotov_bounds") visuals.world.molotov_bounds = (value == "1");
			else if (key == "world_drops") visuals.world.drops = (value == "1");
			else if (key == "world_drops_bounds") visuals.world.drops_bounds = (value == "1");
			else if (key == "world_bomb") visuals.world.bomb = (value == "1");
			else if (key == "world_bomb_timer_world") visuals.world.bomb_timer_world = (value == "1");
			else if (key == "world_bomb_timer_hud") visuals.world.bomb_timer_hud = (value == "1");
			else if (key == "world_bomb_timer_color_r") visuals.world.bomb_timer_color[0] = std::stof(value);
			else if (key == "world_bomb_timer_color_g") visuals.world.bomb_timer_color[1] = std::stof(value);
			else if (key == "world_bomb_timer_color_b") visuals.world.bomb_timer_color[2] = std::stof(value);
			else if (key == "world_bomb_timer_duration") visuals.world.bomb_timer_duration = std::stof(value);
            else if (key == "world_bomb_timer_window_pos_x") visuals.world.bomb_timer_window_pos[0] = std::stof(value);
            else if (key == "world_bomb_timer_window_pos_y") visuals.world.bomb_timer_window_pos[1] = std::stof(value);
			else if (key == "spectator_enabled") visuals.spectator.enabled = (value == "1");
			else if (key == "spectator_name") visuals.spectator.name = (value == "1");
			else if (key == "spectator_text_color_r") visuals.spectator.text_color[0] = std::stof(value);
			else if (key == "spectator_text_color_g") visuals.spectator.text_color[1] = std::stof(value);
			else if (key == "spectator_text_color_b") visuals.spectator.text_color[2] = std::stof(value);
			else if (key == "spectator_window_pos_x") visuals.spectator.window_pos[0] = std::stof(value);
			else if (key == "spectator_window_pos_y") visuals.spectator.window_pos[1] = std::stof(value);
			else if (key == "movement_local_trail") visuals.movement_tracers.local_trail = (value == "1");
			else if (key == "movement_enemy_trail") visuals.movement_tracers.enemy_trail = (value == "1");
		}
		else if (section == "[SPECTATOR]") {
			if (key == "enabled") visuals.spectator.enabled = (value == "1");
			else if (key == "name") visuals.spectator.name = (value == "1");
			else if (key == "text_color_r") visuals.spectator.text_color[0] = std::stof(value);
			else if (key == "text_color_g") visuals.spectator.text_color[1] = std::stof(value);
			else if (key == "text_color_b") visuals.spectator.text_color[2] = std::stof(value);
			else if (key == "window_pos_x") visuals.spectator.window_pos[0] = std::stof(value);
			else if (key == "window_pos_y") visuals.spectator.window_pos[1] = std::stof(value);
		}
		        else if (section == "[MISC]") {
            if (key == "vsync") misc.vsync = (value == "1");
            else if (key == "no_wait") misc.do_no_wait = (value == "1");
            else if (key == "watermark") misc.watermark = (value == "1");
            else if (key == "no_flash") misc.no_flash = (value == "1");
            else if (key == "no_flash_alpha") misc.no_flash_alpha = std::stof(value);
            else if (key == "bunnyhop") misc.bunnyhop = (value == "1");
            else if (key == "bunnyhop_debug") misc.bunnyhop_debug = (value == "1");
            else if (key == "crosshair_overlay") misc.crosshair_overlay = (value == "1");
            else if (key == "crosshair_use_theme_color") misc.crosshair_use_theme_color = (value == "1");
            else if (key == "crosshair_size") misc.crosshair_size = std::stof(value);
            else if (key == "crosshair_thickness") misc.crosshair_thickness = std::stof(value);
            else if (key == "crosshair_color_r") misc.crosshair_color[0] = std::stof(value);
            else if (key == "crosshair_color_g") misc.crosshair_color[1] = std::stof(value);
            else if (key == "crosshair_color_b") misc.crosshair_color[2] = std::stof(value);
            else if (key == "night_mode") misc.night_mode = (value == "1");
            else if (key == "night_strength") misc.night_mode_strength = std::stof(value);
            else if (key == "third_person") misc.third_person = (value == "1");
            else if (key == "third_person_key") misc.third_person_key = std::stoi(value);
            else if (key == "hitmarker") misc.hitmarker = (value == "1");
            else if (key == "hitmarker_volume") misc.hitmarker_volume = std::stoi(value);
            else if (key == "hitmarker_sound_path") {
                strncpy_s(misc.hitmarker_sound_path, sizeof(misc.hitmarker_sound_path), value.c_str(), _TRUNCATE);
            }
        }
		else if (section == "[TRIGGERBOT]") {
			if (key == "enabled") triggerbot.enabled = (value == "1");
			else if (key == "hotkey") triggerbot.hotkey = std::stoi(value);
			// legacy keys: map to new fields only if new not provided later
			else if (key == "reaction_time") triggerbot.first_shot_delay_ms = std::stoi(value);
			else if (key == "time_between_shots") triggerbot.delay_between_shots_ms = std::stoi(value);
			// new keys override legacy values
			else if (key == "first_shot_delay_ms") triggerbot.first_shot_delay_ms = std::stoi(value);
			else if (key == "delay_between_shots_ms") triggerbot.delay_between_shots_ms = std::stoi(value);
			else if (key == "hitgroup_filter") triggerbot.hitgroup_filter = std::stoi(value);
			else if (key == "burst_mode") triggerbot.burst_mode = (value == "1");
		}
		else if (section == "[HOTKEYS]") {
			if (key == "triggerbot_toggle") hotkeys.triggerbot_toggle = std::stoi(value);
			else if (key == "menu_toggle") hotkeys.menu_toggle = std::stoi(value);
			else if (key == "aimbot_toggle") hotkeys.aimbot_toggle = std::stoi(value);
			else if (key == "esp_toggle") hotkeys.esp_toggle = std::stoi(value);
			else if (key == "third_person") hotkeys.third_person = std::stoi(value);
			else if (key == "night_mode") hotkeys.night_mode = std::stoi(value);
			else if (key == "stream_proof") hotkeys.stream_proof = std::stoi(value);
			else if (key == "fps_counter") hotkeys.fps_counter = std::stoi(value);
			else if (key == "ping_display") hotkeys.ping_display = std::stoi(value);
			else if (key == "no_flash") hotkeys.no_flash = std::stoi(value);
			else if (key == "crosshair_overlay") hotkeys.crosshair_overlay = std::stoi(value);
		}
		else if (section == "[THEMES]") {
			if (key == "current_theme") themes.current_theme = std::stoi(value);
			else if (key == "esp_color_r") themes.esp_color[0] = std::stof(value);
			else if (key == "esp_color_g") themes.esp_color[1] = std::stof(value);
			else if (key == "esp_color_b") themes.esp_color[2] = std::stof(value);
			else if (key == "fov_color_r") themes.fov_color[0] = std::stof(value);
			else if (key == "fov_color_g") themes.fov_color[1] = std::stof(value);
			else if (key == "fov_color_b") themes.fov_color[2] = std::stof(value);
		}
		else if (section == "[EXTRAS]") {
			if (key == "fps_counter") extras.fps_counter = (value == "1");
			else if (key == "ping_display") extras.ping_display = (value == "1");
			else if (key == "stream_proof") extras.stream_proof = (value == "1");
			else if (key == "fps_cap_enabled") extras.fps_cap_enabled = (value == "1");
			else if (key == "fps_cap_limit") extras.fps_cap_limit = std::stoi(value);
		}
		else if (section == "[DEATH_MATCH]") {
			if (key == "enabled") death_match.enabled = (value == "1");
			else if (key == "highlight_all") death_match.highlight_all = (value == "1");
			else if (key == "ignore_team") death_match.ignore_team = (value == "1");
			else if (key == "show_enemy_names") death_match.show_enemy_names = (value == "1");
			else if (key == "debug") death_match.debug = (value == "1");
		}

	}

	file.close();
}

void core_c::log_error( const std::string& error_message )
{
	// Log error to console and optionally to file
	printf("[ERROR] %s\n", error_message.c_str());
	fflush(stdout);
	
	// You can also log to a file here if needed
	// std::ofstream log_file("premkiller_error.log", std::ios::app);
	// if (log_file.is_open()) {
	//     auto now = std::chrono::system_clock::now();
	//     auto time_t = std::chrono::system_clock::to_time_t(now);
	//     log_file << std::ctime(&time_t) << "[ERROR] " << error_message << std::endl;
	//     log_file.close();
	// }
}
