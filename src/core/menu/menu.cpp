#include <include/global.hpp>
#include <commdlg.h>
#include <cstdlib>
#pragma comment(lib, "comdlg32.lib")

void menu_c::run( )
{
	// Safety check for global pointer
	if (!g) {
		return;
	}

	static bool menu_key_down = false;
	static int debounce_counter = 0;

	try {
		if ( GetAsyncKeyState( g->core.get_settings().hotkeys.menu_toggle ) & 0x8000 )
	{
		if ( !menu_key_down && debounce_counter <= 0 )
		{
			menu_key_down = true;
			this->is_visible = !this->is_visible;
			debounce_counter = 10;
		}
	}
	else
	{
		menu_key_down = false;
	}

	if ( debounce_counter > 0 ) debounce_counter--;

	try {
		this->handle_hotkeys();
	} catch (const std::exception& e) {
		// Log hotkey errors but continue execution
		this->show_notification("Hotkey error: " + std::string(e.what()));
	} catch (...) {
		// Ignore unknown hotkey errors
		this->show_notification("Unknown hotkey error occurred");
	}

	if ( !this->is_styled || this->theme_changed )
	{
		this->apply_theme( g->core.get_settings().themes.current_theme );
		this->is_styled = true;
		this->theme_changed = false;
	}

	// Handle hotkey setting
	if ( g->core.get_settings().hotkeys.setting_key && g->core.get_settings().hotkeys.current_setting )
	{
			// Check keyboard keys
		for ( int key = 0x08; key <= 0xFE; key++ )
		{
			if ( key == g->core.get_settings().hotkeys.menu_toggle ) continue;
			// Skip mouse buttons in keyboard loop
			if ( key == VK_LBUTTON || key == VK_RBUTTON || key == VK_MBUTTON || key == VK_XBUTTON1 || key == VK_XBUTTON2 ) continue;
			if ( GetAsyncKeyState( key ) & 0x8000 )
			{
				*g->core.get_settings().hotkeys.current_setting = key;
				g->core.get_settings().hotkeys.setting_key = false;
				g->core.get_settings().hotkeys.current_setting = nullptr;
				this->show_notification("Hotkey assigned: " + std::string(this->get_key_name(key)));
				break;
			}
		}
		
		// Check mouse buttons separately
		int mouse_buttons[] = { VK_LBUTTON, VK_RBUTTON, VK_MBUTTON, VK_XBUTTON1, VK_XBUTTON2 };
		for ( int mouse_key : mouse_buttons )
		{
			if ( GetAsyncKeyState( mouse_key ) & 0x8000 )
			{
				*g->core.get_settings().hotkeys.current_setting = mouse_key;
				g->core.get_settings().hotkeys.setting_key = false;
				g->core.get_settings().hotkeys.current_setting = nullptr;
				this->show_notification("Hotkey assigned: " + std::string(this->get_key_name(mouse_key)));
				break;
			}
		}
	}

    // FPS Counter
    if ( g->core.get_settings().extras.fps_counter )
    {
        static auto fps_smooth = 60.0f;
        fps_smooth = fps_smooth * 0.95f + ImGui::GetIO( ).Framerate * 0.05f;

        char fps_text[ 16 ];
        snprintf( fps_text, sizeof( fps_text ), "FPS: %.0f", fps_smooth );

        const auto pos = ImVec2( 20.0f, 20.0f );
        const auto text_size = ImGui::CalcTextSize( fps_text );
        const auto bg_min = ImVec2( pos.x - 10.0f, pos.y - 5.0f );
        const auto bg_max = ImVec2( pos.x + text_size.x + 10.0f, pos.y + text_size.y + 5.0f );

        ImGui::GetBackgroundDrawList( )->AddRectFilled( bg_min, bg_max, IM_COL32( 25, 25, 25, 180 ), 0.0f );
        ImGui::GetBackgroundDrawList( )->AddRect( bg_min, bg_max, IM_COL32( 45, 45, 45, 255 ), 0.0f );

        const auto color = fps_smooth >= 100.0f ? IM_COL32( 0, 255, 0, 255 ) : fps_smooth >= 60.0f ? IM_COL32( 255, 255, 0, 255 ) : IM_COL32( 255, 0, 0, 255 );
        ImGui::GetBackgroundDrawList( )->AddText( pos, color, fps_text );
    }

    // Notification system
    this->render_notifications();

    // Ping Display
    if ( g->core.get_settings().extras.ping_display )
    {
        static int ping = 0;
        static auto last_ping_update = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();
        
        if ( std::chrono::duration_cast<std::chrono::milliseconds>(now - last_ping_update).count() > 1000 )
        {
            ping = 20 + (rand() % 80); // Simulate ping 20-100ms
            last_ping_update = now;
        }

        char ping_text[ 16 ];
        snprintf( ping_text, sizeof( ping_text ), "Ping: %dms", ping );

        const auto fps_offset = g->core.get_settings().extras.fps_counter ? 25.0f : 0.0f;
        const auto pos = ImVec2( 20.0f, 20.0f + fps_offset );
        const auto text_size = ImGui::CalcTextSize( ping_text );
        const auto bg_min = ImVec2( pos.x - 10.0f, pos.y - 5.0f );
        const auto bg_max = ImVec2( pos.x + text_size.x + 10.0f, pos.y + text_size.y + 5.0f );

        ImGui::GetBackgroundDrawList( )->AddRectFilled( bg_min, bg_max, IM_COL32( 25, 25, 25, 180 ), 0.0f );
        ImGui::GetBackgroundDrawList( )->AddRect( bg_min, bg_max, IM_COL32( 45, 45, 45, 255 ), 0.0f );

        const auto ping_color = ping <= 30 ? IM_COL32( 0, 255, 0, 255 ) : ping <= 60 ? IM_COL32( 255, 255, 0, 255 ) : IM_COL32( 255, 0, 0, 255 );
        ImGui::GetBackgroundDrawList( )->AddText( pos, ping_color, ping_text );
    }

    // FPS Cap
    if ( g->core.get_settings().extras.fps_cap_enabled )
    {
        static auto last_frame_time = std::chrono::high_resolution_clock::now();
        auto current_time = std::chrono::high_resolution_clock::now();
        
        float target_frame_time = 1000.0f / g->core.get_settings().extras.fps_cap_limit; // ms per frame
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - last_frame_time).count();
        
        if ( elapsed < target_frame_time )
        {
            Sleep( static_cast<DWORD>(target_frame_time - elapsed) );
        }
        
        last_frame_time = std::chrono::high_resolution_clock::now();
    }

    // Stream-Proof Mode (only apply when setting changes)
    static bool last_stream_proof_state = false;
    if ( g->core.get_settings().extras.stream_proof != last_stream_proof_state )
    {
        HWND current_hwnd = GetActiveWindow();
        if ( g->core.get_settings().extras.stream_proof )
        {
            this->apply_stream_proof( current_hwnd );
        }
        else
        {
            this->remove_stream_proof( current_hwnd );
        }
        last_stream_proof_state = g->core.get_settings().extras.stream_proof;
    }

	if ( this->is_visible )
	{
		try {
			auto& settings = g->core.get_settings( );

        const auto display_size = ImVec2(g->core.get_display().width, g->core.get_display().height);
        const auto window_size = ImVec2(std::min(1400.0f, display_size.x * 0.95f), std::min(900.0f, display_size.y * 0.95f));
        // One-time enforce a larger size so long names fit immediately
        static bool menu_resized_once = false;
        if (!menu_resized_once) {
            ImGui::SetNextWindowSize(window_size, ImGuiCond_Always);
            menu_resized_once = true;
        } else {
            ImGui::SetNextWindowSize(window_size, ImGuiCond_FirstUseEver);
        }
        ImGui::SetNextWindowPos(ImVec2(display_size.x * 0.5f - window_size.x * 0.5f, display_size.y * 0.5f - window_size.y * 0.5f), ImGuiCond_FirstUseEver);
        // Prevent the window from shrinking too small on different resolutions/DPI
        ImGui::SetNextWindowSizeConstraints(ImVec2(1000.0f, 680.0f), ImVec2(display_size.x * 0.98f, display_size.y * 0.98f));
        ImGui::Begin( "PREMKILLER", nullptr, ImGuiWindowFlags_NoCollapse );

        // Top-right controls: Reinject and Power Off
        {
            ImGuiStyle& style = ImGui::GetStyle();
            const float btnW = 100.0f, btnH = 26.0f;
            const float spacing = style.ItemSpacing.x;
            // Position cursor to right side
            float rightX = ImGui::GetWindowContentRegionMax().x;
            float startX = rightX - (btnW * 2.0f + spacing);
            float curY = ImGui::GetCursorPosY();
            ImGui::SetCursorPos(ImVec2(startX, curY));

            // Reinject button
            if ( ImGui::Button( "Reinject", ImVec2( btnW, btnH ) ) )
            {
                char self_path[MAX_PATH] = {};
                if ( GetModuleFileNameA(nullptr, self_path, MAX_PATH) > 0 )
                {
                    ShellExecuteA(nullptr, "open", self_path, nullptr, nullptr, SW_SHOWNORMAL);
                }
                this->is_running = false;
                try { g->updater.shutdown(); } catch (...) {}
                std::exit(0);
            }
            ImGui::SameLine();

            // Power Off button
            if ( ImGui::Button( "Power Off", ImVec2( btnW, btnH ) ) )
            {
                this->is_running = false;
                try { g->updater.shutdown(); } catch (...) {}
                std::exit(0);
            }
            ImGui::Separator();
        }

        // Launcher controls are now in a standalone window (see src/launcher.cpp)

        // Begin vertical sidebar + content layout
        {
            const float sidebar_width = 280.0f;
            const float full_height = ImGui::GetContentRegionAvail().y - 6.0f;
            static int selected_section = 0; // 0=Legit Bot,1=Visuals,2=World,3=Misc,4=Extras,5=Config

            // Sidebar
            ImGui::BeginChild("##sidebar", ImVec2(sidebar_width, full_height), true);
            ImGui::TextDisabled("Sections");
            ImGui::Separator();
            const char* items[] = {
                "Legit Bot",
                "Visuals",
                "World",
                "Misc",
                "Extras",
                "Config",
                "Triggerbot"
            };
            for (int i = 0; i < (int)(sizeof(items)/sizeof(items[0])); ++i) {
                bool sel = (selected_section == i);
                if (ImGui::Selectable(items[i], sel)) selected_section = i;
            }
            ImGui::EndChild();

            ImGui::SameLine();

            // Content
            ImGui::BeginChild("##content", ImVec2(0, full_height), true);
            {
                // LEGIT BOT (Aim)
                if (selected_section == 0)
                {
                ImGui::Spacing();

                // Two-column structured layout with card-like panels
                if (ImGui::BeginTable("##aim_grid", 2, ImGuiTableFlags_SizingStretchProp))
                {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::BeginChild("##aim_left", ImVec2(0, 0), true);
                    ImGui::Text("General");
                    ImGui::Separator();

                    ImGui::Checkbox( "Enable Aimbot", &g->core.get_settings().aim.enabled );
                    ImGui::SameLine();
                    {
                        auto& hk = g->core.get_settings().hotkeys;
                        ImGui::TextDisabled("Toggle Key: %s", this->get_key_name(hk.aimbot_toggle));
                        ImGui::SameLine();
                        if (ImGui::Button("Set##AimbotToggle")) { hk.setting_key = true; hk.current_setting = &hk.aimbot_toggle; this->show_notification("Press a key for Aimbot Toggle"); }
                    }
                    // Move Aim Key and Aim Type directly under Enable Aimbot
                    ImGui::Text( "Aim Key:" );
                    ImGui::SameLine( );
                    static const char* key_names[ ] = { "Mouse 4", "Mouse 5", "Left Click", "Right Click" };
                    static int key_values[ ] = { VK_XBUTTON1, VK_XBUTTON2, VK_LBUTTON, VK_RBUTTON };

                    int current_key_index = 1;
                    for ( int i = 0; i < 4; i++ )
                    {
                        if ( key_values[ i ] == g->core.get_settings().aim.key ) 
                        {
                            current_key_index = i;
                            break;
                        }
                    }

                    if ( ImGui::Combo( "##AimKey", &current_key_index, key_names, 4 ) ) 
                    {
                        g->core.get_settings().aim.key = key_values[ current_key_index ];
                    }

                    ImGui::Spacing( );

                    ImGui::Text( "Aim Type:" );
                    static const char* aim_types[ ] = { "Mouse", "Angles", "Silent" };
                    ImGui::Combo( "##AimType", &g->core.get_settings().aim.type, aim_types, 3 );
                    ImGui::Checkbox( "RCS", &g->core.get_settings().aim.rcs );
                    if ( g->core.get_settings().aim.rcs )
                    {
                        ImGui::SliderFloat( "RCS Strength", &g->core.get_settings().aim.rcs_strength, 0.0f, 2.0f, "%.2f" );
                        ImGui::SliderFloat( "RCS Factor", &g->core.get_settings().aim.rcs_factor, 0.0f, 20.0f, "%.1f" );
                        ImGui::SliderFloat( "RCS Smooth", &g->core.get_settings().aim.rcs_smooth, 1.0f, 20.0f, "%.1f" );
                        ImGui::SliderFloat( "RCS X", &g->core.get_settings().aim.rcs_x_scale, 0.0f, 0.5f, "%.2f" );
                        ImGui::SliderFloat( "RCS Y", &g->core.get_settings().aim.rcs_y_scale, 1.0f, 2.0f, "%.2f" );
                        ImGui::SliderInt(  "RCS Start Bullets", &g->core.get_settings().aim.rcs_start_bullets, 1, 10, "%d" );
                    }

                    

                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();

                    // Old smoothing UI removed; use the unified Smooth slider in Values for all weapons

                    ImGui::EndChild();

                    ImGui::TableSetColumnIndex(1);
                    ImGui::BeginChild("##aim_right", ImVec2(0, 0), true);
                    ImGui::Text("Values");
                    ImGui::Separator();

                    // Values for all weapons
                    ImGui::Text("Values for all weapons");
                    static const char* targeting_modes_all[] = { "Near to Crosair", "Lowest Health" };
                    ImGui::Combo("Hitbox", &g->core.get_settings().aim.targeting_mode, targeting_modes_all, 2);
                    ImGui::SliderFloat( "FOV", &g->core.get_settings().aim.fov, 1.0f, 30.0f, "%.1f" );
                    ImGui::SliderFloat( "Smooth", &g->core.get_settings().aim.smooth, 1.0f, 30.0f, "%.1f" );
                    ImGui::SliderFloat( "Humanized Smooth", &g->core.get_settings().aim.humanized_smooth, 0.0f, 1.0f, "%.2f" );
                    ImGui::SliderFloat( "Jitter Aim", &g->core.get_settings().aim.jitter_aim, 0.0f, 1.0f, "%.2f" );
                    ImGui::SliderInt( "Hit Chance (%)", &g->core.get_settings().aim.hit_chance_percent, 1, 100, "%d%%" );

                    ImGui::SliderInt("Target Debounce (ms)", &g->core.get_settings().aim.target_debounce_ms, 0, 300, "%d ms");

                    ImGui::Spacing( );

                    

                    ImGui::Text( "Target Hitboxes:" );
                    ImGui::Checkbox( "Head", &g->core.get_settings().aim.hitbox_head );
                    ImGui::SameLine();
                    ImGui::Checkbox( "Chest", &g->core.get_settings().aim.hitbox_chest );
                    ImGui::SameLine();
                    ImGui::Checkbox( "Pelvis", &g->core.get_settings().aim.hitbox_pelvis );
                    ImGui::Checkbox( "Arms", &g->core.get_settings().aim.hitbox_arms );
                    ImGui::SameLine();
                    ImGui::Checkbox( "Legs", &g->core.get_settings().aim.hitbox_legs );
                    
                    // Preferred hitbox override
                    ImGui::Spacing();
                    ImGui::Checkbox("Use Preferred Hitbox", &g->core.get_settings().aim.use_preferred_hitbox);
                    ImGui::BeginDisabled(!g->core.get_settings().aim.use_preferred_hitbox);
                    static const char* preferred_hitbox_items[] = { "Head", "Chest", "Pelvis" };
                    ImGui::Combo("Preferred Hitbox", &g->core.get_settings().aim.preferred_hitbox, preferred_hitbox_items, IM_ARRAYSIZE(preferred_hitbox_items));
                    ImGui::EndDisabled();

                    ImGui::Spacing();
                    ImGui::Text( "Checks:" );
                    ImGui::Checkbox( "Visibility Check", &g->core.get_settings().aim.vis_check );
                    ImGui::Checkbox( "Flash Check", &g->core.get_settings().aim.flash_check );
                    ImGui::Checkbox( "Smoke Check", &g->core.get_settings().aim.smoke_check );

                    // Visibility tuning
                    ImGui::Spacing();
                    static const char* vis_mode_items[] = { "Relaxed", "Normal", "Strict" };
                    ImGui::Combo("Visibility Mode", &g->core.get_settings().aim.visibility_mode, vis_mode_items, IM_ARRAYSIZE(vis_mode_items));
                    ImGui::Checkbox("Anti-Flicker", &g->core.get_settings().aim.anti_flicker);

                    ImGui::Spacing( );
                    ImGui::Separator( );
                    ImGui::Spacing( );

                    ImGui::Text( "Visualization:" );
                    ImGui::Checkbox( "Show FOV Circle", &g->core.get_settings().aim.visualization.fov );
                    ImGui::Checkbox( "Target Line", &g->core.get_settings().aim.visualization.line );
                    ImGui::Checkbox( "Target Dot", &g->core.get_settings().aim.visualization.dot );

                    ImGui::EndChild();

                    ImGui::EndTable();
                }
                // End Aim panel

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // Triggerbot panel moved to its own section
                }
                // VISUALS SECTION
                else if (selected_section == 1)
                {
                    auto& vis = g->core.get_settings().visuals;

                    if (ImGui::BeginTable("##visuals_grid", 2, ImGuiTableFlags_SizingStretchProp))
                    {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::BeginChild("##vis_left", ImVec2(0, 0), true);
                        // Player ESP block
                        ImGui::Text("Player ESP");
                        ImGui::Separator();
                        ImGui::Checkbox("Enable Player ESP", &vis.player_esp_enabled);
                        ImGui::SameLine();
                        {
                            auto& hk = g->core.get_settings().hotkeys;
                            ImGui::TextDisabled("ESP Toggle: %s", this->get_key_name(hk.esp_toggle));
                            ImGui::SameLine();
                            if (ImGui::Button("Set##Esptoggle")) { hk.setting_key = true; hk.current_setting = &hk.esp_toggle; this->show_notification("Press a key for ESP Toggle"); }
                            ImGui::SameLine();
                            ImGui::TextDisabled("Disable: %s", this->get_key_name(hk.esp_disable));
                            ImGui::SameLine();
                            if (ImGui::Button("Set##Espdisable")) { hk.setting_key = true; hk.current_setting = &hk.esp_disable; this->show_notification("Press a key for ESP Disable"); }
                        }
                        ImGui::BeginDisabled(!vis.player_esp_enabled);
                        {
                            ImGui::Checkbox("Box", &vis.player.box);
                            ImGui::SameLine();
                            ImGui::Checkbox("Skeleton", &vis.player.skeleton);

                            ImGui::Checkbox("Health Bar", &vis.player.health);
                            ImGui::Checkbox("Name", &vis.player.name);
                            ImGui::SameLine();
                            ImGui::Checkbox("Weapon", &vis.player.weapon);
                            ImGui::Checkbox("Distance", &vis.player.distance);

                            ImGui::Spacing();
                            ImGui::Checkbox("OOF Arrows", &vis.player.oof_arrows);

                            ImGui::Spacing();
                            ImGui::Checkbox("Chams", &vis.player.chamies);

                            ImGui::Checkbox("Head Circle", &vis.player.head_circle);

                            ImGui::Spacing();
                            ImGui::Text("Flags");
                            ImGui::Checkbox("Show Flags", &vis.player.flags);
                            if (vis.player.flags)
                            {
                                ImGui::ColorEdit3("Flag Color", vis.player.flag_color, ImGuiColorEditFlags_NoInputs);
                                ImGui::SliderFloat("Flag Text Size (0=auto)", &vis.player.flag_text_size, 0.0f, 24.0f, "%.1f");
                                // Bitmask: 0..4 correspond to FLASHED, SCOPED, DEFUSING, HIGH PING, LAG SPIKE
                                static const char* flag_names[] = { "Flashed", "Scoped", "Defusing", "High Ping", "Lag Spike" };
                                for (int bit = 0; bit < 5; ++bit)
                                {
                                    bool bit_set = (vis.player.flags_mask & (1 << bit)) != 0;
                                    if (ImGui::Checkbox(flag_names[bit], &bit_set))
                                    {
                                        if (bit_set) vis.player.flags_mask |= (1 << bit); else vis.player.flags_mask &= ~(1 << bit);
                                    }
                                    if (bit < 4) ImGui::SameLine();
                                }

                                ImGui::Spacing();
                                ImGui::Checkbox("Show Ping", &vis.player.show_ping);
                                ImGui::SameLine();
                                ImGui::Checkbox("Show Network Info", &vis.player.show_network_info);
                                ImGui::SliderFloat("Ping Warning Threshold (ms)", &vis.player.ping_warning_threshold, 20.0f, 300.0f, "%.0f ms");
                                ImGui::SliderFloat("Lag Spike Threshold", &vis.player.lag_spike_threshold, 0.01f, 0.25f, "%.3f");
                            }
                        }
                        ImGui::EndDisabled();

                        ImGui::Spacing();
                        ImGui::Separator();
                        ImGui::Spacing();

                        // Spectator block
                        ImGui::Text("Spectator");
                        ImGui::Separator();
                        {
                            auto& s = vis.spectator;
                            ImGui::Checkbox("Enable Spectator List", &s.enabled);
                            ImGui::BeginDisabled(!s.enabled);
                            ImGui::Checkbox("Show Name", &s.name);
                            ImGui::ColorEdit3("Text Color", s.text_color, ImGuiColorEditFlags_NoInputs);
                            ImGui::EndDisabled();
                        }

                        ImGui::Spacing();
                        ImGui::Separator();
                        ImGui::Spacing();

                        // Movement Tracers block
                        ImGui::Text("Movement Tracers");
                        ImGui::Separator();
                        {
                            auto& mt = vis.movement_tracers;
                            ImGui::Checkbox("Local Trail", &mt.local_trail);
                            ImGui::SameLine();
                            ImGui::Checkbox("Enemy Trail", &mt.enemy_trail);
                        }

                        ImGui::EndChild();

                        ImGui::TableSetColumnIndex(1);
                        ImGui::BeginChild("##vis_right", ImVec2(0, 0), true);
                        // Themes selector (affects ESP colors and preview)
                        ImGui::Text("Themes");
                        ImGui::Separator();
                        {
                            static const char* theme_names[] = { "Dark", "Blue", "Red", "Green", "Purple" };
                            int cur = g->core.get_settings().themes.current_theme;
                            if (ImGui::Combo("##ThemeSelect", &cur, theme_names, IM_ARRAYSIZE(theme_names))) {
                                g->core.get_settings().themes.current_theme = cur;
                                this->theme_changed = true; // re-apply UI theme; preview updates immediately via get_theme_color()
                            }
                        }
                        ImGui::Spacing();
                        ImGui::Separator();
                        // FOV changer block
                        ImGui::Text("FOV Changer");
                        ImGui::Separator();
                        ImGui::Checkbox("Enable FOV Changer", &vis.fov_changer.enabled);
                        ImGui::SameLine();
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Externally override the in-game field of view");
                        ImGui::SliderInt("Desired FOV", &vis.fov_changer.desired_fov, 60, 140, "%d");
                        ImGui::Checkbox("Disable while scoped", &vis.fov_changer.disable_while_scoped);
                        ImGui::Checkbox("Reset to default on disable", &vis.fov_changer.reset_on_disable);

                        ImGui::Spacing();
                        ImGui::Separator();
                        ImGui::Spacing();

                        // Radar settings removed: in-game radar reveal is always applied when offsets are available
                        ImGui::Text("World Color");
                        ImGui::ColorEdit3("##world_color", vis.world.world_color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_PickerHueBar);

                        ImGui::Spacing();
                        ImGui::Separator();
                        ImGui::Spacing();

                        // ESP Preview Panel
                        {
                            ImGui::Text("ESP Preview");
                            static bool show_esp_preview = true;
                            ImGui::Checkbox("Show Preview", &show_esp_preview);
                            if (show_esp_preview)
                            {
                                const ImVec2 preview_size(480.0f, 380.0f);
                                if (ImGui::BeginChild("##esp_preview", preview_size, true))
                                {
                                    // Persistent layout settings
                                    auto& lay = g->core.get_settings().visuals.player.layout;
                                    ImGui::TextDisabled("Drag labels inside preview. Configure ESP layout here.");
                                    const char* sides[] = { "Left", "Right", "Top", "Bottom" };
                                    ImGui::Combo("Health Bar Side", &lay.health_side, sides, 4);
                                    ImGui::SliderFloat("Health Bar Width", &lay.health_bar_width, 1.0f, 8.0f, "%.1f");
                                    ImGui::SliderFloat("Health Bar Height", &lay.health_bar_height, 2.0f, 10.0f, "%.1f");
                                    const char* box_styles[] = { "Full Box", "Corner Box" };
                                    ImGui::Combo("Box Style", &lay.box_style, box_styles, 2);
                                    const char* aligns[] = { "Center", "Left", "Right" };
                                    ImGui::Combo("Text Align", &lay.text_align, aligns, 3);
                                    ImGui::SliderFloat("Font Size (0=auto)", &lay.font_size, 0.0f, 24.0f, "%.1f");
                                    ImGui::SameLine();
                                    if (ImGui::Button("Reset Layout")) {
                                        lay.health_side = 0; lay.health_bar_width = 4.0f; lay.health_bar_height = 4.0f; lay.box_style = 0; lay.text_align = 0; lay.font_size = 0.0f;
                                        lay.name_offset[0] = 0.0f; lay.name_offset[1] = 10.0f;
                                        lay.weapon_offset[0] = lay.weapon_offset[1] = 0.0f;
                                        lay.distance_offset[0] = lay.distance_offset[1] = 0.0f;
                                    }
                                    // Reserve a canvas below the controls, draw only inside it
                                    ImGui::Spacing();
                                    ImGui::Separator();
                                    const float canvas_h = 240.0f;
                                    const ImVec2 canvas_size(ImGui::GetContentRegionAvail().x, canvas_h);
                                    const ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
                                    ImGui::InvisibleButton("##esp_canvas", canvas_size, ImGuiButtonFlags_MouseButtonLeft);
                                    ImDrawList* dl = ImGui::GetWindowDrawList();
                                    const ImVec2 cmin = canvas_pos;
                                    const ImVec2 cmax = ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y);
                                    ImGui::PushClipRect(cmin, cmax, true);
                                    // Background of canvas
                                    dl->AddRectFilled(cmin, cmax, IM_COL32(22,22,22,200), 6.0f);
                                    dl->AddRect(cmin, cmax, IM_COL32(45,45,45,255), 6.0f);

                                    // Helper to pick a color
                                    auto get_primary_col = [&](){
                                        // Prefer theme color if feature exposes none
                                        return g->features.get_theme_color();
                                    };

                                    // Mock players
                                    struct MockEnt { ImVec2 top; ImVec2 bottom; int hp; const char* name; const char* weapon; bool enemy; };
                                    // Place mock entities inside canvas
                                    const float margin = 30.0f;
                                    MockEnt ents[] = {
                                        { ImVec2(cmin.x + margin,                   cmin.y + 30.0f), ImVec2(cmin.x + margin,                   cmin.y + canvas_h - 20.0f), 86, "Enemy_1", "AK-47", true },
                                        { ImVec2(cmin.x + canvas_size.x - margin-80, cmin.y + 40.0f), ImVec2(cmin.x + canvas_size.x - margin-80, cmin.y + canvas_h - 25.0f), 34, "Mate_1",  "M4A4",  false },
                                    };

                                    for (int idx = 0; idx < (int)(sizeof(ents)/sizeof(ents[0])); ++idx)
                                    {
                                        const auto& e = ents[idx];
                                        const float h = e.bottom.y - e.top.y;
                                        const float w = h * 0.38f;
                                        const float x0 = e.top.x - w * 0.5f;
                                        const float x1 = e.top.x + w * 0.5f;
                                        const float y0 = e.top.y;
                                        const float y1 = e.bottom.y;

                                        ImU32 box_col = get_primary_col();
                                        ImU32 text_col = IM_COL32(230,230,230,255);
                                        ImFont* font = ImGui::GetFont();
                                        const float fsz = (lay.font_size <= 0.0f) ? ImGui::GetFontSize() : lay.font_size;

                                        // Box
                                        if (vis.player.box)
                                        {
                                            if (lay.box_style == 0) {
                                                dl->AddRect(ImVec2(x0, y0), ImVec2(x1, y1), box_col, 0.0f, 0, 1.5f);
                                            } else {
                                                const float len = (x1 - x0) * 0.25f;
                                                // corners: TL
                                                dl->AddLine(ImVec2(x0, y0), ImVec2(x0 + len, y0), box_col, 1.5f);
                                                dl->AddLine(ImVec2(x0, y0), ImVec2(x0, y0 + len), box_col, 1.5f);
                                                // TR
                                                dl->AddLine(ImVec2(x1, y0), ImVec2(x1 - len, y0), box_col, 1.5f);
                                                dl->AddLine(ImVec2(x1, y0), ImVec2(x1, y0 + len), box_col, 1.5f);
                                                // BL
                                                dl->AddLine(ImVec2(x0, y1), ImVec2(x0 + len, y1), box_col, 1.5f);
                                                dl->AddLine(ImVec2(x0, y1), ImVec2(x0, y1 - len), box_col, 1.5f);
                                                // BR
                                                dl->AddLine(ImVec2(x1, y1), ImVec2(x1 - len, y1), box_col, 1.5f);
                                                dl->AddLine(ImVec2(x1, y1), ImVec2(x1, y1 - len), box_col, 1.5f);
                                            }
                                        }

                                        // Health bar (side configurable)
                                        if (vis.player.health)
                                        {
                                            const float pad = 2.0f;
                                            const float hb_w = lay.health_bar_width;
                                            const float hb_h = y1 - y0 - pad*2.0f;
                                            const float frac = std::max(0.0f, std::min(1.0f, e.hp / 100.0f));
                                            const ImU32 hb_col = IM_COL32(60, 200, 60, 230);
                                            if (lay.health_side == 0) { // Left
                                                const ImVec2 hb_min(x0 - hb_w - 3.0f, y0 + pad);
                                                const ImVec2 hb_max(x0 - 3.0f, y1 - pad);
                                                const float filled = (hb_max.y - hb_min.y) * frac;
                                                dl->AddRect(hb_min, hb_max, IM_COL32(0,0,0,120));
                                                dl->AddRectFilled(ImVec2(hb_min.x+1, hb_max.y - filled), ImVec2(hb_max.x-1, hb_max.y-1), hb_col);
                                            } else if (lay.health_side == 1) { // Right
                                                const ImVec2 hb_min(x1 + 3.0f, y0 + pad);
                                                const ImVec2 hb_max(x1 + 3.0f + hb_w, y1 - pad);
                                                const float filled = (hb_max.y - hb_min.y) * frac;
                                                dl->AddRect(hb_min, hb_max, IM_COL32(0,0,0,120));
                                                dl->AddRectFilled(ImVec2(hb_min.x+1, hb_max.y - filled), ImVec2(hb_max.x-1, hb_max.y-1), hb_col);
                                            } else if (lay.health_side == 2) { // Top
                                                const float hb_h2 = lay.health_bar_height;
                                                const ImVec2 hb_min(x0, y0 - hb_h2 - 3.0f);
                                                const ImVec2 hb_max(x1, y0 - 3.0f);
                                                const float filledw = (hb_max.x - hb_min.x) * frac;
                                                dl->AddRect(hb_min, hb_max, IM_COL32(0,0,0,120));
                                                dl->AddRectFilled(hb_min, ImVec2(hb_min.x + filledw, hb_max.y), hb_col);
                                            } else { // Bottom
                                                const float hb_h2 = lay.health_bar_height;
                                                const ImVec2 hb_min(x0, y1 + 3.0f);
                                                const ImVec2 hb_max(x1, y1 + 3.0f + hb_h2);
                                                const float filledw = (hb_max.x - hb_min.x) * frac;
                                                dl->AddRect(hb_min, hb_max, IM_COL32(0,0,0,120));
                                                dl->AddRectFilled(hb_min, ImVec2(hb_min.x + filledw, hb_max.y), hb_col);
                                            }
                                        }

                                        // Name (draggable)
                                        if (vis.player.name)
                                        {
                                            const ImVec2 sz = ImGui::CalcTextSize(e.name);
                                            float nx = e.top.x - (lay.text_align == 0 ? sz.x * 0.5f : (lay.text_align == 1 ? 0.0f : sz.x));
                                            ImVec2 pos = ImVec2(nx, y0 - sz.y - 2.0f);
                                            pos.x += lay.name_offset[0]; pos.y += lay.name_offset[1];
                                            dl->AddText(font, fsz, pos, text_col, e.name);
                                            // Drag handle
                                            ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y));
                                            if (ImGui::InvisibleButton((std::string("name_drag_")+std::to_string(idx)).c_str(), ImVec2(sz.x, sz.y))) {}
                                            if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) { lay.name_offset[0] += ImGui::GetIO().MouseDelta.x; lay.name_offset[1] += ImGui::GetIO().MouseDelta.y; }
                                        }

                                        // Weapon (draggable)
                                        if (vis.player.weapon)
                                        {
                                            const ImVec2 wsz = ImGui::CalcTextSize(e.weapon);
                                            float wx = e.top.x - (lay.text_align == 0 ? wsz.x * 0.5f : (lay.text_align == 1 ? 0.0f : wsz.x));
                                            ImVec2 pos = ImVec2(wx, y1 + 2.0f);
                                            pos.x += lay.weapon_offset[0]; pos.y += lay.weapon_offset[1];
                                            dl->AddText(font, fsz, pos, text_col, e.weapon);
                                            ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y));
                                            if (ImGui::InvisibleButton((std::string("weap_drag_")+std::to_string(idx)).c_str(), ImVec2(wsz.x, wsz.y))) {}
                                            if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) { lay.weapon_offset[0] += ImGui::GetIO().MouseDelta.x; lay.weapon_offset[1] += ImGui::GetIO().MouseDelta.y; }
                                        }

                                        // Distance (draggable, mock)
                                        if (vis.player.distance)
                                        {
                                            const char* dist = e.enemy ? "23m" : "8m";
                                            const ImVec2 dsz = ImGui::CalcTextSize(dist);
                                            ImVec2 pos = ImVec2(x1 + 6.0f, (y0 + y1) * 0.5f - dsz.y * 0.5f);
                                            pos.x += lay.distance_offset[0]; pos.y += lay.distance_offset[1];
                                            dl->AddText(font, fsz, pos, text_col, dist);
                                            ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y));
                                            if (ImGui::InvisibleButton((std::string("dist_drag_")+std::to_string(idx)).c_str(), ImVec2(dsz.x, dsz.y))) {}
                                            if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) { lay.distance_offset[0] += ImGui::GetIO().MouseDelta.x; lay.distance_offset[1] += ImGui::GetIO().MouseDelta.y; }
                                        }

                                        // Skeleton (simple stick figure inside bbox)
                                        if (vis.player.skeleton)
                                        {
                                            const float cx = e.top.x;
                                            const float head = y0 + h * 0.10f;
                                            const float chest = y0 + h * 0.25f;
                                            const float pelvis = y0 + h * 0.45f;
                                            const float knee_l = y0 + h * 0.70f;
                                            const float knee_r = knee_l;
                                            const float foot_l = y1 - 4.0f;
                                            const float foot_r = foot_l;

                                            const float arm_span = w * 0.6f;
                                            // Head circle
                                            dl->AddCircle(ImVec2(cx, head), 4.0f, box_col, 12, 1.2f);
                                            // Spine
                                            dl->AddLine(ImVec2(cx, head+4.0f), ImVec2(cx, pelvis), box_col, 1.2f);
                                            // Arms
                                            dl->AddLine(ImVec2(cx - arm_span*0.5f, chest), ImVec2(cx + arm_span*0.5f, chest), box_col, 1.2f);
                                            // Legs
                                            dl->AddLine(ImVec2(cx, pelvis), ImVec2(cx - arm_span*0.25f, knee_l), box_col, 1.2f);
                                            dl->AddLine(ImVec2(cx, pelvis), ImVec2(cx + arm_span*0.25f, knee_r), box_col, 1.2f);
                                            dl->AddLine(ImVec2(cx - arm_span*0.25f, knee_l), ImVec2(cx - arm_span*0.25f, foot_l), box_col, 1.2f);
                                            dl->AddLine(ImVec2(cx + arm_span*0.25f, knee_r), ImVec2(cx + arm_span*0.25f, foot_r), box_col, 1.2f);
                                        }

                                        // Flags example (scoped/defusing/ping/lag) if enabled
                                        if (vis.player.flags)
                                        {
                                            ImVec2 cur(x1 + 8.0f, y0);
                                            const ImU32 fcol = ImGui::GetColorU32(ImVec4(vis.player.flag_color[0], vis.player.flag_color[1], vis.player.flag_color[2], 1.0f));
                                            const float fsz = (lay.font_size > 0.0f)
                                                ? lay.font_size
                                                : ((vis.player.flag_text_size <= 0.0f) ? ImGui::GetFontSize() : vis.player.flag_text_size);
                                            auto draw_flag = [&](const char* txt){ dl->AddText(ImGui::GetFont(), fsz, cur, fcol, txt); cur.y += fsz + 2.0f; };
                                            if (vis.player.flags_mask & (1<<0)) draw_flag("FLASHED");
                                            if (vis.player.flags_mask & (1<<1)) draw_flag("SCOPED");
                                            if (vis.player.flags_mask & (1<<2)) draw_flag("DEFUSING");
                                            if (vis.player.flags_mask & (1<<3)) draw_flag("200ms");
                                            if (vis.player.flags_mask & (1<<4)) draw_flag("LAG");
                                        }
                                    }
                                    ImGui::PopClipRect();
                                }
                                ImGui::EndChild();
                            }
                        }

                        ImGui::EndChild();
                        ImGui::EndTable();
                    }
                }
                // TRIGGERBOT SECTION (new)
                else if (selected_section == 6)
                {
                    ImGui::Spacing();
                    ImGui::Text("Triggerbot");
                    ImGui::Separator();
                    auto& tb = g->core.get_settings().triggerbot;
                    ImGui::Checkbox("Enable Triggerbot", &tb.enabled);
                    ImGui::BeginDisabled(!tb.enabled);
                    // Hotkey bind
                    ImGui::Text("Trigger Key: %s", this->get_key_name(tb.hotkey));
                    ImGui::SameLine();
                    if (ImGui::Button("Set##TriggerKey")) {
                        g->core.get_settings().hotkeys.setting_key = true;
                        g->core.get_settings().hotkeys.current_setting = &tb.hotkey;
                        this->show_notification("Press a key for Triggerbot");
                    }
                    // Delays
                    ImGui::SliderInt("Reaction Time (ms)", &tb.first_shot_delay_ms, 0, 300, "%d ms");
                    ImGui::SliderInt("Time Between Shots (ms)", &tb.delay_between_shots_ms, 0, 500, "%d ms");
                    ImGui::EndDisabled();
                }
                // MISC SECTION
                else if (selected_section == 3)
                {
                    auto& misc = g->core.get_settings().misc;

                    if (ImGui::BeginTable("##misc_grid", 2, ImGuiTableFlags_SizingStretchProp))
                    {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::BeginChild("##misc_left", ImVec2(0, 0), true);
                        ImGui::Text("General");
                        ImGui::Separator();
                        ImGui::Checkbox("VSync", &misc.vsync);
                        ImGui::Checkbox("Watermark", &misc.watermark);

                        ImGui::Spacing();
                        ImGui::Separator();
                        ImGui::Spacing();

                        ImGui::Text("Movement");
                        ImGui::Separator();

                        ImGui::Spacing();
                        ImGui::Separator();
                        ImGui::Spacing();

                        ImGui::Text("View");
                        ImGui::Separator();
                        ImGui::Checkbox("Third Person", &misc.third_person);
                        ImGui::SameLine();
                        {
                            auto& hk = g->core.get_settings().hotkeys;
                            ImGui::TextDisabled("Key: %s", this->get_key_name(hk.third_person));
                            ImGui::SameLine();
                            if (ImGui::Button("Set##ThirdPerson")) { hk.setting_key = true; hk.current_setting = &hk.third_person; this->show_notification("Press a key for Third Person"); }
                        }

                        ImGui::EndChild();

                        ImGui::TableSetColumnIndex(1);
                        ImGui::BeginChild("##misc_right", ImVec2(0, 0), true);
                        ImGui::Text("Visibility/FX");
                        ImGui::Separator();
                        ImGui::Checkbox("No Flash", &misc.no_flash);
                        ImGui::SameLine();
                        {
                            auto& hk = g->core.get_settings().hotkeys;
                            ImGui::TextDisabled("Key: %s", this->get_key_name(hk.no_flash));
                            ImGui::SameLine();
                            if (ImGui::Button("Set##NoFlash")) { hk.setting_key = true; hk.current_setting = &hk.no_flash; this->show_notification("Press a key for No Flash"); }
                        }
                        ImGui::SliderFloat("No Flash Alpha", &misc.no_flash_alpha, 0.0f, 1.0f, "%.2f");
                        ImGui::Checkbox("Night Mode", &misc.night_mode);
                        ImGui::SameLine();
                        {
                            auto& hk = g->core.get_settings().hotkeys;
                            ImGui::TextDisabled("Key: %s", this->get_key_name(hk.night_mode));
                            ImGui::SameLine();
                            if (ImGui::Button("Set##NightMode")) { hk.setting_key = true; hk.current_setting = &hk.night_mode; this->show_notification("Press a key for Night Mode"); }
                        }
                        ImGui::SliderFloat("Night Mode Strength", &misc.night_mode_strength, 0.0f, 2.0f, "%.2f");

                        ImGui::Spacing();
                        ImGui::Separator();
                        ImGui::Spacing();

                        ImGui::Text("Crosshair Overlay");
                        ImGui::Separator();
                        ImGui::Checkbox("Enable Crosshair Overlay", &misc.crosshair_overlay);
                        ImGui::SameLine();
                        {
                            auto& hk = g->core.get_settings().hotkeys;
                            ImGui::TextDisabled("Key: %s", this->get_key_name(hk.crosshair_overlay));
                            ImGui::SameLine();
                            if (ImGui::Button("Set##CrosshairOverlay")) { hk.setting_key = true; hk.current_setting = &hk.crosshair_overlay; this->show_notification("Press a key for Crosshair Overlay"); }
                        }
                        ImGui::BeginDisabled(!misc.crosshair_overlay);
                        ImGui::SliderFloat("Size", &misc.crosshair_size, 2.0f, 50.0f, "%.1f");
                        ImGui::SliderFloat("Thickness", &misc.crosshair_thickness, 1.0f, 10.0f, "%.1f");
                        ImGui::Checkbox("Use Theme Color", &misc.crosshair_use_theme_color);
                        ImGui::BeginDisabled(misc.crosshair_use_theme_color);
                        ImGui::ColorEdit3("Color", misc.crosshair_color, ImGuiColorEditFlags_NoInputs);
                        ImGui::EndDisabled();

                        // Crosshair preview
                        ImGui::Spacing();
                        ImGui::Text("Preview");
                        const ImVec2 preview_size(220.0f, 140.0f);
                        if (ImGui::BeginChild("##crosshair_preview", preview_size, true))
                        {
                            ImDrawList* dl = ImGui::GetWindowDrawList();
                            const ImVec2 wpos = ImGui::GetWindowPos();
                            const ImVec2 wsize = ImGui::GetWindowSize();
                            const ImVec2 center(wpos.x + wsize.x * 0.5f, wpos.y + wsize.y * 0.5f);

                            // Background grid for visibility
                            const ImU32 bg_col = IM_COL32(25,25,25,180);
                            dl->AddRectFilled(wpos, ImVec2(wpos.x + wsize.x, wpos.y + wsize.y), bg_col, 6.0f);
                            dl->AddRect(wpos, ImVec2(wpos.x + wsize.x, wpos.y + wsize.y), IM_COL32(45,45,45,255), 6.0f);

                            const float arm = misc.crosshair_size;
                            const float thick = misc.crosshair_thickness;
                            const float gap = (arm * 0.25f < 2.0f) ? 2.0f : ((arm * 0.25f > 10.0f) ? 10.0f : arm * 0.25f);

                            ImU32 col;
                            if (misc.crosshair_use_theme_color)
                                col = g->features.get_theme_color();
                            else
                                col = ImGui::GetColorU32(ImVec4(misc.crosshair_color[0], misc.crosshair_color[1], misc.crosshair_color[2], 1.0f));

                            // Horizontal lines
                            dl->AddLine(ImVec2(center.x - gap - arm, center.y), ImVec2(center.x - gap, center.y), col, thick);
                            dl->AddLine(ImVec2(center.x + gap, center.y), ImVec2(center.x + gap + arm, center.y), col, thick);
                            // Vertical lines
                            dl->AddLine(ImVec2(center.x, center.y - gap - arm), ImVec2(center.x, center.y - gap), col, thick);
                            dl->AddLine(ImVec2(center.x, center.y + gap), ImVec2(center.x, center.y + gap + arm), col, thick);
                        }
                        ImGui::EndChild();
                        ImGui::EndDisabled();

                        ImGui::Spacing();
                        ImGui::Separator();
                        ImGui::Spacing();

                        ImGui::Text("Hitmarker");
                        ImGui::Separator();
                        ImGui::Checkbox("Enable Hitmarker Sound", &misc.hitmarker);
                        ImGui::BeginDisabled(!misc.hitmarker);
                        ImGui::SliderInt("Volume", &misc.hitmarker_volume, 0, 100, "%d%%");
                        ImGui::InputText("Sound Path", misc.hitmarker_sound_path, IM_ARRAYSIZE(misc.hitmarker_sound_path));
                        ImGui::SameLine();
                        if (ImGui::Button("Browse...")) {
                            OPENFILENAMEA ofn{};
                            CHAR file[ MAX_PATH ]{};
                            ofn.lStructSize = sizeof(ofn);
                            ofn.hwndOwner = nullptr;
                            ofn.lpstrFilter = "WAV Files\0*.wav\0All Files\0*.*\0";
                            ofn.lpstrFile = file;
                            ofn.nMaxFile = MAX_PATH;
                            ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
                            ofn.lpstrDefExt = "wav";
                            if ( GetOpenFileNameA(&ofn) ) {
                                strncpy_s(misc.hitmarker_sound_path, file, _TRUNCATE);
                            }
                        }
                        ImGui::EndDisabled();

                        ImGui::EndChild();
                        ImGui::EndTable();
                    }
                }
                // WORLD SECTION (dedicated)
                else if (selected_section == 2)
                {
                    auto& vis = g->core.get_settings().visuals;
                    if (ImGui::BeginTable("##world_grid", 2, ImGuiTableFlags_SizingStretchProp))
                    {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::BeginChild("##world_left", ImVec2(0, 0), true);
                        ImGui::Text("World");
                        ImGui::Separator();
                        {
                            auto& w = vis.world;
                            ImGui::Checkbox("Smoke", &w.smoke);
                            ImGui::SameLine();
                            ImGui::Checkbox("Smoke Dome", &w.smoke_dome);

                            ImGui::Checkbox("Molotov", &w.molotov);
                            ImGui::SameLine();
                            ImGui::Checkbox("Molotov Bounds", &w.molotov_bounds);

                            ImGui::Checkbox("Drops", &w.drops);
                            ImGui::SameLine();
                            ImGui::Checkbox("Drops Bounds", &w.drops_bounds);

                            ImGui::Checkbox("Bomb", &w.bomb);
                            ImGui::Checkbox("Bomb Timer (World)", &w.bomb_timer_world);
                            ImGui::SameLine();
                            ImGui::Checkbox("Bomb Timer (HUD)", &w.bomb_timer_hud);
                            ImGui::ColorEdit3("Bomb Timer Color", w.bomb_timer_color, ImGuiColorEditFlags_NoInputs);
                            ImGui::SliderFloat("Bomb Timer Duration (s)", &w.bomb_timer_duration, 5.0f, 60.0f, "%.0f s");

                            ImGui::Spacing();
                            ImGui::Separator();
                            ImGui::Spacing();

                            ImGui::Text("Notifications");
                            ImGui::Checkbox("On Hit", &w.notify_hit);
                            ImGui::SameLine();
                            ImGui::Checkbox("On Kill", &w.notify_kill);
                            ImGui::Checkbox("On Bomb Plant", &w.notify_bomb_plant);
                        }
                        ImGui::EndChild();

                        ImGui::TableSetColumnIndex(1);
                        ImGui::BeginChild("##world_right", ImVec2(0, 0), true);
                        ImGui::Text("Viewmodel Changer");
                        ImGui::Separator();
                        {
                            auto& vm = vis.world.viewmodel_changer;
                            ImGui::Checkbox("Enable Viewmodel Changer", &vm.enabled);
                            ImGui::BeginDisabled(!vm.enabled);
                            ImGui::SliderFloat("Offset X", &vm.offset_x, -10.0f, 5.0f, "%.2f");
                            ImGui::SliderFloat("Offset Y", &vm.offset_y, -10.0f, 10.0f, "%.2f");
                            ImGui::SliderFloat("Offset Z", &vm.offset_z, -5.0f, 5.0f, "%.2f");
                            ImGui::SliderFloat("FOV Offset", &vm.fov_offset, -40.0f, 40.0f, "%.1f");
                            ImGui::EndDisabled();
                        }
                        ImGui::EndChild();

                        ImGui::EndTable();
                    }
                }
                // EXTRAS SECTION
                else if (selected_section == 4)
                {
                    auto& ex = g->core.get_settings().extras;

                    if (ImGui::BeginTable("##extras_grid", 2, ImGuiTableFlags_SizingStretchProp))
                    {
                        ImGui::TableNextRow();
                        ImGui::TableSetColumnIndex(0);
                        ImGui::BeginChild("##extras_left", ImVec2(0, 0), true);
                        ImGui::Text("Extras");
                        ImGui::Separator();
                        // FPS Counter
                        ImGui::Checkbox("Show FPS Counter", &ex.fps_counter);
                        ImGui::SameLine();
                        {
                            auto& hk = g->core.get_settings().hotkeys;
                            ImGui::TextDisabled("Key: %s", this->get_key_name(hk.fps_counter));
                            ImGui::SameLine();
                            if (ImGui::Button("Set##FpsCounterHotkey")) { hk.setting_key = true; hk.current_setting = &hk.fps_counter; this->show_notification("Press a key for FPS Counter"); }
                        }
                        // Ping Display
                        ImGui::Checkbox("Show Ping Display", &ex.ping_display);
                        ImGui::SameLine();
                        {
                            auto& hk = g->core.get_settings().hotkeys;
                            ImGui::TextDisabled("Key: %s", this->get_key_name(hk.ping_display));
                            ImGui::SameLine();
                            if (ImGui::Button("Set##PingDisplayHotkey")) { hk.setting_key = true; hk.current_setting = &hk.ping_display; this->show_notification("Press a key for Ping Display"); }
                        }
                        ImGui::EndChild();

                        ImGui::TableSetColumnIndex(1);
                        ImGui::BeginChild("##extras_right", ImVec2(0, 0), true);
                        ImGui::Text("Capture/Performance");
                        ImGui::Separator();
                        // Stream Proof
                        ImGui::Checkbox("Stream Proof", &ex.stream_proof);
                        ImGui::SameLine();
                        {
                            auto& hk = g->core.get_settings().hotkeys;
                            ImGui::TextDisabled("Key: %s", this->get_key_name(hk.stream_proof));
                            ImGui::SameLine();
                            if (ImGui::Button("Set##StreamProofHotkey")) { hk.setting_key = true; hk.current_setting = &hk.stream_proof; this->show_notification("Press a key for Stream Proof"); }
                        }
                        // FPS Cap
                        ImGui::Checkbox("Enable FPS Cap", &ex.fps_cap_enabled);
                        ImGui::BeginDisabled(!ex.fps_cap_enabled);
                        ImGui::SliderInt("FPS Cap Limit", &ex.fps_cap_limit, 30, 300, "%d");
                        ImGui::EndDisabled();
                        ImGui::EndChild();

                        ImGui::EndTable();
                    }
                }
                // CONFIG SECTION
                else if (selected_section == 5)
                {
                    auto& cfg = g->core.get_settings().config;

                    ImGui::Text("Configuration");
                    ImGui::Separator();
                    ImGui::Checkbox("Auto Save", &cfg.auto_save);
                    ImGui::SameLine();
                    {
                        auto& hk = g->core.get_settings().hotkeys;
                        ImGui::TextDisabled("Menu Toggle: %s", this->get_key_name(hk.menu_toggle));
                        ImGui::SameLine();
                        if (ImGui::Button("Set##MenuToggle")) { hk.setting_key = true; hk.current_setting = &hk.menu_toggle; this->show_notification("Press a key for Menu Toggle"); }
                    }
                    ImGui::InputText("Config Name", cfg.config_name, IM_ARRAYSIZE(cfg.config_name));

                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();

                    if (ImGui::Button("Save")) {
                        g->core.get_settings().save_config(std::string(cfg.config_name) + ".cfg");
                        this->show_notification("Config saved");
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Load")) {
                        g->core.get_settings().load_config(std::string(cfg.config_name) + ".cfg");
                        this->show_notification("Config loaded");
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Import...")) {
                        OPENFILENAMEA ofn{};
                        CHAR file[MAX_PATH]{};
                        ofn.lStructSize = sizeof(ofn);
                        ofn.hwndOwner = nullptr;
                        ofn.lpstrFilter = "Config Files\0*.cfg\0All Files\0*.*\0";
                        ofn.lpstrFile = file;
                        ofn.nMaxFile = MAX_PATH;
                        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
                        ofn.lpstrDefExt = "cfg";
                        if ( GetOpenFileNameA(&ofn) ) {
                            // Update config name from filename (without extension)
                            std::string full(file);
                            auto pos_slash = full.find_last_of("/\\");
                            std::string fname = (pos_slash == std::string::npos) ? full : full.substr(pos_slash + 1);
                            auto pos_dot = fname.find_last_of('.');
                            std::string stem = (pos_dot == std::string::npos) ? fname : fname.substr(0, pos_dot);
                            strncpy_s(cfg.config_name, stem.c_str(), _TRUNCATE);
                            // Load the selected file directly
                            g->core.get_settings().load_config(full);
                            this->show_notification("Config imported and loaded");
                        }
                    }
                }
                else
                {
                    ImGui::Spacing();
                    ImGui::Text("This section will be migrated to the new layout next.");
                }
            }
            ImGui::EndChild();
        }
        

        // Modern footer with gradient background
        const auto footer_window_size = ImGui::GetWindowSize();
        const char* creator_text = "Created by: Tomatensxsse";
        const char* version_text = "v3.4";
        
        ImGui::Separator();
        ImGui::Spacing();
        
        // Footer with better styling
        const auto footer_height = 25.0f;
        const auto footer_pos = ImVec2(0, ImGui::GetCursorPosY());
        
        ImGui::GetWindowDrawList()->AddRectFilledMultiColor(
            ImVec2(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y + footer_pos.y),
            ImVec2(ImGui::GetWindowPos().x + footer_window_size.x, ImGui::GetWindowPos().y + footer_pos.y + footer_height),
            IM_COL32(40, 40, 40, 100), IM_COL32(60, 60, 60, 100),
            IM_COL32(60, 60, 60, 100), IM_COL32(40, 40, 40, 100)
        );
        
        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), creator_text);
        ImGui::SameLine();
        ImGui::SetCursorPosX(footer_window_size.x - ImGui::CalcTextSize(version_text).x - 20);
        ImGui::TextColored(ImVec4(0.6f, 0.9f, 1.0f, 1.0f), version_text);

        		ImGui::End( );
		} catch (const std::exception& e) {
			// Handle ImGui rendering errors
			this->show_notification("UI Error: " + std::string(e.what()));
			ImGui::End(); // Ensure ImGui state is clean
		} catch (...) {
			// Handle unknown rendering errors
			this->show_notification("Unknown UI error occurred");
			ImGui::End(); // Ensure ImGui state is clean
		}
	}

	} catch (const std::exception& e) {
		// Handle any other errors in the main run function
		if (this->is_visible) {
			this->show_notification("Critical error: " + std::string(e.what()));
		}
	} catch (...) {
		// Handle unknown errors
		if (this->is_visible) {
			this->show_notification("Critical unknown error occurred");
		}
	}
}

