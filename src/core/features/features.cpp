#include <include/global.hpp>
#include <random>

ImColor features_c::get_theme_color() const
{
	int theme = g->core.get_settings().themes.current_theme;
	if (theme >= 0 && theme < 5) {
		return theme_colors[theme];
	}
	return ImColor(255, 255, 255, 255);
}

ImColor features_c::get_fov_color() const
{
	// Use theme color instead of custom fov color
	return get_theme_color();
}

void features_c::aim_c::reset_closest_target( )
{
	this->closest_distance = std::numeric_limits<float>::max( );
	this->has_closest_target = false;

	this->is_aimbotting = false;
}

void features_c::aim_c::find_closest_target( const math::vector2& position_projected, const target_t& player )
{
	const auto center = math::vector2{ g->core.get_display( ).width * 0.5f, g->core.get_display( ).height * 0.5f };
	const auto delta = position_projected - center;
	const auto distance_squared = delta.x * delta.x + delta.y * delta.y;

	const auto max_distance_squared = this->fov * this->fov;

	if ( distance_squared < this->closest_distance && distance_squared < max_distance_squared )
	{
		this->closest_distance = distance_squared;
		this->closest_target = player;
		this->has_closest_target = true;
	}
}

bool features_c::aim_c::is_outside_fov( const math::vector2& position_projected ) const
{
	const auto center = math::vector2{ g->core.get_display( ).width * 0.5f, g->core.get_display( ).height * 0.5f };
	const auto delta = position_projected - center;
	const auto distance_squared = delta.x * delta.x + delta.y * delta.y;

	return distance_squared > ( this->fov * this->fov );
}

const features_c::aim_c::target_t* features_c::aim_c::get_closest_target( ) const
{
	return this->has_closest_target ? &this->closest_target : nullptr;
}

std::vector<math::vector3> features_c::aim_c::get_enabled_hitboxes( const sdk_c::bones_c::data_t& bone_data )
{
	using bone = sdk_c::bones_c::bone_id;
	std::vector<math::vector3> hitboxes;
	const auto& settings = g->core.get_settings().aim;

	if ( settings.hitbox_head )
		hitboxes.push_back( bone_data.get_position( bone::head ) );
	if ( settings.hitbox_chest )
		hitboxes.push_back( bone_data.get_position( bone::spine_2 ) );
	if ( settings.hitbox_pelvis )
		hitboxes.push_back( bone_data.get_position( bone::pelvis ) );
	if ( settings.hitbox_arms )
	{
		hitboxes.push_back( bone_data.get_position( bone::left_upper_arm ) );
		hitboxes.push_back( bone_data.get_position( bone::right_upper_arm ) );
	}
	if ( settings.hitbox_legs )
	{
		hitboxes.push_back( bone_data.get_position( bone::left_knee ) );
		hitboxes.push_back( bone_data.get_position( bone::right_knee ) );
	}

	return hitboxes;
}

math::vector3 features_c::aim_c::get_closest_hitbox( const std::vector<math::vector3>& hitboxes )
{
	if ( hitboxes.empty() ) return math::vector3{};

	const auto center = math::vector2{ g->core.get_display().width * 0.5f, g->core.get_display().height * 0.5f };
	float closest_distance = std::numeric_limits<float>::max();
	math::vector3 closest_hitbox = hitboxes[0];

	for ( const auto& hitbox : hitboxes )
	{
		const auto projected = g->sdk.w2s.project( hitbox );
		if ( !g->sdk.w2s.is_valid( projected ) ) continue;

		const auto delta = projected - center;
		const auto distance = delta.x * delta.x + delta.y * delta.y;

		if ( distance < closest_distance )
		{
			closest_distance = distance;
			closest_hitbox = hitbox;
		}
	}

	return closest_hitbox;
}

float& features_c::aim_c::get_fov( )
{
	return this->fov;
}

