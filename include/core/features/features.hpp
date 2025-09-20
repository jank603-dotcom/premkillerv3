#include <deque>
#ifndef FEATURES_HPP
#define FEATURES_HPP

enum PlayerFlags {
	FLAG_FLASHED  = 1 << 0,
	FLAG_SCOPED   = 1 << 1,
	FLAG_DEFUSING = 1 << 2,
	FLAG_HIGH_PING = 1 << 3,    // High ping (>100ms)
	FLAG_LAG_SPIKE = 1 << 4     // Lag spike detected
};

class features_c
{
public:
	ImColor get_theme_color() const;
	ImColor get_fov_color() const;
	class aim_c
	{
	public:
		struct target_t
		{
			math::vector3 position{};
			std::uintptr_t cs_player_pawn{};
		};

		void reset_closest_target( );
		void find_closest_target( const math::vector2& position_projected, const target_t& player );
		std::vector<math::vector3> get_enabled_hitboxes( const sdk_c::bones_c::data_t& bone_data );
		math::vector3 get_closest_hitbox( const std::vector<math::vector3>& hitboxes );
		bool is_outside_fov( const math::vector2& position_projected ) const;
		const target_t* get_closest_target( ) const;
		float& get_fov( );

		void apply_aim_toward( const math::vector3& world_position );

		bool is_aimbotting;

	private:
		float fov;
		float closest_distance;
		bool has_closest_target;
		target_t closest_target;
	};

	class visuals_c
	{
	public:
		class player_c
		{
		public:
			void box( ImDrawList* drawlist, const sdk_c::bounds_c::data_t& bound_data, ImColor color );
			void skeleton( ImDrawList* drawlist, const sdk_c::bones_c::data_t& bone_data, ImColor color );
			void health_bar( ImDrawList* drawlist, const sdk_c::bounds_c::data_t& bound_data, int health );
			void distance( ImDrawList* drawlist, const sdk_c::bounds_c::data_t& bound_data, int distance );
			void name( ImDrawList* drawlist, const sdk_c::bounds_c::data_t& bound_data, const std::string& name );
			void weapon( ImDrawList* drawlist, const sdk_c::bounds_c::data_t& bound_data, const std::string& weapon );
			void flags( ImDrawList* drawlist, const sdk_c::bounds_c::data_t& bound_data, int flag_mask );
			void ping_info( ImDrawList* drawlist, const sdk_c::bounds_c::data_t& bound_data, float ping, float interpolation );
			void network_info( ImDrawList* drawlist, const sdk_c::bounds_c::data_t& bound_data, float ping, float interpolation, int server_tick );

			void line( ImDrawList* drawlist, const math::vector2& target, int distance );
			void dot( ImDrawList* drawlist, const math::vector2& target, int distance );

			// Flag cache for performance optimization
			struct flag_cache_t {
				std::uintptr_t player_pawn = 0;
				int last_flag_mask = 0;
				float last_ping = 0.0f;
				float last_interpolation = 0.0f;
				int last_server_tick = 0;
				std::deque<float> ping_history;              // Store recent ping values for spike detection
				std::chrono::steady_clock::time_point last_update = std::chrono::steady_clock::now();
				static constexpr auto update_interval = std::chrono::milliseconds(50); // Update every 50ms
				static constexpr size_t max_ping_history = 10; // Keep last 10 ping values
			};
			mutable std::unordered_map<std::uintptr_t, flag_cache_t> flag_cache;

			class chamies_c
			{
			public:
				void run( ImDrawList* drawlist, const sdk_c::hitboxes_c::data_t& hitbox_data, const sdk_c::bones_c::data_t& bone_data, ImColor color );

			private:
				void draw_capsule( ImDrawList* drawlist, const math::vector3& start, const math::vector3& end, float radius, const math::quaternion& rotation, const math::vector3& origin, ImColor color, int segments_max );

				void draw_capsule_filled( ImDrawList* drawlist, const math::vector3& top, const math::vector3& bottom, const math::vector3& axis, const math::vector3& u, const math::vector3& v, float radius, ImColor color, const std::vector<float>& sin_cache, const std::vector<float>& cos_cache, int segments, float distance );
				void draw_capsule_outline( ImDrawList* drawlist, const math::vector3& top, const math::vector3& bottom, const math::vector3& axis, const math::vector3& u, const math::vector3& v, float radius, ImColor color, const std::vector<float>& sin_cache, const std::vector<float>& cos_cache, int segments );

