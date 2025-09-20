#ifndef GLOBAL_HPP
#define GLOBAL_HPP

#define IMGUI_DISABLE_DEBUG_TOOLS
#define IMGUI_USE_BGRA_PACKED_COLOR
#define IMGUI_DEFINE_MATH_OPERATORS
#define NOMINMAX

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "shell32.lib")

// windows
#include <windows.h>
#include <shellapi.h>
#include <intrin.h>
#include <winternl.h>
#include <tlhelp32.h>
#include <dwmapi.h>

// standard lib
#include <array>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <numbers>
#include <vector>
#include <filesystem>
#include <span>
#include <variant>

// directx
#include <d3d11.h>
#include <dxgi1_3.h>

#include <include/util/external/obfusheader.hpp>
#include <include/util/external/ecrypter.hpp>
#include <include/util/external/unordered_dense.hpp>
#include <include/util/external/json.hpp>

#include <include/util/external/imgui/imgui.h>
#include <include/util/external/imgui/imgui_internal.h>
#include <include/util/external/imgui/backend/imgui_impl_dx11.h>
#include <include/util/external/imgui/backend/imgui_impl_win32.h>

#include <include/util/shadow/shadow.hpp>
#include <include/util/memory/memory.hpp>

#include <include/util/dbs/offset_db/offset_db.hpp>
#include <include/util/dbs/skin_db/skin_db.hpp>
#include <include/util/dbs/offsets.hpp>
#include <include/util/dbs/skins.hpp>

#include <include/util/util.hpp>

#include <include/core/sdk/dependent/math.hpp>
#include <include/core/sdk/dependent/raycasting/raycasting.hpp>
#include <include/core/sdk/sdk.hpp>

#include <include/core/updater/udata.hpp>
#include <include/core/updater/updater.hpp>

#include <include/core/overlay/overlay.hpp>
#include <include/core/features/features.hpp>
#include <include/core/dispatch/dispatch.hpp>
#include <include/core/menu/menu.hpp>

#include <include/core/core.hpp>

class global_c
{
public:
	util_c util;
	memory_c memory;

	overlay_c overlay;
	features_c features;
	dispatch_c dispatch;
	menu_c menu;
	offset_db_c offset_db;
	skin_db_c skin_db;
	raycasting_c raycasting;
	sdk_c sdk;
	udata_c udata;
	updater_c updater;

	core_c core;
};

inline auto g = std::make_unique<global_c>( );

#endif // !GLOBAL_HPP
