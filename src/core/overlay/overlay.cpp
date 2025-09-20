#include <include/global.hpp>

#include <include/util/external/fonts/proggy_tiny.hpp>
#include <include/util/external/fonts/tahoma_bold.hpp>

bool overlay_c::initialize( )
{
	if ( !this->setup_window( ) )
	{
		return false;
	}

	if ( !this->setup_d3d11( ) )
	{
		return false;
	}

	return true;
}

void overlay_c::loop( )
{
	call_function( &SetPriorityClass, call_function( &GetCurrentProcess ), HIGH_PRIORITY_CLASS );
	call_function( &SetThreadPriority, call_function( &GetCurrentThread ), THREAD_PRIORITY_HIGHEST );

	constexpr float clear_color[ 4 ] = { 0.0f, 0.0f, 0.0f, 0.0f };
	MSG msg{};

	auto last_refresh_time = std::chrono::steady_clock::now( );

	while ( msg.message != WM_QUIT && g->menu.is_running )
	{
		while ( PeekMessage( &msg, this->hwnd, 0, 0, PM_REMOVE ) )
		{
			if ( msg.message == WM_QUIT )
			{
				break;
			}

			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}

		auto& io = ImGui::GetIO( );
		io.DeltaTime = 1.0f / 60.0f;

		if ( g->menu.is_visible )
		{
			POINT cursor_pos{};
			GetCursorPos( &cursor_pos );
			io.MousePos = ImVec2( static_cast< float >( cursor_pos.x ), static_cast< float >( cursor_pos.y ) );
			io.MouseDown[ 0 ] = ( GetAsyncKeyState( VK_LBUTTON ) & 0x8000 ) != 0;
		}

		{
			ImGui_ImplDX11_NewFrame( );
			ImGui_ImplWin32_NewFrame( );
		}

		ImGui::NewFrame( );
		{
			const auto drawlist = ImGui::GetBackgroundDrawList( );

			g->dispatch.run( drawlist );
			g->menu.run( );
		}
		ImGui::Render( );

		{
			this->device_context->OMSetRenderTargets( 1, &this->render_target_view, nullptr );
			this->device_context->ClearRenderTargetView( this->render_target_view, clear_color );
		}

		{
			ImGui_ImplDX11_RenderDrawData( ImGui::GetDrawData( ) );
		}

		if ( g->core.get_settings( ).misc.do_no_wait )
		{
			this->swap_chain->Present( g->core.get_settings( ).misc.vsync, DXGI_PRESENT_DO_NOT_WAIT );
		}
		else
		{
			this->swap_chain->Present( g->core.get_settings( ).misc.vsync, 0 );
		}
	}

	ImGui_ImplDX11_Shutdown( );
	ImGui_ImplWin32_Shutdown( );
	ImGui::DestroyContext( );
	
	if ( this->render_target_view ) this->render_target_view->Release();
	if ( this->swap_chain ) this->swap_chain->Release();
	if ( this->device_context ) this->device_context->Release();
	if ( this->device ) this->device->Release();
}

