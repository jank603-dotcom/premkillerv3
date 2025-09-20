#include <include/global.hpp>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dwmapi.h>
#include <gdiplus.h>
#include <windowsx.h> // for GET_X_LPARAM / GET_Y_LPARAM
#include <shellapi.h>
#pragma comment(lib, "Dwmapi.lib")
#pragma comment(lib, "Gdiplus.lib")
#pragma comment(lib, "Shell32.lib")

void launch_premkiller()
{
        // Allocate console for premkiller if none exists
        if ( GetConsoleWindow() == nullptr )
        {
            AllocConsole();
        }
        // Ensure console title branding
        SetConsoleTitleW(L"premkillerv3");
        
        // Redirect stdout to console
        freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
        freopen_s((FILE**)stderr, "CONOUT$", "w", stderr);
        
        // Keep title consistent
        SetConsoleTitleW(L"premkillerv3");
        printf("=== premkillerv3 ===\n");
        printf("Console initialized successfully!\n");
        fflush(stdout);
        
        // Original premkiller code
        if (!g->util.console.initialize()) {
            printf("Failed to initialize util console!\n");
            system("pause");
            return;
        }
        printf("Util console initialized!\n");
        fflush(stdout);

        printf("Loading offset database...\n");
        if (!g->offset_db.load_all()) {
            printf("Failed to load offsets database!\n");
            system("pause");
            return;
        }
        printf("Offset database loaded!\n");
        fflush(stdout);

        // Load offsets
        offsets::dw_csgo_input = g->offset_db.get_flat(ecrypt("dwCSGOInput"));
        offsets::dw_entity_list = g->offset_db.get_flat(ecrypt("dwEntityList"));
        offsets::dw_global_vars = g->offset_db.get_flat(ecrypt("dwGlobalVars"));
        offsets::dw_local_player_controller = g->offset_db.get_flat(ecrypt("dwLocalPlayerController"));
        offsets::dw_local_player_pawn = g->offset_db.get_flat(ecrypt("dwLocalPlayerPawn"));
        offsets::dw_view_matrix = g->offset_db.get_flat(ecrypt("dwViewMatrix"));
        offsets::dw_view_angles = g->offset_db.get_flat(ecrypt("dwViewAngles"));
        offsets::dw_network_game_client = g->offset_db.get_flat(ecrypt("dwNetworkGameClient"));
        offsets::dw_network_game_client_delta_tick = g->offset_db.get_flat(ecrypt("dwNetworkGameClient_deltaTick"));
        offsets::m_i_health = g->offset_db.get(ecrypt("C_CSPlayerPawn"), ecrypt("m_iHealth"));
        offsets::m_i_team_num = g->offset_db.get(ecrypt("C_CSPlayerPawn"), ecrypt("m_iTeamNum"));
        offsets::m_i_class_id = g->offset_db.get(ecrypt("C_BaseEntity"), ecrypt("m_iClassId")); // Added for triggerbot
        offsets::m_b_dormant = g->offset_db.get(ecrypt("C_BaseEntity"), ecrypt("m_bDormant")); // Added for triggerbot
        offsets::m_i_shots_fired = g->offset_db.get(ecrypt("C_CSPlayerPawn"), ecrypt("m_iShotsFired"));
        offsets::m_h_player_pawn = g->offset_db.get_flat(ecrypt("m_hPlayerPawn"));
        offsets::m_p_entity = g->offset_db.get_flat(ecrypt("m_pEntity"));
        offsets::m_s_sanitized_player_name = g->offset_db.get_flat(ecrypt("m_sSanitizedPlayerName"));
        offsets::m_p_collision = g->offset_db.get_flat(ecrypt("m_pCollision"));
        offsets::m_vec_origin = g->offset_db.get(ecrypt("CGameSceneNode"), ecrypt("m_vecOrigin"));
        offsets::m_vec_view_offset = g->offset_db.get_flat(ecrypt("m_vecViewOffset"));
        offsets::m_ang_eye_angles = g->offset_db.get_flat(ecrypt("m_angEyeAngles"));
        offsets::m_ang_rotation = g->offset_db.get_flat(ecrypt("m_angRotation"));
        offsets::v_angle = g->offset_db.get_flat(ecrypt("v_angle"));
        offsets::m_vec_mins = g->offset_db.get_flat(ecrypt("m_vecMins"));
        offsets::m_vec_maxs = g->offset_db.get_flat(ecrypt("m_vecMaxs"));
        offsets::m_p_game_scene_node = g->offset_db.get_flat(ecrypt("m_pGameSceneNode"));
        offsets::m_model_state = g->offset_db.get_flat(ecrypt("m_modelState"));
        offsets::m_i_rarity_override = g->offset_db.get_flat(ecrypt("m_iRarityOverride"));
        offsets::m_p_clipping_weapon = g->offset_db.get_flat(ecrypt("m_pClippingWeapon"));
        offsets::m_p_bullet_services = g->offset_db.get_flat(ecrypt("m_pBulletServices"));
        offsets::m_p_weapon_services = g->offset_db.get_flat(ecrypt("m_pWeaponServices"));
        offsets::m_total_hits_on_server = g->offset_db.get_flat(ecrypt("m_totalHitsOnServer"));
        offsets::m_aim_punch_cache = g->offset_db.get_flat(ecrypt("m_aimPunchCache"));
        offsets::m_v_smoke_color = g->offset_db.get(ecrypt("C_SmokeGrenadeProjectile"), ecrypt("m_vSmokeColor"));
        offsets::m_v_smoke_detonation_pos = g->offset_db.get(ecrypt("C_SmokeGrenadeProjectile"), ecrypt("m_vSmokeDetonationPos"));
        offsets::m_b_smoke_effect_spawned = g->offset_db.get(ecrypt("C_SmokeGrenadeProjectile"), ecrypt("m_bSmokeEffectSpawned"));
        offsets::m_n_smoke_effect_tick_begin = g->offset_db.get(ecrypt("C_SmokeGrenadeProjectile"), ecrypt("m_nSmokeEffectTickBegin"));
        offsets::m_fire_count = g->offset_db.get_flat(ecrypt("m_fireCount"));
        offsets::m_b_fire_is_burning = g->offset_db.get_flat(ecrypt("m_bFireIsBurning"));
        offsets::m_fire_positions = g->offset_db.get_flat(ecrypt("m_firePositions"));
        offsets::m_b_exposure_control = g->offset_db.get(ecrypt("C_PostProcessingVolume"), ecrypt("m_bExposureControl"));
        offsets::m_fl_min_exposure = g->offset_db.get(ecrypt("C_PostProcessingVolume"), ecrypt("m_flMinExposure"));
        offsets::m_fl_max_exposure = g->offset_db.get(ecrypt("C_PostProcessingVolume"), ecrypt("m_flMaxExposure"));
        offsets::m_fl_flash_max_alpha = g->offset_db.get(ecrypt("C_CSPlayerPawn"), ecrypt("m_flFlashMaxAlpha"));
        offsets::m_b_is_scoped = g->offset_db.get(ecrypt("C_CSPlayerPawn"), ecrypt("m_bIsScoped"));
        offsets::m_b_is_defusing = g->offset_db.get(ecrypt("C_CSPlayerPawn"), ecrypt("m_bIsDefusing"));
        offsets::m_fl_ping = g->offset_db.get(ecrypt("CCSPlayerController"), ecrypt("m_iPing"));
        offsets::m_fl_interpolation_amount = g->offset_db.get(ecrypt("C_BaseEntity"), ecrypt("m_flAnimTime"));
        offsets::m_n_last_server_tick = g->offset_db.get(ecrypt("C_BaseEntity"), ecrypt("m_nSimulationTick"));
        offsets::dw_force_attack = g->offset_db.get_flat(ecrypt("dwForceAttack"));
        offsets::m_i_id_ent_index = g->offset_db.get_flat(ecrypt("m_iIDEntIndex"));
        offsets::dw_force_jump = g->offset_db.get_flat(ecrypt("dwForceJump"));
        offsets::m_f_flags = g->offset_db.get_flat(ecrypt("m_fFlags"));

        // Optional FOV fields for FOV Changer (robust lookup with fallbacks)
        offsets::m_iFOV = g->offset_db.get(ecrypt("C_BasePlayerPawn"), ecrypt("m_iFOV"));
        if (!offsets::m_iFOV)
            offsets::m_iFOV = g->offset_db.get(ecrypt("C_CSPlayerPawn"), ecrypt("m_iFOV"));
        if (!offsets::m_iFOV)
            offsets::m_iFOV = g->offset_db.get_flat(ecrypt("m_iFOV"));

        offsets::m_iDefaultFOV = g->offset_db.get(ecrypt("C_BasePlayerPawn"), ecrypt("m_iDefaultFOV"));
        if (!offsets::m_iDefaultFOV)
            offsets::m_iDefaultFOV = g->offset_db.get(ecrypt("C_CSPlayerPawn"), ecrypt("m_iDefaultFOV"));
        if (!offsets::m_iDefaultFOV)
            offsets::m_iDefaultFOV = g->offset_db.get_flat(ecrypt("m_iDefaultFOV"));

        // Observer / spectator offsets
        // Try CCSPlayerController first, then CBasePlayerController, then flat map as fallback
        offsets::m_h_observer_target = g->offset_db.get(ecrypt("CCSPlayerController"), ecrypt("m_hObserverTarget"));
        if (!offsets::m_h_observer_target)
            offsets::m_h_observer_target = g->offset_db.get(ecrypt("CBasePlayerController"), ecrypt("m_hObserverTarget"));
        if (!offsets::m_h_observer_target)
            offsets::m_h_observer_target = g->offset_db.get_flat(ecrypt("m_hObserverTarget"));

        offsets::m_i_observer_mode = g->offset_db.get(ecrypt("CCSPlayerController"), ecrypt("m_iObserverMode"));
        if (!offsets::m_i_observer_mode)
            offsets::m_i_observer_mode = g->offset_db.get(ecrypt("CBasePlayerController"), ecrypt("m_iObserverMode"));
        if (!offsets::m_i_observer_mode)
            offsets::m_i_observer_mode = g->offset_db.get_flat(ecrypt("m_iObserverMode"));

        // Alternatives
        offsets::m_h_observer_pawn = g->offset_db.get(ecrypt("CCSPlayerController"), ecrypt("m_hObserverPawn"));
        if (!offsets::m_h_observer_pawn)
            offsets::m_h_observer_pawn = g->offset_db.get(ecrypt("CBasePlayerController"), ecrypt("m_hObserverPawn"));
        if (!offsets::m_h_observer_pawn)
            offsets::m_h_observer_pawn = g->offset_db.get_flat(ecrypt("m_hObserverPawn"));

        // Observer services on pawn
        offsets::m_p_observer_services = g->offset_db.get(ecrypt("C_BasePlayerPawn"), ecrypt("m_pObserverServices"));
        if (!offsets::m_p_observer_services)
            offsets::m_p_observer_services = g->offset_db.get_flat(ecrypt("m_pObserverServices"));

        offsets::m_h_observer_target_services = g->offset_db.get(ecrypt("CPlayer_ObserverServices"), ecrypt("m_hObserverTarget"));
        if (!offsets::m_h_observer_target_services)
            offsets::m_h_observer_target_services = g->offset_db.get_flat(ecrypt("m_hObserverTarget"));

        printf("Loading skin database...\n");
        if (!g->skin_db.load_all()) {
            printf("Failed to load skins database!\n");
            system("pause");
            return;
        }
        printf("Skin database loaded!\n");
        fflush(stdout);

        skins::ak47_skin_id = g->skin_db.get(ecrypt("AK-47 | Inheritance"));

        printf("Searching for CS2 process...\n");
        auto& process = g->core.get_process_info();
        
        // Retry logic for CS2 detection
        int retries = 0;
        do {
            process.id = g->memory.get_process_id(ecrypt(L"cs2.exe"));
            if (process.id != 0) break;
            
            printf("CS2 not found, retrying... (%d/5)\n", ++retries);
            Sleep(2000);
        } while (retries < 5);
        
        if (process.id == 0) {
            printf("ERROR: CS2 process not found after 5 attempts!\n");
            printf("Please make sure CS2 is running and try again.\n");
            system("pause");
            return;
        }
        printf("SUCCESS: Found CS2 process (PID: %d)\n", process.id);
        
        printf("Getting process info...\n");
        process.base = 0;
        process.dtb = 0;
        process.client_base = g->memory.get_module_base(process.id, ecrypt(L"client.dll"));
        process.engine2_base = g->memory.get_module_base(process.id, ecrypt(L"engine2.dll"));
        printf("Process base: 0x%llx\n", process.base);
        printf("Client base: 0x%llx\n", process.client_base);

        printf("Attaching to process...\n");
        if (!g->memory.attach(process.id)) {
            printf("Failed to attach to CS2 process!\n");
            system("pause");
            return;
        }
        printf("Successfully attached to CS2!\n");
        fflush(stdout);

        printf("Initializing overlay...\n");
        if (!g->overlay.initialize()) {
            printf("Failed to initialize overlay!\n");
            system("pause");
            return;
        }
        printf("Overlay initialized successfully!\n");
        fflush(stdout);

        printf("\n=== PREMKILLERV2 READY ===\n");
        printf("Press INSERT to toggle menu\n");
        printf("Starting main loop...\n\n");
        fflush(stdout);
        
        // Load default config on startup
        g->core.get_settings().load_config("default.cfg");
        
        g->updater.deploy();
        g->overlay.loop();
        
        // Auto-save on exit if enabled
        if (g->core.get_settings().config.auto_save) {
            g->core.get_settings().save_config("default.cfg");
        }
        
        printf("\nPremkillerv2 stopped.\n");
        
        // Keep console open
        system("pause");
}

