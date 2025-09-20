#include <include/global.hpp>

bool raycasting_c::load( const std::string& map_name )
{
	char exe_path[ MAX_PATH ]{};
	call_function( &GetModuleFileNameA, nullptr, exe_path, MAX_PATH );

	std::string path = exe_path;
	const size_t last_slash = path.find_last_of( "\\/" );
	if ( last_slash != std::string::npos )
	{
		path = path.substr( 0, last_slash );
	}

	path += "\\maps\\" + map_name;

	char full_path[ MAX_PATH ];
	call_function( &GetFullPathNameA, path.c_str( ), MAX_PATH, full_path, nullptr );

	std::ifstream file( full_path, std::ios::binary );
	if ( !file.is_open( ) )
	{
		g->util.console.print( ecrypt( "(raycasting) failed to open file: %s" ), map_name );
		return false;
	}

	file.seekg( 0, std::ios::end );
	const size_t size = file.tellg( );
	file.seekg( 0, std::ios::beg );

	if ( size % sizeof( triangle_t ) != 0 )
	{
		g->util.console.print( ecrypt( "(raycasting) invalid file size (%llu bytes): %s" ), size, map_name );
		file.close( );
		return false;
	}

	std::vector<triangle_t> tris( size / sizeof( triangle_t ) );
	if ( !file.read( reinterpret_cast< char* >( tris.data( ) ), size ) )
	{
		g->util.console.print( ecrypt( "(raycasting) failed to read triangle data: %s" ), map_name );
		file.close( );
		return false;
	}

	file.close( );

	this->root = this->build_kdtree( tris );

	return true;
}

void raycasting_c::unload( )
{
	this->root.reset( );
}

bool raycasting_c::is_visible( const math::vector3& start, const math::vector3& end ) const
{
	if ( !this->root )
	{
		g->util.console.print( ecrypt( "[raycasting] WARNING: KD-tree not loaded, blocking shot (map not loaded)" ) );
		return false;
	}
	return !this->ray_hits_node( this->root.get( ), start, end );
}

void raycasting_c::update_map( )
{
	const auto client_base = g->core.get_process_info( ).client_base;
	if ( !client_base )
	{
		this->unload( );
		this->current_map_name.clear( );
		return;
	}

	const auto global_vars = g->memory.read<std::uintptr_t>( client_base + offsets::dw_global_vars );
	if ( !global_vars || global_vars < 0x1000 )
	{
		this->unload( );
		this->current_map_name.clear( );
		return;
	}

	const auto map_name_ptr = g->memory.read<std::uintptr_t>( global_vars + 0x180 );
	if ( !map_name_ptr || map_name_ptr < 0x1000 )
	{
		this->unload( );
		this->current_map_name.clear( );
		return;
	}

	char buf[ 64 ]{};
	if ( !g->memory.read_process_memory( map_name_ptr, buf, sizeof( buf ) - 1 ) )
	{
		this->unload( );
		this->current_map_name.clear( );
		return;
	}

	buf[ sizeof( buf ) - 1 ] = '\0';
	std::string new_map_name( buf );

	if ( new_map_name.empty( ) )
	{
		this->unload( );
		this->current_map_name.clear( );
		return;
	}

	new_map_name.erase( 0, new_map_name.find_first_not_of( " \t\n\r" ) );
	new_map_name.erase( new_map_name.find_last_not_of( " \t\n\r" ) + 1 );

	const auto weird = std::any_of( new_map_name.begin( ), new_map_name.end( ), [ ]( unsigned char c ) { return !std::isalnum( c ) && c != '_' && c != '-' && c != '.' && c != ' '; } );
	if ( weird )
	{
		this->unload( );
		this->current_map_name.clear( );
		return;
	}

	static const std::vector<std::string> ishit = { "<empty>", "SNDLVL_35dB", "SNDLVL_0dB", "default", "NULL" };

	if ( std::any_of( ishit.begin( ), ishit.end( ), [ &new_map_name ]( const std::string& s ) { return new_map_name == s; } ) )
	{
		this->unload( );
		this->current_map_name.clear( );
		return;
	}

	if ( new_map_name == this->current_map_name )
	{
		return;
	}

	this->unload( );

	if ( this->load( new_map_name + ".tri" ) )
	{
		this->current_map_name = new_map_name;
	}
}

const std::string& raycasting_c::get_current_map_name( ) const
{
	return this->current_map_name;
}