void features_c::aim_c::apply_aim_toward( const math::vector3& world_position )
{
	this->is_aimbotting = true;

	using namespace std::chrono;

	auto screen_target = g->sdk.w2s.project( world_position );
	auto punch_angle = math::vector3{};
	bool has_punch = false;

	if ( g->core.get_settings( ).aim.rcs )
	{
		const auto local_player = g->udata.get_owning_player( ).cs_player_pawn;
		if ( local_player )
		{
			const auto shots_fired = g->memory.read<int>( local_player + offsets::m_i_shots_fired );
			if ( shots_fired > 1 )
			{
				struct c_utl_vector
				{
					std::uintptr_t count;
					std::uintptr_t data;
				};

				const auto punch_cache = g->memory.read<c_utl_vector>( local_player + offsets::m_aim_punch_cache );
				if ( punch_cache.count > 0 && punch_cache.count < 0xFFFF )
				{
					punch_angle = g->memory.read<math::vector3>( punch_cache.data + ( punch_cache.count - 1 ) * sizeof( math::vector3 ) );
					has_punch = true;

					const auto punch_scale = 22.0f * g->core.get_settings().aim.rcs_strength;
					const auto screen_x = std::clamp( punch_angle.y * punch_scale, -100.f, 100.f );
					const auto screen_y = std::clamp( punch_angle.x * punch_scale, -100.f, 100.f );

					screen_target.x += screen_x;
					screen_target.y -= screen_y;
				}
			}
		}
	}

	const auto delta = world_position - g->sdk.camera.get_data( ).position;
	const auto hyp = std::sqrt( delta.x * delta.x + delta.y * delta.y );

	math::vector3 final_angles{};
	final_angles.x = -std::atan2( delta.z, hyp ) * 180.0f / std::numbers::pi;
	final_angles.y = std::atan2( delta.y, delta.x ) * 180.0f / std::numbers::pi;
	final_angles.z = 0.0f;

	if ( has_punch )
	{
		const auto punch_scale = 2.0f * g->core.get_settings().aim.rcs_strength;
		final_angles.x -= punch_angle.x * punch_scale;
		final_angles.y -= punch_angle.y * punch_scale;
	}

	if ( g->core.get_settings( ).aim.type == 0 )
	{
		static auto last_injection_time = high_resolution_clock::now( );
		static auto accumulated_mouse_x_error = 0.0f;
		static auto accumulated_mouse_y_error = 0.0f;
        // Humanize jitter state
        static auto last_jitter_time = high_resolution_clock::now();
        static float jitter_dx = 0.0f;
        static float jitter_dy = 0.0f;
        static std::mt19937 rng{ std::random_device{}() };

		const auto delta = screen_target - math::vector2{ g->core.get_display( ).width * 0.5f, g->core.get_display( ).height * 0.5f };
		
		float move_x = delta.x / std::max(1.0f, g->core.get_settings( ).aim.smooth);
		float move_y = delta.y / std::max(1.0f, g->core.get_settings( ).aim.smooth);

        // No FOV-based sensitivity scaling by request

        // Suppress mouse movement briefly after FOV changes to avoid sudden jumps (if enabled)
        if ( g->core.get_settings().visuals.fov_changer.pause_aim_during_change )
        {
            using clock = std::chrono::steady_clock;
            static int s_last_seen_fov = -1;
            static clock::time_point s_last_fov_change = clock::now();
            constexpr int k_fov_change_cooldown_ms = 300;
            try {
                const auto lp = g->udata.get_owning_player().cs_player_pawn;
                if (lp && offsets::m_iFOV) {
                    int cur = -1;
                    try { cur = g->memory.read<int>(lp + offsets::m_iFOV); } catch (...) { cur = -1; }
                    if (cur > 0) {
                        if (s_last_seen_fov == -1) s_last_seen_fov = cur;
                        if (cur != s_last_seen_fov) {
                            s_last_seen_fov = cur;
                            s_last_fov_change = clock::now();
                        }
                    }
                }
            } catch (...) { /* ignore */ }

            const auto since_ms = std::chrono::duration_cast<std::chrono::milliseconds>(clock::now() - s_last_fov_change).count();
            if (since_ms >= 0 && since_ms < k_fov_change_cooldown_ms) {
                // Skip movement this tick while FOV settles
                move_x = 0.0f;
                move_y = 0.0f;
            }
        }

        // Apply humanize jitter if enabled
        if ( g->core.get_settings().aim.humanize )
        {
            const float rate_hz = std::clamp(g->core.get_settings().aim.jitter_rate_hz, 1.0f, 60.0f);
            const auto now = high_resolution_clock::now();
            const auto jitter_interval_ms = static_cast<long long>(1000.0f / rate_hz);
            const auto since = duration_cast<milliseconds>(now - last_jitter_time).count();
            if ( since >= jitter_interval_ms )
            {
                const float amt = std::clamp(g->core.get_settings().aim.jitter_amount_px, 0.0f, 3.0f);
                std::uniform_real_distribution<float> dist(-amt, amt);
                jitter_dx = dist(rng);
                jitter_dy = dist(rng);
                last_jitter_time = now;
            }

            // Add jitter to the intended movement (keep small)
            move_x += jitter_dx;
            move_y += jitter_dy;

            // Avoid excessive micro-oscillation when near target
            if ( std::abs(delta.x) < 2.0f ) move_x = std::clamp(move_x, -1.0f, 1.0f);
            if ( std::abs(delta.y) < 2.0f ) move_y = std::clamp(move_y, -1.0f, 1.0f);
        }

        // Final safety clamp on per-tick movement (prevents extreme jumps under unusual FOVs)
        {
            const float base_cap = 6.0f; // pixels per injection tick
            const float cap = base_cap;  // could scale with fov_correction if desired
            move_x = std::clamp(move_x, -cap, cap);
            move_y = std::clamp(move_y, -cap, cap);
        }

		accumulated_mouse_x_error += move_x;
		accumulated_mouse_y_error += move_y;

		const auto mouse_dx = static_cast< int >( accumulated_mouse_x_error );
		const auto mouse_dy = static_cast< int >( accumulated_mouse_y_error );

		accumulated_mouse_x_error -= mouse_dx;
		accumulated_mouse_y_error -= mouse_dy;

		const auto now = high_resolution_clock::now( );
		const auto elapsed = duration_cast< milliseconds >( now - last_injection_time ).count( );

		if ( ( mouse_dx != 0 || mouse_dy != 0 ) && elapsed >= 5 )
		{
			g->memory.inject_mouse( mouse_dx, mouse_dy, 0 );
			last_injection_time = now;
		}
	}
	else if ( g->core.get_settings( ).aim.type == 1 )
	{
		static auto current_angles = math::vector3{};
		static auto last_update_time = high_resolution_clock::now( );
        // Humanize jitter state (angles)
        static auto last_jitter_time_ang = high_resolution_clock::now();
        static float jitter_pitch_deg = 0.0f;
        static float jitter_yaw_deg = 0.0f;
        static std::mt19937 rng_ang{ std::random_device{}() };

		current_angles = g->memory.read<math::vector3>( g->core.get_process_info( ).client_base + offsets::dw_csgo_input + 0x7b0 ); // CCSGOInput::ViewAngles

		auto angle_delta = final_angles - current_angles;

		while ( angle_delta.y > 180.0f ) angle_delta.y -= 360.0f;
		while ( angle_delta.y < -180.0f ) angle_delta.y += 360.0f;

        const auto smooth_factor = std::max(1.0f, g->core.get_settings( ).aim.smooth * 15);
        math::vector3 smoothed_angles = current_angles + ( angle_delta / smooth_factor );

		while ( smoothed_angles.y > 180.0f ) smoothed_angles.y -= 360.0f;
		while ( smoothed_angles.y < -180.0f ) smoothed_angles.y += 360.0f;

        // Apply humanize jitter (small angle offsets)
        if ( g->core.get_settings().aim.humanize )
        {
            const float rate_hz = std::clamp(g->core.get_settings().aim.jitter_rate_hz, 1.0f, 60.0f);
            const auto now = high_resolution_clock::now();
            const auto jitter_interval_ms = static_cast<long long>(1000.0f / rate_hz);
            const auto since = duration_cast<milliseconds>(now - last_jitter_time_ang).count();
            if ( since >= jitter_interval_ms )
            {
                // Map pixel jitter to a tiny degree offset (heuristic scale)
                const float px_amt = std::clamp(g->core.get_settings().aim.jitter_amount_px, 0.0f, 3.0f);
                const float deg_amt_yaw = px_amt * 0.05f;   // yaw can tolerate slightly larger
                const float deg_amt_pitch = px_amt * 0.03f; // keep pitch subtler
                std::uniform_real_distribution<float> dist_yaw(-deg_amt_yaw, deg_amt_yaw);
                std::uniform_real_distribution<float> dist_pitch(-deg_amt_pitch, deg_amt_pitch);
                jitter_yaw_deg = dist_yaw(rng_ang);
                jitter_pitch_deg = dist_pitch(rng_ang);
                last_jitter_time_ang = now;
            }

            smoothed_angles.x += jitter_pitch_deg;
            smoothed_angles.y += jitter_yaw_deg;
        }

		smoothed_angles.x = std::clamp( smoothed_angles.x, -89.0f, 89.0f );
		smoothed_angles.z = 0.0f;

		g->memory.write<math::vector3>( g->core.get_process_info( ).client_base + offsets::dw_csgo_input + 0x7b0, smoothed_angles ); // CCSGOInput::ViewAngles

		current_angles = smoothed_angles;
	}
	else if ( g->core.get_settings( ).aim.type == 2 )
	{
        // Humanize jitter state (silent)
        static auto last_jitter_time_silent = high_resolution_clock::now();
        static float jitter_pitch_deg_s = 0.0f;
        static float jitter_yaw_deg_s = 0.0f;
        static std::mt19937 rng_silent{ std::random_device{}() };

        auto write_angles = final_angles;
        if ( g->core.get_settings().aim.humanize )
        {
            const float rate_hz = std::clamp(g->core.get_settings().aim.jitter_rate_hz, 1.0f, 60.0f);
            const auto now = high_resolution_clock::now();
            const auto jitter_interval_ms = static_cast<long long>(1000.0f / rate_hz);
            const auto since = duration_cast<milliseconds>(now - last_jitter_time_silent).count();
            if ( since >= jitter_interval_ms )
            {
                const float px_amt = std::clamp(g->core.get_settings().aim.jitter_amount_px, 0.0f, 3.0f);
                const float deg_amt_yaw = px_amt * 0.05f;
                const float deg_amt_pitch = px_amt * 0.03f;
                std::uniform_real_distribution<float> dist_yaw(-deg_amt_yaw, deg_amt_yaw);
                std::uniform_real_distribution<float> dist_pitch(-deg_amt_pitch, deg_amt_pitch);
                jitter_yaw_deg_s = dist_yaw(rng_silent);
                jitter_pitch_deg_s = dist_pitch(rng_silent);
                last_jitter_time_silent = now;
            }

            write_angles.x += jitter_pitch_deg_s;
            write_angles.y += jitter_yaw_deg_s;
        }

        // Normalize and clamp
        while ( write_angles.y > 180.0f ) write_angles.y -= 360.0f;
        while ( write_angles.y < -180.0f ) write_angles.y += 360.0f;
        write_angles.x = std::clamp( write_angles.x, -89.0f, 89.0f );
        write_angles.z = 0.0f;

        // True silent aim via camera writer dislocation
        const auto client_base = g->core.get_process_info( ).client_base;
        const auto pitch_addr = client_base + offsets::cam_write_pitch;
        const auto yaw_addr   = client_base + offsets::cam_write_yaw;
        const auto len_pitch  = std::min<std::size_t>( std::max<std::size_t>( offsets::cam_write_pitch_len, 1 ), 32 );
        const auto len_yaw    = std::min<std::size_t>( std::max<std::size_t>( offsets::cam_write_yaw_len,   1 ), 32 );

        bool patched = false;
        std::vector<std::uint8_t> orig_pitch, orig_yaw;
        if ( pitch_addr && yaw_addr && len_pitch > 0 && len_yaw > 0 )
        {
            orig_pitch.resize( len_pitch );
            orig_yaw.resize( len_yaw );
            std::vector<std::uint8_t> nop_pitch( len_pitch, 0x90 );
            std::vector<std::uint8_t> nop_yaw( len_yaw, 0x90 );

            if ( g->memory.read_process_memory( pitch_addr, orig_pitch.data( ), orig_pitch.size( ) ) &&
                 g->memory.read_process_memory( yaw_addr,   orig_yaw.data( ),   orig_yaw.size( ) ) )
            {
                g->memory.write_process_memory( pitch_addr, nop_pitch.data( ), nop_pitch.size( ) );
                g->memory.write_process_memory( yaw_addr,   nop_yaw.data( ),   nop_yaw.size( ) );
                patched = true;
            }
        }

        // Apply aim silently while camera writes are NOP'd
        g->memory.write<math::vector3>( g->udata.get_owning_player( ).cs_player_pawn + offsets::v_angle, write_angles );

        // Restore original camera writer instructions
        if ( patched )
        {
            g->memory.write_process_memory( pitch_addr, orig_pitch.data( ), orig_pitch.size( ) );
            g->memory.write_process_memory( yaw_addr,   orig_yaw.data( ),   orig_yaw.size( ) );
        }
    }

    // Auto Fire is now handled in dispatch.cpp via crosshair-over-enemy scan
}

