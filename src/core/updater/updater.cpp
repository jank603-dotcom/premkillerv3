#include <include/global.hpp>
#include <shared_mutex>

// Shared cache mutex (writer in cleanup, readers in dispatch)
std::shared_mutex g_sdk_cache_mtx;

// Safe memory read helpers
namespace {
    template<typename T>
    static inline std::optional<T> try_read(std::uintptr_t addr) {
        try { return g->memory.read<T>(addr); } catch (...) { return std::nullopt; }
    }
    template<typename T>
    static inline T try_read_or(std::uintptr_t addr, T fallback) {
        try { return g->memory.read<T>(addr); } catch (...) { return fallback; }
    }
}

void updater_c::deploy( )
{
    using namespace std::chrono;

    stop_requested.store( false, std::memory_order_relaxed );

    t_maintenance = std::thread( [ this ]( ) {
        auto last_clean = std::chrono::steady_clock::now( );

        while ( !this->stop_requested.load( std::memory_order_relaxed ) )
        {
            try
            {
                this->update_roots( g->udata );

                {
                    g->raycasting.update_map( );

                    const auto& current_map = g->raycasting.get_current_map_name( );
                    if ( !current_map.empty( ) && this->last_known_map != current_map )
                    {
                        g->util.console.print( "map loaded: %s", current_map.c_str( ) );
                        this->last_known_map = current_map;

                        this->cleanup( );
                    }
                }

                const auto now = steady_clock::now( );
                if ( duration_cast< seconds >( now - last_clean ).count( ) >= 10 )
                {
                    this->cleanup( );
                    last_clean = now;
                }

                std::this_thread::sleep_for( milliseconds( 1000 ) );
            }
            catch ( ... )
            {
                std::this_thread::sleep_for( milliseconds( 1000 ) );
            }
        }
        } );

    t_local = std::thread( [ this ]( ) {
        while ( !this->stop_requested.load( std::memory_order_relaxed ) ) {
            try {
                this->update_owning_player_entity( g->udata );
                std::this_thread::sleep_for( milliseconds( 100 ) );
            }
            catch ( ... ) {
                std::this_thread::sleep_for( milliseconds( 100 ) );
            }
        }
        } );

    t_players = std::thread( [ this ]( ) {
        while ( !this->stop_requested.load( std::memory_order_relaxed ) ) {
            try {
                this->process_player_entities( g->udata );
                std::this_thread::sleep_for( milliseconds( 5 ) );
            }
            catch ( ... ) {
                std::this_thread::sleep_for( milliseconds( 5 ) );
            }
        }
        } );

    t_misc = std::thread( [ this ]( )
        {
            while ( !this->stop_requested.load( std::memory_order_relaxed ) )
            {
                try {
                    this->process_misc_entities( g->udata );
                    std::this_thread::sleep_for( milliseconds( 15 ) );
                }
                catch ( ... ) {
                    std::this_thread::sleep_for( milliseconds( 15 ) );
                }
            }
        } );
}

void updater_c::shutdown()
{
    stop_requested.store( true, std::memory_order_relaxed );

    auto join_if = [](std::thread& th) {
        if (th.joinable()) th.join();
    };
    join_if(t_misc);
    join_if(t_players);
    join_if(t_local);
    join_if(t_maintenance);
}

std::vector<udata_c::player_entity_t> updater_c::get_player_entities( ) const
{
    this->player_entity_buffer.update_read_buffer( );
    return this->player_entity_buffer.get_read_buffer( );
}

std::vector<udata_c::misc_entity_t> updater_c::get_misc_entities( ) const
{
    this->misc_entity_buffer.update_read_buffer( );
    return this->misc_entity_buffer.get_read_buffer( );
}

