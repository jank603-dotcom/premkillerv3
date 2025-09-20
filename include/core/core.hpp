#ifndef CORE_HPP
#define CORE_HPP

#include <windows.h>
#include <string>
#include <cstdint>

class core_c
{
public:
	class display_c
	{
	public:
		int width;
		int height;
		float aspect;

		display_c( ) : width( GetSystemMetrics( SM_CXSCREEN ) ), height( GetSystemMetrics( SM_CYSCREEN ) ), aspect( width ? static_cast< float >( width ) / height : 0.0f ) { }
	};

	class settings_c
	{
	public:
		struct aim_t
		{
			bool enabled{ true };
			bool rcs{ true };
			float rcs_strength{ 1.0f };
			// Extended RCS parameters
			float rcs_factor{ 1.0f };          // 0..20
			float rcs_smooth{ 1.0f };          // 1..20
			float rcs_x_scale{ 0.0f };         // 0..0.5
			float rcs_y_scale{ 1.0f };         // 1..2
			int   rcs_start_bullets{ 1 };      // 1..10
			int key{ VK_XBUTTON1 };
			int type{ 0 };
			// Target selection mode: 0 = Closest to crosshair, 1 = Lowest health
			int targeting_mode{ 0 };
			// Debounce window to continue scanning/applying after key release (ms)
			int target_debounce_ms{ 120 };
			float smooth{ 10.0f };
			float fov{ 100.0f };
			// Minimum hit chance requirement in percent (1..100)
			int hit_chance_percent{ 50 };
			bool hitbox_head{ true };
			bool hitbox_chest{ false };
			bool hitbox_pelvis{ false };
			bool hitbox_arms{ false };
			bool hitbox_legs{ false };
			bool vis_check{ true };
			bool flash_check{ false };
			bool smoke_check{ false };
			// Humanize/jitter settings
			bool humanize{ false };           // enable subtle jitter for more human-like motion
			float jitter_amount_px{ 0.5f };   // max pixel jitter per update (0..3 px)
			float jitter_rate_hz{ 12.0f };    // how often to change jitter direction (updates per second)
			// Additional global tuning
			float humanized_smooth{ 0.0f };   // 0..1
			float jitter_aim{ 0.0f };         // 0..1
			// Visibility mode and anti-flicker
			// 0 = Relaxed, 1 = Normal, 2 = Strict
			int visibility_mode{ 1 };
			// Smooth out rapid visibility state changes
			bool anti_flicker{ true };
			// Preferred hitbox selection (optional override)
			bool use_preferred_hitbox{ false };
			// 0=Head, 1=Chest, 2=Pelvis
			int preferred_hitbox{ 0 };
			struct visualization_t
			{
				bool fov{ false };
				bool line{ false };
				bool dot{ false };
			} visualization;
		};

		struct triggerbot_t
		{
			bool enabled{ false };
			int hotkey{ VK_LBUTTON };
			int hitgroup_filter{ 3 }; // 0=Head, 1=Chest, 2=Stomach, 3=All
			bool burst_mode{ false };
			int time_between_shots{ 100 }; // legacy key for backward compatibility
			// New delay controls
			int first_shot_delay_ms{ 0 };         // default: no delay for first shot
			int delay_between_shots_ms{ 100 };    // default: 100ms between shots
		};

		struct visuals_t
		{
			bool player_esp_enabled{ true };
			struct player_t
			{
				bool box{ true };
				bool skeleton{ false };

				bool health{ true };

				bool name{ true };
				bool weapon{ true };
				bool distance{ true };

				bool oof_arrows{ true };

				bool chamies{ false };
				bool chamies_health_based{ false };
				bool head_circle{ false };
				bool flags{ false };
				float flag_color[3]{ 1.0f, 1.0f, 1.0f };
				// Bitmask to choose which flags to display (bit 0..4 correspond to FLASHED, SCOPED, DEFUSING, HIGH PING, LAG SPIKE)
				int flags_mask{ 0x1F }; // default: all 5 flags on
				// Optional custom text size for flags; 0 or negative uses current font size
				float flag_text_size{ 0.0f };
				bool show_ping{ false };
				bool show_network_info{ false };
				float ping_warning_threshold{ 100.0f };    // Ping threshold for warning (ms)
				float lag_spike_threshold{ 0.05f };        // Interpolation change threshold for lag spike detection

				// Persistent layout for ESP elements (used by preview and in-game renderer)
				struct layout_t
				{
					// 0=Left, 1=Right, 2=Top, 3=Bottom
					int health_side{ 0 };
					// Offsets relative to default anchor positions
					float name_offset[2]{ 0.0f, 10.0f };
					float weapon_offset[2]{ 0.0f, 0.0f };
					float distance_offset[2]{ 0.0f, 0.0f };
					// Optional style tuning
					float health_bar_width{ 4.0f };
					float health_bar_height{ 4.0f }; // used for top/bottom style
					// 0=Full box, 1=Corner box
					int box_style{ 0 };
					// 0=Center, 1=Left, 2=Right for text under/over box
					int text_align{ 0 };
					// 0 to use font default; >0 overrides
					float font_size{ 0.0f };
				} layout;


				struct radar_t
				{
					bool enabled{ false };
					bool distance{ false };
					bool bomb{ false };
					float size{ 200.0f };
					float zoom{ 1.0f };
					bool show_names{ true };
					bool show_health{ true };
				} radar;
			} player;

			struct world_t
			{
				bool smoke{ false };
				bool smoke_dome{ false };

				bool molotov{ false };
				bool molotov_bounds{ false };