void features_c::visuals_c::player_c::box( ImDrawList* drawlist, const sdk_c::bounds_c::data_t& b, ImColor color )
{
	const auto x = std::round( b.min_x );
	const auto y = std::round( b.min_y );
	const auto w = std::round( b.max_x - b.min_x );
	const auto h = std::round( b.max_y - b.min_y );
	const auto base_color = color.Value;

	const int box_style = g->core.get_settings().visuals.player.layout.box_style; // 0=full,1=corner
	if ( box_style == 0 )
	{
		const auto top_left = ImVec2( x + 1, y + 1 );
		const auto bottom_right = ImVec2( x + w - 1, y + h - 1 );

		const auto top = ImColor( base_color.x * 0.8f, base_color.y * 0.8f, base_color.z * 0.8f, 0.15f );
		const auto bottom = ImColor( base_color.x, base_color.y, base_color.z, 0.4f );
		drawlist->AddRectFilledMultiColor( top_left, bottom_right, top, top, bottom, bottom );

		drawlist->AddRect( ImVec2( x + 1, y + 1 ), ImVec2( x + w - 1, y + h - 1 ), ImColor( base_color.x * 1.3f, base_color.y * 1.3f, base_color.z * 1.3f, 0.08f ), 0.0f, 0, 1.0f );

		drawlist->AddRect( ImVec2( x, y ), ImVec2( x + w, y + h ), ImColor( 0, 0, 0, 240 ), 0.0f, 0, 2.0f );
		drawlist->AddRect( ImVec2( x, y ), ImVec2( x + w, y + h ), g->features.get_theme_color(), 0.0f, 0, 1.0f );
	}
	else
	{
		const float len = std::max( 2.0f, w * 0.25f );
		const ImU32 col_outer = ImColor( 0, 0, 0, 240 );
		const ImU32 col_inner = g->features.get_theme_color();
		// TL
		drawlist->AddLine( ImVec2( x, y ), ImVec2( x + len, y ), col_outer, 2.0f );
		drawlist->AddLine( ImVec2( x, y ), ImVec2( x, y + len ), col_outer, 2.0f );
		drawlist->AddLine( ImVec2( x, y ), ImVec2( x + len, y ), col_inner, 1.0f );
		drawlist->AddLine( ImVec2( x, y ), ImVec2( x, y + len ), col_inner, 1.0f );
		// TR
		drawlist->AddLine( ImVec2( x + w, y ), ImVec2( x + w - len, y ), col_outer, 2.0f );
		drawlist->AddLine( ImVec2( x + w, y ), ImVec2( x + w, y + len ), col_outer, 2.0f );
		drawlist->AddLine( ImVec2( x + w, y ), ImVec2( x + w - len, y ), col_inner, 1.0f );
		drawlist->AddLine( ImVec2( x + w, y ), ImVec2( x + w, y + len ), col_inner, 1.0f );
		// BL
		drawlist->AddLine( ImVec2( x, y + h ), ImVec2( x + len, y + h ), col_outer, 2.0f );
		drawlist->AddLine( ImVec2( x, y + h ), ImVec2( x, y + h - len ), col_outer, 2.0f );
		drawlist->AddLine( ImVec2( x, y + h ), ImVec2( x + len, y + h ), col_inner, 1.0f );
		drawlist->AddLine( ImVec2( x, y + h ), ImVec2( x, y + h - len ), col_inner, 1.0f );
		// BR
		drawlist->AddLine( ImVec2( x + w, y + h ), ImVec2( x + w - len, y + h ), col_outer, 2.0f );
		drawlist->AddLine( ImVec2( x + w, y + h ), ImVec2( x + w, y + h - len ), col_outer, 2.0f );
		drawlist->AddLine( ImVec2( x + w, y + h ), ImVec2( x + w - len, y + h ), col_inner, 1.0f );
		drawlist->AddLine( ImVec2( x + w, y + h ), ImVec2( x + w, y + h - len ), col_inner, 1.0f );
	}
}

void features_c::visuals_c::player_c::skeleton( ImDrawList* drawlist, const sdk_c::bones_c::data_t& bone_data, ImColor color )
{
	using bone = sdk_c::bones_c::bone_id;

	const std::vector<std::pair<bone, bone>> boner_chains = {
		{ bone::head, bone::neck },
		{ bone::neck, bone::spine_3 },
		{ bone::spine_3, bone::spine_2 },
		{ bone::spine_2, bone::spine_1 },
		{ bone::spine_1, bone::spine_0 },
		{ bone::spine_0, bone::pelvis },

		{ bone::neck, bone::left_shoulder },
		{ bone::left_shoulder, bone::left_upper_arm },
		{ bone::left_upper_arm, bone::left_hand },

		{ bone::neck, bone::right_shoulder },
		{ bone::right_shoulder, bone::right_upper_arm },
		{ bone::right_upper_arm, bone::right_hand },

		{ bone::pelvis, bone::left_hip },
		{ bone::left_hip, bone::left_knee },
		{ bone::left_knee, bone::left_foot },

		{ bone::pelvis, bone::right_hip },
		{ bone::right_hip, bone::right_knee },
		{ bone::right_knee, bone::right_foot }
	};

	for ( const auto& [from, to] : boner_chains )
	{
		const auto from_projected = g->sdk.w2s.project( bone_data.get_position( from ) );
		const auto to_projected = g->sdk.w2s.project( bone_data.get_position( to ) );

		drawlist->AddLine( from_projected.vec( ), to_projected.vec( ), ImColor( color.Value.x, color.Value.y, color.Value.z, 0.3f ), 2.0f );
		drawlist->AddLine( from_projected.vec( ), to_projected.vec( ), g->features.get_theme_color(), 1.0f );
	}
}

void features_c::visuals_c::player_c::health_bar( ImDrawList* drawlist, const sdk_c::bounds_c::data_t& b, int health )
{
	const auto& lay = g->core.get_settings().visuals.player.layout;
	const int side = lay.health_side; // 0 L,1 R,2 T,3 B
	const float bar_w = std::max(1.0f, lay.health_bar_width);
	const float pad = 2.0f;

	const auto x0 = std::round( b.min_x );
	const auto y0 = std::round( b.min_y );
	const auto x1 = std::round( b.max_x );
	const auto y1 = std::round( b.max_y );
	const auto h = std::round( y1 - y0 );
	const auto w = std::round( x1 - x0 );

	const auto chealth = std::clamp( health, 0, 100 );
	const auto fraction = chealth / 100.0f;

	ImColor color;
	if ( chealth > 75 )
	{
		color = ImColor( 0, 255, 0, 230 );
	}
	else if ( chealth > 25 )
	{
		color = ImColor( 255, 255, 0, 230 );
	}
	else
	{
		color = ImColor( 255, 0, 0, 230 );
	}

	if ( side == 0 )
	{
		const float rx0 = x0 - bar_w - 3.0f;
		const float ry0 = y0 + pad;
		const float rx1 = x0 - 3.0f;
		const float ry1 = y1 - pad;
		const float filled = (ry1 - ry0) * fraction;
		drawlist->AddRect( ImVec2( rx0, ry0 ), ImVec2( rx1, ry1 ), IM_COL32(0,0,0,120) );
		drawlist->AddRectFilled( ImVec2( rx0+1, ry1 - filled ), ImVec2( rx1-1, ry1-1 ), color );
	}
	else if ( side == 1 )
	{
		const float rx0 = x1 + 3.0f;
		const float ry0 = y0 + pad;
		const float rx1 = x1 + 3.0f + bar_w;
		const float ry1 = y1 - pad;
		const float filled = (ry1 - ry0) * fraction;
		drawlist->AddRect( ImVec2( rx0, ry0 ), ImVec2( rx1, ry1 ), IM_COL32(0,0,0,120) );
		drawlist->AddRectFilled( ImVec2( rx0+1, ry1 - filled ), ImVec2( rx1-1, ry1-1 ), color );
	}
	else if ( side == 2 )
	{
		const float hb_h = std::max(2.0f, lay.health_bar_height);
		const float rx0 = x0;
		const float ry0 = y0 - hb_h - 3.0f;
		const float rx1 = x1;
		const float ry1 = y0 - 3.0f;
		const float filledw = (rx1 - rx0) * fraction;
		drawlist->AddRect( ImVec2( rx0, ry0 ), ImVec2( rx1, ry1 ), IM_COL32(0,0,0,120) );
		drawlist->AddRectFilled( ImVec2( rx0+1, ry0+1 ), ImVec2( rx0 + filledw, ry1-1 ), color );
	}
	else
	{
		const float hb_h = std::max(2.0f, lay.health_bar_height);
		const float rx0 = x0;
		const float ry0 = y1 + 3.0f;
		const float rx1 = x1;
		const float ry1 = y1 + 3.0f + hb_h;
		const float filledw = (rx1 - rx0) * fraction;
		drawlist->AddRect( ImVec2( rx0, ry0 ), ImVec2( rx1, ry1 ), IM_COL32(0,0,0,120) );
		drawlist->AddRectFilled( ImVec2( rx0+1, ry0+1 ), ImVec2( rx0 + filledw, ry1-1 ), color );
	}
}

void features_c::visuals_c::player_c::distance( ImDrawList* drawlist, const sdk_c::bounds_c::data_t& b, int distance )
{
    const auto text = std::to_string( distance ) + "m";
    const auto& lay = g->core.get_settings().visuals.player.layout;
    ImFont* font = ImGui::GetFont();
    const float fsz = (lay.font_size > 0.0f) ? lay.font_size : ImGui::GetFontSize();
    ImVec2 text_size = font->CalcTextSizeA( fsz, FLT_MAX, 0.0f, text.c_str() );

	float base_x = 0.0f;
	if ( lay.text_align == 0 ) base_x = std::round( ( b.min_x + b.max_x ) * 0.5f - text_size.x * 0.5f );
	else if ( lay.text_align == 1 ) base_x = std::round( b.min_x );
	else base_x = std::round( b.max_x - text_size.x );
	float x = base_x + lay.distance_offset[0];
	float y = std::round( b.max_y + 2.0f ) + lay.distance_offset[1];

	ImColor color;
	if ( distance < 5 )
	{
		color = ImColor( 255, 100, 100, 255 );
	}
	else if ( distance < 10 )
	{
		color = ImColor( 255, 140, 80, 255 );
	}
	else if ( distance < 25 )
	{
		color = ImColor( 255, 200, 60, 255 );
	}
	else if ( distance < 50 )
	{
		color = ImColor( 255, 255, 255, 255 );
	}
	else if ( distance < 100 )
	{
		color = ImColor( 200, 200, 200, 220 );
	}
	else
	{
		color = ImColor( 150, 150, 150, 180 );
	}

	drawlist->AddText( font, fsz, ImVec2( x, y ), color, text.c_str( ) );
}

void features_c::visuals_c::player_c::name( ImDrawList* drawlist, const sdk_c::bounds_c::data_t& bound_data, const std::string& name )
{
    const auto& lay = g->core.get_settings().visuals.player.layout;
    ImFont* font = ImGui::GetFont();
    const float fsz = (lay.font_size > 0.0f) ? lay.font_size : ImGui::GetFontSize();
    ImVec2 sz = font->CalcTextSizeA( fsz, FLT_MAX, 0.0f, name.c_str() );

	float base_x = 0.0f;
	if ( lay.text_align == 0 ) base_x = std::round( ( bound_data.min_x + bound_data.max_x ) * 0.5f - sz.x * 0.5f );
	else if ( lay.text_align == 1 ) base_x = std::round( bound_data.min_x );
	else base_x = std::round( bound_data.max_x - sz.x );

	float x = base_x + lay.name_offset[0];
	float y = bound_data.min_y - sz.y - 2.0f + lay.name_offset[1];

	drawlist->AddText( font, fsz, ImVec2( x, y ), ImColor( 240, 240, 240, 255 ), name.c_str() );
}

