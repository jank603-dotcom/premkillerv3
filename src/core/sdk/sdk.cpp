#include <include/global.hpp>	

bool sdk_c::bones_c::data_t::is_valid( ) const
{
	const auto& root = this->bones[ 0 ].position;
	return root.x != 0.0f || root.y != 0.0f || root.z != 0.0f;
}

math::vector3 sdk_c::bones_c::data_t::get_position( bone_id id ) const
{
	const auto index = static_cast< std::size_t >( id );
	if ( index >= this->bones.size( ) )
	{
		return { };
	}

	return this->bones[ index ].position;
}

math::quaternion sdk_c::bones_c::data_t::get_rotation( bone_id id ) const
{
	const auto index = static_cast< std::size_t >( id );
	if ( index >= this->bones.size( ) )
	{
		return { };
	}

	return this->bones[ index ].rotation;
}

sdk_c::bones_c::data_t sdk_c::bones_c::get_data( std::uintptr_t bone_array ) const
{
	data_t out{ };

	struct alignas( 16 ) c_bone_data
	{
		math::vector3 position;
		float scale;
		math::quaternion rotation;
	};

	std::array<c_bone_data, out.bones.size( )> raw_bones{};
	if ( !g->memory.read_process_memory( bone_array, raw_bones.data( ), sizeof( raw_bones ) ) )
	{
		return out;
	}

	for ( std::size_t i = 0; i < out.bones.size( ); ++i )
	{
		out.bones[ i ].position = raw_bones[ i ].position;
		out.bones[ i ].rotation = raw_bones[ i ].rotation;
	}

	return out;
}

bool sdk_c::bones_c::is_any_major_bone_visible( const data_t& bone_data )
{
	using bone = sdk_c::bones_c::bone_id;

	const std::vector<bone> boners = {
		bone::head,
		bone::spine_3,
		bone::spine_2,
		bone::pelvis,
		bone::left_shoulder,
		bone::right_shoulder,
		bone::left_hand,
		bone::right_hand,
		bone::left_knee,
		bone::right_knee
	};

	const auto camera_pos = this->parent->camera.get_data().position;
	int visible_count = 0;
	int total_checked = 0;

	for ( const auto& b : boners )
	{
		const auto pos = bone_data.get_position( b );
		const auto screen = this->parent->w2s.project( pos );

		if ( !this->parent->w2s.is_valid( screen ) )
		{
			continue;
		}

		total_checked++;
		if ( g->raycasting.is_visible( camera_pos, pos ) )
		{
			visible_count++;
		}
	}

	// If no raycasting data available or less than 3 bones checked, be more permissive
	if ( total_checked < 3 || g->raycasting.get_current_map_name().empty() )
	{
		return visible_count > 0 || total_checked == 0;
	}

	// At least 30% of bones should be visible
	return visible_count > 0 && (static_cast<float>(visible_count) / total_checked) >= 0.3f;
}

sdk_c::bounds_c::data_t sdk_c::bounds_c::get_data( const bones_c::data_t& bone_data ) const
{
	using bone = bones_c::bone_id;

	const std::vector<bone> boners = {
		bone::head,
		bone::left_hip,
		bone::right_hip,
		bone::left_knee,
		bone::right_knee,
		bone::left_foot,
		bone::right_foot,
		bone::left_shoulder,
		bone::right_shoulder,
		bone::left_hand,
		bone::right_hand
	};

	const auto first_screen = parent->w2s.project( bone_data.get_position( boners[ 0 ] ) );

	auto min_x = first_screen.x;
	auto max_x = first_screen.x;
	auto min_y = first_screen.y;
	auto max_y = first_screen.y;

	for ( std::size_t i = 1; i < boners.size( ); ++i )
	{
		const auto screen = parent->w2s.project( bone_data.get_position( boners[ i ] ) );

		min_x = std::min( min_x, screen.x );
		max_x = std::max( max_x, screen.x );
		min_y = std::min( min_y, screen.y );
		max_y = std::max( max_y, screen.y );
	}

	const auto height = max_y - min_y;
	const auto width = max_x - min_x;

	const auto width_pad = width * 0.25f;
	const auto height_top_pad = height * 0.175f;
	const auto height_bottom_pad = height * 0.15f;

	return {
		min_x - width_pad,
		max_x + width_pad,
		min_y - height_top_pad,
		max_y + height_bottom_pad
	};
}