// --- Simple Win32 Launcher ---
namespace {
    constexpr int ID_BTN_CS2 = 1001;
    constexpr int ID_BTN_LOADER = 1002;
    constexpr int ID_BTN_QUIT = 1003;
    constexpr int ID_BTN_CFG = 1004;
    static HFONT g_hFont = nullptr;
    static ULONG_PTR g_gdiplusToken = 0;
    static int g_hoveredBtn = 0; // 0: none, otherwise one of ID_BTN_*
    static bool g_trackingLeave = false;
    // Frameless + titlebar state
    static bool g_frameless = true;
    static RECT g_titlebarRc{0,0,0,0};
    static RECT g_btnCloseRc{0,0,0,0};
    static RECT g_btnMinRc{0,0,0,0};
    static bool g_titleTrackingLeave = false;
    static int g_titleHover = 0; // 0 none, 1 min, 2 close
    static int g_titlePressed = 0;
    static bool g_themeHover = false;
    static RECT g_autoBtnRc{0,0,0,0};
    // Hover animations
    static float g_animThemeHover = 0.f;
    static float g_animAutoHover = 0.f;
    // Animations
    static float g_animMainCs2 = 0.f, g_animMainLoader = 0.f, g_animMainQuit = 0.f;
    static float g_animTitleMin = 0.f, g_animTitleClose = 0.f;
    // Theme selection
    enum class ThemePreset : int { DarkBlue = 0, TealMagenta = 1, OrangePink = 2 };
    static ThemePreset g_preset = ThemePreset::DarkBlue;
    static RECT g_themeBtnRc{0,0,0,0};
    // Logo
    static std::unique_ptr<Gdiplus::Image> g_logoImg;
    // Tray icon
    static const UINT WMAPP_TRAY = WM_APP + 1;
    static NOTIFYICONDATAW g_nid{};
    static HMENU g_hTrayMenu = nullptr;
    static bool g_inTray = false;
    // Persisted window bounds
    static RECT g_lastWinRect{ CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT };

// ... (unchanged code)