void features_c::visuals_c::player_c::weapon( ImDrawList* drawlist, const sdk_c::bounds_c::data_t& bound_data, const std::string& weapon )
{
    const auto& lay = g->core.get_settings().visuals.player.layout;
    ImFont* font = ImGui::GetFont();
    const float fsz = (lay.font_size > 0.0f) ? lay.font_size : ImGui::GetFontSize();
    ImVec2 sz = font->CalcTextSizeA( fsz, FLT_MAX, 0.0f, weapon.c_str() );

	float base_x = 0.0f;
	if ( lay.text_align == 0 ) base_x = std::round( ( bound_data.min_x + bound_data.max_x ) * 0.5f - sz.x * 0.5f );
	else if ( lay.text_align == 1 ) base_x = std::round( bound_data.min_x );
	else base_x = std::round( bound_data.max_x - sz.x );

	float x = base_x + lay.weapon_offset[0];
	float y = bound_data.max_y + 2.0f + lay.weapon_offset[1];
	// If distance is enabled and rendered below, push weapon further down similar to preview stacking
	if ( g->core.get_settings().visuals.player.distance )
	{
		y += sz.y + 2.0f;
	}

	drawlist->AddText( font, fsz, ImVec2( x, y ), ImColor( 200, 200, 200, 255 ), weapon.c_str() );
}

void features_c::visuals_c::player_c::flags( ImDrawList* drawlist, const sdk_c::bounds_c::data_t& bound_data, int flag_mask )
{
    if ( !flag_mask ) return;

    // Lookup table for flag strings (bit 0..4)
    struct flag_info_t {
        const char* text;
    };

    static const std::array<flag_info_t, 5> flag_lookup = {{
        { "FLASHED" },
        { "SCOPED" },
        { "DEFUSING" },
        { "HIGH PING" },
        { "LAG SPIKE" }
    }};

    // Intersect runtime mask with user-selected mask
    const int user_mask = g->core.get_settings().visuals.player.flags_mask;
    const int visible_mask = flag_mask & user_mask;
    if ( !visible_mask ) return;

    // Resolve text color from settings
    const auto& flag_col = g->core.get_settings().visuals.player.flag_color;
    const ImU32 color_u32 = IM_COL32( (int)(flag_col[0] * 255), (int)(flag_col[1] * 255), (int)(flag_col[2] * 255), 255 );

    // Determine text size: layout override > flag-specific size > default
    const auto& lay = g->core.get_settings().visuals.player.layout;
    const float configured_size = g->core.get_settings().visuals.player.flag_text_size;
    const float text_size = (lay.font_size > 0.0f)
        ? lay.font_size
        : ((configured_size > 0.0f) ? configured_size : ImGui::GetFontSize());

    const auto box_right = bound_data.max_x + 5.0f;
    auto flag_y = bound_data.min_y;

    // Draw matching flags
    for ( int i = 0; i < 5; ++i )
    {
        const int bit = 1 << i;
        if ( (visible_mask & bit) == 0 )
            continue;

        drawlist->AddText( ImGui::GetFont(), text_size, ImVec2( box_right, flag_y ), color_u32, flag_lookup[i].text );
        flag_y += text_size + 2.0f;
    }
}

void features_c::visuals_c::player_c::line( ImDrawList* drawlist, const math::vector2& target, int distance )
{
	const auto start = ImVec2( g->core.get_display( ).width * 0.5f, g->core.get_display( ).height * 0.5f );
	const auto end = ImVec2( target.x, target.y );

	ImColor color;
	if ( distance < 5 )
	{
		color = ImColor( 255, 100, 100, 255 );
	}
	else if ( distance < 10 )
	{
		color = ImColor( 255, 140, 80, 255 );
	}
	else if ( distance < 25 )
	{
		color = ImColor( 255, 200, 60, 255 );
	}
	else if ( distance < 50 )
	{
		color = ImColor( 255, 255, 255, 255 );
	}
	else if ( distance < 100 )
	{
		color = ImColor( 200, 200, 200, 220 );
	}
	else
	{
		color = ImColor( 150, 150, 150, 180 );
	}

	const auto thickness = distance < 25 ? 1.5f : 1.0f;
	drawlist->AddLine( start, end, g->features.get_theme_color(), thickness );
}

void features_c::visuals_c::player_c::dot( ImDrawList* drawlist, const math::vector2& target, int distance )
{
	auto radius = 28.0f / std::powf( distance + 1, 0.75f );
	radius = std::clamp( radius, 2.0f, 8.0f );

	ImColor color;
	if ( distance < 5 )
	{
		color = ImColor( 255, 100, 100, 255 );
	}
	else if ( distance < 10 )
	{
		color = ImColor( 255, 140, 80, 255 );
	}
	else if ( distance < 25 )
	{
		color = ImColor( 255, 200, 60, 255 );
	}
	else if ( distance < 50 )
	{
		color = ImColor( 255, 255, 255, 255 );
	}
	else if ( distance < 100 )
	{
		color = ImColor( 200, 200, 200, 220 );
	}
	else
	{
		color = ImColor( 150, 150, 150, 180 );
	}

	drawlist->AddCircleFilled( target.vec( ), radius, g->features.get_theme_color(), 32 );
	drawlist->AddCircle( target.vec( ), radius, ImColor( 0, 0, 0, 200 ), 32, 1.5f );
}

void features_c::visuals_c::player_c::chamies_c::run( ImDrawList* drawlist, const sdk_c::hitboxes_c::data_t& hitbox_data, const sdk_c::bones_c::data_t& bone_data, ImColor color )
{
	for ( const auto& hbox : hitbox_data )
	{
		if ( hbox.bone_index == -1 )
		{
			continue;
		}

		const auto bone_id = static_cast< sdk_c::bones_c::bone_id >( hbox.bone_index );
		const auto position = bone_data.get_position( bone_id );
		const auto rotation = bone_data.get_rotation( bone_id );

		this->draw_capsule( drawlist, hbox.bb_max, hbox.bb_min, hbox.radius, rotation, position, g->features.get_theme_color(), 24 );
	}
}

void features_c::visuals_c::player_c::chamies_c::draw_capsule( ImDrawList* drawlist, const math::vector3& start, const math::vector3& end, float radius, const math::quaternion& rotation, const math::vector3& origin, ImColor color, int segments_max )
{
	const auto top = rotation.rotate_vector( start ) + origin;
	const auto bottom = rotation.rotate_vector( end ) + origin;

	const auto axis = ( bottom - top ).normalized( );
	const auto arbitrary = std::abs( axis.x ) < 0.99f ? math::vector3( 1, 0, 0 ) : math::vector3( 0, 1, 0 );
	const auto u = axis.cross( arbitrary ).normalized( );
	const auto v = axis.cross( u );

	static auto last_camera_pos = math::vector3{};
	static auto cached_distance = 0.0f;
	const auto capsule_mid_point = ( top + bottom ) * 0.5f;
	const auto current_camera_pos = g->sdk.camera.get_data( ).position;
	
	if ( current_camera_pos.distance( last_camera_pos ) > 10.0f ) {
		cached_distance = capsule_mid_point.distance( current_camera_pos ) / 52.5f;
		last_camera_pos = current_camera_pos;
	}
	const auto distance_meters = cached_distance;

	const auto start_reduction_distance = 15.0f;
	const auto end_reduction_distance = 70.0f;

	int min_segments = 4;
	int current_segments;

	if ( distance_meters <= start_reduction_distance )
	{
		current_segments = segments_max;
	}
	else if ( distance_meters >= end_reduction_distance )
	{
		current_segments = min_segments;
	}
	else
	{
		auto normalized_distance = ( distance_meters - start_reduction_distance ) / ( end_reduction_distance - start_reduction_distance );
		normalized_distance = std::clamp( normalized_distance, 0.0f, 1.0f );
		current_segments = static_cast< int >( std::lerp( static_cast< float >( segments_max ), static_cast< float >( min_segments ), normalized_distance ) );
		current_segments = std::max( current_segments, min_segments );
	}

	std::vector<float> sin_cache, cos_cache;
	this->precompute_sincos( current_segments, sin_cache, cos_cache );

	if ( distance_meters > 20.0f )
	{
		this->draw_capsule_outline( drawlist, top, bottom, axis, u, v, radius, color, sin_cache, cos_cache, current_segments );
	}
	else
	{
		this->draw_capsule_filled( drawlist, top, bottom, axis, u, v, radius, color, sin_cache, cos_cache, current_segments, distance_meters );
	}
}