sdk_c::hitboxes_c::data_t sdk_c::hitboxes_c::get_data( std::uintptr_t m_p_game_scene_node ) const
{
	const auto model_handle = g->memory.read<std::uintptr_t>( m_p_game_scene_node + offsets::m_model_state + 0xD0 );
	if ( !model_handle )
	{
		return {};
	}

	const auto cmodel = g->memory.read<std::uintptr_t>( model_handle );
	if ( !cmodel )
	{
		return {};
	}

	const auto it = this->hitbox_cache.find( cmodel );
	if ( it != this->hitbox_cache.end( ) )
	{
		return it->second;
	}

	data_t out{};

	const auto render_meshes = g->memory.read<std::uintptr_t>( g->memory.read<std::uintptr_t>( cmodel + 0x78 ) );
	if ( !render_meshes )
	{
		return out;
	}

	const auto hitbox_data = g->memory.read<std::uintptr_t>( render_meshes + 0x140 );
	if ( !hitbox_data )
	{
		return out;
	}

	const auto hitbox_count = g->memory.read<int>( hitbox_data + 0x28 );
	if ( hitbox_count <= 0 || hitbox_count > 64 )
	{
		return out;
	}

	const auto hitbox_array = g->memory.read<std::uintptr_t>( hitbox_data + 0x30 );
	if ( !hitbox_array )
	{
		return out;
	}

	for ( int i = 0; i < hitbox_count; ++i )
	{
		hitbox_t hbox{};
		hbox.index = i;

		const auto hb_addr = hitbox_array + static_cast< std::uintptr_t >( i ) * 0x70;

		hbox.bb_min = g->memory.read<math::vector3>( hb_addr + 0x18 );
		hbox.bb_max = g->memory.read<math::vector3>( hb_addr + 0x24 );
		hbox.radius = g->memory.read<float>( hb_addr + 0x30 );

		if ( hbox.radius < 0.f || hbox.radius > 100.f )
		{
			continue;
		}

		hbox.bone_index = this->get_bone_id_from_hitbox( i );
		if ( hbox.bone_index == -1 )
		{
			continue;
		}

		out.push_back( std::move( hbox ) );
	}

	this->hitbox_cache[ cmodel ] = out;

	return out;
}

void sdk_c::hitboxes_c::clear_cache( )
{
	this->hitbox_cache.clear( );
}

void sdk_c::w2s_c::update( )
{
	this->view_matrix = g->memory.read<math::matrix4x4>( g->core.get_process_info( ).client_base + offsets::dw_view_matrix );
}

math::vector2 sdk_c::w2s_c::project( const math::vector3& position )
{
	const auto& m = this->view_matrix;

	const auto x = m[ 0 ][ 0 ] * position.x + m[ 0 ][ 1 ] * position.y + m[ 0 ][ 2 ] * position.z + m[ 0 ][ 3 ];
	const auto y = m[ 1 ][ 0 ] * position.x + m[ 1 ][ 1 ] * position.y + m[ 1 ][ 2 ] * position.z + m[ 1 ][ 3 ];
	const auto w = m[ 3 ][ 0 ] * position.x + m[ 3 ][ 1 ] * position.y + m[ 3 ][ 2 ] * position.z + m[ 3 ][ 3 ];

	if ( w < 0.01f )
	{
		return { FLT_MAX, FLT_MAX };
	}

	const auto inv_w = 1.0f / w;
	const auto screen_x = ( g->core.get_display( ).width * 0.5f ) + ( 0.5f * x * inv_w * g->core.get_display( ).width );
	const auto screen_y = ( g->core.get_display( ).height * 0.5f ) - ( 0.5f * y * inv_w * g->core.get_display( ).height );

	return { screen_x, screen_y };
}

bool sdk_c::w2s_c::is_valid( const math::vector2& projected )
{
	return projected.x != FLT_MAX && projected.y != FLT_MAX;
}

bool sdk_c::w2s_c::is_on_screen( const math::vector2& projected ) const
{
	return projected.x >= 0.0f && projected.x <= static_cast< float >( g->core.get_display( ).width ) && projected.y >= 0.0f && projected.y <= static_cast< float >( g->core.get_display( ).height );
}