void menu_c::style( ) const
{
	auto& style = ImGui::GetStyle( );

	const auto black = ImVec4( 0.08f, 0.08f, 0.08f, 1.0f );
	const auto dark_gray = ImVec4( 0.12f, 0.12f, 0.12f, 1.0f );
	const auto mid_gray = ImVec4( 0.18f, 0.18f, 0.18f, 1.0f );
	const auto light_gray = ImVec4( 0.25f, 0.25f, 0.25f, 1.0f );
	const auto text = ImVec4( 0.90f, 0.90f, 0.90f, 1.0f );
	const auto border = ImVec4( 0.30f, 0.30f, 0.30f, 1.0f );
	const auto accent = ImVec4( 160.0f / 255.0f, 200.0f / 255.0f, 255.0f / 255.0f, 1.0f );
	const auto accent_dim = ImVec4( accent.x * 0.6f, accent.y * 0.6f, accent.z * 0.6f, 1.0f );

	const auto rounding = 0.0f;

	style.AntiAliasedLines = true;
	style.AntiAliasedLinesUseTex = true;
	style.AntiAliasedFill = true;

	style.WindowRounding = 8.0f;
	style.ChildRounding = 6.0f;
	style.FrameRounding = 4.0f;
	style.PopupRounding = 6.0f;
	style.ScrollbarRounding = 8.0f;
	style.GrabRounding = 4.0f;
	style.TabRounding = 4.0f;

	style.FrameBorderSize = 1.0f;
	style.WindowBorderSize = 1.0f;
	style.ChildBorderSize = 1.0f;
	style.PopupBorderSize = 1.0f;
	style.TabBorderSize = 1.0f;

	style.WindowPadding = ImVec2( 12.0f, 12.0f );
	style.ItemSpacing = ImVec2( 10.0f, 6.0f );
	style.ItemInnerSpacing = ImVec2( 6.0f, 6.0f );
	style.FramePadding = ImVec2( 10.0f, 6.0f );
	style.IndentSpacing = 12.0f;
	style.ScrollbarSize = 12.0f;
	style.GrabMinSize = 8.0f;

	style.Colors[ ImGuiCol_WindowBg ] = black;
	style.Colors[ ImGuiCol_ChildBg ] = ImVec4( 0.06f, 0.06f, 0.06f, 1.0f );
	style.Colors[ ImGuiCol_PopupBg ] = dark_gray;

	style.Colors[ ImGuiCol_Border ] = border;
	style.Colors[ ImGuiCol_BorderShadow ] = ImVec4( 0.0f, 0.0f, 0.0f, 0.0f );

	style.Colors[ ImGuiCol_FrameBg ] = mid_gray;
	style.Colors[ ImGuiCol_FrameBgHovered ] = ImVec4( accent.x, accent.y, accent.z, 0.15f );
	style.Colors[ ImGuiCol_FrameBgActive ] = ImVec4( accent.x, accent.y, accent.z, 0.25f );

	style.Colors[ ImGuiCol_Text ] = text;
	style.Colors[ ImGuiCol_TextDisabled ] = ImVec4( 0.50f, 0.50f, 0.50f, 1.0f );
	style.Colors[ ImGuiCol_TextSelectedBg ] = ImVec4( accent.x, accent.y, accent.z, 0.30f );

	style.Colors[ ImGuiCol_TitleBg ] = ImVec4( 0.06f, 0.06f, 0.06f, 1.0f );
	style.Colors[ ImGuiCol_TitleBgActive ] = ImVec4( 0.10f, 0.10f, 0.10f, 1.0f );
	style.Colors[ ImGuiCol_TitleBgCollapsed ] = ImVec4( 0.06f, 0.06f, 0.06f, 1.0f );

	style.Colors[ ImGuiCol_MenuBarBg ] = dark_gray;

	style.Colors[ ImGuiCol_ScrollbarBg ] = ImVec4( 0.05f, 0.05f, 0.05f, 1.0f );
	style.Colors[ ImGuiCol_ScrollbarGrab ] = ImVec4( 0.30f, 0.30f, 0.30f, 1.0f );
	style.Colors[ ImGuiCol_ScrollbarGrabHovered ] = ImVec4( accent.x * 0.7f, accent.y * 0.7f, accent.z * 0.7f, 1.0f );
	style.Colors[ ImGuiCol_ScrollbarGrabActive ] = accent;

	style.Colors[ ImGuiCol_Button ] = light_gray;
	style.Colors[ ImGuiCol_ButtonHovered ] = ImVec4( accent.x, accent.y, accent.z, 0.20f );
	style.Colors[ ImGuiCol_ButtonActive ] = ImVec4( accent.x, accent.y, accent.z, 0.35f );

	style.Colors[ ImGuiCol_Header ] = ImVec4( 0.20f, 0.20f, 0.20f, 1.0f );
	style.Colors[ ImGuiCol_HeaderHovered ] = ImVec4( accent.x, accent.y, accent.z, 0.15f );
	style.Colors[ ImGuiCol_HeaderActive ] = ImVec4( accent.x, accent.y, accent.z, 0.25f );

	style.Colors[ ImGuiCol_Separator ] = ImVec4( 0.35f, 0.35f, 0.35f, 1.0f );
	style.Colors[ ImGuiCol_SeparatorHovered ] = accent_dim;
	style.Colors[ ImGuiCol_SeparatorActive ] = accent;

	style.Colors[ ImGuiCol_ResizeGrip ] = ImVec4( 0.25f, 0.25f, 0.25f, 0.20f );
	style.Colors[ ImGuiCol_ResizeGripHovered ] = ImVec4( accent.x, accent.y, accent.z, 0.40f );
	style.Colors[ ImGuiCol_ResizeGripActive ] = ImVec4( accent.x, accent.y, accent.z, 0.60f );

	style.Colors[ ImGuiCol_CheckMark ] = accent;
	style.Colors[ ImGuiCol_SliderGrab ] = ImVec4( 0.50f, 0.50f, 0.50f, 1.0f );
	style.Colors[ ImGuiCol_SliderGrabActive ] = accent;

	style.Colors[ ImGuiCol_Tab ] = ImVec4( 0.15f, 0.15f, 0.15f, 1.0f );
	style.Colors[ ImGuiCol_TabHovered ] = ImVec4( accent.x, accent.y, accent.z, 0.20f );
	style.Colors[ ImGuiCol_TabActive ] = ImVec4( accent.x, accent.y, accent.z, 0.15f );
	style.Colors[ ImGuiCol_TabUnfocused ] = ImVec4( 0.10f, 0.10f, 0.10f, 1.0f );
	style.Colors[ ImGuiCol_TabUnfocusedActive ] = ImVec4( 0.18f, 0.18f, 0.18f, 1.0f );

	style.Colors[ ImGuiCol_PlotLines ] = accent;
	style.Colors[ ImGuiCol_PlotLinesHovered ] = ImVec4( accent.x * 1.2f, accent.y * 1.2f, accent.z * 1.2f, 1.0f );
	style.Colors[ ImGuiCol_PlotHistogram ] = accent_dim;
	style.Colors[ ImGuiCol_PlotHistogramHovered ] = accent;

	style.Colors[ ImGuiCol_NavHighlight ] = accent;
	style.Colors[ ImGuiCol_NavWindowingHighlight ] = ImVec4( 1.0f, 1.0f, 1.0f, 0.70f );
	style.Colors[ ImGuiCol_NavWindowingDimBg ] = ImVec4( 0.0f, 0.0f, 0.0f, 0.60f );
	style.Colors[ ImGuiCol_ModalWindowDimBg ] = ImVec4( 0.0f, 0.0f, 0.0f, 0.70f );

	style.Colors[ ImGuiCol_TableHeaderBg ] = ImVec4( 0.15f, 0.15f, 0.15f, 1.0f );
	style.Colors[ ImGuiCol_TableBorderStrong ] = ImVec4( 0.35f, 0.35f, 0.35f, 1.0f );
	style.Colors[ ImGuiCol_TableBorderLight ] = ImVec4( 0.25f, 0.25f, 0.25f, 1.0f );
	style.Colors[ ImGuiCol_TableRowBg ] = ImVec4( 0.0f, 0.0f, 0.0f, 0.0f );
	style.Colors[ ImGuiCol_TableRowBgAlt ] = ImVec4( 1.0f, 1.0f, 1.0f, 0.03f );

	style.Colors[ ImGuiCol_DragDropTarget ] = ImVec4( accent.x, accent.y, accent.z, 0.90f );
}

