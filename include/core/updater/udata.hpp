#ifndef UDATA_HPP
#define UDATA_HPP

class udata_c
{
public:
	struct roots_t
	{
		std::uintptr_t dw_entity_list;
		std::uintptr_t dw_local_player_controller;
	};

	struct owning_player_entity_t
	{
		std::uintptr_t cs_player_pawn;
		std::uintptr_t m_p_bullet_services;
		std::uintptr_t m_p_weapon_services;
		std::uintptr_t m_p_game_scene_node;

		int team{};

		sdk_c::weapon_c::data_t weapon_data;
	};

	struct player_entity_t
	{
		std::uintptr_t cs_player_controller;
		std::uintptr_t cs_player_pawn;
		std::uintptr_t m_p_game_scene_node;
		std::uintptr_t bone_array;

		int team;
		int health;

		sdk_c::hitboxes_c::data_t hitbox_data;
		sdk_c::name_c::data_t name_data;
		sdk_c::weapon_c::data_t weapon_data;
	};

	struct misc_entity_t
	{
		std::uintptr_t entity;

		enum class type_t
		{
			invalid,
			smoke,
			molotov,
			bomb,
			drop,
			post_processing_volume,
			env_sky
		} type = type_t::invalid;

		struct smoke_data_t
		{
			math::vector3 detonation_pos;
		};

		struct molotov_data_t
		{
			std::vector<math::vector3> fire_points;
			math::vector3 center;
		};

		struct bomb_data_t
		{
			math::vector3 position;
		};

		struct drop_data_t
		{
			math::vector3 position;
			std::string name;
			math::vector3 mins;
			math::vector3 maxs;
			math::transform node_transform;
		};

		std::variant<std::monostate, smoke_data_t, molotov_data_t, std::monostate, bomb_data_t, drop_data_t> data;
	};

public:
	roots_t& get_roots( ) { return this->roots; }
	owning_player_entity_t& get_owning_player( ) { return this->owning_player; }

private:
	roots_t roots;
	owning_player_entity_t owning_player;
};

#endif // !UDATA_HPP
