#include <include/global.hpp>
#include <shared_mutex>
#include <optional>
#include <algorithm>
#include <cstdint>
#include <cmath>
#include "../updater/offsets_loader.hpp"

extern std::shared_mutex g_sdk_cache_mtx;
#include <unordered_map>
#include <chrono>

// Safe memory read helpers for this TU
namespace {
    template<typename T>
    static inline std::optional<T> try_read(std::uintptr_t addr) {
        try { return g->memory.read<T>(addr); } catch (...) { return std::nullopt; }
    }
    template<typename T>
    static inline T try_read_or(std::uintptr_t addr, T fallback) {
        try { return g->memory.read<T>(addr); } catch (...) { return fallback; }
    }

    // Safe memory write helper
    template<typename T>
    static inline bool try_write(std::uintptr_t addr, const T& value) {
        try { g->memory.write<T>(addr, value); return true; } catch (...) { return false; }
    }
} // namespace

void dispatch_c::run( ImDrawList* drawlist )
{
    std::shared_lock cache_read_lock(g_sdk_cache_mtx);
    // Cache last-known health per enemy pawn to detect damage events
    static std::unordered_map<std::uintptr_t, int> s_last_health;
    static bool s_last_in_match = false;
    // Flash flag debouncing: cache last alpha and active flash windows
    static std::unordered_map<std::uintptr_t, float> s_last_flash_alpha;
    static std::unordered_map<std::uintptr_t, std::chrono::steady_clock::time_point> s_flash_until;
    // Track local player's last shot time and mouse state for hitmarker gating
    static std::chrono::steady_clock::time_point s_last_player_shot = std::chrono::steady_clock::time_point{};
    static bool s_prev_lmb = false;
    // Simple frame counter to throttle heavy rendering paths
    static std::uint32_t s_frame_counter = 0;
    s_frame_counter++;
    const bool do_heavy_this_frame = (s_frame_counter & 1u) == 0;     // every other frame
    const bool do_rare_debug = (s_frame_counter % 10u) == 0;          // every 10th frame
    // Ensure offsets are loaded from client_dll.cs (no-op after first call)
    load_offsets_from_client_dll();

    // Lightweight debug state for FOV/Viewmodel writes
    using steady = std::chrono::steady_clock;
    static int s_dbg_vm_ok = 0, s_dbg_vm_fail = 0;
    static int s_dbg_fov_ok = 0, s_dbg_fov_fail = 0;
    static steady::time_point s_dbg_last_vm_write{};
    static steady::time_point s_dbg_last_fov_write{};
    static float s_dbg_last_vm_fov = 0.0f;
    static int   s_dbg_last_fov_written = 0;

    try {
        // Per-frame SDK updates
        g->sdk.camera.update();
        g->sdk.w2s.update();
        // Update map raycast data (loads triangles for current map as needed)
        try { g->raycasting.update_map(); } catch (...) { /* ignore raycasting map update errors */ }

        // Sync aimbot FOV from settings each frame
        g->features.aim.get_fov() = g->core.get_settings().aim.fov;
        // Stream-proof flag (skip rendering overlays when enabled)
        const bool stream_proof = g->core.get_settings().extras.stream_proof;

        // Diagnostics: entity counts
        const auto players = g->updater.get_player_entities();
        int alive_enemies = 0;
        for (const auto& p : players) {
            if (p.cs_player_pawn && p.health > 0) ++alive_enemies;
        }
        if ( !stream_proof && do_rare_debug )
            drawlist->AddText(ImVec2(10, 10), IM_COL32(255,255,0,255),
                              ("entities: " + std::to_string((int)players.size()) + ", alive: " + std::to_string(alive_enemies)).c_str());

        // Match detection (hysteresis) to avoid heavy reads when not in game
        static bool s_in_match = false;
        static int s_yes = 0, s_no = 0;
        static std::chrono::steady_clock::time_point s_last_match_true = std::chrono::steady_clock::time_point{};
        const std::uintptr_t local_pawn = g->udata.get_owning_player().cs_player_pawn;
        const bool cond_in = (local_pawn != 0) && (players.size() > 1);
        if (cond_in) { s_yes++; s_no = 0; if (s_yes > 30) s_in_match = true; }
        else { s_no++; s_yes = 0; if (s_no > 30) s_in_match = false; }
        if ( s_in_match ) {
            s_last_match_true = std::chrono::steady_clock::now();
        }
        if ( !stream_proof && do_rare_debug )
            drawlist->AddText(ImVec2(10, 26), s_in_match ? IM_COL32(100,255,100,200) : IM_COL32(255,120,120,200), s_in_match ? "match: yes" : "match: no");

        // Record manual fire (LMB edge) as a player shot for hitmarker gating
        {
            const bool lmb = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
            if ( lmb && !s_prev_lmb )
                s_last_player_shot = std::chrono::steady_clock::now();
            s_prev_lmb = lmb;
        }

        // Removed debug: red dot at enemy origin

        // Precompute local player origin for distance calculations
        const float UNITS_PER_METER = 39.62f; // CS2 world units per meter
        std::optional<math::vector3> local_origin_opt;
        try {
            const auto lp_pawn = g->udata.get_owning_player().cs_player_pawn;
            const auto lp_node = g->udata.get_owning_player().m_p_game_scene_node;
            if ( lp_node ) {
                local_origin_opt = g->memory.read<math::vector3>( lp_node + offsets::m_vec_origin );
            } else if ( lp_pawn ) { // fallback
                local_origin_opt = g->memory.read<math::vector3>( lp_pawn + offsets::m_vec_origin );
            }
        } catch ( ... ) { local_origin_opt.reset(); }

        // Player ESP (uses SDK features only if toggled)
        const bool do_box = g->core.get_settings().visuals.player.box;
        const bool do_skeleton = g->core.get_settings().visuals.player.skeleton;
        const bool do_health = g->core.get_settings().visuals.player.health;
        const bool do_distance = g->core.get_settings().visuals.player.distance;
        const bool do_name = g->core.get_settings().visuals.player.name;
        const bool do_weapon = g->core.get_settings().visuals.player.weapon;
        const bool do_chams = g->core.get_settings().visuals.player.chamies;
        const bool do_flags = g->core.get_settings().visuals.player.flags;
        const bool do_head_circle = g->core.get_settings().visuals.player.head_circle;

        // Deathmatch settings
        const auto& dm = g->core.get_settings().death_match;
        const int local_team = g->udata.get_owning_player().team;
        auto is_enemy = [&](int team)->bool {
            if (dm.enabled && dm.ignore_team) return true;      // DM: everyone is target
            // If either team is unknown (0), do NOT mark as enemy to avoid false positives
            if (local_team == 0 || team == 0) return false;
            return team != local_team;
        };

        if (!s_in_match) {
            // Reset caches when leaving match
            if (s_last_in_match) {
                s_last_health.clear();
                s_last_flash_alpha.clear();
                s_flash_until.clear();
            }
        }
        s_last_in_match = s_in_match;

        // Allow brief grace period after match false to prevent flicker in ESP drawing
        const auto now_time = std::chrono::steady_clock::now();
        const bool draw_allowed = s_in_match || (s_last_match_true.time_since_epoch().count() != 0 &&
            std::chrono::duration_cast<std::chrono::milliseconds>(now_time - s_last_match_true).count() <= 1500);

        if (draw_allowed && !stream_proof && (do_box || do_skeleton || do_health || do_distance || do_name || do_weapon || do_chams || do_flags || do_head_circle)) {
            for (const auto& p : players) {
                if (!p.cs_player_pawn) continue;

                // Health change tracking and notifications (hit/kill)
                {
                    auto it = s_last_health.find(p.cs_player_pawn);
                    if (it == s_last_health.end()) {
                        s_last_health[p.cs_player_pawn] = p.health;
                    } else {
                        const int prev = it->second;
                        if (p.health < prev) {
                            const auto now = std::chrono::steady_clock::now();
                            const bool recent_player_shot = (s_last_player_shot.time_since_epoch().count() != 0) &&
                                (std::chrono::duration_cast<std::chrono::milliseconds>(now - s_last_player_shot).count() <= 200);
                            if (recent_player_shot && is_enemy(p.team)) {
                                // Play hitmarker sound if enabled
                                if (g->core.get_settings().misc.hitmarker) {
                                    g->core.play_hitmarker_sound();
                                }
                                // Show hit notification if enabled, including damage amount
                                if (g->core.get_settings().visuals.world.notify_hit) {
                                    const std::string who = p.name_data.name.empty() ? std::string("enemy") : p.name_data.name;
                                    const int dmg = std::max(0, prev - p.health);
                                    std::string msg = std::string("Hit ") + who + " (-" + std::to_string(dmg) + ")";
                                    if (p.health <= 0) msg += " [lethal]";
                                    g->menu.show_notification(msg);
                                }
                            }
                            // Kill notification when crossing to zero
                            if (prev > 0 && p.health <= 0 && is_enemy(p.team)) {
                                if (g->core.get_settings().visuals.world.notify_kill) {
                                    const std::string who = p.name_data.name.empty() ? std::string("enemy") : p.name_data.name;
                                    g->menu.show_notification(std::string("Killed ") + who);
                                }
                            }
                            it->second = p.health;
                        } else if (p.health != prev) {
                            it->second = p.health;
                        }
                    }
                }

                if (p.health <= 0) continue;
                if (p.cs_player_pawn == local_pawn) continue; // skip local player

                // Apply deathmatch ESP filter: if not highlighting all, skip teammates
                if (dm.enabled && !dm.highlight_all && !is_enemy(p.team))
                    continue;

                const auto bones = g->sdk.bones.get_data(p.bone_array);
                if (!bones.is_valid()) continue;
                const auto bounds = g->sdk.bounds.get_data(bones);

                int dist_m = 0;
                if (do_distance || do_head_circle) {
                    try {
                        math::vector3 org{};
                        if ( p.m_p_game_scene_node )
                            org = g->memory.read<math::vector3>(p.m_p_game_scene_node + offsets::m_vec_origin);
                        else
                            org = g->memory.read<math::vector3>( p.cs_player_pawn + offsets::m_vec_origin );
                        if ( local_origin_opt.has_value() )
                            dist_m = static_cast<int>( local_origin_opt.value().distance(org) / UNITS_PER_METER );
                        else
                            dist_m = 0;
                    } catch (...) { dist_m = 0; }
                }

                if (do_box)
                    g->features.visuals.player.box(drawlist, bounds, g->features.get_theme_color());
                if (do_skeleton)
                    g->features.visuals.player.skeleton(drawlist, bones, g->features.get_theme_color());
                if (do_health)
                    g->features.visuals.player.health_bar(drawlist, bounds, p.health);
                if (do_distance)
                    g->features.visuals.player.distance(drawlist, bounds, dist_m);
                // Names: optionally restrict to enemies only when DM->show_enemy_names is enabled
                if (do_name && !p.name_data.name.empty())
                {
                    bool allow_name = true;
                    if ( dm.enabled && dm.show_enemy_names )
                        allow_name = is_enemy(p.team);
                    if ( allow_name )
                        g->features.visuals.player.name(drawlist, bounds, p.name_data.name);
                }
                if (do_weapon && !p.weapon_data.name.empty())
                    g->features.visuals.player.weapon(drawlist, bounds, p.weapon_data.name);
                if (do_chams && !p.hitbox_data.empty())
                    g->features.visuals.player.chamies.run(drawlist, p.hitbox_data, bones, g->features.get_theme_color());

                // Head circle: project head bone and draw a distance-scaled dot
                if (do_head_circle)
                {
                    const auto head_world = bones.get_position( sdk_c::bones_c::bone_id::head );
                    const auto head_proj = g->sdk.w2s.project( head_world );
                    if ( g->sdk.w2s.is_valid( head_proj ) )
                    {
                        g->features.visuals.player.dot( drawlist, head_proj, dist_m );
                    }
                }

                // Deathmatch debug overlay per player
                if ( dm.enabled && dm.debug && !stream_proof )
                {
                    const char* rel = is_enemy(p.team) ? "ENEMY" : "ALLY";
                    std::string dbg = std::string("[DM] ") + rel;
                    drawlist->AddText(ImVec2(bounds.min_x, bounds.min_y - 12.0f), is_enemy(p.team) ? IM_COL32(255,80,80,200) : IM_COL32(120,180,255,200), dbg.c_str());
                }

                // Player flags (runtime mask: flashed/scoped/defusing; ping/lag optional)
                if (do_flags && do_heavy_this_frame)
                {
                    int runtime_mask = 0;
                    try {
                        // Timed FLASHED flag: detect spikes in flash alpha and keep flag for a short window
                        const auto now = std::chrono::steady_clock::now();
                        float max_alpha = g->memory.read<float>( p.cs_player_pawn + offsets::m_fl_flash_max_alpha );
                        float prev = 0.0f;
                        auto it_prev = s_last_flash_alpha.find(p.cs_player_pawn);
                        if ( it_prev == s_last_flash_alpha.end() ) {
                            s_last_flash_alpha[p.cs_player_pawn] = max_alpha;
                        } else {
                            prev = it_prev->second;
                            // Consider a flash event if alpha rises notably
                            if ( max_alpha - prev >= 20.0f && max_alpha >= 60.0f ) {
                                s_flash_until[p.cs_player_pawn] = now + std::chrono::milliseconds(2500);
                            }
                            it_prev->second = max_alpha;
                        }

                        auto it_until = s_flash_until.find(p.cs_player_pawn);
                        if ( it_until != s_flash_until.end() ) {
                            if ( now <= it_until->second ) {
                                runtime_mask |= (1 << 0); // FLASHED active within window
                            } else {
                                s_flash_until.erase(it_until);
                            }
                        }
                    } catch (...) {}
                    try {
                        // bit1: SCOPED
                        bool scoped = g->memory.read<bool>( p.cs_player_pawn + offsets::m_b_is_scoped );
                        if ( scoped ) runtime_mask |= (1 << 1);
                    } catch (...) {}
                    try {
                        // bit2: DEFUSING
                        bool defusing = g->memory.read<bool>( p.cs_player_pawn + offsets::m_b_is_defusing );
                        if ( defusing ) runtime_mask |= (1 << 2);
                    } catch (...) {}

                    g->features.visuals.player.flags( drawlist, bounds, runtime_mask );
                }

                // Hitmarker hook: only play shortly after local player's shot and on enemies
                auto it = s_last_health.find(p.cs_player_pawn);
                if (it == s_last_health.end()) {
                    s_last_health[p.cs_player_pawn] = p.health;
                } else {
                    const int prev = it->second;
                    if (p.health < prev) {
                        const auto now = std::chrono::steady_clock::now();
                        const bool recent_player_shot = (s_last_player_shot.time_since_epoch().count() != 0) &&
                            (std::chrono::duration_cast<std::chrono::milliseconds>(now - s_last_player_shot).count() <= 200);
                        if ( recent_player_shot && is_enemy(p.team) && g->core.get_settings().misc.hitmarker )
                        {
                            g->core.play_hitmarker_sound();
                        }
                        it->second = p.health;
                    } else if (p.health != prev) {
                        it->second = p.health;
                    }
                }
            }
        }

        // Off-screen directional arrows (OOF)
        try {
            if ( s_in_match && !stream_proof && g->core.get_settings().visuals.player.oof_arrows )
            {
                for ( const auto& p : players )
                {
                    if ( !p.cs_player_pawn || p.health <= 0 ) continue;
                    if ( p.cs_player_pawn == local_pawn ) continue;
                    if (dm.enabled && !dm.highlight_all && !is_enemy(p.team)) continue;

                    // Choose a representative target point: best hitbox if possible, else origin
                    ImVec2 proj_point{};
                    bool have_proj = false;
                    {
                        const auto bones = g->sdk.bones.get_data( p.bone_array );
                        if ( bones.is_valid() )
                        {
                            const auto hitboxes = g->features.aim.get_enabled_hitboxes( bones );
                            if ( !hitboxes.empty() )
                            {
                                const auto best_hitbox_world = g->features.aim.get_closest_hitbox( hitboxes );
                                const auto proj = g->sdk.w2s.project( best_hitbox_world );
                                proj_point = proj.vec();
                                have_proj = true; // may be off-screen, is_valid() not required for arrow
                            }
                        }
                    }
                    if ( !have_proj )
                    {
                        math::vector3 org{};
                        if ( p.m_p_game_scene_node )
                            org = try_read_or<math::vector3>( p.m_p_game_scene_node + offsets::m_vec_origin, math::vector3{} );
                        else
                            org = try_read_or<math::vector3>( p.cs_player_pawn + offsets::m_vec_origin, math::vector3{} );
                        const auto proj = g->sdk.w2s.project( org );
                        proj_point = proj.vec();
                        have_proj = true;
                    }

                    if ( have_proj )
                    {
                        // Only draw arrow if target is not on-screen
                        const auto proj = math::vector2{ proj_point.x, proj_point.y };
                        const bool on_screen = g->sdk.w2s.is_on_screen( proj );
                        if ( !on_screen )
                        {
                            g->features.visuals.indicators.oof_arrow( drawlist, proj, g->features.get_theme_color() );
                        }
                    }
                }
            }
        } catch ( ... ) { /* ignore OOF errors */ }

        // Bomb plant notification: detect appearance of C_PlantedC4 in misc entities
        try {
            static bool s_bomb_present = false;
            bool bomb_now = false;
            const auto misc = g->updater.get_misc_entities();
            for (const auto& m : misc) {
                if (m.type == udata_c::misc_entity_t::type_t::bomb) { bomb_now = true; break; }
            }
            if (!s_bomb_present && bomb_now) {
                if (g->core.get_settings().visuals.world.notify_bomb_plant) {
                    g->menu.show_notification("Bomb planted");
                }
            }
            s_bomb_present = bomb_now;
        } catch (...) { /* ignore bomb notification errors */ }

        // Aimbot: gated selection and execution with targeting modes and debounce
        try {
            const auto& aim = g->core.get_settings().aim;
            if ( s_in_match && aim.enabled )
            {
                using clock = std::chrono::steady_clock;
                static auto s_last_aim_time = clock::now();
                const bool key_down = ( GetAsyncKeyState( aim.key ) & 0x8000 ) != 0;
                if ( key_down ) s_last_aim_time = clock::now();
                const auto debounce_ms = std::max(0, aim.target_debounce_ms);
                const bool in_window = std::chrono::duration_cast<std::chrono::milliseconds>(clock::now() - s_last_aim_time).count() <= debounce_ms;
                const bool allow_scan = key_down || in_window;

                math::vector3 selected_target_world{};
                bool have_selected = false;

                // Anti-flicker cache for aimbot visibility (per pawn)
                using vis_clock = std::chrono::steady_clock;
                static std::unordered_map<std::uintptr_t, std::pair<bool, vis_clock::time_point>> s_vis_cache_aim;

                if ( allow_scan )
                {
                    // Reset accumulator for closest-to-crosshair mode
                    g->features.aim.reset_closest_target();
                    int best_health = std::numeric_limits<int>::max();

                    for ( const auto& p : players )
                    {
                        // Basic validity checks
                        if ( !p.cs_player_pawn || p.health <= 0 ) continue;
                        // Skip local player
                        if ( p.cs_player_pawn == local_pawn ) continue;
                        // Enemies only according to DM and team logic encapsulated by is_enemy()
                        if ( !is_enemy(p.team) ) continue;

                        const auto bones = g->sdk.bones.get_data( p.bone_array );
                        if ( !bones.is_valid() ) continue;

                        // Build candidate bones based on preferred hitbox or enabled groups
                        using bone = sdk_c::bones_c::bone_id;
                        std::vector<bone> candidates;
                        if ( aim.use_preferred_hitbox ) {
                            switch ( aim.preferred_hitbox ) {
                                case 0: candidates = { bone::head, bone::spine_3, bone::spine_2 }; break; // Head focus
                                case 1: candidates = { bone::spine_3, bone::spine_2, bone::left_shoulder, bone::right_shoulder }; break; // Chest focus
                                default: candidates = { bone::pelvis, bone::spine_2, bone::left_hip, bone::right_hip }; break; // Pelvis focus
                            }
                        } else {
                            if ( aim.hitbox_head ) candidates.push_back( bone::head );
                            if ( aim.hitbox_chest ) { candidates.push_back( bone::spine_3 ); candidates.push_back( bone::spine_2 ); }
                            if ( aim.hitbox_pelvis ) candidates.push_back( bone::pelvis );
                            if ( aim.hitbox_arms ) { candidates.push_back( bone::left_shoulder ); candidates.push_back( bone::right_shoulder ); candidates.push_back( bone::left_hand ); candidates.push_back( bone::right_hand ); }
                            if ( aim.hitbox_legs ) { candidates.push_back( bone::left_knee ); candidates.push_back( bone::right_knee ); }
                            if ( candidates.empty() ) candidates = { bone::head, bone::spine_3, bone::pelvis };
                        }

                        // Pick best candidate by closest to crosshair among projected/valid points
                        const auto center = math::vector2{ g->core.get_display().width * 0.5f, g->core.get_display().height * 0.5f };
                        math::vector3 best_hitbox_world{};
                        math::vector2 best_proj{};
                        float best_dist = std::numeric_limits<float>::max();
                        for ( const auto& b : candidates ) {
                            const auto pos = bones.get_position( b );
                            const auto proj = g->sdk.w2s.project( pos );
                            if ( !g->sdk.w2s.is_valid( proj ) ) continue;
                            const auto d = (proj - center); float dsq = d.x * d.x + d.y * d.y;
                            if ( dsq < best_dist ) { best_dist = dsq; best_hitbox_world = pos; best_proj = proj; }
                        }
                        if ( best_dist == std::numeric_limits<float>::max() ) continue; // nothing projectable

                        // Within FOV only
                        if ( g->features.aim.is_outside_fov( best_proj ) ) continue;

                        // Visibility gating with modes and anti-flicker (reuses candidates)
                        bool visible_ok = true;
                        try {
                            if ( local_origin_opt.has_value() && aim.vis_check )
                            {
                                const auto cam = local_origin_opt.value();
                                int visible_cnt = 0, total_cnt = 0;
                                for ( const auto& b : candidates ) {
                                    const auto pos = bones.get_position( b );
                                    const auto proj = g->sdk.w2s.project( pos );
                                    if ( !g->sdk.w2s.is_valid( proj ) ) continue;
                                    total_cnt++;
                                    if ( g->raycasting.is_visible( cam, pos ) ) visible_cnt++;
                                }

                                bool pass = false;
                                if ( total_cnt == 0 || g->raycasting.get_current_map_name().empty() ) {
                                    pass = true; // permissive
                                } else {
                                    switch ( aim.visibility_mode ) {
                                        case 0: pass = (visible_cnt >= 1); break; // Relaxed
                                        case 2: pass = (visible_cnt >= 2) && (static_cast<float>(visible_cnt) / total_cnt >= 0.5f); break; // Strict
                                        default: pass = (visible_cnt >= 1) && (static_cast<float>(visible_cnt) / total_cnt >= 0.25f); break; // Normal
                                    }
                                }

                                if ( aim.anti_flicker ) {
                                    const int debounce_ms = std::clamp(aim.target_debounce_ms, 30, 300);
                                    const auto now = vis_clock::now();
                                    auto itv = s_vis_cache_aim.find( p.cs_player_pawn );
                                    if ( itv != s_vis_cache_aim.end() ) {
                                        const bool last_vis = itv->second.first;
                                        const auto age = std::chrono::duration_cast<std::chrono::milliseconds>( now - itv->second.second ).count();
                                        if ( !pass && last_vis && age <= debounce_ms ) {
                                            pass = true; // hold briefly
                                        }
                                    }
                                    s_vis_cache_aim[p.cs_player_pawn] = { pass, vis_clock::now() };
                                }

                                visible_ok = pass;
                            }
                        } catch ( ... ) { visible_ok = true; }

                        if ( !visible_ok ) continue;

                        if ( aim.targeting_mode == 0 )
                        {
                            // Closest to crosshair (existing behavior)
                            features_c::aim_c::target_t t{ best_hitbox_world, p.cs_player_pawn };
                            g->features.aim.find_closest_target( best_proj, t );
                        }
                        else if ( aim.targeting_mode == 1 )
                        {
                            // Lowest health within FOV
                            if ( p.health < best_health )
                            {
                                best_health = p.health;
                                selected_target_world = best_hitbox_world;
                                have_selected = true;
                            }

                            // Optional visualization of candidates
                            if ( aim.visualization.dot || aim.visualization.line )
                            {
                                int dist_m = 0;
                                try {
                                    math::vector3 org{};
                                    if ( p.m_p_game_scene_node )
                                        org = g->memory.read<math::vector3>( p.m_p_game_scene_node + offsets::m_vec_origin );
                                    else
                                        org = g->memory.read<math::vector3>( p.cs_player_pawn + offsets::m_vec_origin );
                                    if ( local_origin_opt.has_value() )
                                        dist_m = static_cast<int>( local_origin_opt.value().distance( org ) / UNITS_PER_METER );
                                } catch (...) { dist_m = 0; }

                                if ( aim.visualization.dot )
                                    g->features.visuals.player.dot( drawlist, best_proj, dist_m );
                                if ( aim.visualization.line )
                                    g->features.visuals.player.line( drawlist, best_proj, dist_m );
                            }
                        }
                        else
                        {
                            // Fallback to closest if unknown mode
                            features_c::aim_c::target_t t{ best_hitbox_world, p.cs_player_pawn };
                            g->features.aim.find_closest_target( best_proj, t );
                        }
                    }
                }
                // Resolve selected target for closest-to-crosshair
                if ( aim.targeting_mode != 1 )
                {
                    if ( const auto* t = g->features.aim.get_closest_target() )
                    {
                        selected_target_world = t->position;
                        have_selected = true;
                    }
                }

                // Apply aim only when key is down or within debounce window
                if ( allow_scan && have_selected )
                {
                    g->features.aim.apply_aim_toward( selected_target_world );
                }
            }
        } catch ( ... ) { /* ignore aim errors */ }

        // Triggerbot: fires when crosshair overlaps an enemy box; honors reaction and burst delays
        try {
            const auto& tb = g->core.get_settings().triggerbot;
            if ( s_in_match && tb.enabled )
            {
                using clock = std::chrono::steady_clock;
                static auto s_key_down_since = clock::time_point{};
                static auto s_last_shot_time = clock::time_point{};
                static bool s_first_fired = false;

                const bool key_down = ( GetAsyncKeyState( tb.hotkey ) & 0x8000 ) != 0;
                if ( !key_down ) {
                    s_key_down_since = clock::time_point{};
                    s_first_fired = false;
                } else if ( s_key_down_since.time_since_epoch().count() == 0 ) {
                    s_key_down_since = clock::now();
                    s_first_fired = false;
                }

                // If crosshair target index offset is available, require it to be non-zero when key is down
                int crosshair_id = -1;
                if ( key_down && offsets::m_i_id_ent_index != 0 ) {
                    try {
                        const auto lp_pawn = g->udata.get_owning_player().cs_player_pawn;
                        if ( lp_pawn ) {
                            crosshair_id = g->memory.read<int>( lp_pawn + offsets::m_i_id_ent_index );
                        }
                    } catch ( ... ) { crosshair_id = -1; }
                }

                // Find if crosshair center overlaps an enemy's on-screen bounds
                bool have_target = false;
                std::uintptr_t target_pawn = 0;
                const float cx = g->core.get_display().width * 0.5f;
                const float cy = g->core.get_display().height * 0.5f;
                ImVec2 center(cx, cy);

                if ( key_down )
                {
                    for ( const auto& p : players )
                    {
                        if ( !p.cs_player_pawn || p.health <= 0 ) continue;
                        if ( p.cs_player_pawn == local_pawn ) continue;
                        if ( !is_enemy(p.team) ) continue;

                        const auto bones = g->sdk.bones.get_data( p.bone_array );
                        if ( !bones.is_valid() ) continue;
                        const auto bounds = g->sdk.bounds.get_data( bones );

                        // Bounds valid and center inside
                        if ( bounds.min_x <= center.x && center.x <= bounds.max_x &&
                             bounds.min_y <= center.y && center.y <= bounds.max_y )
                        {
                            // If we have a crosshair index, require it to be valid (>0); otherwise skip gate
                            if ( offsets::m_i_id_ent_index != 0 && crosshair_id <= 0 ) {
                                continue;
                            }
                            // Visibility check using raycast with modes and anti-flicker
                            bool visible_ok = true;
                            try {
                                const auto& aim = g->core.get_settings().aim;
                                if ( local_origin_opt.has_value() && aim.vis_check )
                                {
                                    const auto cam = local_origin_opt.value();

                                    // Build candidate bones based on user preference
                                    using bone = sdk_c::bones_c::bone_id;
                                    std::vector<bone> candidates;
                                    if ( aim.use_preferred_hitbox ) {
                                        switch ( aim.preferred_hitbox ) {
                                            case 0: candidates = { bone::head, bone::spine_3, bone::spine_2 }; break; // Head focus
                                            case 1: candidates = { bone::spine_3, bone::spine_2, bone::left_shoulder, bone::right_shoulder }; break; // Chest focus
                                            default: candidates = { bone::pelvis, bone::spine_2, bone::left_hip, bone::right_hip }; break; // Pelvis focus
                                        }
                                    } else {
                                        // From enabled hitbox checkboxes
                                        if ( aim.hitbox_head ) candidates.push_back( bone::head );
                                        if ( aim.hitbox_chest ) { candidates.push_back( bone::spine_3 ); candidates.push_back( bone::spine_2 ); }
                                        if ( aim.hitbox_pelvis ) candidates.push_back( bone::pelvis );
                                        if ( aim.hitbox_arms ) { candidates.push_back( bone::left_shoulder ); candidates.push_back( bone::right_shoulder ); candidates.push_back( bone::left_hand ); candidates.push_back( bone::right_hand ); }
                                        if ( aim.hitbox_legs ) { candidates.push_back( bone::left_knee ); candidates.push_back( bone::right_knee ); }
                                        if ( candidates.empty() ) candidates = { bone::head, bone::spine_3, bone::pelvis };
                                    }

                                    int visible_cnt = 0, total_cnt = 0;
                                    for ( const auto& b : candidates ) {
                                        const auto pos = bones.get_position( b );
                                        const auto screen = g->sdk.w2s.project( pos );
                                        if ( !g->sdk.w2s.is_valid( screen ) ) continue;
                                        total_cnt++;
                                        if ( g->raycasting.is_visible( cam, pos ) ) visible_cnt++;
                                    }

                                    bool pass = false;
                                    if ( total_cnt == 0 || g->raycasting.get_current_map_name().empty() ) {
                                        pass = true; // permissive if no data
                                    } else {
                                        switch ( aim.visibility_mode ) {
                                            case 0: // Relaxed
                                                pass = (visible_cnt >= 1);
                                                break;
                                            case 2: // Strict
                                                pass = (visible_cnt >= 2) && (static_cast<float>(visible_cnt) / total_cnt >= 0.5f);
                                                break;
                                            default: // Normal
                                                pass = (visible_cnt >= 1) && (static_cast<float>(visible_cnt) / total_cnt >= 0.25f);
                                                break;
                                        }
                                    }

                                    // Anti-flicker temporal caching per pawn
                                    if ( aim.anti_flicker ) {
                                        using clock = std::chrono::steady_clock;
                                        static std::unordered_map<std::uintptr_t, std::pair<bool, clock::time_point>> s_vis_cache;
                                        const int debounce_ms = std::clamp(g->core.get_settings().aim.target_debounce_ms, 30, 300);
                                        const auto now = clock::now();
                                        auto it = s_vis_cache.find( p.cs_player_pawn );
                                        if ( it != s_vis_cache.end() ) {
                                            const bool last_vis = it->second.first;
                                            const auto age = std::chrono::duration_cast<std::chrono::milliseconds>( now - it->second.second ).count();
                                            if ( !pass && last_vis && age <= debounce_ms ) {
                                                pass = true; // hold last visible briefly
                                            }
                                            it->second = { pass, now };
                                        } else {
                                            s_vis_cache[p.cs_player_pawn] = { pass, now };
                                        }
                                    }

                                    visible_ok = pass;
                                }
                            } catch ( ... ) { visible_ok = true; }

                            if ( visible_ok ) {
                                // Raw memory double-checks for team/health when offsets are available
                                bool team_ok = true;
                                bool health_ok = true;
                                if ( offsets::m_i_team_num != 0 ) {
                                    try {
                                        const int tgt_team = g->memory.read<int>( p.cs_player_pawn + offsets::m_i_team_num );
                                        team_ok = is_enemy( tgt_team );
                                    } catch ( ... ) { team_ok = true; }
                                }
                                if ( offsets::m_i_health != 0 ) {
                                    try {
                                        const int tgt_hp = g->memory.read<int>( p.cs_player_pawn + offsets::m_i_health );
                                        health_ok = (tgt_hp > 0);
                                    } catch ( ... ) { health_ok = true; }
                                }

                                if ( team_ok && health_ok ) { have_target = true; target_pawn = p.cs_player_pawn; break; }
                            }
                        }
                    }
                }

                const auto now = clock::now();
                const int first_delay = std::max(0, tb.first_shot_delay_ms);
                const int between_ms = std::max(0, tb.delay_between_shots_ms);

                bool can_shoot = false;
                if ( key_down && have_target )
                {
                    if ( !s_first_fired )
                    {
                        const auto held_ms = std::chrono::duration_cast<std::chrono::milliseconds>( now - s_key_down_since ).count();
                        if ( held_ms >= first_delay )
                            can_shoot = true;
                    }
                    else
                    {
                        const auto since_last = std::chrono::duration_cast<std::chrono::milliseconds>( now - s_last_shot_time ).count();
                        if ( since_last >= between_ms )
                            can_shoot = true;
                    }
                }

                if ( can_shoot )
                {
                    // Prefer SendInput for safety/compatibility
                    g->memory.inject_mouse( 0, 0, 1 /*down*/ | 2 /*up*/ );
                    s_last_shot_time = now;
                    s_first_fired = true;
                    s_last_player_shot = now; // reuse for hitmarker gating
                }
            }
        } catch ( ... ) { /* ignore triggerbot errors */ }

        // Crosshair Overlay (runtime draw)
        {
            const auto& misc = g->core.get_settings().misc;
            const auto& extras = g->core.get_settings().extras;
            if ( misc.crosshair_overlay && !extras.stream_proof )
            {
                ImDrawList* bdl = ImGui::GetBackgroundDrawList();
                const float arm = std::max(2.0f, misc.crosshair_size);
                const float thick = std::max(1.0f, misc.crosshair_thickness);
                const float gap = std::clamp(arm * 0.25f, 2.0f, 10.0f);

                const float cx = g->core.get_display().width * 0.5f;
                const float cy = g->core.get_display().height * 0.5f;

                ImU32 col = 0xFFFFFFFF;
                if ( misc.crosshair_use_theme_color )
                    col = g->features.get_theme_color();
                else
                    col = ImGui::GetColorU32(ImVec4(misc.crosshair_color[0], misc.crosshair_color[1], misc.crosshair_color[2], 1.0f));

                // Horizontal lines
                bdl->AddLine(ImVec2(cx - gap - arm, cy), ImVec2(cx - gap, cy), col, thick);
                bdl->AddLine(ImVec2(cx + gap, cy), ImVec2(cx + gap + arm, cy), col, thick);
                // Vertical lines
                bdl->AddLine(ImVec2(cx, cy - gap - arm), ImVec2(cx, cy - gap), col, thick);
                bdl->AddLine(ImVec2(cx, cy + gap), ImVec2(cx, cy + gap + arm), col, thick);
            }
        }

        // Bunnyhop
        if ( s_in_match && g->core.get_settings().misc.bunnyhop ) {
            bool space_pressed = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;
            std::uintptr_t local_pawn = 0;
            int flags = 0;
            bool on_ground = false;
            bool air_65664 = false;
            std::uintptr_t force_jump_addr = 0;
            enum class JumpState { Idle, PressHeld };
            static JumpState s_jump_state = JumpState::Idle; // two-step press
            static int s_last_jump_write = 0;                // debug: last value written
            static int s_frames_since_press = 0;             // frames since last press
            static int s_landings = 0;                       // count landings
            try {
                local_pawn = g->udata.get_owning_player().cs_player_pawn;
                if ( local_pawn ) {
                    flags = g->memory.read<int>( local_pawn + offsets::m_f_flags );
                    // Derive ground state per your observation: 0 = ground, 65664 = air
                    on_ground = (flags == 0) || ((flags & 1) == 1);
                    air_65664 = (flags == 65664);
                    force_jump_addr = g->core.get_process_info().client_base + offsets::dw_force_jump;

                    // Two-frame press: press on landing (6), release next frame (4)
                    if ( space_pressed ) {
                        if ( on_ground ) {
                            if ( s_jump_state == JumpState::Idle ) {
                                // Landing detected with key held
                                ++s_landings;
                                g->memory.write<int>( force_jump_addr, 6 );  // press (alt)
                                s_last_jump_write = 6;
                                s_jump_state = JumpState::PressHeld; // hold for one frame
                                s_frames_since_press = 0;
                            } else { // PressHeld
                                // Release after one frame (or more)
                                g->memory.write<int>( force_jump_addr, 4 );  // release (alt)
                                s_last_jump_write = 4;
                                s_jump_state = JumpState::Idle;
                                s_frames_since_press++;
                            }
                        } else {
                            // In air: ensure release and reset timers
                            g->memory.write<int>( force_jump_addr, 256 );
                            s_last_jump_write = 256;
                            s_frames_since_press = 0;
                        }
                    } else {
                        // Space not held: keep released and reset state
                        g->memory.write<int>( force_jump_addr, 256 );
                        s_last_jump_write = 256;
                        s_jump_state = JumpState::Idle;
                        s_frames_since_press = 0;
                    }
                }
            } catch ( ... ) { /* ignore bhop errors */ }

            // BHOP DEBUG overlay
            if ( g->core.get_settings().misc.bunnyhop_debug && !stream_proof ) {
                const float x = 10.0f;
                float y = 46.0f; // below the match text
                auto add = [&](const char* label, const std::string& val, ImU32 col) {
                    std::string line = std::string(label) + val;
                    drawlist->AddText(ImVec2(x, y), col, line.c_str());
                    y += 14.0f;
                };
                add("[BHOP] space:", space_pressed ? "down" : "up", space_pressed ? IM_COL32(0,255,0,220) : IM_COL32(255,120,120,220));
                add("       flags:", std::to_string(flags), IM_COL32(255,255,255,220));
                add("       ground:", on_ground ? "yes" : "no", on_ground ? IM_COL32(0,255,0,220) : IM_COL32(255,120,120,220));
                add("       air(65664):", air_65664 ? "yes" : "no", air_65664 ? IM_COL32(255,200,0,220) : IM_COL32(200,200,200,220));
                add("       last write:", std::to_string(s_last_jump_write), IM_COL32(180,220,255,220));
                add("       state:", (s_jump_state == JumpState::Idle ? "Idle" : "PressHeld"), IM_COL32(180,220,255,220));
                add("       landings:", std::to_string(s_landings), IM_COL32(200,255,200,220));
            }
        }

        // Crosshair overlay
        if ( g->core.get_settings().misc.crosshair_overlay && !stream_proof )
        {
            const auto& misc = g->core.get_settings().misc;
            const float size = misc.crosshair_size;
            const float thickness = misc.crosshair_thickness;

            ImU32 col;
            if ( misc.crosshair_use_theme_color )
                col = g->features.get_theme_color();
            else
                col = IM_COL32(
                    static_cast<int>(misc.crosshair_color[0] * 255.0f),
                    static_cast<int>(misc.crosshair_color[1] * 255.0f),
                    static_cast<int>(misc.crosshair_color[2] * 255.0f),
                    255
                );

            const float cx = g->core.get_display().width * 0.5f;
            const float cy = g->core.get_display().height * 0.5f;
            const ImVec2 center(cx, cy);

            // Horizontal line
            drawlist->AddLine(ImVec2(center.x - size, center.y), ImVec2(center.x + size, center.y), col, thickness);
            // Vertical line
            drawlist->AddLine(ImVec2(center.x, center.y - size), ImVec2(center.x, center.y + size), col, thickness);
        }

        // FOV circle visualization
        if ( g->core.get_settings().aim.visualization.fov && !stream_proof )
        {
            g->features.visuals.hud.ring( drawlist );
        }

        // No Flash: clamp flash max alpha
        try {
            const auto& misc = g->core.get_settings().misc;
            if ( s_in_match && misc.no_flash )
            {
                if ( const auto lp = g->udata.get_owning_player().cs_player_pawn )
                {
                    const float alpha = std::clamp(misc.no_flash_alpha, 0.0f, 255.0f);
                    g->memory.write<float>( lp + offsets::m_fl_flash_max_alpha, alpha );
                }
            }
        } catch (...) { /* ignore no-flash errors */ }

        // Night Mode: apply exposure via post-processing volume
        try {
            if ( g->core.get_settings().misc.night_mode )
            {
                for ( const auto& m : g->updater.get_misc_entities() )
                {
                    if ( m.type == udata_c::misc_entity_t::type_t::post_processing_volume )
                    {
                        g->features.exploits.night_mode.run( m.entity );
                        break;
                    }
                }
            }
        } catch (...) { /* ignore night mode errors */ }

        // Viewmodel Changer: apply X/Y/Z offsets and viewmodel FOV offset
        try {
            const auto& vm = g->core.get_settings().visuals.world.viewmodel_changer;
            static auto s_last_apply = std::chrono::steady_clock::now();
            static float s_last_x = 0.0f, s_last_y = 0.0f, s_last_z = 0.0f, s_last_fov = 0.0f;
            static std::optional<float> s_base_vm_fov; // cache baseline viewmodel FOV
            constexpr int k_apply_ms = 100; // throttle to 10 Hz

            if ( s_in_match && vm.enabled )
            {
                const auto now = std::chrono::steady_clock::now();
                const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - s_last_apply).count();
                if ( ms >= k_apply_ms )
                {
                    s_last_apply = now;

                    const auto lp = g->udata.get_owning_player().cs_player_pawn;
                    if ( lp )
                    {
                        // Clamp inputs to safe ranges (match UI)
                        const float x = std::clamp(vm.offset_x, -10.0f, 5.0f);
                        const float y = std::clamp(vm.offset_y, -10.0f, 10.0f);
                        const float z = std::clamp(vm.offset_z, -5.0f, 5.0f);
                        const float fov_off = std::clamp(vm.fov_offset, -40.0f, 40.0f);

                        auto nearly_equal = [](float a, float b){ return std::fabs(a - b) <= 0.01f; };

                        // Write offsets if available and changed
                        if ( offsets::m_flViewmodelOffsetX ) {
                            if (!nearly_equal(x, s_last_x)) {
                                const bool ok = try_write<float>( lp + offsets::m_flViewmodelOffsetX, x );
                                if (ok) { ++s_dbg_vm_ok; s_dbg_last_vm_write = steady::now(); } else { ++s_dbg_vm_fail; }
                            }
                        }
                        if ( offsets::m_flViewmodelOffsetY ) {
                            if (!nearly_equal(y, s_last_y)) {
                                const bool ok = try_write<float>( lp + offsets::m_flViewmodelOffsetY, y );
                                if (ok) { ++s_dbg_vm_ok; s_dbg_last_vm_write = steady::now(); } else { ++s_dbg_vm_fail; }
                            }
                        }
                        if ( offsets::m_flViewmodelOffsetZ ) {
                            if (!nearly_equal(z, s_last_z)) {
                                const bool ok = try_write<float>( lp + offsets::m_flViewmodelOffsetZ, z );
                                if (ok) { ++s_dbg_vm_ok; s_dbg_last_vm_write = steady::now(); } else { ++s_dbg_vm_fail; }
                            }
                        }

                        s_last_x = x; s_last_y = y; s_last_z = z;

                        // Viewmodel FOV offset (baseline + offset)
                        if ( offsets::m_flViewmodelFOV )
                        {
                            if ( !s_base_vm_fov.has_value() )
                            {
                                try { s_base_vm_fov = g->memory.read<float>( lp + offsets::m_flViewmodelFOV ); }
                                catch (...) { s_base_vm_fov = 68.0f; }
                            }
                            const float desired_vm_fov = std::clamp( s_base_vm_fov.value_or(68.0f) + fov_off, 30.0f, 140.0f );
                            if (!nearly_equal(desired_vm_fov, s_last_fov))
                            {
                                const bool ok = try_write<float>( lp + offsets::m_flViewmodelFOV, desired_vm_fov );
                                if (ok) { ++s_dbg_vm_ok; s_dbg_last_vm_write = steady::now(); s_dbg_last_vm_fov = desired_vm_fov; }
                                else { ++s_dbg_vm_fail; }
                                s_last_fov = desired_vm_fov;
                            }
                        }
                    }
                }
            }
            else
            {
                // Reset cache when disabled or out of match
                s_base_vm_fov.reset();
            }
        } catch ( ... ) { /* ignore viewmodel changer errors */ }

        // FOV Changer: external write to pawn's FOV if offsets available (debounced apply)
        try {
            const auto& fovc = g->core.get_settings().visuals.fov_changer;
            const bool have_offsets = (offsets::m_iFOV != 0) || (offsets::m_p_camera_services != 0);
            static bool s_fov_applied = false;
            static int s_prev_written_fov = 0;
            static std::optional<int> s_saved_default_fov; // last known default/current before we started
            // Debounce state
            using clock = std::chrono::steady_clock;
            static int s_last_desired = 0;
            static clock::time_point s_last_change_time = clock::now();
            static bool s_apply_pending = false;
            constexpr int k_debounce_ms = 200; // wait after last change before applying

            if ( have_offsets && s_in_match )
            {
                const auto lp = g->udata.get_owning_player().cs_player_pawn;
                if ( lp )
                {
                    bool scoped = false;
                    try { scoped = g->memory.read<bool>( lp + offsets::m_b_is_scoped ); } catch (...) { scoped = false; }

                    // Determine whether we should actively apply custom FOV this frame
                    const bool should_apply = fovc.enabled && !( fovc.disable_while_scoped && scoped );

                    if ( should_apply )
                    {
                        // Save default FOV once (if available) to restore later
                        if ( !s_saved_default_fov.has_value() )
                {
                    if ( offsets::m_iDefaultFOV != 0 )
                    {
                        try { s_saved_default_fov = g->memory.read<int>( lp + offsets::m_iDefaultFOV ); } catch (...) { s_saved_default_fov.reset(); }
                    }
                    if ( !s_saved_default_fov.has_value() )
                    {
                        // Fall back to current FOV as baseline
                        try { s_saved_default_fov = g->memory.read<int>( lp + offsets::m_iFOV ); } catch (...) { s_saved_default_fov = 90; }
                    }
                }

                // Clamp target and debounce application to a single write after settling
                int desired = std::clamp( fovc.desired_fov, 60, 140 );
                const auto now = clock::now();
                if (desired != s_last_desired) {
                    s_last_desired = desired;
                    s_last_change_time = now;
                    s_apply_pending = true;
                }

                const auto ms_since_change = std::chrono::duration_cast<std::chrono::milliseconds>(now - s_last_change_time).count();
                if (s_apply_pending && ms_since_change >= k_debounce_ms)
                {
                    try {
                        g->memory.write<int>( lp + offsets::m_iFOV, desired );
                        s_prev_written_fov = desired;
                        s_fov_applied = true;
                        s_apply_pending = false;
                        ++s_dbg_fov_ok; s_dbg_last_fov_write = steady::now(); s_dbg_last_fov_written = desired;
                        // Mirror default when available
                        if ( offsets::m_iDefaultFOV != 0 )
                        {
                            try { g->memory.write<int>( lp + offsets::m_iDefaultFOV, desired ); } catch (...) { /* ignore */ }
                        }
                        // Best-effort float writes
                        try { g->memory.write<float>( lp + offsets::m_iFOV, static_cast<float>(desired) ); } catch (...) { /* ignore */ }
                        if ( offsets::m_iDefaultFOV != 0 )
                        {
                            try { g->memory.write<float>( lp + offsets::m_iDefaultFOV, static_cast<float>(desired) ); } catch (...) { /* ignore */ }
                        }
                        // Camera services path, write once
                        if ( offsets::m_p_camera_services != 0 )
                        {
                            try {
                                const auto cam = g->memory.read<std::uintptr_t>( lp + offsets::m_p_camera_services );
                                if ( cam )
                                {
                                    const std::uintptr_t cs_iFOV      = offsets::cs_m_iFOV      ? offsets::cs_m_iFOV      : 0x288;
                                    const std::uintptr_t cs_iFOVStart = offsets::cs_m_iFOVStart ? offsets::cs_m_iFOVStart : 0x28C;
                                    const std::uintptr_t cs_flFOVTime = offsets::cs_m_flFOVTime ? offsets::cs_m_flFOVTime : 0x290;
                                    const std::uintptr_t cs_flFOVRate = offsets::cs_m_flFOVRate ? offsets::cs_m_flFOVRate : 0x294;
                                    if ( cs_iFOV ) {
                                        try { g->memory.write<std::uint32_t>( cam + cs_iFOV, static_cast<std::uint32_t>(desired) ); } catch (...) {}
                                        try { g->memory.write<float>( cam + cs_iFOV, static_cast<float>(desired) ); } catch (...) {}
                                    }
                                    if ( cs_iFOVStart ) {
                                        try { g->memory.write<std::uint32_t>( cam + cs_iFOVStart, static_cast<std::uint32_t>(desired) ); } catch (...) {}
                                        try { g->memory.write<float>( cam + cs_iFOVStart, static_cast<float>(desired) ); } catch (...) {}
                                    }
                                    if ( cs_flFOVRate ) {
                                        try { g->memory.write<float>( cam + cs_flFOVRate, 0.0f ); } catch (...) {}
                                    }
                                    if ( cs_flFOVTime ) {
                                        try { g->memory.write<float>( cam + cs_flFOVTime, 0.0f ); } catch (...) {}
                                    }
                                }
                            } catch (...) { /* ignore camera path errors */ }
                        }
                    } catch (...) { ++s_dbg_fov_fail; /* ignore fov write errors */ }
                }
            } // end of if (should_apply)
        } // end of if (lp)
    } // end of if (have_offsets && s_in_match)
    else
    {
        // Out of match or offsets missing: clear state and do no writes
        s_fov_applied = false;
        s_saved_default_fov.reset();
    }
    
} catch ( ... ) { /* ignore fov changer errors */ }
 

        // Radar: Force in-game radar (spotted) always when offsets are present; overlay drawing is removed
        try {
            static auto s_last_spot = std::chrono::steady_clock::now();
            const auto now = std::chrono::steady_clock::now();
            const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - s_last_spot).count();
            if ( ms >= 50 ) // throttle to ~20 Hz
            {
                s_last_spot = now;
                if ( offsets::m_entity_spotted_state && offsets::m_b_spotted )
                {
                    for ( const auto& p : players )
                    {
                        if ( !p.cs_player_pawn || p.health <= 0 ) continue;
                        if ( p.cs_player_pawn == local_pawn ) continue; // skip local
                        if ( dm.enabled && !dm.highlight_all && !is_enemy(p.team) ) continue; // respect DM filter

                        const std::uintptr_t spotted_addr = p.cs_player_pawn + offsets::m_entity_spotted_state + offsets::m_b_spotted;
                        try_write<bool>( spotted_addr, true );
                    }
                }
            }
        } catch ( ... ) { /* ignore radar errors */ }

        // World ESP: smoke, molotov, bomb, drops
        try {
            const auto& w = g->core.get_settings().visuals.world;
            if ( (w.smoke || w.smoke_dome || w.molotov || w.molotov_bounds || w.bomb || w.drops) && !stream_proof )
            {
                for ( const auto& m : g->updater.get_misc_entities() )
                {
                    if ( m.type == udata_c::misc_entity_t::type_t::smoke && (w.smoke || w.smoke_dome) )
                    {
                        if ( const auto* s = std::get_if<udata_c::misc_entity_t::smoke_data_t>( &m.data ) )
                        {
                            int dist_m = 0;
                            if ( local_origin_opt.has_value() ) dist_m = static_cast<int>( local_origin_opt.value().distance( s->detonation_pos ) / UNITS_PER_METER );
                            g->features.visuals.world.smoke( drawlist, s->detonation_pos, dist_m );
                        }
                    }
                    else if ( m.type == udata_c::misc_entity_t::type_t::molotov && (w.molotov || w.molotov_bounds) )
                    {
                        if ( const auto* mo = std::get_if<udata_c::misc_entity_t::molotov_data_t>( &m.data ) )
                        {
                            int dist_m = 0;
                            if ( local_origin_opt.has_value() ) dist_m = static_cast<int>( local_origin_opt.value().distance( mo->center ) / UNITS_PER_METER );
                            if ( w.molotov ) g->features.visuals.world.molotov( drawlist, mo->center, dist_m );
                            if ( w.molotov_bounds && !mo->fire_points.empty() ) g->features.visuals.world.molotov_bounds( drawlist, mo->fire_points );
                        }
                    }
                    else if ( m.type == udata_c::misc_entity_t::type_t::bomb && w.bomb )
                    {
                        if ( const auto* b = std::get_if<udata_c::misc_entity_t::bomb_data_t>( &m.data ) )
                        {
                            int dist_m = 0;
                            if ( local_origin_opt.has_value() ) dist_m = static_cast<int>( local_origin_opt.value().distance( b->position ) / UNITS_PER_METER );
                            g->features.visuals.world.bomb( drawlist, b->position, dist_m );
                        }
                    }
                    else if ( m.type == udata_c::misc_entity_t::type_t::drop && w.drops )
                    {
                        if ( const auto* d = std::get_if<udata_c::misc_entity_t::drop_data_t>( &m.data ) )
                        {
                            int dist_m = 0;
                            if ( local_origin_opt.has_value() ) dist_m = static_cast<int>( local_origin_opt.value().distance( d->position ) / UNITS_PER_METER );
                            const ImColor color = g->sdk.drops.get_color( d->name );
                            g->features.visuals.world.drop( drawlist, d->position, d->name, dist_m, color );
                            if ( w.drops_bounds )
                                g->features.visuals.world.drop_bounds( drawlist, d->node_transform, d->mins, d->maxs );
                        }
                    }
                }
            }
        } catch (...) { /* ignore world esp errors */ }

        // Spectator list: show players currently observing local player
        try {
            const auto& sset = g->core.get_settings().visuals.spectator;
            if ( s_in_match && sset.enabled )
            {
                // Helper: resolve entity handle to pointer using roots
                auto resolve_handle = [&]( std::uint32_t handle ) -> std::uintptr_t {
                    if ( !handle ) return 0ull;
                    const auto roots = g->udata.get_roots();
                    const auto list_entry = try_read<std::uintptr_t>( roots.dw_entity_list + 0x8ull * ( ( handle & 0x7FFF ) >> 9 ) + 0x10 );
                    if ( !list_entry.has_value() || !list_entry.value() ) return 0ull;
                    const auto ent = try_read<std::uintptr_t>( list_entry.value() + 120ull * ( handle & 0x1FF ) );
                    return ent.value_or(0ull);
                };

                std::vector<std::string> spectators;
                spectators.reserve( 16 );

                const auto localPawn = g->udata.get_owning_player().cs_player_pawn;
                const auto roots = g->udata.get_roots();

                // We want to support two modes:
                // 1) Normal: show who is spectating the local player
                // 2) When we are spectating someone: show who is spectating that target
                std::uintptr_t targetPawnForList = localPawn;
                std::string targetName;

                if ( roots.dw_entity_list )
                {
                    // First pass: find local controller (by matching pawn), and if we are observing someone, set targetPawnForList accordingly.
                    std::uintptr_t localController = 0ull;
                    for ( int i = 0; i < 64; ++i )
                    {
                        std::uintptr_t controller = 0;
                        try {
                            const auto list_entry = g->memory.read<std::uintptr_t>( roots.dw_entity_list + 0x8ull * ( i >> 9 ) + 0x10 );
                            if ( !list_entry ) continue;
                            controller = g->memory.read<std::uintptr_t>( list_entry + 120ull * ( i & 0x1FF ) );
                        } catch ( ... ) { continue; }
                        if ( !controller ) continue;

                        // Does this controller own the local pawn?
                        std::uintptr_t controllerPawnPtr = 0ull;
                        try {
                            const auto ph = try_read<std::uint32_t>( controller + offsets::m_h_player_pawn ).value_or(0);
                            if ( ph ) controllerPawnPtr = resolve_handle( ph );
                        } catch ( ... ) {}
                        if ( controllerPawnPtr && controllerPawnPtr == localPawn )
                        {
                            localController = controller;
                            break;
                        }
                    }

                    // If we found the local controller, try to resolve who we are spectating (if any)
                    if ( localController )
                    {
                        // Primary: controller's m_hObserverTarget
                        std::uintptr_t observedPawn = 0ull;
                        if ( const auto oh = try_read<std::uint32_t>( localController + offsets::m_h_observer_target ) )
                        {
                            const auto obs_handle = oh.value();
                            if ( obs_handle ) observedPawn = resolve_handle( obs_handle );
                        }
                        // Secondary: CCSPlayerController::m_hObserverPawn (alternative path)
                        if ( !observedPawn )
                        {
                            if ( const auto op = try_read<std::uint32_t>( localController + offsets::m_h_observer_pawn ) )
                            {
                                const auto pawn_handle = op.value();
                                if ( pawn_handle ) observedPawn = resolve_handle( pawn_handle );
                            }
                        }
                        // Fallback: via our pawn observer services
                        if ( !observedPawn )
                        {
                            try {
                                const auto lp_handle = try_read<std::uint32_t>( localController + offsets::m_h_player_pawn ).value_or(0);
                                if ( lp_handle )
                                {
                                    const auto lp_ptr = resolve_handle( lp_handle );
                                    if ( lp_ptr )
                                    {
                                        const auto obs_services = try_read_or<std::uintptr_t>( lp_ptr + offsets::m_p_observer_services, 0ull );
                                        if ( obs_services )
                                        {
                                            const auto svc_target = try_read<std::uint32_t>( obs_services + offsets::m_h_observer_target_services ).value_or(0);
                                            if ( svc_target ) observedPawn = resolve_handle( svc_target );
                                        }
                                    }
                                }
                            } catch ( ... ) {}
                        }

                        // If we are spectating someone, switch target
                        if ( observedPawn )
                            targetPawnForList = observedPawn;
                    }

                    // Second pass: build spectator list for targetPawnForList, and try to find target name
                    std::uintptr_t targetController = 0ull;
                    for ( int i = 0; i < 64; ++i )
                    {
                        std::uintptr_t controller = 0;
                        try {
                            const auto list_entry = g->memory.read<std::uintptr_t>( roots.dw_entity_list + 0x8ull * ( i >> 9 ) + 0x10 );
                            if ( !list_entry ) continue;
                            controller = g->memory.read<std::uintptr_t>( list_entry + 120ull * ( i & 0x1FF ) );
                        } catch ( ... ) { continue; }
                        if ( !controller ) continue;

                        // Resolve this controller's own pawn (for target name and to skip nulls)
                        std::uintptr_t controllerPawnPtr = 0ull;
                        try {
                            const auto ph = try_read<std::uint32_t>( controller + offsets::m_h_player_pawn ).value_or(0);
                            if ( ph ) controllerPawnPtr = resolve_handle( ph );
                        } catch ( ... ) {}
                        if ( controllerPawnPtr && controllerPawnPtr == targetPawnForList )
                            targetController = controller; // store for name lookup

                        // Resolve whom this controller is observing
                        std::uint32_t obs_handle = 0;
                        std::uintptr_t target_pawn = 0;
                        if ( const auto oh = try_read<std::uint32_t>( controller + offsets::m_h_observer_target ) ) { obs_handle = oh.value(); }
                        if ( obs_handle )
                            target_pawn = resolve_handle( obs_handle );

                        // Secondary path: controller's m_hObserverPawn
                        if ( !target_pawn )
                        {
                            if ( const auto op = try_read<std::uint32_t>( controller + offsets::m_h_observer_pawn ) )
                            {
                                const auto pawn_handle = op.value();
                                if ( pawn_handle ) target_pawn = resolve_handle( pawn_handle );
                            }
                        }

                        if ( !target_pawn )
                        {
                            try {
                                if ( controllerPawnPtr )
                                {
                                    const auto obs_services = try_read_or<std::uintptr_t>( controllerPawnPtr + offsets::m_p_observer_services, 0ull );
                                    if ( obs_services )
                                    {
                                        const auto svc_target = try_read<std::uint32_t>( obs_services + offsets::m_h_observer_target_services ).value_or(0);
                                        if ( svc_target ) target_pawn = resolve_handle( svc_target );
                                    }
                                }
                            } catch ( ... ) {}
                        }

                        if ( target_pawn && target_pawn == targetPawnForList )
                        {
                            // This controller is spectating our target
                            try {
                                const auto name_data = g->sdk.name.get_data( controller );
                                if ( !name_data.name.empty() )
                                    spectators.emplace_back( name_data.name );
                            } catch ( ... ) { /* ignore name errors */ }
                        }
                    }

                    // Resolve target name if possible
                    if ( targetController )
                    {
                        try {
                            const auto name_data = g->sdk.name.get_data( targetController );
                            if ( !name_data.name.empty() ) targetName = name_data.name;
                        } catch ( ... ) {}
                    }
                }

                // Draw spectator list (draggable panel)
                if ( !g->core.get_settings().extras.stream_proof && sset.enabled )
                {
                    ImGuiIO& io = ImGui::GetIO();
                    const ImU32 col = IM_COL32( (int)(sset.text_color[0]*255.0f), (int)(sset.text_color[1]*255.0f), (int)(sset.text_color[2]*255.0f), 255 );

                    // Layout metrics
                    const float line_h = ImGui::GetFontSize() + 2.0f;
                    const float pad = 6.0f;
                    const float header_h = line_h;
                    float content_h = 0.0f;
                    if ( sset.name )
                    {
                        content_h = spectators.empty() ? line_h : (float)spectators.size() * line_h;
                    }
                    float panel_w = 0.0f;
                    {
                        // Estimate width: max of header or names + padding
                        std::string header = targetName.empty() ? std::string("Spectators:") : (std::string("Spectators of ") + targetName + ":");
                        ImVec2 sz = ImGui::CalcTextSize(header.c_str());
                        panel_w = std::max(panel_w, sz.x);
                        if ( sset.name )
                        {
                            if ( spectators.empty() )
                            {
                                ImVec2 ns = ImGui::CalcTextSize("None");
                                panel_w = std::max(panel_w, ns.x + 8.0f);
                            }
                            else
                            {
                                for ( const auto& nm : spectators )
                                {
                                    ImVec2 ns = ImGui::CalcTextSize(nm.c_str());
                                    panel_w = std::max(panel_w, ns.x + 8.0f); // indent for names
                                }
                            }
                        }
                        panel_w += pad * 2.0f;
                    }
                    const float panel_h = pad + header_h + (content_h > 0.0f ? content_h + pad : pad);

                    // Draggable behavior
                    static bool s_dragging_spec = false;
                    static ImVec2 s_drag_offset{};
                    static bool s_user_moved_spec = false; // track if user repositioned panel this session

                    // Current position
                    float x = sset.window_pos[0];
                    float y = sset.window_pos[1];

                    // If user hasn't moved it this session, anchor under Ping Display
                    if ( !s_user_moved_spec )
                    {
                        // Ping tile in menu is drawn at (20, 20 + fps_offset) with padding 10x and 5y
                        const bool fps_on = g->core.get_settings().extras.fps_counter;
                        const float fps_offset = fps_on ? 25.0f : 0.0f;
                        const float ping_x = 20.0f;
                        const float ping_y = 20.0f + fps_offset;
                        // Approximate ping text height using typical string
                        const ImVec2 ping_text_sz = ImGui::CalcTextSize("Ping: 000ms");
                        const float ping_bg_h = ping_text_sz.y + 10.0f; // top/bottom padding from menu.cpp
                        const float gap = 8.0f;
                        x = ping_x; // left aligned with ping tile
                        y = ping_y + ping_bg_h + gap; // directly below ping tile
                    }

                    const ImVec2 p_min(x, y);
                    const ImVec2 p_max(x + panel_w, y + panel_h);
                    const bool hovered = io.MousePos.x >= p_min.x && io.MousePos.x <= p_max.x && io.MousePos.y >= p_min.y && io.MousePos.y <= p_max.y;

                    // Start drag when LMB pressed on panel
                    if ( hovered && ImGui::IsMouseClicked(0) )
                    {
                        s_dragging_spec = true;
                        s_drag_offset = ImVec2(io.MousePos.x - x, io.MousePos.y - y);
                    }
                    // End drag when LMB released
                    if ( s_dragging_spec && !ImGui::IsMouseDown(0) )
                    {
                        s_dragging_spec = false;
                    }
                    // Update position while dragging
                    if ( s_dragging_spec )
                    {
                        x = io.MousePos.x - s_drag_offset.x;
                        y = io.MousePos.y - s_drag_offset.y;
                        // Clamp to screen
                        const float sw = g->core.get_display().width;
                        const float sh = g->core.get_display().height;
                        x = std::clamp(x, 0.0f, std::max(0.0f, sw - panel_w));
                        y = std::clamp(y, 0.0f, std::max(0.0f, sh - panel_h));
                        g->core.get_settings().visuals.spectator.window_pos[0] = x;
                        g->core.get_settings().visuals.spectator.window_pos[1] = y;
                        s_user_moved_spec = true;
                    }

                    // Background
                    const ImU32 bg = IM_COL32(15, 15, 15, 180);
                    const ImU32 br = IM_COL32(10, 10, 10, 220);
                    drawlist->AddRectFilled( ImVec2(x, y), ImVec2(x + panel_w, y + panel_h), bg, 6.0f );
                    drawlist->AddRect( ImVec2(x, y), ImVec2(x + panel_w, y + panel_h), br, 6.0f, 0, 1.0f );

                    // Header text
                    float cy = y + pad;
                    if ( !targetName.empty() )
                    {
                        std::string hdr = std::string("Spectators of ") + targetName + ":";
                        drawlist->AddText( ImVec2( x + pad, cy ), col, hdr.c_str() );
                    }
                    else
                    {
                        drawlist->AddText( ImVec2( x + pad, cy ), col, "Spectators:" );
                    }
                    cy += header_h;
                    if ( sset.name )
                    {
                        if ( spectators.empty() )
                        {
                            drawlist->AddText( ImVec2( x + pad + 8.0f, cy ), col, "None" );
                        }
                        else
                        {
                            for ( const auto& nm : spectators )
                            {
                                drawlist->AddText( ImVec2( x + pad + 8.0f, cy ), col, nm.c_str() );
                                cy += line_h;
                            }
                        }
                    }
                }
            }
        } catch ( ... ) { /* ignore spectator errors */ }

        // Third Person wiring (use public run)
        try {
            g->features.exploits.third_person.run();
        } catch ( ... ) { /* ignore third-person errors */ }

    } catch (const std::exception& e) {
        g->core.log_error("Dispatch error: " + std::string(e.what()));
        if ( !g->core.get_settings().extras.stream_proof ) {
            const auto center = ImVec2(g->core.get_display().width * 0.5f, g->core.get_display().height * 0.5f);
            drawlist->AddText(center, ImColor(255, 0, 0, 255), "An unexpected error occurred. Please check the console.");
        }
    } catch (...) {
        g->core.log_error("Dispatch error: An unknown error occurred.");
        if ( !g->core.get_settings().extras.stream_proof ) {
            const auto center = ImVec2(g->core.get_display().width * 0.5f, g->core.get_display().height * 0.5f);
            drawlist->AddText(center, ImColor(255, 0, 0, 255), "An unexpected error occurred. Please check the console.");
        }
    }
}