void sdk_c::camera_c::update( )
{
	const auto origin = g->memory.read<math::vector3>( g->udata.get_owning_player( ).m_p_game_scene_node + offsets::m_vec_origin );
	const auto view_offset = g->memory.read<math::vector3>( g->udata.get_owning_player( ).cs_player_pawn + offsets::m_vec_view_offset );
	const auto view_angles = g->memory.read<math::vector3>( g->udata.get_owning_player( ).cs_player_pawn + offsets::m_ang_eye_angles );

	this->data.position = origin + view_offset;
	this->data.rotation = math::func::from_euler_angles( view_angles );
	this->data.fov = 0.0f;
}

sdk_c::name_c::data_t sdk_c::name_c::get_data( std::uintptr_t cs_player_controller ) const
{
	if ( const auto it = this->name_cache.find( cs_player_controller ); it != this->name_cache.end( ) )
	{
		return it->second;
	}

	data_t out{ };

	const auto name_ptr = g->memory.read<std::uintptr_t>( cs_player_controller + offsets::m_s_sanitized_player_name );
	if ( name_ptr )
	{
		char buffer[ 64 ]{};
		if ( g->memory.read_process_memory( name_ptr, buffer, sizeof( buffer ) ) )
		{
			out.name = buffer;
		}
	}

	this->name_cache[ cs_player_controller ] = out;
	return out;
}

void sdk_c::name_c::clear_cache( )
{
	this->name_cache.clear( );
}

sdk_c::weapon_c::data_t sdk_c::weapon_c::get_data_from_entity( std::uintptr_t entity ) const
{
	static const ankerl::unordered_dense::map<std::string, data_t> weapon_map = {
		{ "hkp2000",   { "P2000",        "secondary" } },
		{ "glock",     { "Glock",        "secondary" } },
		{ "deagle",    { "Deagle",       "secondary" } },
		{ "p250",      { "P250",         "secondary" } },
		{ "elite",     { "Dual Berettas","secondary" } },
		{ "fiveseven", { "Five-Seven",   "secondary" } },
		{ "tec9",      { "Tec-9",        "secondary" } },
		{ "m4a1",      { "M4A1-S",       "primary" } },
		{ "ak47",      { "AK-47",        "primary" } },
		{ "aug",       { "AUG",          "primary" } },
		{ "sg556",     { "SG 553",       "primary" } },
		{ "galilar",   { "Galil AR",     "primary" } },
		{ "famas",     { "FAMAS",        "primary" } },
		{ "ssg08",     { "SSG 08",       "primary" } },
		{ "scar20",    { "SCAR-20",      "primary" } },
		{ "m249",      { "M249",         "primary" } },
		{ "g3sg1",     { "G3SG1",        "primary" } },
		{ "ump45",     { "UMP-45",       "primary" } },
		{ "mp7",       { "MP7",          "primary" } },
		{ "nova",      { "Nova",         "primary" } },
		{ "xm1014",    { "XM1014",       "primary" } },
		{ "mp9",       { "MP9",          "primary" } },
		{ "mac10",     { "MAC-10",       "primary" } },
		{ "p90",       { "P90",          "primary" } },
		{ "awp",       { "AWP",          "primary" } },
		{ "bizon",     { "PP-Bizon",     "primary" } },
		{ "sawedoff",  { "Sawed-Off",    "primary" } },
		{ "mag7",      { "MAG-7",        "primary" } },
		{ "negev",     { "Negev",        "primary" } },
		{ "smokegrenade", { "Smoke",     "grenade" } },
		{ "hegrenade",    { "HE Grenade", "grenade" } },
		{ "flashbang",    { "Flash",      "grenade" } },
		{ "decoy",        { "Decoy",      "grenade" } },
		{ "c4",           { "C4",         "c4" } },
		{ "knife",        { "Knife",      "knife" } }
	};

	static const data_t unknown{ "unknown", "unknown" };

	if ( !entity )
	{
		return unknown;
	}

	if ( const auto it = this->weapon_cache.find( entity ); it != this->weapon_cache.end( ) )
	{
		return it->second;
	}

	const auto name_ptr = g->memory.read<std::uintptr_t>( g->memory.read<std::uintptr_t>( entity + 0x10 ) + 0x20 );
	if ( !name_ptr )
	{
		return unknown;
	}

	char buffer[ 64 ]{};
	if ( !g->memory.read_process_memory( name_ptr, buffer, sizeof( buffer ) ) )
	{
		return unknown;
	}

	std::string name = buffer;
	std::erase( name, '"' );
	std::erase( name, '\n' );
	std::erase( name, '\r' );

	if ( name.starts_with( "weapon_" ) )
	{
		name = name.substr( 7 );
	}

	std::ranges::transform( name, name.begin( ), ::tolower );

	const auto it = weapon_map.find( name );
	const auto& result = it != weapon_map.end( ) ? it->second : unknown;

	this->weapon_cache[ entity ] = result;
	return result;
}