std::uintptr_t updater_c::resolve_entity_handle( std::uint32_t handle, udata_c& udata ) const
{
    if ( !handle )
    {
        return 0;
    }

    const auto list_entry = try_read<std::uintptr_t>( udata.get_roots( ).dw_entity_list + 0x8ull * ( ( handle & 0x7FFF ) >> 9 ) + 0x10 );
    if ( !list_entry.has_value() || !list_entry.value() )
    {
        return 0;
    }

    const auto ent = try_read<std::uintptr_t>( list_entry.value() + 120ull * ( handle & 0x1FF ) );
    return ent.value_or(0);
}

std::uintptr_t updater_c::get_entity_at_index( int index, udata_c& udata ) const
{
    const auto list_entry = try_read<std::uintptr_t>( udata.get_roots( ).dw_entity_list + 0x8ull * ( index >> 9 ) + 0x10 );
    if ( !list_entry.has_value() || !list_entry.value() )
    {
        return 0;
    }

    const auto ent = try_read<std::uintptr_t>( list_entry.value() + 120ull * ( index & 0x1FF ) );
    return ent.value_or(0);
}

void updater_c::update_roots( udata_c& udata )
{
    udata.get_roots( ).dw_entity_list = g->memory.read<std::uintptr_t>( g->core.get_process_info( ).client_base + offsets::dw_entity_list );
    udata.get_roots( ).dw_local_player_controller = g->memory.read<std::uintptr_t>( g->core.get_process_info( ).client_base + offsets::dw_local_player_controller );
}

void updater_c::update_owning_player_entity( udata_c& udata )
{
    // Make sure roots are populated
    if ( !udata.get_roots( ).dw_local_player_controller || !udata.get_roots( ).dw_entity_list )
    {
        this->update_roots( udata );
    }

    std::uintptr_t pawn = 0;

    // Primary path: resolve pawn handle from local player controller
    if ( const auto m_h_player_pawn = try_read<std::uint32_t>( udata.get_roots( ).dw_local_player_controller + offsets::m_h_player_pawn ) )
    {
        if ( m_h_player_pawn.value() )
            pawn = this->resolve_entity_handle( m_h_player_pawn.value(), udata );
    }

    // Fallback path: read pawn directly via dw_local_player_pawn
    if ( !pawn )
    {
        pawn = try_read_or<std::uintptr_t>( g->core.get_process_info( ).client_base + offsets::dw_local_player_pawn, 0ull );
    }

    if ( !pawn )
    {
        return;
    }

    auto& owning_player = udata.get_owning_player( );
    owning_player.cs_player_pawn = pawn;
    owning_player.m_p_bullet_services = try_read_or<std::uintptr_t>( pawn + offsets::m_p_bullet_services, 0ull );
    owning_player.m_p_weapon_services = try_read_or<std::uintptr_t>( pawn + offsets::m_p_weapon_services, 0ull );
    owning_player.m_p_game_scene_node = try_read_or<std::uintptr_t>( pawn + offsets::m_p_game_scene_node, 0ull );
    owning_player.team = try_read_or<int>( pawn + offsets::m_i_team_num, 0 );
    owning_player.weapon_data = g->sdk.weapon.get_data( pawn );
}