				void precompute_sincos( int segments, std::vector<float>& sin_cache, std::vector<float>& cos_cache );
				void create_circle( const math::vector3& center, const math::vector3& u, const math::vector3& v, float radius, const std::vector<float>& sin_cache, const std::vector<float>& cos_cache, std::vector<math::vector3>& out, int segments );
			};

			chamies_c chamies;
		};

		class indicators_c
		{
		public:
			void oof_arrow( ImDrawList* drawlist, const math::vector2& position_projected, ImColor color );
		};

		class world_c
		{
		public:
			void molotov( ImDrawList* drawlist, const math::vector3& center, int distance );
			void molotov_bounds( ImDrawList* drawlist, const std::vector<math::vector3>& fire_points );

			void smoke( ImDrawList* drawlist, const math::vector3& detonation_pos, int distance );
			//void smoke_dome( ImDrawList* drawlist, const std::vector<math::vector3>& voxel_points );

			void bomb( ImDrawList* drawlist, const math::vector3& position, int distance );

			void drop( ImDrawList* drawlist, const math::vector3& position, const std::string& name, int distance, ImColor color );
			void drop_bounds( ImDrawList* drawlist, const math::transform& node_transform, const math::vector3& collision_min, const math::vector3& collision_ma );

		private:
			std::vector<ImVec2> compute_convex_hull( std::vector<ImVec2>& points ) const;
		};

		class hud_c
		{
		public:
			void ring( ImDrawList* drawlist );
		};

		class radar_c
		{
		public:
			void render( ImDrawList* drawlist );
			void draw_player( ImDrawList* drawlist, const math::vector3& world_pos, const math::vector3& local_pos, bool is_enemy, int health, const std::string& name );
			void draw_bomb( ImDrawList* drawlist, const math::vector3& world_pos, const math::vector3& local_pos );
		private:
			ImVec2 world_to_radar( const math::vector3& world_pos, const math::vector3& local_pos, const ImVec2& radar_center, float radar_size, float zoom );
		};

		player_c player;
		indicators_c indicators;
		world_c world;
		hud_c hud;
		radar_c radar;
	};

	class exploits_c
	{
	public:
		class anti_aim_c 
		{
		public:
			struct aa_data_t 
			{
				math::vector3 real_angles;
				math::vector3 fake_angles;
			} data;

			void run( );
		};

		class night_mode_c
		{
		public:
			void run( std::uintptr_t post_processing_volume );

		private:
			bool has_original;
			float original_min;
			float original_max;
		};

		class third_person_c
		{
		public:
			void run( );

		private:
			static constexpr std::uintptr_t jne_patch = 0x7E3697;
			static constexpr std::array<std::uint8_t, 2> original_jne = { 0x75, 0x10 };
			static constexpr std::array<std::uint8_t, 2> patched_jne = { 0xEB, 0x10 };

			bool currently_active;
			bool patch_applied;
			std::uintptr_t cached_player_pawn;
			bool hotkey_pressed;

			void apply_patch( );
			void remove_patch( );
			void set_third_person_state( bool enabled );
			bool needs_reapply( std::uintptr_t current_player_pawn ) const;
			void handle_hotkey( );
		};

		class knife_bot_c
		{
		public:
			void run( );

		private:
			std::chrono::steady_clock::time_point last_equip_time;
			bool knife_equipped = false;
			bool check_enemy_in_range( );
			void equip_knife( );
			void attack( );
		};

		anti_aim_c anti_aim;
		night_mode_c night_mode;
		third_person_c third_person;
		knife_bot_c knife_bot;
	};

	aim_c aim;
	visuals_c visuals;
	exploits_c exploits;

private:
	ImColor theme_colors[5] = {
		ImColor(160, 200, 255, 255), // Dark
		ImColor(51, 153, 255, 255),  // Blue
		ImColor(255, 77, 77, 255),   // Red
		ImColor(77, 255, 77, 255),   // Green
		ImColor(204, 77, 255, 255)   // Purple
	};
};

#endif // !FEATURES_HPP