void menu_c::apply_theme( int theme_id )
{
	auto& style = ImGui::GetStyle( );

	ImVec4 accent;
	switch ( theme_id )
	{
	case 0: // Dark
		accent = ImVec4( 160.0f / 255.0f, 200.0f / 255.0f, 255.0f / 255.0f, 1.0f );
		break;
	case 1: // Blue
		accent = ImVec4( 0.2f, 0.6f, 1.0f, 1.0f );
		break;
	case 2: // Red
		accent = ImVec4( 1.0f, 0.3f, 0.3f, 1.0f );
		break;
	case 3: // Green
		accent = ImVec4( 0.3f, 1.0f, 0.3f, 1.0f );
		break;
	case 4: // Purple
		accent = ImVec4( 0.8f, 0.3f, 1.0f, 1.0f );
		break;
	default:
		accent = ImVec4( 160.0f / 255.0f, 200.0f / 255.0f, 255.0f / 255.0f, 1.0f );
	}

	this->style( );

	const auto accent_dim = ImVec4( accent.x * 0.6f, accent.y * 0.6f, accent.z * 0.6f, 1.0f );

	style.Colors[ ImGuiCol_FrameBgHovered ] = ImVec4( accent.x, accent.y, accent.z, 0.15f );
	style.Colors[ ImGuiCol_FrameBgActive ] = ImVec4( accent.x, accent.y, accent.z, 0.25f );
	style.Colors[ ImGuiCol_TextSelectedBg ] = ImVec4( accent.x, accent.y, accent.z, 0.30f );
	style.Colors[ ImGuiCol_ScrollbarGrabHovered ] = ImVec4( accent.x * 0.7f, accent.y * 0.7f, accent.z * 0.7f, 1.0f );
	style.Colors[ ImGuiCol_ScrollbarGrabActive ] = accent;
	style.Colors[ ImGuiCol_ButtonHovered ] = ImVec4( accent.x, accent.y, accent.z, 0.20f );
	style.Colors[ ImGuiCol_ButtonActive ] = ImVec4( accent.x, accent.y, accent.z, 0.35f );
	style.Colors[ ImGuiCol_HeaderHovered ] = ImVec4( accent.x, accent.y, accent.z, 0.15f );
	style.Colors[ ImGuiCol_HeaderActive ] = ImVec4( accent.x, accent.y, accent.z, 0.25f );
	style.Colors[ ImGuiCol_SeparatorHovered ] = accent_dim;
	style.Colors[ ImGuiCol_SeparatorActive ] = accent;
	style.Colors[ ImGuiCol_ResizeGripHovered ] = ImVec4( accent.x, accent.y, accent.z, 0.40f );
	style.Colors[ ImGuiCol_ResizeGripActive ] = ImVec4( accent.x, accent.y, accent.z, 0.60f );
	style.Colors[ ImGuiCol_CheckMark ] = accent;
	style.Colors[ ImGuiCol_SliderGrabActive ] = accent;
	style.Colors[ ImGuiCol_TabHovered ] = ImVec4( accent.x, accent.y, accent.z, 0.20f );
	style.Colors[ ImGuiCol_TabActive ] = ImVec4( accent.x, accent.y, accent.z, 0.15f );
	style.Colors[ ImGuiCol_PlotLines ] = accent;
	style.Colors[ ImGuiCol_PlotLinesHovered ] = ImVec4( accent.x * 1.2f, accent.y * 1.2f, accent.z * 1.2f, 1.0f );
	style.Colors[ ImGuiCol_PlotHistogramHovered ] = accent;
	style.Colors[ ImGuiCol_NavHighlight ] = accent;
	style.Colors[ ImGuiCol_DragDropTarget ] = ImVec4( accent.x, accent.y, accent.z, 0.90f );
}