void updater_c::process_player_entities( udata_c& udata )
{
    const auto roots = udata.get_roots();
    const auto owner = udata.get_owning_player();
    if ( !roots.dw_entity_list ) { std::this_thread::sleep_for(std::chrono::milliseconds(25)); return; }
    if ( !owner.cs_player_pawn ) { std::this_thread::sleep_for(std::chrono::milliseconds(25)); return; }

    auto& temp_player_entities = this->player_entity_buffer.get_write_buffer( );
    temp_player_entities.clear( );
    temp_player_entities.reserve( 64 );

    for ( int i = 0; i < 64; ++i )
    {
        std::uintptr_t entity = this->get_entity_at_index( i, udata );
        if ( !entity )
        {
            continue;
        }

        const auto pawn_opt = this->resolve_player_entity( entity );
        if ( !pawn_opt.has_value( ) )
        {
            continue;
        }

        const auto pawn = pawn_opt.value( );
        if ( !pawn || pawn == udata.get_owning_player( ).cs_player_pawn )
        {
            continue;
        }

        int health = try_read_or<int>( pawn + offsets::m_i_health, 0 );
        if ( health <= 0 || health > 1337 )
        {
            continue;
        }

        int team = try_read_or<int>( pawn + offsets::m_i_team_num, 0 );
        if ( team == udata.get_owning_player( ).team )
        {
            continue;
        }

        std::uintptr_t m_p_game_scene_node = try_read_or<std::uintptr_t>( pawn + offsets::m_p_game_scene_node, 0ull );
        if ( !m_p_game_scene_node )
        {
            continue;
        }

        std::uintptr_t bone_array = try_read_or<std::uintptr_t>( m_p_game_scene_node + offsets::m_model_state + 0x80, 0ull );
        if ( !bone_array )
        {
            continue;
        }

        udata_c::player_entity_t player{};
        player.cs_player_controller = entity;
        player.cs_player_pawn = pawn;
        player.m_p_game_scene_node = m_p_game_scene_node;
        player.bone_array = bone_array;
        player.team = team;
        player.health = health;

        if ( g->core.get_settings( ).visuals.player.chamies )
        {
            try { player.hitbox_data = g->sdk.hitboxes.get_data( m_p_game_scene_node ); } catch (...) {}
        }

        if ( g->core.get_settings( ).visuals.player.name )
        {
            try { player.name_data = g->sdk.name.get_data( entity ); } catch (...) {}
        }

        if ( g->core.get_settings( ).visuals.player.weapon )
        {
            try { player.weapon_data = g->sdk.weapon.get_data( pawn ); } catch (...) {}
        }

        temp_player_entities.emplace_back( std::move( player ) );
    }

    this->player_entity_buffer.swap_write_buffer( );
}