sdk_c::weapon_c::data_t sdk_c::weapon_c::get_data( std::uintptr_t cs_player_pawn ) const
{
	const auto clipping_weapon = g->memory.read<std::uintptr_t>( cs_player_pawn + offsets::m_p_clipping_weapon );
	return this->get_data_from_entity( clipping_weapon );
}

void sdk_c::weapon_c::clear_cache( )
{
	this->weapon_cache.clear( );
}

void sdk_c::drops_c::add_to_allowed( std::vector<const char*>& allowed_types )
{
	allowed_types.emplace_back( "C_AK47" );
	allowed_types.emplace_back( "C_WeaponM4A1" );
	allowed_types.emplace_back( "C_WeaponM4A1Silencer" );
	allowed_types.emplace_back( "C_WeaponAWP" );
	allowed_types.emplace_back( "C_WeaponAug" );
	allowed_types.emplace_back( "C_WeaponFamas" );
	allowed_types.emplace_back( "C_WeaponGalilAR" );
	allowed_types.emplace_back( "C_WeaponSG556" );
	allowed_types.emplace_back( "C_WeaponG3SG1" );
	allowed_types.emplace_back( "C_WeaponSCAR20" );
	allowed_types.emplace_back( "C_WeaponSSG08" );

	allowed_types.emplace_back( "C_WeaponMAC10" );
	allowed_types.emplace_back( "C_WeaponMP5SD" );
	allowed_types.emplace_back( "C_WeaponMP7" );
	allowed_types.emplace_back( "C_WeaponMP9" );
	allowed_types.emplace_back( "C_WeaponBizon" );
	allowed_types.emplace_back( "C_WeaponP90" );
	allowed_types.emplace_back( "C_WeaponUMP45" );

	allowed_types.emplace_back( "C_WeaponNOVA" );
	allowed_types.emplace_back( "C_WeaponSawedoff" );
	allowed_types.emplace_back( "C_WeaponXM1014" );
	allowed_types.emplace_back( "C_WeaponMag7" );

	allowed_types.emplace_back( "C_WeaponM249" );
	allowed_types.emplace_back( "C_WeaponNegev" );

	allowed_types.emplace_back( "C_DEagle" );
	allowed_types.emplace_back( "C_WeaponElite" );
	allowed_types.emplace_back( "C_WeaponFiveSeven" );
	allowed_types.emplace_back( "C_WeaponGlock" );
	allowed_types.emplace_back( "C_WeaponHKP2000" );
	allowed_types.emplace_back( "C_WeaponUSPSilencer" );
	allowed_types.emplace_back( "C_WeaponP250" );
	allowed_types.emplace_back( "C_WeaponCZ75a" );
	allowed_types.emplace_back( "C_WeaponTec9" );
	allowed_types.emplace_back( "C_WeaponRevolver" );
	allowed_types.emplace_back( "C_WeaponTaser" );

	allowed_types.emplace_back( "C_C4" );
	allowed_types.emplace_back( "C_Knife" );

	allowed_types.emplace_back( "C_HEGrenade" );
	allowed_types.emplace_back( "C_Flashbang" );
	allowed_types.emplace_back( "C_SmokeGrenade" );
	allowed_types.emplace_back( "C_DecoyGrenade" );
	allowed_types.emplace_back( "C_MolotovGrenade" );
	allowed_types.emplace_back( "C_IncendiaryGrenade" );
}

