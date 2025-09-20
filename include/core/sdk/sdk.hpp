#ifndef SDK_HPP
#define SDK_HPP

class sdk_c
{
public:
	sdk_c( )
	{
		this->bones.parent = this;
		this->bounds.parent = this;
		this->hitboxes.parent = this;
		this->w2s.parent = this;
		this->camera.parent = this;
		this->name.parent = this;
		this->weapon.parent = this;
		this->flash.parent = this;
		this->smoke.parent = this;
	}

	class bones_c
	{
	public:
		enum class bone_id : int
		{
			head = 6,
			neck = 5,
			spine_0 = 1,
			spine_1 = 2,
			spine_2 = 3,
			spine_3 = 4,
			pelvis = 0,

			left_shoulder = 8,
			left_upper_arm = 9,
			left_hand = 10,

			right_shoulder = 13,
			right_upper_arm = 14,
			right_hand = 15,

			left_hip = 22,
			left_knee = 23,
			left_foot = 24,

			right_hip = 25,
			right_knee = 26,
			right_foot = 27,
		};

	public:
		struct data_t
		{
			struct bone_t
			{
				math::vector3 position{};
				math::quaternion rotation{};
			};

			std::array<bone_t, 28> bones;

			bool is_valid( ) const;
			math::vector3 get_position( bone_id id ) const;
			math::quaternion get_rotation( bone_id id ) const;
		};

		data_t get_data( std::uintptr_t bone_array ) const;

		bool is_any_major_bone_visible( const data_t& bone_data );

	public:
		sdk_c* parent = nullptr;
	};

	class bounds_c
	{
	public:
		struct data_t
		{
			float min_x = 0.0f;
			float max_x = 0.0f;
			float min_y = 0.0f;
			float max_y = 0.0f;
		};

		sdk_c::bounds_c::data_t get_data( const bones_c::data_t& bone_data ) const;

	public:
		sdk_c* parent = nullptr;
	};

	class hitboxes_c
	{
	public:
		struct hitbox_t
		{
			int index = -1;
			int bone_index = -1;

			math::vector3 bb_min{};
			math::vector3 bb_max{};
			float radius = 0.0f;
		};

		using data_t = std::vector<hitbox_t>;

		data_t get_data( std::uintptr_t m_p_game_scene_node ) const;

		void clear_cache( );

	private:
		static constexpr int hitbox_to_bone_map[ ] = { 6, -1, 0, 1, 2, 3, 4, 22, 25, 23, 26, 24, 27, 10, 15, 9, 8, 14, 13 };

		static int get_bone_id_from_hitbox( int hitbox_id )
		{
			if ( hitbox_id < 0 || hitbox_id >= static_cast< int >( std::size( hitbox_to_bone_map ) ) )
			{
				return -1;
			}

			return hitbox_to_bone_map[ hitbox_id ];
		}

		mutable ankerl::unordered_dense::map<std::uintptr_t, data_t> hitbox_cache;

	public:
		sdk_c* parent = nullptr;
	};

	class w2s_c
	{
	public:
		void update( );

		math::vector2 project( const math::vector3& position );

		bool is_valid( const math::vector2& projected );
		bool is_on_screen( const math::vector2& projected ) const;

		const math::matrix4x4& get_view_matrix( ) const { return this->view_matrix; }

	private:
		math::matrix4x4 view_matrix;

	public:
		sdk_c* parent = nullptr;
	};

	class camera_c
	{
	public:
		struct data_t
		{
			math::vector3 position;
			math::quaternion rotation;
			float fov;
		};

		void update( );

		data_t get_data( ) const { return this->data; }
	private:
		data_t data;

	public:
		sdk_c* parent = nullptr;
	};

	class name_c
	{
	public:
		struct data_t
		{
			std::string name;
		};

		data_t get_data( std::uintptr_t cs_player_controller ) const;

		void clear_cache( );

	private:
		mutable ankerl::unordered_dense::map<std::uintptr_t, data_t> name_cache;

	public:
		sdk_c* parent = nullptr;
	};

	class weapon_c
	{
	public:
		struct data_t
		{
			std::string name;
			std::string type;
		};

		data_t get_data_from_entity( std::uintptr_t entity ) const;
		data_t get_data( std::uintptr_t cs_player_pawn ) const;

		void clear_cache( );

	private:
		mutable ankerl::unordered_dense::map<std::uintptr_t, data_t> weapon_cache;

	public:
		sdk_c* parent = nullptr;
	};

	class drops_c
	{
	public:
		void add_to_allowed( std::vector<const char*>& allowed_types );
		bool is( const std::string& entity_type );
		std::string get_display_name( const std::string& schema );
		ImColor get_color( const std::string& name );
	};

	class flash_c
	{
	public:
		bool can_aim() const;
	public:
		sdk_c* parent = nullptr;
	};

	class smoke_c
	{
	public:
		bool is_in_smoke(const math::vector3& from, const math::vector3& to) const;
	public:
		sdk_c* parent = nullptr;
	};

	bones_c bones;
	bounds_c bounds;
	hitboxes_c hitboxes;
	w2s_c w2s;
	camera_c camera;
	name_c name;
	weapon_c weapon;
	drops_c drops;
	flash_c flash;
	smoke_c smoke;
};

#endif // !SDK_HPP