void updater_c::process_misc_entities( udata_c& udata ) // shit coded
{
    const auto roots = udata.get_roots();
    const auto owner = udata.get_owning_player();
    if ( !roots.dw_entity_list ) { std::this_thread::sleep_for(std::chrono::milliseconds(25)); return; }
    if ( !owner.cs_player_pawn ) { std::this_thread::sleep_for(std::chrono::milliseconds(25)); return; }

    std::vector<const char*> allowed_types;

    if ( g->core.get_settings( ).visuals.world.smoke || g->core.get_settings( ).visuals.world.smoke_dome )
    {
        allowed_types.emplace_back( "C_SmokeGrenadeProjectile" );
    }
    if ( g->core.get_settings( ).visuals.world.molotov || g->core.get_settings( ).visuals.world.molotov_bounds )
    {
        allowed_types.emplace_back( "C_Inferno" );
    }
    if ( g->core.get_settings( ).misc.night_mode )
    {
        allowed_types.emplace_back( "C_PostProcessingVolume" );
    }
    if ( g->core.get_settings( ).visuals.world.bomb )
    {
        allowed_types.emplace_back( "C_PlantedC4" );
    }
    if ( g->core.get_settings( ).visuals.world.drops )
    {
        g->sdk.drops.add_to_allowed( allowed_types );
    }

    if ( allowed_types.empty( ) )
    {
        return;
    }

    auto& temp_misc_entities = this->misc_entity_buffer.get_write_buffer( );
    temp_misc_entities.clear( );
    temp_misc_entities.reserve( 1000 ); // eh

    for ( int i = 64; i < 1024; ++i )
    {
        std::uintptr_t entity = 0;
        try { entity = this->get_entity_at_index( i, udata ); } catch (...) { continue; }
        if ( !entity )
        {
            continue;
        }

        const auto entity_type_opt = this->resolve_misc_entity_type( entity, allowed_types );
        if ( !entity_type_opt.has_value( ) )
        {
            continue;
        }

        const auto& entity_type = entity_type_opt.value( );
        if ( entity_type.empty( ) )
        {
            continue;
        }

        auto misc_type = udata_c::misc_entity_t::type_t::invalid;
        if ( _stricmp( entity_type.c_str( ), "C_SmokeGrenadeProjectile" ) == 0 )
        {
            misc_type = udata_c::misc_entity_t::type_t::smoke;
        }
        else if ( _stricmp( entity_type.c_str( ), "C_Inferno" ) == 0 )
        {
            misc_type = udata_c::misc_entity_t::type_t::molotov;
        }
        else if ( _stricmp( entity_type.c_str( ), "C_PostProcessingVolume" ) == 0 )
        {
            misc_type = udata_c::misc_entity_t::type_t::post_processing_volume;
        }
        else if ( _stricmp( entity_type.c_str( ), "C_EnvSky" ) == 0 )
        {
            misc_type = udata_c::misc_entity_t::type_t::env_sky;
        }
        else if ( _stricmp( entity_type.c_str( ), "C_PlantedC4" ) == 0 )
        {
            misc_type = udata_c::misc_entity_t::type_t::bomb;
        }
        else if ( g->sdk.drops.is( entity_type ) )
        {
            misc_type = udata_c::misc_entity_t::type_t::drop;
        }
        else
        {
            continue;
        }

        udata_c::misc_entity_t misc{};

        misc.entity = entity;
        misc.type = misc_type;

        if ( misc_type == udata_c::misc_entity_t::type_t::smoke )
        {
            bool spawned = false;
            try { spawned = g->memory.read<bool>( entity + offsets::m_b_smoke_effect_spawned ); } catch (...) { spawned = false; }
            if ( spawned )
            {
                try { misc.data = udata_c::misc_entity_t::smoke_data_t{ .detonation_pos = g->memory.read<math::vector3>( entity + offsets::m_v_smoke_detonation_pos ) }; } catch (...) {}
            }
        }
        else if ( misc_type == udata_c::misc_entity_t::type_t::molotov )
        {
            int fire_count = 0;
            try { fire_count = g->memory.read<int>( entity + offsets::m_fire_count ); } catch (...) { continue; }
            if ( fire_count <= 0 || fire_count > 64 )
            {
                continue;
            }

            udata_c::misc_entity_t::molotov_data_t molotov_data{};
            bool any_burning = false;

            for ( int j = 0; j < fire_count; ++j )
            {
                bool is_burning = false;
                try { is_burning = g->memory.read<bool>( entity + offsets::m_b_fire_is_burning + j ); } catch (...) { is_burning = false; }
                if ( !is_burning )
                {
                    continue;
                }

                any_burning = true;
                math::vector3 fire_pos{};
                try { fire_pos = g->memory.read<math::vector3>( entity + offsets::m_fire_positions + sizeof( math::vector3 ) * j ); } catch (...) { continue; }
                molotov_data.fire_points.push_back( fire_pos );
            }

            if ( any_burning && !molotov_data.fire_points.empty( ) )
            {
                math::vector3 sum{};
                for ( const auto& pt : molotov_data.fire_points )
                {
                    sum += pt;
                }

                molotov_data.center = sum / static_cast< float >( molotov_data.fire_points.size( ) );
                misc.data = std::move( molotov_data );
            }
            else
            {
                continue;
            }
        }
        else if ( misc_type == udata_c::misc_entity_t::type_t::bomb )
        {
            std::uintptr_t m_p_game_scene_node = 0;
            try { m_p_game_scene_node = g->memory.read<std::uintptr_t>( entity + offsets::m_p_game_scene_node ); } catch (...) { m_p_game_scene_node = 0; }
            if ( m_p_game_scene_node )
            {
                try { misc.data = udata_c::misc_entity_t::bomb_data_t{ .position = g->memory.read<math::vector3>( m_p_game_scene_node + offsets::m_vec_origin ) }; } catch (...) {}
            }
        }
        else if ( misc_type == udata_c::misc_entity_t::type_t::drop ) // meh
        {
            std::uintptr_t m_p_game_scene_node = 0;
            try { m_p_game_scene_node = g->memory.read<std::uintptr_t>( entity + offsets::m_p_game_scene_node ); } catch (...) { m_p_game_scene_node = 0; }
            if ( m_p_game_scene_node )
            {
                std::uintptr_t m_p_collision = 0;
                try { m_p_collision = g->memory.read<std::uintptr_t>( entity + offsets::m_p_collision ); } catch (...) { m_p_collision = 0; }
                if ( m_p_collision )
                {
                    math::vector3 collision_min{};
                    math::vector3 collision_max{};
                    try { collision_min = g->memory.read<math::vector3>( m_p_collision + 0x40 ); } catch (...) {}
                    try { collision_max = g->memory.read<math::vector3>( m_p_collision + 0x4C ); } catch (...) {}

                    math::transform node_to_world{};
                    try { node_to_world = g->memory.read<math::transform>( m_p_game_scene_node + 0x10 ); } catch (...) {}

                    misc.data = udata_c::misc_entity_t::drop_data_t{
                        .position = [&]{ try { return g->memory.read<math::vector3>( m_p_game_scene_node + offsets::m_vec_origin ); } catch (...) { return math::vector3{}; } }(),
                        .name = g->sdk.drops.get_display_name( entity_type ),
                        .mins = collision_min,
                        .maxs = collision_max,
                        .node_transform = node_to_world,
                    };
                }
            }
        }

        temp_misc_entities.emplace_back( std::move( misc ) );
    }

    this->misc_entity_buffer.swap_write_buffer( );
}