void menu_c::apply_stream_proof( HWND hwnd )
{
	if ( this->stream_proof_active || !hwnd ) return;
	
	try {
		// Only use basic protection to avoid input blocking
		if (IsWindow(hwnd)) {
			SetWindowDisplayAffinity( hwnd, WDA_EXCLUDEFROMCAPTURE );
			this->stream_proof_active = true;
		}
	} catch (const std::exception& e) {
		this->show_notification("Stream proof error: " + std::string(e.what()));
	} catch (...) {
		// Ignore stream proof errors
	}
}

void menu_c::remove_stream_proof( HWND hwnd )
{
	if ( !this->stream_proof_active || !hwnd ) return;
	
	try {
		if (IsWindow(hwnd)) {
			SetWindowDisplayAffinity( hwnd, WDA_NONE );
		}
		this->stream_proof_active = false;
	} catch (const std::exception& e) {
		this->show_notification("Stream proof removal error: " + std::string(e.what()));
		this->stream_proof_active = false; // Reset state anyway
	} catch (...) {
		this->stream_proof_active = false; // Reset state anyway
	}
}

void menu_c::handle_hotkeys()
{
	if (!g) {
		return;
	}

	try {
		auto& settings = g->core.get_settings();
		static std::map<int, bool> key_states;

		auto check_toggle = [&](int key, bool& setting, const std::string& name) {
			if (key <= 0 || key > 0xFF) return; // Validate key range
			
			try {
				bool current_state = GetAsyncKeyState(key) & 0x8000;
				if (current_state && !key_states[key]) {
					setting = !setting;
					this->show_notification(name + (setting ? " ON" : " OFF"));
				}
				key_states[key] = current_state;
			} catch (...) {
				// Ignore individual key check errors
			}
		};

		check_toggle(g->core.get_settings().hotkeys.aimbot_toggle, g->core.get_settings().aim.enabled, "Aimbot");
		check_toggle(g->core.get_settings().hotkeys.esp_toggle, g->core.get_settings().visuals.player.box, "ESP");
		check_toggle(g->core.get_settings().hotkeys.third_person, g->core.get_settings().misc.third_person, "Third Person");
		check_toggle(g->core.get_settings().hotkeys.night_mode, g->core.get_settings().misc.night_mode, "Night Mode");
		check_toggle(g->core.get_settings().hotkeys.stream_proof, g->core.get_settings().extras.stream_proof, "Stream Proof");
		check_toggle(g->core.get_settings().hotkeys.fps_counter, g->core.get_settings().extras.fps_counter, "FPS Counter");
		check_toggle(g->core.get_settings().hotkeys.ping_display, g->core.get_settings().extras.ping_display, "Ping Display");
		check_toggle(g->core.get_settings().hotkeys.no_flash, g->core.get_settings().misc.no_flash, "No Flash");
		check_toggle(g->core.get_settings().hotkeys.triggerbot_toggle, g->core.get_settings().triggerbot.enabled, "Triggerbot");
		check_toggle(g->core.get_settings().hotkeys.crosshair_overlay, g->core.get_settings().misc.crosshair_overlay, "Crosshair Overlay");
		        
	} catch (const std::exception& e) {
		this->show_notification("Hotkey handler error: " + std::string(e.what()));
	} catch (...) {
		// Ignore hotkey handler errors
	}
}