bool overlay_c::setup_d3d11( )
{
	DXGI_SWAP_CHAIN_DESC sc_desc{};
	sc_desc.BufferCount = 2;
	sc_desc.BufferDesc.Width = 0;
	sc_desc.BufferDesc.Height = 0;
	sc_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sc_desc.BufferDesc.RefreshRate.Numerator = 0;
	sc_desc.BufferDesc.RefreshRate.Denominator = 0;
	sc_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	sc_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sc_desc.OutputWindow = this->hwnd;
	sc_desc.SampleDesc.Count = 1;
	sc_desc.SampleDesc.Quality = 0;
	sc_desc.Windowed = TRUE;
	sc_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	const D3D_FEATURE_LEVEL feature_levels[ ] = { D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0 };
	D3D_FEATURE_LEVEL selected_feature_level;

	constexpr std::uint32_t device_flags = D3D11_CREATE_DEVICE_SINGLETHREADED | D3D11_CREATE_DEVICE_PREVENT_INTERNAL_THREADING_OPTIMIZATIONS;

	auto result = call_function( &D3D11CreateDeviceAndSwapChain, nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, device_flags, feature_levels, _countof( feature_levels ), D3D11_SDK_VERSION, &sc_desc, &this->swap_chain, &this->device, &selected_feature_level, &this->device_context );
	if ( FAILED( result ) )
	{
		return false;
	}

	ID3D11Texture2D* back_buffer = nullptr;
	result = this->swap_chain->GetBuffer( 0, IID_PPV_ARGS( &back_buffer ) );
	if ( FAILED( result ) || back_buffer == nullptr )
	{
		return false;
	}

	result = this->device->CreateRenderTargetView( back_buffer, nullptr, &this->render_target_view );
	back_buffer->Release( );

	if ( FAILED( result ) )
	{
		return false;
	}

	IDXGIDevice1* dxgi_device = nullptr;
	if ( SUCCEEDED( this->device->QueryInterface( __uuidof( IDXGIDevice1 ), ( void** )&dxgi_device ) ) )
	{
		dxgi_device->SetMaximumFrameLatency( 1 );
		dxgi_device->Release( );
	}

	IDXGISwapChain2* swap_chain2 = nullptr;
	if ( SUCCEEDED( this->swap_chain->QueryInterface( IID_PPV_ARGS( &swap_chain2 ) ) ) )
	{
		swap_chain2->SetMaximumFrameLatency( 1 );
		swap_chain2->Release( );
	}

	ImGui::CreateContext( );

	auto& io = ImGui::GetIO( );
	io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
	io.IniFilename = nullptr;
	io.LogFilename = nullptr;

	// Build a font atlas that supports multiple alphabets for name rendering (UTF-8)
    // Base: embedded Tahoma Bold
    io.Fonts->Clear();
    ImFontConfig base_cfg;
    base_cfg.OversampleH = 3;
    base_cfg.OversampleV = 2;
    base_cfg.PixelSnapH = false;
    base_cfg.MergeMode = false;
    ImFont* base_font = io.Fonts->AddFontFromMemoryTTF(tahoma_bold, sizeof(tahoma_bold), 13.0f, &base_cfg, io.Fonts->GetGlyphRangesDefault());

    // Merge fallback fonts for other scripts from Windows fonts when available
    ImFontConfig merge_cfg = base_cfg;
    merge_cfg.MergeMode = true;
    merge_cfg.GlyphMinAdvanceX = 0.0f; // keep same size

    // Helper to add a font file with a given range
    auto try_add_font_with_range = [&](const char* path, const ImWchar* ranges) -> void {
        if (!path || !ranges) return;
        io.Fonts->AddFontFromFileTTF(path, 13.0f, &merge_cfg, ranges);
    };

    // Build a composite range for Latin + Cyrillic + Greek + Arabic + Thai + Vietnamese
    ImFontGlyphRangesBuilder builder;
    builder.AddRanges(io.Fonts->GetGlyphRangesDefault());
    builder.AddRanges(io.Fonts->GetGlyphRangesCyrillic());
    builder.AddRanges(io.Fonts->GetGlyphRangesThai());
    builder.AddRanges(io.Fonts->GetGlyphRangesVietnamese());
    // Greek (U+0370–U+03FF, Greek Extended U+1F00–U+1FFF)
    static const ImWchar greek_ranges[] = { 0x0370, 0x03FF, 0x1F00, 0x1FFF, 0 };
    builder.AddRanges(greek_ranges);
    // Arabic (U+0600–U+06FF, Supplement U+0750–U+077F, Extended-A U+08A0–U+08FF)
    static const ImWchar arabic_ranges[] = { 0x0600, 0x06FF, 0x0750, 0x077F, 0x08A0, 0x08FF, 0 };
    builder.AddRanges(arabic_ranges);
    ImVector<ImWchar> merged_ranges;
    builder.BuildRanges(&merged_ranges);

    // Use Segoe UI for many scripts on Windows
    try_add_font_with_range("C:/Windows/Fonts/segoeui.ttf", merged_ranges.Data);

    // CJK - try common Windows fonts (merge in order; whichever exists will provide glyphs)
    try_add_font_with_range("C:/Windows/Fonts/msyh.ttc", io.Fonts->GetGlyphRangesChineseFull());    // Microsoft YaHei (Simplified Chinese)
    try_add_font_with_range("C:/Windows/Fonts/simhei.ttf", io.Fonts->GetGlyphRangesChineseFull());  // SimHei fallback
    try_add_font_with_range("C:/Windows/Fonts/msjh.ttc", io.Fonts->GetGlyphRangesChineseFull());    // Microsoft JhengHei (Traditional Chinese)
    try_add_font_with_range("C:/Windows/Fonts/meiryo.ttc", io.Fonts->GetGlyphRangesJapanese());     // Japanese
    try_add_font_with_range("C:/Windows/Fonts/msgothic.ttc", io.Fonts->GetGlyphRangesJapanese());   // Japanese fallback
    try_add_font_with_range("C:/Windows/Fonts/malgun.ttf", io.Fonts->GetGlyphRangesKorean());       // Korean

    // Finalize font atlas
    io.Fonts->Build();

	ImGui_ImplWin32_Init( this->hwnd );
	ImGui_ImplDX11_Init( this->device, this->device_context );

	return true;
}