void features_c::visuals_c::player_c::chamies_c::draw_capsule_filled( ImDrawList* drawlist, const math::vector3& top, const math::vector3& bottom, const math::vector3& axis, const math::vector3& u, const math::vector3& v, float radius, ImColor color, const std::vector<float>& sin_cache, const std::vector<float>& cos_cache, int segments, float distance )
{
	const auto alpha_multiplier = std::clamp( 1.2f - ( distance / 25.0f ), 0.4f, 1.0f );
	const auto edge_alpha = std::min( alpha_multiplier * 0.8f, 0.9f );

	auto center_color = ImColor( std::min( 255, static_cast< int >( color.Value.x * 255 * 1.15f ) ), std::min( 255, static_cast< int >( color.Value.y * 255 * 1.15f ) ), std::min( 255, static_cast< int >( color.Value.z * 255 * 1.15f ) ), static_cast< int >( color.Value.w * 255 * alpha_multiplier ) );
	auto edge_color = ImColor( static_cast< int >( color.Value.x * 255 * 0.7f ), static_cast< int >( color.Value.y * 255 * 0.7f ), static_cast< int >( color.Value.z * 255 * 0.7f ), static_cast< int >( color.Value.w * 255 * edge_alpha ) );

	const auto hemisphere_segments = std::max( 4, segments / 2 );
	std::vector<std::vector<math::vector3>> top_hemisphere_rings, bottom_hemisphere_rings;

	for ( int ring = 0; ring <= hemisphere_segments; ++ring )
	{
		const auto phi = ( std::numbers::pi_v<float> / 2.0f ) * ( static_cast< float >( ring ) / hemisphere_segments );
		const auto ring_radius = radius * std::cos( phi );
		const auto ring_height = radius * std::sin( phi );

		std::vector<math::vector3> ring_points;
		const auto ring_center = top - axis * ring_height;

		this->create_circle( ring_center, u, v, ring_radius, sin_cache, cos_cache, ring_points, segments );
		top_hemisphere_rings.push_back( ring_points );
	}

	for ( int ring = 0; ring <= hemisphere_segments; ++ring )
	{
		const auto phi = ( std::numbers::pi_v<float> / 2.0f ) * ( static_cast< float >( ring ) / hemisphere_segments );
		const auto ring_radius = radius * std::cos( phi );
		const auto ring_height = radius * std::sin( phi );

		std::vector<math::vector3> ring_points;
		const auto ring_center = bottom + axis * ring_height;

		this->create_circle( ring_center, u, v, ring_radius, sin_cache, cos_cache, ring_points, segments );
		bottom_hemisphere_rings.push_back( ring_points );
	}

	std::vector<math::vector3> top_circle, bottom_circle;
	this->create_circle( top, u, v, radius, sin_cache, cos_cache, top_circle, segments );
	this->create_circle( bottom, u, v, radius, sin_cache, cos_cache, bottom_circle, segments );

	std::vector<std::vector<ImVec2>> wtop_hemisphere_rings, wbottom_hemisphere_rings;
	std::vector<ImVec2> wtop_circle( segments + 1 ), wbottom_circle( segments + 1 );

	for ( const auto& ring : top_hemisphere_rings )
	{
		std::vector<ImVec2> projected_ring( segments + 1 );
		for ( int i = 0; i <= segments; ++i )
		{
			projected_ring[ i ] = g->sdk.w2s.project( ring[ i ] ).vec( );
		}

		wtop_hemisphere_rings.push_back( projected_ring );
	}

	for ( const auto& ring : bottom_hemisphere_rings )
	{
		std::vector<ImVec2> projected_ring( segments + 1 );
		for ( int i = 0; i <= segments; ++i )
		{
			projected_ring[ i ] = g->sdk.w2s.project( ring[ i ] ).vec( );
		}

		wbottom_hemisphere_rings.push_back( projected_ring );
	}

	for ( int i = 0; i <= segments; ++i )
	{
		wtop_circle[ i ] = g->sdk.w2s.project( top_circle[ i ] ).vec( );
		wbottom_circle[ i ] = g->sdk.w2s.project( bottom_circle[ i ] ).vec( );
	}

	for ( int i = 0; i < segments; ++i )
	{
		const auto next_i = i + 1;

		ImVec2 body_points[ ] = { wtop_circle[ i ], wtop_circle[ next_i ], wbottom_circle[ next_i ], wbottom_circle[ i ] };

		const auto current_color = ( i % 3 == 0 ) ? center_color : color;
		drawlist->AddConvexPolyFilled( body_points, 4, current_color );
	}

	for ( int ring = 0; ring < static_cast< int >( wtop_hemisphere_rings.size( ) ) - 1; ++ring )
	{
		for ( int i = 0; i < segments; ++i )
		{
			const auto next_i = i + 1;

			const auto ring_fact = static_cast< float >( ring ) / ( wtop_hemisphere_rings.size( ) - 1 );
			const auto ring_color = ImColor( static_cast< int >( color.Value.x * 255 * ( 1.0f + ring_fact * 0.2f ) ), static_cast< int >( color.Value.y * 255 * ( 1.0f + ring_fact * 0.2f ) ), static_cast< int >( color.Value.z * 255 * ( 1.0f + ring_fact * 0.2f ) ), static_cast< int >( color.Value.w * 255 * alpha_multiplier ) );

			ImVec2 hemisphere_points[ ] = { wtop_hemisphere_rings[ ring ][ i ], wtop_hemisphere_rings[ ring ][ next_i ], wtop_hemisphere_rings[ ring + 1 ][ next_i ], wtop_hemisphere_rings[ ring + 1 ][ i ] };
			drawlist->AddConvexPolyFilled( hemisphere_points, 4, ring_color );
		}
	}

	for ( int ring = 0; ring < static_cast< int >( wbottom_hemisphere_rings.size( ) ) - 1; ++ring )
	{
		for ( int i = 0; i < segments; ++i )
		{
			const auto next_i = i + 1;

			const auto ring_fact = static_cast< float >( ring ) / ( wbottom_hemisphere_rings.size( ) - 1 );
			const auto ring_color = ImColor( static_cast< int >( color.Value.x * 255 * ( 1.0f + ring_fact * 0.2f ) ), static_cast< int >( color.Value.y * 255 * ( 1.0f + ring_fact * 0.2f ) ), static_cast< int >( color.Value.z * 255 * ( 1.0f + ring_fact * 0.2f ) ), static_cast< int >( color.Value.w * 255 * alpha_multiplier ) );

			ImVec2 hemisphere_points[ ] = { wbottom_hemisphere_rings[ ring ][ i ], wbottom_hemisphere_rings[ ring ][ next_i ], wbottom_hemisphere_rings[ ring + 1 ][ next_i ], wbottom_hemisphere_rings[ ring + 1 ][ i ] };
			drawlist->AddConvexPolyFilled( hemisphere_points, 4, ring_color );
		}
	}

	if ( distance < 15.0f )
	{
		const auto outline_color = ImColor( static_cast< int >( color.Value.x * 255 * 0.3f ), static_cast< int >( color.Value.y * 255 * 0.3f ), static_cast< int >( color.Value.z * 255 * 0.3f ), static_cast< int >( color.Value.w * 255 * 0.6f ) );

		for ( int i = 0; i < segments; ++i )
		{
			drawlist->AddLine( wtop_circle[ i ], wtop_circle[ static_cast< std::vector<ImVec2, std::allocator<ImVec2>>::size_type >( i ) + 1 ], outline_color, 0.8f );
			drawlist->AddLine( wbottom_circle[ i ], wbottom_circle[ static_cast< std::vector<ImVec2, std::allocator<ImVec2>>::size_type >( i ) + 1 ], outline_color, 0.8f );
		}

		const auto quarter = segments / 4;
		const auto half = segments / 2;
		const auto three_quarter = ( 3 * segments ) / 4;

		drawlist->AddLine( wtop_circle[ 0 ], wbottom_circle[ 0 ], outline_color, 0.6f );
		drawlist->AddLine( wtop_circle[ quarter ], wbottom_circle[ quarter ], outline_color, 0.6f );
		drawlist->AddLine( wtop_circle[ half ], wbottom_circle[ half ], outline_color, 0.6f );
		drawlist->AddLine( wtop_circle[ three_quarter ], wbottom_circle[ three_quarter ], outline_color, 0.6f );
	}
}

void features_c::visuals_c::player_c::chamies_c::draw_capsule_outline( ImDrawList* drawlist, const math::vector3& top, const math::vector3& bottom, const math::vector3& axis, const math::vector3& u, const math::vector3& v, float radius, ImColor color, const std::vector<float>& sin_cache, const std::vector<float>& cos_cache, int segments )
{
	std::vector<math::vector3> top_circle, bottom_circle;
	this->create_circle( top, u, v, radius, sin_cache, cos_cache, top_circle, segments );
	this->create_circle( bottom, u, v, radius, sin_cache, cos_cache, bottom_circle, segments );

	const auto hemisphere_segments = std::max( 3, segments / 3 );

	std::vector<ImVec2> wtop( segments + 1 ), wbottom( segments + 1 );
	for ( int i = 0; i <= segments; ++i )
	{
		wtop[ i ] = g->sdk.w2s.project( top_circle[ i ] ).vec( );
		wbottom[ i ] = g->sdk.w2s.project( bottom_circle[ i ] ).vec( );
	}

	const auto thickness = 1.5f;
	auto new_color = ImColor( std::min( 255, static_cast< int >( color.Value.x * 255 * 1.1f ) ), std::min( 255, static_cast< int >( color.Value.y * 255 * 1.1f ) ), std::min( 255, static_cast< int >( color.Value.z * 255 * 1.1f ) ), static_cast< int >( color.Value.w * 255 * 0.9f ) );

	for ( int i = 0; i < segments; ++i )
	{
		drawlist->AddLine( wtop[ i ], wtop[ static_cast< std::vector<ImVec2, std::allocator<ImVec2>>::size_type >( i ) + 1 ], new_color, thickness );
		drawlist->AddLine( wbottom[ i ], wbottom[ static_cast< std::vector<ImVec2, std::allocator<ImVec2>>::size_type >( i ) + 1 ], new_color, thickness );
	}

	for ( int h = 0; h < hemisphere_segments; ++h )
	{
		const auto phi = ( std::numbers::pi_v<float> / 2.0f ) * ( static_cast< float >( h + 1 ) / hemisphere_segments );
		const auto ring_radius = radius * std::cos( phi );
		const auto ring_height = radius * std::sin( phi );

		std::vector<math::vector3> top_arc, bottom_arc;
		const auto top_ring_center = top - axis * ring_height;
		const auto bottom_ring_center = bottom + axis * ring_height;

		this->create_circle( top_ring_center, u, v, ring_radius, sin_cache, cos_cache, top_arc, segments );
		this->create_circle( bottom_ring_center, u, v, ring_radius, sin_cache, cos_cache, bottom_arc, segments );

		for ( int i = 0; i < segments; ++i )
		{
			const auto top_p1 = g->sdk.w2s.project( top_arc[ i ] ).vec( );
			const auto top_p2 = g->sdk.w2s.project( top_arc[ i + 1 ] ).vec( );
			const auto bottom_p1 = g->sdk.w2s.project( bottom_arc[ i ] ).vec( );
			const auto bottom_p2 = g->sdk.w2s.project( bottom_arc[ i + 1 ] ).vec( );

			drawlist->AddLine( top_p1, top_p2, new_color, thickness * 0.7f );
			drawlist->AddLine( bottom_p1, bottom_p2, new_color, thickness * 0.7f );
		}
	}

	const auto quarter = segments / 4;
	const auto half = segments / 2;
	const auto three_quarter = ( 3 * segments ) / 4;

	drawlist->AddLine( wtop[ 0 ], wbottom[ 0 ], new_color, thickness );
	drawlist->AddLine( wtop[ half ], wbottom[ half ], new_color, thickness );
	drawlist->AddLine( wtop[ quarter ], wbottom[ quarter ], new_color, thickness * 0.8f );
	drawlist->AddLine( wtop[ three_quarter ], wbottom[ three_quarter ], new_color, thickness * 0.8f );
}