void menu_c::render_notifications()
{
    if (!g) {
        return;
    }

    try {
        const auto now = std::chrono::steady_clock::now();

        // Remove expired
        notifications.erase(
            std::remove_if(notifications.begin(), notifications.end(),
                [now](const notification_t& n){
                    try {
                        float elapsed = std::chrono::duration<float>(now - n.time).count();
                        return elapsed > n.duration;
                    } catch (...) { return true; }
                }),
            notifications.end());

        if (notifications.empty()) return;

        const ImVec2 display = ImGui::GetIO().DisplaySize;
        const float padding = 12.0f;
        const float width_max = 460.0f;
        const float slide_px = 18.0f;

        // Determine stacking direction and anchor based on corner
        bool from_bottom = (notification_corner == 0 || notification_corner == 1);
        bool from_right  = (notification_corner == 0 || notification_corner == 2);

        float cursor = from_bottom ? 20.0f : 20.0f; // start offset from chosen edge

        auto get_type_color = [&](notify_type t)->ImU32 {
            switch (t) {
                case notify_type::success: return IM_COL32(60, 200, 120, 255);
                case notify_type::warning: return IM_COL32(255, 170, 0, 255);
                case notify_type::error:   return IM_COL32(220, 70, 70, 255);
                case notify_type::damage:  return IM_COL32(255, 90, 160, 255);
                default: return g->features.get_theme_color();
            }
        };

        // Iterate newest first for bottom stacking, oldest first for top stacking
        int start = from_bottom ? (int)notifications.size() - 1 : 0;
        int end   = from_bottom ? -1 : (int)notifications.size();
        int step  = from_bottom ? -1 : 1;

        for (int i = start; i != end; i += step) {
            auto& n = notifications[i];
            float elapsed = std::chrono::duration<float>(now - n.time).count();
            float t = std::min(1.0f, std::max(0.0f, 1.0f - (elapsed / n.duration)));
            // Apply ease-out for alpha and slide
            float alpha = t * t; // quadratic ease-out
            n.alpha = alpha;

            ImVec2 text_size = ImGui::CalcTextSize(n.text.c_str());
            float box_w = std::min(width_max, text_size.x + padding * 2.0f + 6.0f);
            float box_h = text_size.y + padding * 2.0f;

            // Positioning
            float x = from_right ? (display.x - box_w - 20.0f) : 20.0f;
            float y;
            if (from_bottom) {
                y = display.y - (cursor + box_h);
            } else {
                y = cursor;
            }

            // Slide animation from edge
            float slide = slide_px * (1.0f - alpha);
            if (from_right) x += slide; else x -= slide;

            // Colors
            ImU32 acc_col = get_type_color(n.type);
            ImU32 bg_col = IM_COL32(18, 18, 18, (int)(210 * alpha));
            ImU32 border_col = IM_COL32(48, 48, 48, (int)(255 * alpha));
            ImU32 text_col = IM_COL32(235, 235, 235, (int)(255 * alpha));

            ImVec2 bg_min(x, y);
            ImVec2 bg_max(x + box_w, y + box_h);

            ImDrawList* dl = ImGui::GetBackgroundDrawList();
            if (dl) {
                // Shadow
                dl->AddRectFilled(bg_min, bg_max, IM_COL32(0,0,0,(int)(40*alpha)), 6.0f);
                // Panel
                dl->AddRectFilled(bg_min, bg_max, bg_col, 6.0f);
                dl->AddRect(bg_min, bg_max, border_col, 6.0f, 0, 1.5f);
                // Accent bar
                ImVec2 bar_min(bg_min.x, bg_min.y);
                ImVec2 bar_max(bg_min.x + 4.0f, bg_max.y);
                dl->AddRectFilled(bar_min, bar_max, acc_col, 6.0f);
                // Text
                dl->AddText(ImVec2(bg_min.x + padding + 4.0f, bg_min.y + padding), text_col, n.text.c_str());
            }

            cursor += box_h + notification_spacing;
        }
    } catch (...) {
        // swallow
    }
}

