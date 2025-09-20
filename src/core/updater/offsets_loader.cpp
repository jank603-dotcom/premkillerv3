#include <include/global.hpp>
#include "offsets_loader.hpp"
#include <fstream>
#include <string>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <vector>
#include <windows.h>

// Simple helper: trim spaces
static inline std::string trim(const std::string& s) {
    size_t a = 0, b = s.size();
    while (a < b && (s[a] == ' ' || s[a] == '\t')) ++a;
    while (b > a && (s[b-1] == ' ' || s[b-1] == '\t' || s[b-1] == '\r' || s[b-1] == '\n')) --b;
    return s.substr(a, b - a);
}

void load_offsets_from_client_dll() {
    static bool s_done = false;
    if (s_done) return;

    // Build candidate paths (relative and executable-relative)
    std::vector<std::filesystem::path> paths;
    paths.emplace_back("premkiller-user/include/core/client_dll.cs");
    paths.emplace_back("include/core/client_dll.cs");
    paths.emplace_back("../include/core/client_dll.cs");
    // Executable-relative
    wchar_t exe_w[MAX_PATH]{};
    if ( GetModuleFileNameW(nullptr, exe_w, MAX_PATH) ) {
        std::filesystem::path exePath(exe_w);
        auto exeDir = exePath.parent_path();
        paths.push_back(exeDir / L"premkiller-user/include/core/client_dll.cs");
        paths.push_back(exeDir / L"include/core/client_dll.cs");
        if (exeDir.has_parent_path())
            paths.push_back(exeDir.parent_path() / L"include/core/client_dll.cs");
    }

    // Log current working directory and probe each candidate
    try {
        wchar_t cwd_w[MAX_PATH]{}; GetCurrentDirectoryW(MAX_PATH, cwd_w);
        std::filesystem::path cwdp(cwd_w);
        g->core.log_error(std::string("[info] CWD: ") + cwdp.string());
    } catch (...) {}

    std::ifstream file;
    for (const auto& p : paths) {
        try {
            const bool ex = std::filesystem::exists(p);
            g->core.log_error(std::string("[trace] probe: ") + p.string() + (ex ? " (exists)" : " (missing)"));
        } catch (...) {}
        file.open(p);
        if (file.is_open()) {
            try { g->core.log_error(std::string("[info] client_dll.cs loaded from: ") + p.string()); } catch (...) {}
            break;
        }
    }
    if (!file.is_open()) {
        try {
            g->core.log_error("client_dll.cs not found. Searched these paths:");
            for (const auto& p : paths) g->core.log_error(std::string(" - ") + p.string());
            g->core.log_error("Offsets will be zero or defaults; features depending on offsets will not work.");
        } catch (...) {}
        s_done = true; return;
    }

    std::string line;
    std::string current_class;
    int brace_depth = 0;

    auto parse_hex = [](const std::string& s)->std::uintptr_t {
        // find 0x...
        auto pos = s.find("0x");
        if (pos == std::string::npos) return 0;
        pos += 2;
        std::uintptr_t val = 0;
        try { val = static_cast<std::uintptr_t>(std::stoull(s.substr(pos), nullptr, 16)); } catch (...) { val = 0; }
        return val;
    };

    std::size_t parsed_count = 0;
    while (std::getline(file, line)) {
        const std::string t = trim(line);
        if (t.rfind("public static class ", 0) == 0) {
            // e.g. public static class C_BasePlayerPawn {
            size_t start = std::string("public static class ").size();
            size_t end = t.find(' ', start);
            if (end == std::string::npos) end = t.find('{', start);
            current_class = (end != std::string::npos) ? t.substr(start, end - start) : std::string();
            brace_depth = 0;
            continue;
        }
        if (!current_class.empty()) {
            // track braces
            for (char c : t) {
                if (c == '{') ++brace_depth;
                else if (c == '}') { if (brace_depth > 0) --brace_depth; }
            }
            if (brace_depth == 0 && t.find('}') != std::string::npos) {
                current_class.clear();
                continue;
            }
        }

        // Parse const lines
        if (t.rfind("public const nint ", 0) == 0) {
            // Extract name
            size_t name_start = std::string("public const nint ").size();
            size_t name_end = t.find(' ', name_start);
            if (name_end == std::string::npos) continue;
            std::string name = t.substr(name_start, name_end - name_start);
            const std::uintptr_t val = parse_hex(t);
            if (!val) continue;

            // Store in generic map for universal access
            try {
                const std::string fq = current_class.empty() ? name : (current_class + "." + name);
                offsets::all[fq] = val;
                offsets::all[name] = val; // simple alias (last one wins if duplicates)
            } catch (...) { /* ignore map errors */ }

            ++parsed_count;

            // Map based on class context to avoid collisions
            if (current_class.find("CameraServices") != std::string::npos) {
                if (name == "m_iFOV") offsets::cs_m_iFOV = val;
                else if (name == "m_iFOVStart") offsets::cs_m_iFOVStart = val;
                else if (name == "m_flFOVTime") offsets::cs_m_flFOVTime = val;
                else if (name == "m_flFOVRate") offsets::cs_m_flFOVRate = val;
                else if (name == "m_hZoomOwner") offsets::cs_m_hZoomOwner = val;
                else if (name == "m_flLastShotFOV") offsets::cs_m_flLastShotFOV = val;
            }
            if (current_class.find("BasePlayerPawn") != std::string::npos || current_class.find("CSPlayerPawn") != std::string::npos) {
                if (name == "m_pCameraServices") offsets::m_p_camera_services = val;
                else if (name == "m_bIsScoped") offsets::m_b_is_scoped = val;
                else if (name == "m_iFOV") offsets::m_iFOV = val;
                else if (name == "m_iDefaultFOV") offsets::m_iDefaultFOV = val;
                else if (name == "m_entitySpottedState") offsets::m_entity_spotted_state = val;
                // Angles on pawn: map both canonical and alias for silent aim path
                else if (name == "m_angEyeAngles") { offsets::m_ang_eye_angles = val; offsets::v_angle = val; }
                else if (name == "v_angle") { offsets::v_angle = val; }
                // Viewmodel offsets on pawn (populate from client_dll.cs)
                else if (name == "m_flViewmodelOffsetX") offsets::m_flViewmodelOffsetX = val;
                else if (name == "m_flViewmodelOffsetY") offsets::m_flViewmodelOffsetY = val;
                else if (name == "m_flViewmodelOffsetZ") offsets::m_flViewmodelOffsetZ = val;
                else if (name == "m_flViewmodelFOV")     offsets::m_flViewmodelFOV     = val;
            }

            // EntitySpottedState_t
            if (current_class.find("EntitySpottedState_t") != std::string::npos) {
                if (name == "m_bSpotted") offsets::m_b_spotted = val;
                else if (name == "m_bSpottedByMask") offsets::m_b_spotted_by_mask = val;
            }

            // Globals and other frequently used fields: accept regardless of class when safe
            if (name == "dwEntityList") offsets::dw_entity_list = val;
            else if (name == "dwLocalPlayerPawn") offsets::dw_local_player_pawn = val;
            else if (name == "dwLocalPlayerController") offsets::dw_local_player_controller = val;
            else if (name == "dwViewMatrix") offsets::dw_view_matrix = val;
            else if (name == "dwViewAngles") offsets::dw_view_angles = val;
            else if (name == "dwCSGOInput") offsets::dw_csgo_input = val;
            else if (name == "dwGlobalVars") offsets::dw_global_vars = val;
            else if (name == "dwNetworkGameClient") offsets::dw_network_game_client = val;
            // Triggerbot and movement related globals
            else if (name == "dwForceAttack" || name == "dwForceAttackAlt") offsets::dw_force_attack = val;
            else if (name == "dwForceJump") offsets::dw_force_jump = val;
            else if (name == "m_hObserverTarget") offsets::m_h_observer_target = val;
            else if (name == "m_iObserverMode") offsets::m_i_observer_mode = val;
            else if (name == "m_hObserverPawn") offsets::m_h_observer_pawn = val;
            else if (name == "m_pObserverServices") offsets::m_p_observer_services = val;
            else if (name == "m_hObserverTarget") offsets::m_h_observer_target_services = val; // when seen under observer services
            // Camera writer addresses for true silent aim (present as raw addresses in client_dll.cs)
            else if (name == "cam_write_pitch") offsets::cam_write_pitch = val;
            else if (name == "cam_write_yaw")   offsets::cam_write_yaw   = val;
            else if (name == "cam_write_pitch_len") offsets::cam_write_pitch_len = static_cast<std::size_t>(val);
            else if (name == "cam_write_yaw_len")   offsets::cam_write_yaw_len   = static_cast<std::size_t>(val);

            // Common pawn/controller fields for triggerbot validation
            if (name == "m_iHealth") offsets::m_i_health = val;
            else if (name == "m_iTeamNum") offsets::m_i_team_num = val;
            else if (name == "m_iShotsFired") offsets::m_i_shots_fired = val;
            else if (name == "m_fFlags" || name == "m_f_flags") offsets::m_f_flags = val;
            else if (name == "m_iIDEntIndex" || name == "m_iIdEntIndex" || name == "m_iIDEnt") offsets::m_i_id_ent_index = val;
        }
    }

    try { g->core.log_error(std::string("[info] Offsets parsed: ") + std::to_string(parsed_count)); } catch (...) {}

    // Validate camera writer offsets for true silent aim
    try {
        if (offsets::cam_write_pitch == 0 || offsets::cam_write_yaw == 0) {
            g->core.log_error("[warn] cam_write_pitch/cam_write_yaw missing; true silent aim camera patch will be disabled.");
        } else {
            auto to_hex = [](std::uintptr_t v) {
                char buf[32];
                snprintf(buf, sizeof(buf), "%llX", static_cast<unsigned long long>(v));
                return std::string(buf);
            };
            g->core.log_error(std::string("[info] cam_write_pitch=0x") + to_hex(offsets::cam_write_pitch) +
                              ", len=" + std::to_string(offsets::cam_write_pitch_len));
            g->core.log_error(std::string("[info] cam_write_yaw=0x") + to_hex(offsets::cam_write_yaw) +
                              ", len=" + std::to_string(offsets::cam_write_yaw_len));
            // Simple sanity on lengths
            if (offsets::cam_write_pitch_len == 0 || offsets::cam_write_pitch_len > 16 ||
                offsets::cam_write_yaw_len == 0   || offsets::cam_write_yaw_len   > 16) {
                g->core.log_error("[warn] Camera write NOP lengths look unusual; verify client_dll.cs values.");
            }
        }
    } catch (...) { /* ignore log errors */ }

    s_done = true;
}