void features_c::visuals_c::player_c::chamies_c::precompute_sincos( int segments, std::vector<float>& sin_cache, std::vector<float>& cos_cache )
{
	sin_cache.resize( static_cast< std::vector<float, std::allocator<float>>::size_type >( segments ) + 1 );
	cos_cache.resize( static_cast< std::vector<float, std::allocator<float>>::size_type >( segments ) + 1 );

	const auto angle_step = 2.0f * std::numbers::pi_v<float> / segments;
	for ( int i = 0; i <= segments; ++i )
	{
		const auto angle = angle_step * i;
		sin_cache[ i ] = std::sin( angle );
		cos_cache[ i ] = std::cos( angle );
	}
}

void features_c::visuals_c::player_c::chamies_c::create_circle( const math::vector3& center, const math::vector3& u, const math::vector3& v, float radius, const std::vector<float>& sin_cache, const std::vector<float>& cos_cache, std::vector<math::vector3>& out, int segments )
{
    out.clear( );
    out.reserve( static_cast< std::vector<math::vector3, std::allocator<math::vector3>>::size_type >( segments ) + 1 );

    for ( int i = 0; i <= segments; ++i )
    {
        // center + (u*cos + v*sin) * radius
        out.push_back( center + ( u * cos_cache[ i ] + v * sin_cache[ i ] ) * radius );
    }
}

void features_c::visuals_c::indicators_c::oof_arrow( ImDrawList* drawlist, const math::vector2& position_projected, ImColor color )
{
    // Screen center and half extents
    const float sw = g->core.get_display().width;
    const float sh = g->core.get_display().height;
    const auto center = math::vector2{ sw * 0.5f, sh * 0.5f };

    // Direction from center toward off-screen projected point
    const float dx = position_projected.x - center.x;
    const float dy = position_projected.y - center.y;
    const float angle = std::atan2(dy, dx);
    const float cos_a = std::cos(angle);
    const float sin_a = std::sin(angle);

    // Compute intersection point with screen border (inset by margin)
    const float margin = 16.0f;
    const float half_w = (sw * 0.5f) - margin;
    const float half_h = (sh * 0.5f) - margin;

    // Scale factor to hit the nearest border along direction
    float scale_x = std::numeric_limits<float>::infinity();
    float scale_y = std::numeric_limits<float>::infinity();
    if (std::abs(cos_a) > 1e-6f) scale_x = half_w / std::abs(cos_a);
    if (std::abs(sin_a) > 1e-6f) scale_y = half_h / std::abs(sin_a);
    const float scale = std::min(scale_x, scale_y);

    const auto arrow_center = center + math::vector2{ cos_a * scale, sin_a * scale };

    // Arrow dimensions
    const float length = 20.0f;
    const float width = 16.0f;

    // Tip points outward, base sits inward from border
    const auto tip = arrow_center + math::vector2{ cos_a * length, sin_a * length };
    const auto back = arrow_center - math::vector2{ cos_a * (length * 0.6f), sin_a * (length * 0.6f) };
    const auto perp = math::vector2{ -sin_a, cos_a } * (width * 0.5f);
    const auto left = back + perp;
    const auto right = back - perp;

    // Use provided color for fill, draw a dark outline for contrast
    drawlist->AddTriangleFilled( tip.vec(), left.vec(), right.vec(), color );
    drawlist->AddTriangle( tip.vec(), left.vec(), right.vec(), ImColor( 10, 10, 10, 220 ), 1.0f );
}

void features_c::visuals_c::world_c::molotov( ImDrawList* drawlist, const math::vector3& center, int distance )
{
const auto projected = g->sdk.w2s.project( center );
if ( !g->sdk.w2s.is_valid( projected ) )
{
return;
}

	char label[ 64 ];
	std::snprintf( label, sizeof( label ), "molotov [%dm]", distance );

	const auto text_size = ImGui::CalcTextSize( label );

	drawlist->AddText( projected.vec( ) - ImVec2( text_size.x * 0.5f, text_size.y * 0.5f ), ImColor( 255, 255, 255, 255 ), label );
}

void features_c::visuals_c::world_c::molotov_bounds( ImDrawList* drawlist, const std::vector<math::vector3>& fire_points )
{
	std::vector<math::vector3> boundary_points;

	constexpr auto fire_radius = 60.0f;
	constexpr auto points_per_fire = 16;

	for ( const auto& point : fire_points )
	{
		for ( int i = 0; i < points_per_fire; ++i )
		{
			const auto angle = static_cast< float >( i ) / points_per_fire * std::numbers::pi_v<float> *2.0f;
			boundary_points.emplace_back( point + math::vector3{ std::cos( angle ) * fire_radius, std::sin( angle ) * fire_radius, 0.0f } );
		}
	}

	std::vector<ImVec2> screen_points;
	screen_points.reserve( boundary_points.size( ) );

	for ( const auto& point : boundary_points )
	{
		const auto projected = g->sdk.w2s.project( point );
		if ( g->sdk.w2s.is_valid( projected ) )
		{
			screen_points.push_back( projected.vec( ) );
		}
	}

	if ( screen_points.size( ) >= 3 )
	{
		const auto hull = this->compute_convex_hull( screen_points );
		if ( hull.size( ) >= 3 )
		{
			drawlist->AddConvexPolyFilled( hull.data( ), hull.size( ), ImColor( 255, 50, 0, 80 ) );
			drawlist->AddPolyline( hull.data( ), hull.size( ), ImColor( 255, 80, 0, 150 ), ImDrawFlags_Closed, 2.0f );
		}
	}
}

void features_c::visuals_c::world_c::smoke( ImDrawList* drawlist, const math::vector3& detonation_pos, int distance )
{
	const auto projected = g->sdk.w2s.project( detonation_pos );
	if ( !g->sdk.w2s.is_valid( projected ) )
	{
		return;
	}

	char label[ 64 ];
	std::snprintf( label, sizeof( label ), "smoke [%dm]", distance );

	const auto text_size = ImGui::CalcTextSize( label );

	drawlist->AddText( projected.vec( ) - ImVec2( text_size.x * 0.5f, text_size.y * 0.5f ), ImColor( 255, 255, 255, 255 ), label );
}

void features_c::visuals_c::world_c::bomb( ImDrawList* drawlist, const math::vector3& position, int distance )
{
	const auto projected = g->sdk.w2s.project( position );
	if ( !g->sdk.w2s.is_valid( projected ) )
	{
		return;
	}

	char label[ 64 ];
	std::snprintf( label, sizeof( label ), "C4 [%dm]", distance );

	const auto text_size = ImGui::CalcTextSize( label );

	drawlist->AddText( projected.vec( ) - ImVec2( text_size.x * 0.5f, text_size.y * 0.5f ), ImColor( 255, 50, 50, 255 ), label );
}

void features_c::visuals_c::world_c::drop( ImDrawList* drawlist, const math::vector3& position, const std::string& name, int distance, ImColor color )
{
	const auto projected = g->sdk.w2s.project( position );
	if ( !g->sdk.w2s.is_valid( projected ) )
	{
		return;
	}

	char label[ 128 ];
	std::snprintf( label, sizeof( label ), "%s [%dm]", name.c_str( ), distance );

	const auto text_size = ImGui::CalcTextSize( label );

	drawlist->AddText( projected.vec( ) - ImVec2( text_size.x * 0.5f, text_size.y * 0.5f ), color, label );
}