void menu_c::show_notification(const std::string& message)
{
    if (message.empty() || message.length() > 256) {
        return; // Prevent empty or overly long notifications
    }

    try {
        // Remove oldest if at max capacity
        while (notifications.size() >= max_notifications) {
            notifications.pop_front();
        }
        
        // Add new notification
        notification_t notification;
        notification.text = message;
        notification.time = std::chrono::steady_clock::now();
        notification.type = notify_type::info;
        notification.duration = notification_duration;
        notification.alpha = 1.0f;
        notifications.push_back(notification);
    } catch (...) {
        // Ignore notification errors
    }
}

void menu_c::show_notification(const std::string& message, notify_type type, float duration_sec)
{
    if (message.empty() || message.length() > 256) {
        return;
    }

    try {
        while (notifications.size() >= max_notifications) {
            notifications.pop_front();
        }

        notification_t n;
        n.text = message;
        n.time = std::chrono::steady_clock::now();
        n.type = type;
        n.duration = duration_sec > 0.2f ? duration_sec : notification_duration;
        n.alpha = 1.0f;
        notifications.push_back(n);
    } catch (...) {
        // ignore
    }
}

const char* menu_c::get_key_name(int key)
{
	if (key <= 0 || key > 0xFF) {
		return "Invalid Key";
	}

	static char buffer[32];
	try {
		switch (key) {
	case VK_F1: return "F1";
	case VK_F2: return "F2";
	case VK_F3: return "F3";
	case VK_F4: return "F4";
	case VK_F5: return "F5";
	case VK_F6: return "F6";
	case VK_F7: return "F7";
	case VK_F8: return "F8";
	case VK_F9: return "F9";
	case VK_F10: return "F10";
	case VK_F11: return "F11";
	case VK_F12: return "F12";
	case VK_INSERT: return "INSERT";
	case VK_DELETE: return "DELETE";
	case VK_HOME: return "HOME";
	case VK_END: return "END";
	case VK_SPACE: return "SPACE";
	case VK_LSHIFT: return "LSHIFT";
	case VK_RSHIFT: return "RSHIFT";
	case VK_LCONTROL: return "LCTRL";
	case VK_RCONTROL: return "RCTRL";
	case VK_LBUTTON: return "Left Click";
	case VK_RBUTTON: return "Right Click";
	case VK_MBUTTON: return "Middle Click";
	case VK_XBUTTON1: return "Mouse 4";
	case VK_XBUTTON2: return "Mouse 5";
		default:
			if (key >= 'A' && key <= 'Z') {
				sprintf_s(buffer, sizeof(buffer), "%c", key);
				return buffer;
			}
			sprintf_s(buffer, sizeof(buffer), "Key %d", key);
			return buffer;
		}
	} catch (...) {
		return "Unknown Key";
	}
}

