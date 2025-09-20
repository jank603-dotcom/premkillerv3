#ifndef RAYCASTING_HPP
#define RAYCASTING_HPP

class raycasting_c
{
public:
	bool load( const std::string& map_name );
	void unload( );

	bool is_visible( const math::vector3& start, const math::vector3& end ) const;

	void update_map( );
	const std::string& get_current_map_name( ) const;
private:
	struct triangle_t
	{
		math::vector3 p1, p2, p3;
	};

	struct aabb_t
	{
		math::vector3 min;
		math::vector3 max;

		bool intersects( const math::vector3& ray_start, const math::vector3& ray_end ) const;
	};

	struct node_t
	{
		aabb_t bounds;
		std::vector<triangle_t> triangles;
		std::unique_ptr<node_t> left;
		std::unique_ptr<node_t> right;
		int axis = 0;
	};

	std::string current_map_name;

	std::unique_ptr<node_t> build_kdtree( std::vector<triangle_t>& triangles, int depth = 0 );
	bool ray_hits_node( const node_t* node, const math::vector3& start, const math::vector3& end ) const;
	bool ray_hits_triangle( const triangle_t& tri, const math::vector3& start, const math::vector3& end ) const;

	std::unique_ptr<node_t> root;
};

#endif // !RAYCASTING_HPP