void features_c::visuals_c::world_c::drop_bounds( ImDrawList* drawlist, const math::transform& node_transform, const math::vector3& collision_min, const math::vector3& collision_max )
{
	const auto transform_matrix = math::func::transform_to_matrix3x4( node_transform );

	std::vector<ImVec2> screen_points;
	screen_points.reserve( 8 );

	for ( size_t i = 0; i < 8; ++i )
	{
		const auto point = math::vector3( i & 1 ? collision_max.x : collision_min.x, i & 2 ? collision_max.y : collision_min.y, i & 4 ? collision_max.z : collision_min.z ).transform( transform_matrix );
		const auto projected = g->sdk.w2s.project( point );
		if ( !g->sdk.w2s.is_valid( projected ) )
		{
			return;
		}

		screen_points.push_back( projected.vec( ) );
	}

	if ( screen_points.size( ) >= 3 )
	{
		const auto hull_points = this->compute_convex_hull( screen_points );
		if ( hull_points.size( ) >= 3 )
		{
			drawlist->AddConvexPolyFilled( hull_points.data( ), hull_points.size( ), ImColor( 0, 255, 0, 60 ) );
			drawlist->AddPolyline( hull_points.data( ), hull_points.size( ), ImColor( 0, 255, 0, 120 ), ImDrawFlags_Closed, 1.5f );
		}
	}
}

std::vector<ImVec2> features_c::visuals_c::world_c::compute_convex_hull( std::vector<ImVec2>& points ) const
{
	std::sort( points.begin( ), points.end( ), [ ]( const ImVec2& a, const ImVec2& b ) { return a.x < b.x || ( a.x == b.x && a.y < b.y ); } );

	std::vector<ImVec2> lower, upper;

	for ( const auto& p : points )
	{
		while ( lower.size( ) >= 2 )
		{
			const auto& p1 = lower[ lower.size( ) - 2 ];
			const auto& p2 = lower[ lower.size( ) - 1 ];
			if ( ( p2.x - p1.x ) * ( p.y - p1.y ) - ( p2.y - p1.y ) * ( p.x - p1.x ) > 0 )
			{
				break;
			}

			lower.pop_back( );
		}

		lower.push_back( p );
	}

	for ( auto it = points.rbegin( ); it != points.rend( ); ++it )
	{
		while ( upper.size( ) >= 2 )
		{
			const auto& p1 = upper[ upper.size( ) - 2 ];
			const auto& p2 = upper[ upper.size( ) - 1 ];
			if ( ( p2.x - p1.x ) * ( it->y - p1.y ) - ( p2.y - p1.y ) * ( it->x - p1.x ) > 0 )
			{
				break;
			}

			upper.pop_back( );
		}

		upper.push_back( *it );
	}

	lower.pop_back( );
	upper.pop_back( );
	lower.insert( lower.end( ), upper.begin( ), upper.end( ) );

	return lower;
}

void features_c::visuals_c::hud_c::ring( ImDrawList* drawlist )
{
	const auto center = ImVec2( g->core.get_display( ).width * 0.5f, g->core.get_display( ).height * 0.5f );

	drawlist->AddCircle( center, g->core.get_settings( ).aim.fov, ImColor( 10, 10, 10, 255 ), 128, 3.0f );
	drawlist->AddCircle( center, g->core.get_settings( ).aim.fov, g->features.get_fov_color(), 128, 1.5f );
}

void features_c::visuals_c::radar_c::render( ImDrawList* drawlist )
{
	auto& settings = g->core.get_settings().visuals.player.radar;
	if ( !settings.enabled ) return;

	// Radar position (top-right corner)
	const float radar_size = settings.size;
	const ImVec2 radar_pos = ImVec2( g->core.get_display().width - radar_size - 20, 20 );
	const ImVec2 radar_center = ImVec2( radar_pos.x + radar_size * 0.5f, radar_pos.y + radar_size * 0.5f );

	// Background
	drawlist->AddRectFilled( radar_pos, ImVec2( radar_pos.x + radar_size, radar_pos.y + radar_size ), IM_COL32( 0, 0, 0, 150 ), 5.0f );
	drawlist->AddRect( radar_pos, ImVec2( radar_pos.x + radar_size, radar_pos.y + radar_size ), IM_COL32( 255, 255, 255, 100 ), 5.0f, 0, 2.0f );

	// Center crosshair
	drawlist->AddLine( ImVec2( radar_center.x - 5, radar_center.y ), ImVec2( radar_center.x + 5, radar_center.y ), IM_COL32( 255, 255, 255, 200 ), 1.0f );
	drawlist->AddLine( ImVec2( radar_center.x, radar_center.y - 5 ), ImVec2( radar_center.x, radar_center.y + 5 ), IM_COL32( 255, 255, 255, 200 ), 1.0f );

	// Grid lines
	for ( int i = 1; i < 4; i++ )
	{
		float offset = ( radar_size * 0.25f ) * i;
		drawlist->AddLine( ImVec2( radar_pos.x + offset, radar_pos.y ), ImVec2( radar_pos.x + offset, radar_pos.y + radar_size ), IM_COL32( 255, 255, 255, 30 ), 1.0f );
		drawlist->AddLine( ImVec2( radar_pos.x, radar_pos.y + offset ), ImVec2( radar_pos.x + radar_size, radar_pos.y + offset ), IM_COL32( 255, 255, 255, 30 ), 1.0f );
	}
}

void features_c::visuals_c::radar_c::draw_player( ImDrawList* drawlist, const math::vector3& world_pos, const math::vector3& local_pos, bool is_enemy, int health, const std::string& name )
{
	auto& settings = g->core.get_settings().visuals.player.radar;
	if ( !settings.enabled ) return;

	const float radar_size = settings.size;
	const ImVec2 radar_pos = ImVec2( g->core.get_display().width - radar_size - 20, 20 );
	const ImVec2 radar_center = ImVec2( radar_pos.x + radar_size * 0.5f, radar_pos.y + radar_size * 0.5f );

	ImVec2 player_pos = world_to_radar( world_pos, local_pos, radar_center, radar_size, settings.zoom );

	// Clamp to radar bounds instead of returning
	player_pos.x = std::clamp(player_pos.x, radar_pos.x + 5.0f, radar_pos.x + radar_size - 5.0f);
	player_pos.y = std::clamp(player_pos.y, radar_pos.y + 5.0f, radar_pos.y + radar_size - 5.0f);

	// Player dot color based on team and health
	ImU32 dot_color;
	if ( is_enemy )
	{
		dot_color = health > 50 ? IM_COL32( 255, 0, 0, 255 ) : IM_COL32( 255, 100, 100, 255 );
	}
	else
	{
		dot_color = IM_COL32( 0, 255, 0, 255 );
	}

	// Draw player dot
	drawlist->AddCircleFilled( player_pos, 4.0f, dot_color );
	drawlist->AddCircle( player_pos, 4.0f, IM_COL32( 0, 0, 0, 255 ), 0, 1.0f );

	// Health bar
	if ( settings.show_health && is_enemy )
	{
		float health_width = 8.0f;
		float health_height = 2.0f;
		float health_percent = health / 100.0f;
		
		ImVec2 health_pos = ImVec2( player_pos.x - health_width * 0.5f, player_pos.y - 8.0f );
		drawlist->AddRectFilled( health_pos, ImVec2( health_pos.x + health_width, health_pos.y + health_height ), IM_COL32( 50, 50, 50, 200 ) );
		drawlist->AddRectFilled( health_pos, ImVec2( health_pos.x + health_width * health_percent, health_pos.y + health_height ), 
							 health > 75 ? IM_COL32( 0, 255, 0, 255 ) : health > 25 ? IM_COL32( 255, 255, 0, 255 ) : IM_COL32( 255, 0, 0, 255 ) );
	}

	// Name
	if ( settings.show_names && !name.empty() )
	{
		ImVec2 text_size = ImGui::CalcTextSize( name.c_str() );
		ImVec2 text_pos = ImVec2( player_pos.x - text_size.x * 0.5f, player_pos.y + 6.0f );
		drawlist->AddText( text_pos, IM_COL32( 255, 255, 255, 200 ), name.c_str() );
	}
}

void features_c::visuals_c::radar_c::draw_bomb( ImDrawList* drawlist, const math::vector3& world_pos, const math::vector3& local_pos )
{
	auto& settings = g->core.get_settings().visuals.player.radar;
	if ( !settings.enabled || !settings.bomb ) return;

	const float radar_size = settings.size;
	const ImVec2 radar_pos = ImVec2( g->core.get_display().width - radar_size - 20, 20 );
	const ImVec2 radar_center = ImVec2( radar_pos.x + radar_size * 0.5f, radar_pos.y + radar_size * 0.5f );

	ImVec2 bomb_pos = world_to_radar( world_pos, local_pos, radar_center, radar_size, settings.zoom );

	// Check bounds
	if ( bomb_pos.x < radar_pos.x || bomb_pos.x > radar_pos.x + radar_size ||
		 bomb_pos.y < radar_pos.y || bomb_pos.y > radar_pos.y + radar_size )
		return;

	// Draw bomb icon
	drawlist->AddRectFilled( ImVec2( bomb_pos.x - 3, bomb_pos.y - 3 ), ImVec2( bomb_pos.x + 3, bomb_pos.y + 3 ), IM_COL32( 255, 165, 0, 255 ) );
	drawlist->AddRect( ImVec2( bomb_pos.x - 3, bomb_pos.y - 3 ), ImVec2( bomb_pos.x + 3, bomb_pos.y + 3 ), IM_COL32( 0, 0, 0, 255 ), 0.0f, 0, 1.0f );
}

ImVec2 features_c::visuals_c::radar_c::world_to_radar( const math::vector3& world_pos, const math::vector3& local_pos, const ImVec2& radar_center, float radar_size, float zoom )
{
	// Calculate relative position
	float dx = world_pos.x - local_pos.x;
	float dy = world_pos.y - local_pos.y;

	// Scale factor (adjust based on game units)
	float scale = ( radar_size * 0.3f ) / ( 2000.0f / zoom );

	// Convert to radar coordinates
	float radar_x = radar_center.x + dx * scale;
	float radar_y = radar_center.y - dy * scale; // Invert Y for screen coordinates

	return ImVec2( radar_x, radar_y );
}