bool overlay_c::setup_window( )
{
	auto game = call_function( &FindWindowA, nullptr, ecrypt( "Counter-Strike 2" ) );
	if ( !game )
	{
		// Launch CS2 via Steam
		ShellExecuteA(nullptr, "open", "steam://rungameid/730", nullptr, nullptr, SW_HIDE);
		
		// Wait 1 minute for CS2 to launch
		Sleep(60000);
		
		// Try to find the window again
		game = call_function( &FindWindowA, nullptr, ecrypt( "Counter-Strike 2" ) );
		if ( !game )
		{
			return false;
		}
	}

	g->util.console.print( ecrypt( "hwnd.game: 0x%p" ), game );

	if ( !this->find_ime( ) )
	{
		return false;
	}

	g->util.console.print( ecrypt( "overlay.hwnd: 0x%p" ), this->hwnd );

	if ( !this->align_to_game( game ) )
	{
		return false;
	}

	this->set_attributes( );

	g->util.console.space( );

	return true;
}

bool overlay_c::find_ime( )
{
	const auto desktop = call_function( &GetDesktopWindow );
	const auto target = g->core.get_process_info( ).id;

	HWND current = nullptr;
	while ( ( current = call_function( &FindWindowExA, desktop, current, ecrypt( "IME" ), ecrypt( "Default IME" ) ) ) != nullptr )
	{
		DWORD window = 0;
		call_function( &GetWindowThreadProcessId, current, &window );

		if ( window == target )
		{
			this->hwnd = current;
			return true;
		}
	}

	return false;
}

bool overlay_c::align_to_game( HWND hwnd ) const
{
	RECT rect{};
	if ( !call_function( &GetClientRect, hwnd, &rect ) )
	{
		return false;
	}

	auto top_left = POINT( rect.left, rect.top );
	auto bottom_right = POINT( rect.right, rect.bottom );

	call_function( &ClientToScreen, hwnd, &top_left );
	call_function( &ClientToScreen, hwnd, &bottom_right );

	const auto width = bottom_right.x - top_left.x;
	const auto height = bottom_right.y - top_left.y;

	return call_function( &SetWindowPos, this->hwnd, nullptr, top_left.x, top_left.y, width, height, SWP_NOZORDER );
}

void overlay_c::set_attributes( ) const
{
	call_function( &SetWindowLongA, this->hwnd, GWL_EXSTYLE, WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW );

	const MARGINS margins = { -1 };
	call_function( &DwmExtendFrameIntoClientArea, this->hwnd, &margins );

	call_function( &SetLayeredWindowAttributes, this->hwnd, 0, 255, LWA_ALPHA );

	call_function( &UpdateWindow, this->hwnd );
	call_function( &ShowWindow, this->hwnd, SW_SHOW );
}