bool sdk_c::drops_c::is( const std::string& entity_type )
{
	static const ankerl::unordered_dense::set<std::string> weapon_types = {
		"C_AK47", "C_WeaponM4A1", "C_WeaponM4A1Silencer", "C_WeaponAWP",
		"C_WeaponAug", "C_WeaponFamas", "C_WeaponGalilAR", "C_WeaponSG556",
		"C_WeaponG3SG1", "C_WeaponSCAR20", "C_WeaponSSG08",

		"C_WeaponMAC10", "C_WeaponMP5SD", "C_WeaponMP7", "C_WeaponMP9",
		"C_WeaponBizon", "C_WeaponP90", "C_WeaponUMP45",

		"C_WeaponNOVA", "C_WeaponSawedoff", "C_WeaponXM1014", "C_WeaponMag7",

		"C_WeaponM249", "C_WeaponNegev",

		"C_DEagle", "C_WeaponElite", "C_WeaponFiveSeven", "C_WeaponGlock",
		"C_WeaponHKP2000", "C_WeaponUSPSilencer", "C_WeaponP250",
		"C_WeaponCZ75a", "C_WeaponTec9", "C_WeaponRevolver", "C_WeaponTaser",

		"C_C4", "C_Knife",

		"C_HEGrenade", "C_Flashbang", "C_SmokeGrenade", "C_DecoyGrenade",
		"C_MolotovGrenade", "C_IncendiaryGrenade"
	};

	return weapon_types.find( entity_type ) != weapon_types.end( );
}

std::string sdk_c::drops_c::get_display_name( const std::string& schema )
{
	static const ankerl::unordered_dense::map<std::string, std::string> weapon_names = {
		{"C_AK47", "AK-47"},
		{"C_WeaponM4A1", "M4A4"},
		{"C_WeaponM4A1Silencer", "M4A1-S"},
		{"C_WeaponAWP", "AWP"},
		{"C_WeaponAug", "AUG"},
		{"C_WeaponFamas", "FAMAS"},
		{"C_WeaponGalilAR", "Galil AR"},
		{"C_WeaponSG556", "SG 553"},
		{"C_WeaponG3SG1", "G3SG1"},
		{"C_WeaponSCAR20", "SCAR-20"},
		{"C_WeaponSSG08", "SSG 08"},

		{"C_WeaponMAC10", "MAC-10"},
		{"C_WeaponMP5SD", "MP5-SD"},
		{"C_WeaponMP7", "MP7"},
		{"C_WeaponMP9", "MP9"},
		{"C_WeaponBizon", "PP-Bizon"},
		{"C_WeaponP90", "P90"},
		{"C_WeaponUMP45", "UMP-45"},

		{"C_WeaponNOVA", "Nova"},
		{"C_WeaponSawedoff", "Sawed-Off"},
		{"C_WeaponXM1014", "XM1014"},
		{"C_WeaponMag7", "MAG-7"},

		{"C_WeaponM249", "M249"},
		{"C_WeaponNegev", "Negev"},

		{"C_DEagle", "Desert Eagle"},
		{"C_WeaponElite", "Dual Berettas"},
		{"C_WeaponFiveSeven", "Five-SeveN"},
		{"C_WeaponGlock", "Glock-18"},
		{"C_WeaponHKP2000", "P2000"},
		{"C_WeaponUSPSilencer", "USP-S"},
		{"C_WeaponP250", "P250"},
		{"C_WeaponCZ75a", "CZ75-Auto"},
		{"C_WeaponTec9", "Tec-9"},
		{"C_WeaponRevolver", "R8 Revolver"},
		{"C_WeaponTaser", "Zeus x27"},

		{"C_C4", "C4"},
		{"C_Knife", "Knife"},

		{"C_HEGrenade", "HE Grenade"},
		{"C_Flashbang", "Flashbang"},
		{"C_SmokeGrenade", "Smoke Grenade"},
		{"C_DecoyGrenade", "Decoy Grenade"},
		{"C_MolotovGrenade", "Molotov"},
		{"C_IncendiaryGrenade", "Incendiary"}
	};

	auto it = weapon_names.find( schema );
	return ( it != weapon_names.end( ) ) ? it->second : "Unknown Weapon";
}

