#ifndef OFFSETS_HPP
#define OFFSETS_HPP

#include <cstdint>
#include <string>
#include <unordered_map>

namespace offsets {

    // Generic store for ALL parsed offsets from client_dll.cs
    // Keys:
    //  - Fully qualified: "Class.Member" (e.g., "C_CSPlayerPawn.m_flViewmodelFOV")
    //  - Simple alias: "Member" (last one wins if multiple classes define same name)
    inline std::unordered_map<std::string, std::uintptr_t> all;

	inline std::uintptr_t dw_csgo_input;
	inline std::uintptr_t dw_entity_list;
	inline std::uintptr_t dw_global_vars;
	inline std::uintptr_t dw_local_player_controller;
	inline std::uintptr_t dw_local_player_pawn;
	inline std::uintptr_t dw_view_matrix;
	inline std::uintptr_t dw_view_angles;

	inline std::uintptr_t dw_network_game_client;
	inline std::uintptr_t dw_network_game_client_delta_tick;

	inline std::uintptr_t m_i_health;
	inline std::uintptr_t m_i_team_num;
	inline std::uintptr_t m_i_class_id; // Entity class ID for validation
	inline std::uintptr_t m_b_dormant; // Entity dormant state
	inline std::uintptr_t m_i_shots_fired;
	inline std::uintptr_t m_h_player_pawn;
	inline std::uintptr_t m_p_entity;
	inline std::uintptr_t m_s_sanitized_player_name;
	inline std::uintptr_t m_p_collision;

	inline std::uintptr_t m_vec_origin;
	inline std::uintptr_t m_vec_view_offset;
	inline std::uintptr_t m_ang_eye_angles;
	inline std::uintptr_t m_ang_rotation;
	inline std::uintptr_t v_angle;

	inline std::uintptr_t m_vec_mins;
	inline std::uintptr_t m_vec_maxs;

	inline std::uintptr_t m_p_game_scene_node;
	inline std::uintptr_t m_model_state;

	inline std::uintptr_t m_i_rarity_override;

	inline std::uintptr_t m_p_clipping_weapon;
	inline std::uintptr_t m_p_bullet_services;
	inline std::uintptr_t m_p_weapon_services;
	inline std::uintptr_t m_total_hits_on_server;
	inline std::uintptr_t m_aim_punch_cache;

	inline std::uintptr_t m_v_smoke_color;
	inline std::uintptr_t m_v_smoke_detonation_pos;
	inline std::uintptr_t m_b_smoke_effect_spawned;
	inline std::uintptr_t m_n_smoke_effect_tick_begin;

	inline std::uintptr_t m_fire_count;
	inline std::uintptr_t m_b_fire_is_burning;
	inline std::uintptr_t m_fire_positions;

	inline std::uintptr_t m_b_exposure_control;
	inline std::uintptr_t m_fl_min_exposure;
	inline std::uintptr_t m_fl_max_exposure;

	inline std::uintptr_t m_fl_flash_max_alpha;
	inline std::uintptr_t m_b_is_scoped = 0x2778; // C_BasePlayerPawn::m_bIsScoped (bool)
	inline std::uintptr_t m_b_is_defusing;
	inline std::uintptr_t m_fl_ping;                    // Player ping value
	inline std::uintptr_t m_fl_interpolation_amount;    // Interpolation timing
	inline std::uintptr_t m_n_last_server_tick;         // Last server update tick
	inline std::uintptr_t dw_force_attack;
	inline std::uintptr_t m_i_id_ent_index;
	inline std::uintptr_t dw_force_jump;
	inline std::uintptr_t m_f_flags;

    // Spotted state (for in-game radar reveal)
    inline std::uintptr_t m_entity_spotted_state; // on pawn
    // Relative to m_entity_spotted_state (EntitySpottedState_t)
    inline std::uintptr_t m_b_spotted = 0x8;         // bool
    inline std::uintptr_t m_b_spotted_by_mask = 0xC; // uint32[2]

    // Spectator / observer offsets (controller fields)
    inline std::uintptr_t m_h_observer_target; // CBasePlayerController::m_hObserverTarget (handle to pawn)
    inline std::uintptr_t m_i_observer_mode;   // CBasePlayerController::m_iObserverMode (optional)
    inline std::uintptr_t m_h_observer_pawn;   // CCSPlayerController::m_hObserverPawn (alternative)

    // Spectator / observer offsets via services on pawn
    inline std::uintptr_t m_p_observer_services;  // C_BasePlayerPawn::m_pObserverServices
    inline std::uintptr_t m_h_observer_target_services; // CPlayer_ObserverServices::m_hObserverTarget

    // Optional FOV fields on pawn (used by FOV Changer if available)
    inline std::uintptr_t m_iFOV;          // current FOV
    inline std::uintptr_t m_iDefaultFOV;   // default FOV to restore

    // Camera services base pointer on pawn (C_BasePlayerPawn::m_pCameraServices)
    inline std::uintptr_t m_p_camera_services = 0x1438; // pointer to CCSPlayerBase_CameraServices/CPlayer_CameraServices (from client_dll.cs)
    // CCSPlayerBase_CameraServices fields (relative to m_p_camera_services)
    inline std::uintptr_t cs_m_iFOV        = 0x288; // uint32
    inline std::uintptr_t cs_m_iFOVStart   = 0x28C; // uint32
    inline std::uintptr_t cs_m_flFOVTime   = 0x290; // GameTime_t/float
    inline std::uintptr_t cs_m_flFOVRate   = 0x294; // float32
    inline std::uintptr_t cs_m_hZoomOwner  = 0x298; // CHandle<C_BaseEntity>
    inline std::uintptr_t cs_m_flLastShotFOV = 0x29C; // float32

    // Viewmodel Changer (fields on pawn) - placeholder defaults; update with live offsets when available
    inline std::uintptr_t m_flViewmodelOffsetX = 0x249C; // float
    inline std::uintptr_t m_flViewmodelOffsetY = 0x24A0; // float
    inline std::uintptr_t m_flViewmodelOffsetZ = 0x24A4; // float
    inline std::uintptr_t m_flViewmodelFOV     = 0x24A8; // float

    // Camera writer instruction addresses (for true silent aim)
    inline std::uintptr_t cam_write_pitch = 0; // address of instruction writing camera pitch
    inline std::uintptr_t cam_write_yaw   = 0; // address of instruction writing camera yaw
    inline std::size_t    cam_write_pitch_len = 6; // length to NOP (default 6)
    inline std::size_t    cam_write_yaw_len   = 6; // length to NOP (default 6)

} // namespace offsets

#endif // !OFFSETS_HPP