				bool drops{ false };
				bool drops_bounds{ true };

				bool bomb{ false };
				bool bomb_timer_world{ false };     // show countdown above bomb in world
				bool bomb_timer_hud{ false };       // show countdown on HUD overlay
				float bomb_timer_color[3]{ 1.0f, 0.25f, 0.25f }; // default reddish
				float bomb_timer_duration{ 40.0f }; // fallback duration (s) if offsets not available
				// Draggable HUD window position (set at runtime if not initialized)
				float bomb_timer_window_pos[2]{ -1.0f, -1.0f };
				// Global world accent color (for future world rendering accents)
				float world_color[3]{ 1.0f, 1.0f, 1.0f };

				// Notification toggles (top-right UI notifications)
				bool notify_hit{ false };
				bool notify_kill{ false };
				bool notify_bomb_plant{ false };

				// Viewmodel changer (external write on local pawn)
				struct viewmodel_changer_t {
					bool enabled{ false };
					float offset_x{ 0.0f };
					float offset_y{ 0.0f };
					float offset_z{ 0.0f };
					float fov_offset{ 0.0f }; // absolute FOV if non-zero; set 0 to keep default
				} viewmodel_changer;
			} world;

			struct spectator_t
			{
				bool enabled{ true };

				bool name{ true };
				float text_color[3]{ 1.0f, 1.0f, 1.0f };
				float window_pos[2]{ 20.0f, 60.0f }; // default under ping display, draggable
			} spectator;

			struct movement_tracers_t
			{
				bool local_trail{ false };
				bool enemy_trail{ false };
			} movement_tracers;

			// Field-of-View changer (external write)
			struct fov_changer_t
			{
				bool enabled{ false };
				int desired_fov{ 90 };           // Target FOV value
				bool disable_while_scoped{ true }; // Do not apply when scoped
				bool reset_on_disable{ true };     // Reset to default when disabled
				bool pause_aim_during_change{ true }; // Suppress aimbot briefly when FOV changes
			} fov_changer;
		};

		struct death_match_t
		{
			bool enabled{ false };
			bool highlight_all{ false };
			bool ignore_team{ false };
			bool show_enemy_names{ false };
			bool debug{ false }; // optional debug overlay
		};

		struct exploits_t
		{
			// Exploits tab is now empty - features moved to misc
		};

		struct misc_t
		{
			bool vsync{ false };
			bool do_no_wait{ false };
			bool watermark{ true };
			bool no_flash{ false };
			float no_flash_alpha{ 0.22f };
			bool bunnyhop{ false };
			bool bunnyhop_debug{ false }; // Toggle BHOP DEBUG overlay
			bool crosshair_overlay{ false };
			float crosshair_size{ 10.0f };
			float crosshair_thickness{ 2.0f };
			float crosshair_color[3]{ 1.0f, 1.0f, 1.0f };
			bool crosshair_use_theme_color{ true };
			bool night_mode{ false };
			float night_mode_strength{ 1.0f };
			bool third_person{ false };
			int third_person_key{ VK_F3 };
			bool setting_third_person_key{ false };

			// Hitmarker sound on successful hit
			bool hitmarker{ false };
			int hitmarker_volume{ 100 }; // 0..100
			char hitmarker_sound_path[MAX_PATH]{ 0 };
		};

		struct themes_t
		{
			int current_theme{ 0 }; // 0=Dark, 1=Blue, 2=Red, 3=Green, 4=Purple
			float esp_color[3]{ 1.0f, 1.0f, 1.0f }; // RGB for ESP
			float fov_color[3]{ 1.0f, 1.0f, 1.0f }; // RGB for FOV circle
		};

		struct extras_t
		{
			bool fps_counter{ true };
			bool ping_display{ false };
			bool stream_proof{ false };
			bool fps_cap_enabled{ false };
			int fps_cap_limit{ 60 };

		};

		struct config_t
		{
			char config_name[64] = "default";
			bool auto_save{ true };
		};

		struct hotkeys_t
		{
			int menu_toggle{ VK_INSERT };
			int aimbot_toggle{ VK_F1 };
			int esp_toggle{ VK_F2 };
			int esp_disable{ VK_F11 };
			int third_person{ VK_F3 };
			int night_mode{ VK_F4 };
			int stream_proof{ VK_F5 };
			int fps_counter{ VK_F6 };
			int ping_display{ VK_F7 };
			int no_flash{ VK_F8 };
			int triggerbot_toggle{ VK_F9 };
			int crosshair_overlay{ VK_F10 };
			bool setting_key{ false };
			int* current_setting{ nullptr };
		};

		aim_t aim;
		visuals_t visuals;
		exploits_t exploits;
		triggerbot_t triggerbot;
		death_match_t death_match;
		misc_t misc;
		themes_t themes;
		extras_t extras;
		config_t config;
		hotkeys_t hotkeys;

		void save_config(const std::string& filename = "default.cfg");
		void load_config(const std::string& filename = "default.cfg");
	};

	class process_info_c
	{
	public:
		std::uint32_t id;
		std::uintptr_t base;
		std::uintptr_t dtb;
		std::uintptr_t client_base;
		std::uintptr_t engine2_base;
	};

	display_c& get_display( ) { return this->display; }
	settings_c& get_settings( ) { return this->settings; }
	process_info_c& get_process_info( ) { return this->process_info; }
	void log_error( const std::string& error_message );
    // Play hitmarker sound if enabled in settings; uses misc.hitmarker_sound_path
    void play_hitmarker_sound();

private:
	display_c display;
	settings_c settings;
	process_info_c process_info;
};

#endif // !CORE_HPP