ImColor sdk_c::drops_c::get_color( const std::string& name )
{
	auto color = ImColor( 180, 180, 180, 255 );

	if ( name == "AK-47" || name == "M4A4" || name == "M4A1-S" )
	{
		color = ImColor( 255, 215, 0, 255 );
	}
	else if ( name == "AWP" )
	{
		color = ImColor( 255, 140, 0, 255 );
	}
	else if ( name == "AUG" || name == "FAMAS" || name == "Galil AR" || name == "SG 553" )
	{
		color = ImColor( 255, 255, 100, 255 );
	}
	else if ( name == "G3SG1" || name == "SCAR-20" || name == "SSG 08" )
	{
		color = ImColor( 255, 165, 0, 255 );
	}
	else if ( name == "MAC-10" || name == "MP5-SD" || name == "MP7" || name == "MP9" || name == "PP-Bizon" || name == "P90" || name == "UMP-45" )
	{
		color = ImColor( 100, 149, 237, 255 );
	}
	else if ( name == "Nova" || name == "Sawed-Off" || name == "XM1014" || name == "MAG-7" )
	{
		color = ImColor( 147, 112, 219, 255 );
	}
	else if ( name == "M249" || name == "Negev" )
	{
		color = ImColor( 220, 20, 60, 255 );
	}
	else if ( name == "Desert Eagle" || name == "R8 Revolver" )
	{
		color = ImColor( 255, 69, 0, 255 );
	}
	else if ( name == "Dual Berettas" || name == "Five-SeveN" || name == "Glock-18" || name == "P2000" || name == "USP-S" || name == "P250" || name == "CZ75-Auto" || name == "Tec-9" )
	{
		color = ImColor( 64, 224, 208, 255 );
	}
	else if ( name == "Zeus x27" )
	{
		color = ImColor( 0, 191, 255, 255 );
	}
	else if ( name == "C4" )
	{
		color = ImColor( 255, 0, 0, 255 );
	}
	else if ( name == "Knife" )
	{
		color = ImColor( 192, 192, 192, 255 );
	}
	else if ( name == "HE Grenade" )
	{
		color = ImColor( 255, 69, 0, 255 );
	}
	else if ( name == "Flashbang" )
	{
		color = ImColor( 255, 255, 0, 255 );
	}
	else if ( name == "Smoke Grenade" )
	{
		color = ImColor( 128, 128, 128, 255 );
	}
	else if ( name == "Decoy Grenade" )
	{
		color = ImColor( 173, 216, 230, 255 );
	}
	else if ( name == "Molotov" || name == "Incendiary" )
	{
		color = ImColor( 255, 140, 0, 255 );
	}

	return color;
}

bool sdk_c::flash_c::can_aim() const
{
	const auto local_pawn = g->udata.get_owning_player().cs_player_pawn;
	if (!local_pawn) return true;

	const auto flash_alpha = g->memory.read<float>(local_pawn + offsets::m_fl_flash_max_alpha);
	return flash_alpha < 100.0f;
}

bool sdk_c::smoke_c::is_in_smoke(const math::vector3& from, const math::vector3& to) const
{
	for (const auto& misc : g->updater.get_misc_entities())
	{
		if (misc.type != udata_c::misc_entity_t::type_t::smoke) continue;
		
		if (const auto* smoke = std::get_if<udata_c::misc_entity_t::smoke_data_t>(&misc.data))
		{
			if (smoke->detonation_pos.x == 0.0f && smoke->detonation_pos.y == 0.0f && smoke->detonation_pos.z == 0.0f) continue;
			
			const auto smoke_radius = 150.0f;
			const auto line_length = from.distance(to);
			const auto line_dir = (to - from) / line_length;
			const auto to_smoke = smoke->detonation_pos - from;
			const auto proj_length = to_smoke.dot(line_dir);
			
			if (proj_length < 0.0f || proj_length > line_length) continue;
			
			const auto closest_point = from + line_dir * proj_length;
			const auto dist_to_smoke = closest_point.distance(smoke->detonation_pos);
			
			if (dist_to_smoke <= smoke_radius) return true;
		}
	}
	return false;
}