bool raycasting_c::aabb_t::intersects( const math::vector3& ray_start, const math::vector3& ray_end ) const
{
	math::vector3 dir = ray_end - ray_start;

	float tmin = ( this->min.x - ray_start.x ) / dir.x;
	float tmax = ( this->max.x - ray_start.x ) / dir.x;
	if ( tmin > tmax ) std::swap( tmin, tmax );

	float tymin = ( this->min.y - ray_start.y ) / dir.y;
	float tymax = ( this->max.y - ray_start.y ) / dir.y;
	if ( tymin > tymax ) std::swap( tymin, tymax );

	if ( ( tmin > tymax ) || ( tymin > tmax ) )
	{
		return false;
	}

	if ( tymin > tmin ) tmin = tymin;
	if ( tymax < tmax ) tmax = tymax;

	float tzmin = ( this->min.z - ray_start.z ) / dir.z;
	float tzmax = ( this->max.z - ray_start.z ) / dir.z;
	if ( tzmin > tzmax ) std::swap( tzmin, tzmax );

	if ( ( tmin > tzmax ) || ( tzmin > tmax ) )
	{
		return false;
	}

	return true;
}

bool raycasting_c::ray_hits_triangle( const triangle_t& tri, const math::vector3& start, const math::vector3& end ) const
{
	const float epsilon = 1e-5f;

	math::vector3 dir = end - start;
	math::vector3 edge1 = tri.p2 - tri.p1;
	math::vector3 edge2 = tri.p3 - tri.p1;
	math::vector3 h = math::vector3{ dir.y * edge2.z - dir.z * edge2.y,dir.z * edge2.x - dir.x * edge2.z,dir.x * edge2.y - dir.y * edge2.x };

	float a = edge1.dot( h );
	if ( a > -epsilon && a < epsilon )
	{
		return false;
	}

	float f = 1.0f / a;
	math::vector3 s = start - tri.p1;
	float u = f * s.dot( h );
	if ( u < 0.0f || u > 1.0f )
	{
		return false;
	}

	math::vector3 q = math::vector3{ s.y * edge1.z - s.z * edge1.y,s.z * edge1.x - s.x * edge1.z,s.x * edge1.y - s.y * edge1.x };

	float v = f * dir.dot( q );
	if ( v < 0.0f || u + v > 1.0f )
	{
		return false;
	}

	float t = f * edge2.dot( q );
	// Accept t in [epsilon, 1-epsilon] to avoid floating-point errors at the ends
	return t > epsilon && t < (1.0f - epsilon);
}

bool raycasting_c::ray_hits_node( const node_t* node, const math::vector3& start, const math::vector3& end ) const
{
	if ( !node || !node->bounds.intersects( start, end ) )
	{
		return false;
	}

	for ( const auto& tri : node->triangles )
	{
		if ( this->ray_hits_triangle( tri, start, end ) )
		{
			return true;
		}
	}

	return this->ray_hits_node( node->left.get( ), start, end ) || this->ray_hits_node( node->right.get( ), start, end );
}

std::unique_ptr<raycasting_c::node_t> raycasting_c::build_kdtree( std::vector<triangle_t>& triangles, int depth )
{
	if ( triangles.empty( ) )
	{
		return nullptr;
	}

	aabb_t bounds = { triangles[ 0 ].p1, triangles[ 0 ].p1 };
	for ( const auto& tri : triangles )
	{
		for ( const auto& p : { tri.p1, tri.p2, tri.p3 } )
		{
			bounds.min.x = std::min( bounds.min.x, p.x );
			bounds.min.y = std::min( bounds.min.y, p.y );
			bounds.min.z = std::min( bounds.min.z, p.z );
			bounds.max.x = std::max( bounds.max.x, p.x );
			bounds.max.y = std::max( bounds.max.y, p.y );
			bounds.max.z = std::max( bounds.max.z, p.z );
		}
	}

	auto node = std::make_unique<node_t>( );
	node->bounds = bounds;
	node->axis = depth % 3;

	if ( triangles.size( ) <= 4 )
	{
		node->triangles = std::move( triangles );
		return node;
	}

	auto axis = node->axis;
	auto mid = triangles.size( ) / 2;
	auto comparator = [ axis ]( const triangle_t& a, const triangle_t& b ) {
		auto get_axis_value = [ axis ]( const math::vector3& v ) -> float {
			switch ( axis )
			{
			case 0: return v.x;
			case 1: return v.y;
			default: return v.z;
			}
			};

		float a_mid = ( get_axis_value( a.p1 ) + get_axis_value( a.p2 ) + get_axis_value( a.p3 ) ) / 3.0f;
		float b_mid = ( get_axis_value( b.p1 ) + get_axis_value( b.p2 ) + get_axis_value( b.p3 ) ) / 3.0f;
		return a_mid < b_mid;
		};

	std::nth_element( triangles.begin( ), triangles.begin( ) + mid, triangles.end( ), comparator );
	std::vector<triangle_t> left( triangles.begin( ), triangles.begin( ) + mid );
	std::vector<triangle_t> right( triangles.begin( ) + mid, triangles.end( ) );

	node->left = this->build_kdtree( left, depth + 1 );
	node->right = this->build_kdtree( right, depth + 1 );
	return node;
}