    struct LauncherTheme {
        COLORREF bg;
        COLORREF btn;
        COLORREF btnPressed;
        COLORREF text;
// ... (unchanged code)

    void save_prefs_to_cfg(int themeIdx, bool autoLaunch)
    {
        wchar_t exe[MAX_PATH]; GetModuleFileNameW(nullptr, exe, MAX_PATH);
        std::wstring dir(exe); size_t p = dir.find_last_of(L"/\\"); if (p!=std::wstring::npos) dir.resize(p);
        std::wstring cfg = dir + L"\\default.cfg";
        HANDLE h = CreateFileW(cfg.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (h == INVALID_HANDLE_VALUE) return;
        RECT wr = g_lastWinRect;
        int w = (wr.right>wr.left)?(wr.right-wr.left):0;
        int h = (wr.bottom>wr.top)?(wr.bottom-wr.top):0;
        char buf[256]; int n = snprintf(buf, sizeof(buf),
            "theme=%d\nautolaunch=%d\nwin_x=%d\nwin_y=%d\nwin_w=%d\nwin_h=%d\n",
            themeIdx, autoLaunch?1:0, wr.left, wr.top, w, h);
        DWORD wrt = 0; WriteFile(h, buf, (DWORD)n, &wrt, nullptr);
        CloseHandle(h);
    }

    void load_prefs_from_cfg(int& themeIdx, bool& autoLaunch)
    {
        themeIdx = 0; autoLaunch = false;
        wchar_t exe[MAX_PATH]; GetModuleFileNameW(nullptr, exe, MAX_PATH);
        std::wstring dir(exe); size_t p = dir.find_last_of(L"/\\"); if (p!=std::wstring::npos) dir.resize(p);
        std::wstring cfg = dir + L"\\default.cfg";
        HANDLE h = CreateFileW(cfg.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (h == INVALID_HANDLE_VALUE) return;
        char buf[512]{}; DWORD rd=0; if (!ReadFile(h, buf, sizeof(buf)-1, &rd, nullptr) || rd==0) { CloseHandle(h); return; }
        CloseHandle(h);
        // crude parse
        const char* t = strstr(buf, "theme="); if (t) themeIdx = atoi(t+6);
        const char* a = strstr(buf, "autolaunch="); if (a) autoLaunch = atoi(a+11) != 0;
        // Window bounds (optional)
        const char* wx = strstr(buf, "win_x=");
        const char* wy = strstr(buf, "win_y=");
        const char* ww = strstr(buf, "win_w=");
        const char* wh = strstr(buf, "win_h=");
        if (wx && wy && ww && wh) {
            int x = atoi(wx+6), y = atoi(wy+6), w = atoi(ww+6), h = atoi(wh+6);
            if (w > 200 && h > 150) {
                g_lastWinRect.left = x; g_lastWinRect.top = y; g_lastWinRect.right = x + w; g_lastWinRect.bottom = y + h;
            }
        }
    }

    void apply_theme_from_index(int idx)
    {
        // Map to menu-like palettes
// ... (unchanged code)

    LRESULT CALLBACK LauncherWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch ( msg )
        {
        case WM_CREATE:
        {
            // DPI-aware sizes
            float scale = GetDPIScale(hWnd);
            const int bw = S(260, scale), bh = S(42, scale), pad = S(14, scale);

            // Load prefs (theme + autolaunch)
// ... (unchanged code)

            // Make window larger at startup (respect DPI)
        {
            float scale = GetDPIScale(hWnd);
            int w = S(1100, scale);
            int h = S(700, scale);
            // If we have persisted bounds, use them; otherwise use default size centered by OS
            if (g_lastWinRect.left != CW_USEDEFAULT && g_lastWinRect.top != CW_USEDEFAULT &&
                g_lastWinRect.right > g_lastWinRect.left && g_lastWinRect.bottom > g_lastWinRect.top) {
                int px = g_lastWinRect.left, py = g_lastWinRect.top;
                int pw = g_lastWinRect.right - g_lastWinRect.left;
                int ph = g_lastWinRect.bottom - g_lastWinRect.top;
                MoveWindow(hWnd, px, py, pw, ph, FALSE);
            } else {
                SetWindowPos(hWnd, nullptr, 0, 0, w, h, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
            }
        }

            // Frameless: convert to borderless popup (keep resizable via WS_THICKFRAME)
        if (g_frameless) {
            LONG_PTR style = GetWindowLongPtr(hWnd, GWL_STYLE);
            style &= ~(WS_OVERLAPPEDWINDOW);
            style |= WS_POPUP | WS_MINIMIZEBOX | WS_THICKFRAME; // resizable
            SetWindowLongPtr(hWnd, GWL_STYLE, style);
            SetWindowPos(hWnd, nullptr, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_FRAMECHANGED);
        }
        // Enable Mica backdrop on Windows 11+
        {
            const int DWMWA_SYSTEMBACKDROP_TYPE = 38; // undocumented value in older SDKs
            int backdrop = 2; // 2 = Mica
            DwmSetWindowAttribute(hWnd, DWMWA_SYSTEMBACKDROP_TYPE, &backdrop, sizeof(backdrop));
        }
        // Prepare tray menu
        if (!g_hTrayMenu) {
            g_hTrayMenu = CreatePopupMenu();
            AppendMenuW(g_hTrayMenu, MF_STRING, 1, L"Restore");
            AppendMenuW(g_hTrayMenu, MF_STRING, 2, L"Exit");
        }
        return 0;
    }

            // Prefer rounded window corners on Windows 11+
            const int DWMWA_WINDOW_CORNER_PREFERENCE = 33; // avoid header include mismatch
            int cornerPref = 2; // DWMWCP_ROUND
            DwmSetWindowAttribute(hWnd, DWMWA_WINDOW_CORNER_PREFERENCE, &cornerPref, sizeof(cornerPref));

            // Enable immersive dark title bar where supported
// ... (unchanged code)

            LRESULT hit = DefWindowProc(hWnd, msg, wParam, lParam);
            if (hit == HTCLIENT)
            {
                POINT pt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
                ScreenToClient(hWnd, &pt);
                // Allow dragging on top area except over controls
            RECT drag = { rc.left, rc.top, rc.right, rc.top + S(48, scale) };
            if (PtInRect(&drag, pt) && !PtInRect(&g_themeBtnRc, pt) && !PtInRect(&g_autoBtnRc, pt)
                && !PtInRect(&g_btnMinRc, pt) && !PtInRect(&g_btnCloseRc, pt))
                return HTCAPTION;
        }
        return hit;
    }
    case WM_MOUSEMOVE:
    {
        POINT p{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        bool th = PtInRect(&g_themeBtnRc, p) != 0;
        bool ah = PtInRect(&g_autoBtnRc, p) != 0;
        g_animThemeHover += th ? 0.1f : -0.1f;
        g_animAutoHover  += ah ? 0.1f : -0.1f;
        g_animThemeHover = std::clamp(g_animThemeHover, 0.f, 1.f);
        g_animAutoHover  = std::clamp(g_animAutoHover, 0.f, 1.f);
        InvalidateRect(hWnd, nullptr, FALSE);
        break;
    }
    case WM_GETMINMAXINFO:
        {
            float scale = GetDPIScale(hWnd);
            auto mmi = reinterpret_cast<MINMAXINFO*>(lParam);
            mmi->ptMinTrackSize.x = S(900, scale);
            mmi->ptMinTrackSize.y = S(560, scale);
// ... (unchanged code)
            return 0;
        }
        case WM_ERASEBKGND:
        {
            HDC hdc = (HDC)wParam;
            RECT rc; GetClientRect(hWnd, &rc);
            // Modern gradient background with diagonal accent
            RECT topRect = rc;
            DrawRoundedGradientRect(hdc, topRect, 14, g_theme.bg, RGB(
                (GetRValue(g_theme.bg)+GetRValue(g_theme.btn))/2,
                (GetGValue(g_theme.bg)+GetGValue(g_theme.btn))/2,
                (GetBValue(g_theme.bg)+GetBValue(g_theme.btn))/2));

            // Header branding
            Gdiplus::Graphics g(hdc);
            g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
            Gdiplus::SolidBrush accent(Gp(g_theme.accent1));
            Gdiplus::SolidBrush text(Gp(g_theme.text));
            const int pad = 14;
            Gdiplus::RectF headerRect((Gdiplus::REAL)pad, (Gdiplus::REAL)pad, 300.0f, 40.0f);
            Gdiplus::FontFamily ff(L"Segoe UI");
            Gdiplus::Font title(&ff, 18.0f, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
            Gdiplus::Font subtitle(&ff, 10.0f, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
            g.FillEllipse(&accent, (Gdiplus::REAL)pad, (Gdiplus::REAL)pad + 6.0f, 8.0f, 8.0f);
            g.DrawString(L"PREMKILLER", -1, &title, Gdiplus::PointF((Gdiplus::REAL)(pad+16), (Gdiplus::REAL)(pad)), &text);
            g.DrawString(L"Launcher", -1, &subtitle, Gdiplus::PointF((Gdiplus::REAL)(pad+16), (Gdiplus::REAL)(pad+22)), &text);

            // Draw logo (right of header)
            if (g_logoImg) {
                int lw = (int)g_logoImg->GetWidth();
                int lh = (int)g_logoImg->GetHeight();
                int targetH = 40; if (lh > 0) {
                    int targetW = (lw * targetH) / lh;
                    Gdiplus::Rect dest(rc.right - targetW - pad, pad, targetW, targetH);
                    g.DrawImage(g_logoImg.get(), dest);
                }
            }

            // Theme pill (top-right under logo) with subtle hover lightening
            {
                int pillW = 110, pillH = 28;
                g_themeBtnRc = RECT{ rc.right - pillW - pad, pad + 48, rc.right - pad, pad + 48 + pillH };
                Gdiplus::GraphicsPath path;
                float x = (float)g_themeBtnRc.left, y = (float)g_themeBtnRc.top, w = (float)(pillW), h = (float)(pillH), r = h/2.0f;
                path.AddArc(x, y, r, r, 180, 90);
                path.AddArc(x + w - r, y, r, r, 270, 90);
                path.AddArc(x + w - r, y + h - r, r, r, 0, 90);
                path.AddArc(x, y + h - r, r, r, 90, 90);
                path.CloseFigure();
                auto lerp = [](int a,int b,float t){ return a + (int)((b-a)*t); };
                COLORREF top = RGB( lerp(GetRValue(g_theme.btn), GetRValue(g_theme.accent1), g_animThemeHover*0.25f),
                                    lerp(GetGValue(g_theme.btn), GetGValue(g_theme.accent1), g_animThemeHover*0.25f),
                                    lerp(GetBValue(g_theme.btn), GetBValue(g_theme.accent1), g_animThemeHover*0.25f) );
                COLORREF bot = RGB( lerp(GetRValue(g_theme.btnPressed), GetRValue(g_theme.accent2), g_animThemeHover*0.25f),
                                    lerp(GetGValue(g_theme.btnPressed), GetGValue(g_theme.accent2), g_animThemeHover*0.25f),
                                    lerp(GetBValue(g_theme.btnPressed), GetBValue(g_theme.accent2), g_animThemeHover*0.25f) );
                Gdiplus::LinearGradientBrush br(Gdiplus::PointF(x, y), Gdiplus::PointF(x, y+h), Gp(top), Gp(bot));
                g.FillPath(&br, &path);
                Gdiplus::Font pillFont(&ff, 11.0f, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
                const wchar_t* tname = (g_preset==ThemePreset::DarkBlue)?L"Theme: Blue":(g_preset==ThemePreset::TealMagenta?L"Theme: Teal":L"Theme: Sunset");
                g.DrawString(tname, -1, &pillFont, Gdiplus::PointF(x + 14.0f, y + 6.0f), &text);
            }

            // Auto-Launch pill with subtle hover lightening
            {
                int pillW = 150, pillH = 28;
                g_autoBtnRc = RECT{ rc.right - pillW - pad - 120, pad + 48, rc.right - pad - 120, pad + 48 + pillH };
                Gdiplus::GraphicsPath path;
                float x = (float)g_autoBtnRc.left, y = (float)g_autoBtnRc.top, w = (float)(pillW), h = (float)(pillH), r = h/2.0f;
                path.AddArc(x, y, r, r, 180, 90);
                path.AddArc(x + w - r, y, r, r, 270, 90);
                path.AddArc(x + w - r, y + h - r, r, r, 0, 90);
                path.AddArc(x, y + h - r, r, r, 90, 90);
                path.CloseFigure();
                auto lerp = [](int a,int b,float t){ return a + (int)((b-a)*t); };
                COLORREF top = RGB( lerp(GetRValue(g_theme.btn), GetRValue(g_theme.accent1), g_animAutoHover*0.25f),
                                    lerp(GetGValue(g_theme.btn), GetGValue(g_theme.accent1), g_animAutoHover*0.25f),
                                    lerp(GetBValue(g_theme.btn), GetBValue(g_theme.accent1), g_animAutoHover*0.25f) );
                COLORREF bot = RGB( lerp(GetRValue(g_theme.btnPressed), GetRValue(g_theme.accent2), g_animAutoHover*0.25f),
                                    lerp(GetGValue(g_theme.btnPressed), GetGValue(g_theme.accent2), g_animAutoHover*0.25f),
                                    lerp(GetBValue(g_theme.btnPressed), GetBValue(g_theme.accent2), g_animAutoHover*0.25f) );
                Gdiplus::LinearGradientBrush br(Gdiplus::PointF(x, y), Gdiplus::PointF(x, y+h), Gp(top), Gp(bot));
                g.FillPath(&br, &path);
                Gdiplus::Font pillFont(&ff, 11.0f, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
                const wchar_t* aname = g_autoLaunch ? L"Auto-Launch: ON" : L"Auto-Launch: OFF";
                g.DrawString(aname, -1, &pillFont, Gdiplus::PointF(x + 14.0f, y + 6.0f), &text);
            }

            // Status panel (left side)
            {
                int panelW = 360, panelH = 120;
                RECT pr{ pad, pad + 64, pad + panelW, pad + 64 + panelH };
                DrawRoundedGradientRect(hdc, pr, 12, RGB( (GetRValue(g_theme.btn)+20), (GetGValue(g_theme.btn)+20), (GetBValue(g_theme.btn)+20) ), g_theme.btn );
                Gdiplus::Font statusFont(&ff, 12.0f, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
                Gdiplus::Font smallFont(&ff, 10.0f, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
                g.DrawString(g_statusText.c_str(), -1, &statusFont, Gdiplus::PointF((Gdiplus::REAL)(pr.left + 12), (Gdiplus::REAL)(pr.top + 10)), &text);
                const wchar_t* tip = g_tips[g_tipIndex];
                g.DrawString(tip, -1, &smallFont, Gdiplus::PointF((Gdiplus::REAL)(pr.left + 12), (Gdiplus::REAL)(pr.top + 38)), &text);
                const wchar_t* hint = L"Shortcuts: ENTER = Launch Cheat, C = Launch CS2, ESC/Q = Quit";
                g.DrawString(hint, -1, &smallFont, Gdiplus::PointF((Gdiplus::REAL)(pr.left + 12), (Gdiplus::REAL)(pr.top + 62)), &text);
            }

            // Footer brand/version
            {
                Gdiplus::Font foot(&ff, 9.0f, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
                const wchar_t* ver = L"v3";
                g.DrawString(ver, -1, &foot, Gdiplus::PointF((Gdiplus::REAL)(rc.right - 32), (Gdiplus::REAL)(rc.bottom - 20)), &text);
            }

            // Custom title buttons (minimize/close)
            {
                int btnW = 36, btnH = 24;
                g_btnMinRc = RECT{ rc.right - 2*btnW - 6, 6, rc.right - btnW - 6, 6 + btnH };
                g_btnCloseRc = RECT{ rc.right - btnW - 6, 6, rc.right - 6, 6 + btnH };
                auto drawBtn = [&](const RECT& rct, bool hover, bool close){
                    Gdiplus::GraphicsPath p; float x=(float)rct.left, y=(float)rct.top, w=(float)(rct.right-rct.left), h=(float)(rct.bottom-rct.top), r=6.f;
                    p.AddArc(x, y, r, r, 180, 90); p.AddArc(x+w-r, y, r, r, 270, 90); p.AddArc(x+w-r, y+h-r, r, r, 0, 90); p.AddArc(x, y+h-r, r, r, 90, 90); p.CloseFigure();
                    COLORREF a = hover ? g_theme.accent1 : g_theme.btn; COLORREF b = hover ? g_theme.accent2 : g_theme.btnPressed;
                    Gdiplus::LinearGradientBrush br(Gdiplus::PointF(x, y), Gdiplus::PointF(x, y+h), Gp(a), Gp(b)); g.FillPath(&br, &p);
                    Gdiplus::Pen pen(Gp(g_theme.text)); pen.SetWidth(1.5f);
                    if (close) {
                        g.DrawLine(&pen, x+12, y+8, x+w-12, y+h-8);
                        g.DrawLine(&pen, x+w-12, y+8, x+12, y+h-8);
                    } else {
                        g.DrawLine(&pen, x+10, y+h/2, x+w-10, y+h/2);
                    }
                };
                drawBtn(g_btnMinRc, g_titleHover==1, false);
                drawBtn(g_btnCloseRc, g_titleHover==2, true);
            }

            // Busy overlay
            if (g_busy) {
                Gdiplus::SolidBrush overlay(Gdiplus::Color(120,0,0,0));
                g.FillRectangle(&overlay, Gdiplus::RectF(0,0,(Gdiplus::REAL)(rc.right), (Gdiplus::REAL)(rc.bottom)));
                Gdiplus::Font busy(&ff, 14.0f, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
                g.DrawString(L"Launching cheat...", -1, &busy, Gdiplus::PointF((Gdiplus::REAL)(rc.right/2 - 90), (Gdiplus::REAL)(rc.bottom/2 - 10)), &text);
            }
            return 1;
        }
        case WM_SIZE:
        {
            // Recenter vertical buttons
            float scale = GetDPIScale(hWnd);
            const int bw = S(260, scale), bh = S(42, scale), pad = S(14, scale);
            RECT cr{}; GetClientRect(hWnd, &cr);
            int clientW = cr.right - cr.left;
            int x = (clientW - bw) / 2; if (x < pad) x = pad;
            int y = pad;
            HWND hCs2 = GetDlgItem(hWnd, ID_BTN_CS2);
            HWND hLoader = GetDlgItem(hWnd, ID_BTN_LOADER);
            HWND hQuit = GetDlgItem(hWnd, ID_BTN_QUIT);
            HWND hCfg = GetDlgItem(hWnd, ID_BTN_CFG);
            if (hCs2) SetWindowPos(hCs2, nullptr, x, y, bw, bh, SWP_NOZORDER);
            y += bh + pad;
            if (hLoader) SetWindowPos(hLoader, nullptr, x, y, bw, bh, SWP_NOZORDER);
            y += bh + pad;
            if (hQuit) SetWindowPos(hQuit, nullptr, x, y, bw, bh, SWP_NOZORDER);
            y += bh + pad;
            if (hCfg) SetWindowPos(hCfg, nullptr, x, y, bw, bh, SWP_NOZORDER);
            return 0;
        }
        case WM_COMMAND:
        {
            const int id = LOWORD(wParam);
            if ( id == ID_BTN_CS2 )
            {
                ShellExecuteW(nullptr, L"open", L"steam://rungameid/730", nullptr, nullptr, SW_SHOWNORMAL);
                return 0;
            }
            if ( id == ID_BTN_LOADER )
            {
                if (g_busy) return 0;
                g_busy = true; g_statusText = L"Status: Launching cheat...";
                InvalidateRect(hWnd, nullptr, FALSE);
                // Launch a new instance of this exe with --run-cheat in a separate console
                wchar_t self[MAX_PATH] = {};
                GetModuleFileNameW(nullptr, self, MAX_PATH);
                std::wstring cmd = L"\""; cmd += self; cmd += L"\" --run-cheat";
                STARTUPINFOW si{}; si.cb = sizeof(si);
                PROCESS_INFORMATION pi{};
                // Important: pass the command line buffer, not just the exe path
                if ( CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, FALSE, CREATE_NEW_CONSOLE, nullptr, nullptr, &si, &pi) )
                {
                    // Close our handles; new process continues independently
                    CloseHandle(pi.hThread);
                    CloseHandle(pi.hProcess);
                }
                // Close launcher window and exit launcher app
                DestroyWindow(hWnd);
                PostQuitMessage(0);
                return 0;
            }
            if ( id == ID_BTN_QUIT )
            {
                PostQuitMessage(0);
                return 0;
            }
            if ( id == ID_BTN_CFG )
            {
                wchar_t exe[MAX_PATH]; GetModuleFileNameW(nullptr, exe, MAX_PATH);
                std::wstring dir(exe); size_t p = dir.find_last_of(L"/\\"); if (p!=std::wstring::npos) dir.resize(p);
                ShellExecuteW(nullptr, L"open", dir.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
                return 0;
            }
            break;
        }
        case WM_CLOSE:
            DestroyWindow(hWnd);
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        }
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }

    int run_launcher()
    {
        // Initialize GDI+ for modern rendering
        if (g_gdiplusToken == 0) {
            Gdiplus::GdiplusStartupInput gdiplusStartupInput;
            if (Gdiplus::GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, nullptr) != Gdiplus::Ok) {
                g_gdiplusToken = 0;
            }
        }
        const wchar_t* clsName = L"PremkillerLauncherWnd";
        WNDCLASSEXW wc{ sizeof(WNDCLASSEXW) };
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = LauncherWndProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszClassName = clsName;
        if ( !RegisterClassExW(&wc) )
            return 1;

        // DPI scaling for initial size (vertical layout)
        const int bw = 260, bh = 42, pad = 14;
        RECT rc{ 0, 0, bw + 2 * pad, 3 * bh + 4 * pad };
        DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN;
        DWORD exStyle = 0;
        AdjustWindowRectEx(&rc, style, FALSE, exStyle);
        int winW = rc.right - rc.left;
        int winH = rc.bottom - rc.top;

        // Center the window on the monitor with the cursor
        POINT pt; GetCursorPos(&pt);
        HMONITOR hMon = MonitorFromPoint(pt, MONITOR_DEFAULTTOPRIMARY);
        MONITORINFO mi{ sizeof(MONITORINFO) };
        GetMonitorInfoW(hMon, &mi);
        int screenW = mi.rcWork.right - mi.rcWork.left;
        int screenH = mi.rcWork.bottom - mi.rcWork.top;
        int x = mi.rcWork.left + (screenW - winW) / 2;
        int y = mi.rcWork.top + (screenH - winH) / 2;

        HWND hWnd = CreateWindowExW(exStyle, clsName, L"PREMKILLER v3 Launcher",
            style, x, y, winW, winH,
            nullptr, nullptr, GetModuleHandle(nullptr), nullptr);
        if ( !hWnd ) return 1;
        ShowWindow(hWnd, SW_SHOWNORMAL);
        UpdateWindow(hWnd);

        // Keyboard accelerators
        ACCEL accels[] = {
            { FVIRTKEY, VK_RETURN, (WORD)ID_BTN_LOADER },
            { FVIRTKEY, 'L',       (WORD)ID_BTN_LOADER },
            { FVIRTKEY, 'C',       (WORD)ID_BTN_CS2    },
            { FVIRTKEY, VK_ESCAPE, (WORD)ID_BTN_QUIT   },
            { FVIRTKEY, 'Q',       (WORD)ID_BTN_QUIT   },
        };
        HACCEL hAccel = CreateAcceleratorTableW(accels, ARRAYSIZE(accels));

        MSG msg{};
        while ( GetMessage(&msg, nullptr, 0, 0) > 0 )
        {
            if (!TranslateAcceleratorW(hWnd, hAccel, &msg))
                TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        if (hAccel) DestroyAcceleratorTable(hAccel);
        int code = (int)msg.wParam;
        if (g_gdiplusToken) {
            Gdiplus::GdiplusShutdown(g_gdiplusToken);
            g_gdiplusToken = 0;
        }
        return code;
    }
}

int main()
{
    // Enable DPI awareness for crisp UI and correct scaling
    HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
    if (hUser32)
    {
        typedef BOOL (WINAPI *PFN_SetProcessDpiAwarenessContext)(HANDLE);
        auto pSetCtx = (PFN_SetProcessDpiAwarenessContext)GetProcAddress(hUser32, "SetProcessDpiAwarenessContext");
        if (pSetCtx)
        {
            pSetCtx(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        }
        else
        {
            SetProcessDPIAware();
        }
    }
    // If launched with --run-cheat, skip launcher and run the cheat directly
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    bool run_cheat = false;
    for (int i = 1; i < argc; ++i)
    {
        if ( lstrcmpiW(argv[i], L"--run-cheat") == 0 ) { run_cheat = true; break; }
    }
    if ( argv ) LocalFree(argv);

    if ( run_cheat )
    {
        launch_premkiller();
        return 0;
    }
    case WM_SIZE:
    {
        if (wParam == SIZE_MINIMIZED) {
            // Add tray icon and hide window
            if (!g_inTray) {
                g_nid = {};
                g_nid.cbSize = sizeof(g_nid);
                g_nid.hWnd = hWnd;
                g_nid.uID = 1;
                g_nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
                g_nid.uCallbackMessage = WMAPP_TRAY;
                g_nid.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
                lstrcpynW(g_nid.szTip, L"premkiller launcher", ARRAYSIZE(g_nid.szTip));
                Shell_NotifyIconW(NIM_ADD, &g_nid);
                g_inTray = true;
                ShowWindow(hWnd, SW_HIDE);
            }
        }
        // existing size handling below
        
        // Recenter vertical buttons
        float scale = GetDPIScale(hWnd);
        const int bw = S(260, scale), bh = S(42, scale), pad = S(14, scale);
        RECT cr{}; GetClientRect(hWnd, &cr);
        int clientW = cr.right - cr.left;
        int x = (clientW - bw) / 2; if (x < pad) x = pad;
        int y = pad;
        HWND hCs2 = GetDlgItem(hWnd, ID_BTN_CS2);
        HWND hLoader = GetDlgItem(hWnd, ID_BTN_LOADER);
        HWND hQuit = GetDlgItem(hWnd, ID_BTN_QUIT);
        HWND hCfg = GetDlgItem(hWnd, ID_BTN_CFG);
        if (hCs2) SetWindowPos(hCs2, nullptr, x, y, bw, bh, SWP_NOZORDER);
        y += bh + pad;
        if (hLoader) SetWindowPos(hLoader, nullptr, x, y, bw, bh, SWP_NOZORDER);
        y += bh + pad;
        if (hQuit) SetWindowPos(hQuit, nullptr, x, y, bw, bh, SWP_NOZORDER);
        y += bh + pad;
        if (hCfg) SetWindowPos(hCfg, nullptr, x, y, bw, bh, SWP_NOZORDER);
        return 0;
    }
    case WM_MOVE:
    case WM_EXITSIZEMOVE:
    {
        RECT wr{}; if (GetWindowRect(hWnd, &wr)) g_lastWinRect = wr;
        return 0;
    }
    case WM_APP:
    default:
        break;
    }

    if (msg == WMAPP_TRAY) {
        if (lParam == WM_LBUTTONDBLCLK) {
            // Restore window
            ShowWindow(hWnd, SW_SHOW);
            ShowWindow(hWnd, SW_RESTORE);
            SetForegroundWindow(hWnd);
            if (g_inTray) { Shell_NotifyIconW(NIM_DELETE, &g_nid); g_inTray = false; }
            return 0;
        } else if (lParam == WM_RBUTTONUP) {
            POINT pt; GetCursorPos(&pt);
            SetForegroundWindow(hWnd);
            TrackPopupMenu(g_hTrayMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hWnd, nullptr);
            return 0;
        }
    }
    if (msg == WM_COMMAND && HIWORD(wParam) == 0 && (LOWORD(wParam) == 1 || LOWORD(wParam) == 2)) {
        // Tray menu commands
        if (LOWORD(wParam) == 1) {
            // Restore
            ShowWindow(hWnd, SW_SHOW);
            ShowWindow(hWnd, SW_RESTORE);
            SetForegroundWindow(hWnd);
            if (g_inTray) { Shell_NotifyIconW(NIM_DELETE, &g_nid); g_inTray = false; }
        } else if (LOWORD(wParam) == 2) {
            PostMessage(hWnd, WM_CLOSE, 0, 0);
        }
        return 0;
    }
    return run_launcher();
}