std::optional<std::uintptr_t> updater_c::resolve_player_entity( std::uintptr_t entity )
{
    try
    {
        const auto handle = g->memory.read<std::uint32_t>( entity + offsets::m_h_player_pawn );
        if ( !handle )
        {
            return std::nullopt;
        }

        const auto pawn = this->resolve_entity_handle( handle, g->udata );
        if ( !pawn )
        {
            return std::nullopt;
        }

        return pawn;
    }
    catch ( ... )
    {
        return std::nullopt;
    }
}

std::optional<std::string> updater_c::resolve_misc_entity_type( std::uintptr_t entity, std::span<const char*> expected_classes )
{
    try
    {
        const auto ent_identity = g->memory.read<std::uintptr_t>( entity + 0x10 );
        if ( !ent_identity )
        {
            return std::nullopt;
        }

        const auto class_info = g->memory.read<std::uintptr_t>( ent_identity + 0x8 );
        if ( !class_info )
        {
            return std::nullopt;
        }

        const auto schema_info = g->memory.read<std::uintptr_t>( class_info + 0x30 );
        if ( !schema_info )
        {
            return std::nullopt;
        }

        const auto name_ptr = g->memory.read<std::uintptr_t>( schema_info + 0x8 );
        if ( !name_ptr )
        {
            return std::nullopt;
        }

        char ent_name[ 128 ]{};
        if ( !g->memory.read_process_memory( name_ptr, ent_name, sizeof( ent_name ) ) )
        {
            return std::nullopt;
        }

        ent_name[ 127 ] = '\0';

        for ( const auto& expected : expected_classes )
        {
            if ( _stricmp( ent_name, expected ) == 0 )
            {
                return std::string( ent_name );
            }
        }

        return std::nullopt;
    }
    catch ( ... )
    {
        return std::nullopt;
    }
}

void updater_c::cleanup( )
{
    std::unique_lock lk(g_sdk_cache_mtx);
    g->sdk.hitboxes.clear_cache( );
    g->sdk.name.clear_cache( );
    g->sdk.weapon.clear_cache( );
}