void features_c::visuals_c::player_c::ping_info( ImDrawList* drawlist, const sdk_c::bounds_c::data_t& bound_data, float ping, float interpolation )
{
	if ( ping <= 0.0f ) return; // Skip if no ping data

	// Get configurable thresholds
	const auto ping_threshold = g->core.get_settings().visuals.player.ping_warning_threshold;
	const auto lag_threshold = g->core.get_settings().visuals.player.lag_spike_threshold;

	// Color code based on ping value and configurable threshold
	ImColor ping_color;
	if ( ping < ping_threshold * 0.5f ) {
		ping_color = ImColor(0, 255, 0, 255);      // Green for good ping
	} else if ( ping < ping_threshold ) {
		ping_color = ImColor(255, 255, 0, 255);    // Yellow for moderate ping
	} else if ( ping < ping_threshold * 2.0f ) {
		ping_color = ImColor(255, 165, 0, 255);    // Orange for high ping
	} else {
		ping_color = ImColor(255, 0, 0, 255);      // Red for very high ping
	}

	// Format ping text
	char ping_text[32];
	snprintf(ping_text, sizeof(ping_text), "%.0fms", ping);

	// Position ping info below the ESP box
	const auto ping_x = bound_data.min_x;
	const auto ping_y = bound_data.max_y + 2.0f;
	
	// Draw ping value
	drawlist->AddText( ImVec2( ping_x, ping_y ), ping_color, ping_text );
	
	// Draw interpolation info if it's significant
	if ( interpolation > lag_threshold ) {
		char interp_text[32];
		snprintf(interp_text, sizeof(interp_text), "%.1f", interpolation);
		drawlist->AddText( ImVec2( ping_x, ping_y + ImGui::GetFontSize() + 2.0f ), ImColor(255, 100, 100, 255), interp_text );
	}
}

void features_c::exploits_c::anti_aim_c::run( ) // isnt server sided -> very useless -> use if you wanna flex about your external "antiaim"
{
	g->features.exploits.anti_aim.data.real_angles = g->memory.read<math::vector3>( g->core.get_process_info( ).client_base + offsets::dw_view_angles );

	static bool side = false;
	static auto last_switch = std::chrono::steady_clock::now( );
	static auto time = 0.0f;

	auto now = std::chrono::steady_clock::now( );
	auto elapsed = std::chrono::duration_cast< std::chrono::milliseconds >( now - last_switch );

	time += 0.016f;

	const auto sway = std::sin( time * 2.5f ) * 40.0f;
	const auto quick_sway = std::sin( time * 8.0f ) * 15.0f;
	const auto big = std::sin( time * 12.0f ) * 5.0f;
	const auto drift = std::sin( time * 0.8f ) * 25.0f;
	const auto noisy = std::sin( time * 15.0f ) * 8.0f;

	if ( elapsed.count( ) > 80 )
	{
		side = !side;
		last_switch = now;
	}

	this->data.fake_angles.x = 90.0f + std::sin( time * 1.0f ) * 2.0f + std::sin( time * 6.0f ) * 4.0f;
	this->data.fake_angles.y = this->data.real_angles.y + 180.0f + sway + quick_sway + big + drift + noisy + ( side ? 22.0f : -22.0f );
	this->data.fake_angles.z = this->data.real_angles.z;

	if ( g->udata.get_owning_player( ).cs_player_pawn )
	{
		if ( GetAsyncKeyState( VK_LBUTTON ) & 0x8000 )
		{
			g->memory.write<math::vector3>( g->udata.get_owning_player( ).cs_player_pawn + offsets::v_angle, this->data.real_angles );
		}
		else
		{
			g->memory.write<math::vector3>( g->udata.get_owning_player( ).cs_player_pawn + offsets::v_angle, this->data.fake_angles );
		}
	}
}

void features_c::exploits_c::night_mode_c::run( std::uintptr_t post_processing_volume )
{
	if ( !this->has_original )
	{
		this->original_min = g->memory.read<float>( post_processing_volume + offsets::m_fl_min_exposure );
		this->original_max = g->memory.read<float>( post_processing_volume + offsets::m_fl_max_exposure );
		this->has_original = true;
	}

	if ( g->core.get_settings( ).misc.night_mode )
	{
		const auto target = std::lerp( 1.0f, 0.05f, g->core.get_settings( ).misc.night_mode_strength );
		g->memory.write<bool>( post_processing_volume + offsets::m_b_exposure_control, true );
		g->memory.write<float>( post_processing_volume + offsets::m_fl_min_exposure, target );
		g->memory.write<float>( post_processing_volume + offsets::m_fl_max_exposure, target );
	}
	else if ( this->has_original )
	{
		g->memory.write<float>( post_processing_volume + offsets::m_fl_min_exposure, this->original_min );
		g->memory.write<float>( post_processing_volume + offsets::m_fl_max_exposure, this->original_max );
	}
}

void features_c::exploits_c::third_person_c::run( )
{
	this->handle_hotkey( );

	if ( this->needs_reapply( g->udata.get_owning_player( ).cs_player_pawn ) )
	{
		this->currently_active = false;
		this->cached_player_pawn = g->udata.get_owning_player( ).cs_player_pawn;
	}

	if ( g->core.get_settings( ).misc.third_person && !this->currently_active )
	{
		this->apply_patch( );
		this->set_third_person_state( true );
		this->currently_active = true;
	}
	else if ( !g->core.get_settings( ).misc.third_person && this->currently_active )
	{
		this->set_third_person_state( false );
		this->remove_patch( );
		this->currently_active = false;
	}
}

void features_c::exploits_c::third_person_c::apply_patch( )
{
	if ( this->patch_applied )
	{
		return;
	}

	g->memory.write_array<std::uint8_t>( g->core.get_process_info( ).client_base + this->jne_patch, this->patched_jne );
	this->patch_applied = true;
}

void features_c::exploits_c::third_person_c::remove_patch( )
{
	if ( !this->patch_applied )
	{
		return;
	}

	g->memory.write_array<std::uint8_t>( g->core.get_process_info( ).client_base + this->jne_patch, this->original_jne );
	this->patch_applied = false;
}

void features_c::exploits_c::third_person_c::set_third_person_state( bool enabled )
{
	g->memory.write<std::uint8_t>( g->core.get_process_info( ).client_base + offsets::dw_csgo_input + 0x251, enabled ? 1 : 0 ); // CCSGOInput::ThirdPerson
}

bool features_c::exploits_c::third_person_c::needs_reapply( std::uintptr_t current_player_pawn ) const
{
	if ( current_player_pawn != this->cached_player_pawn && current_player_pawn != 0 )
	{
		return true;
	}

	if ( this->currently_active && g->core.get_settings( ).misc.third_person )
	{
		const auto cur_value = g->memory.read<std::uint8_t>( g->core.get_process_info( ).client_base + offsets::dw_csgo_input + 0x251 );
		if ( cur_value == 0 )
		{
			return true;
		}
	}

	return false;
}

void features_c::exploits_c::third_person_c::handle_hotkey( )
{
	const bool key_down = GetAsyncKeyState( g->core.get_settings( ).misc.third_person_key ) & 0x8000;

	if ( key_down && !this->hotkey_pressed )
	{
		g->core.get_settings( ).misc.third_person = !g->core.get_settings( ).misc.third_person;
		this->hotkey_pressed = true;
	}
	else if ( !key_down )
	{
		this->hotkey_pressed = false;
	}
}

void features_c::visuals_c::player_c::network_info( ImDrawList* drawlist, const sdk_c::bounds_c::data_t& bound_data, float ping, float interpolation, int server_tick )
{
	if ( ping <= 0.0f ) return; // Skip if no ping data

	// Get configurable thresholds
	const auto ping_threshold = g->core.get_settings().visuals.player.ping_warning_threshold;
	const auto lag_threshold = g->core.get_settings().visuals.player.lag_spike_threshold;

	// Calculate network quality indicators
	const bool is_high_ping = ping > ping_threshold;
	const bool has_lag_spike = interpolation > lag_threshold;
	
	// Network status text
	const char* status_text = "GOOD";
	ImColor status_color = ImColor(0, 255, 0, 255);
	
	if ( is_high_ping && has_lag_spike ) {
		status_text = "POOR";
		status_color = ImColor(255, 0, 0, 255);
	} else if ( is_high_ping || has_lag_spike ) {
		status_text = "FAIR";
		status_color = ImColor(255, 165, 0, 255);
	}

	// Position network info below ping info
	const auto info_x = bound_data.min_x;
	const auto info_y = bound_data.max_y + ImGui::GetFontSize() * 2 + 6.0f;
	
	// Draw network status
	drawlist->AddText( ImVec2( info_x, info_y ), status_color, status_text );
	
	// Draw server tick info if available
	if ( server_tick > 0 ) {
		char tick_text[32];
		snprintf(tick_text, sizeof(tick_text), "Tick: %d", server_tick);
		drawlist->AddText( ImVec2( info_x, info_y + ImGui::GetFontSize() + 2.0f ), ImColor(200, 200, 200, 255), tick_text );
	}
	
	// Draw interpolation info
	char interp_text[32];
	snprintf(interp_text, sizeof(interp_text), "Interp: %.3f", interpolation);
	ImColor interp_color = has_lag_spike ? ImColor(255, 100, 100, 255) : ImColor(200, 200, 200, 255);
	drawlist->AddText( ImVec2( info_x, info_y + ImGui::GetFontSize() * 2 + 4.0f ), interp_color, interp_text );
}

