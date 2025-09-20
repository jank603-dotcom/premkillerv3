#include <include/global.hpp>

bool util_c::console_c::initialize( )
{
	this->handle = call_function( &GetStdHandle, STD_OUTPUT_HANDLE );
	this->input_handle = call_function( &GetStdHandle, STD_INPUT_HANDLE );

	CONSOLE_FONT_INFOEX cfi{};
	cfi.cbSize = sizeof( CONSOLE_FONT_INFOEX );

	if ( call_function( &GetCurrentConsoleFontEx, this->handle, FALSE, &cfi ) )
	{
		wcscpy_s( cfi.FaceName, ecrypt( L"Consolas" ) );
		cfi.dwFontSize.X = 0;
		cfi.dwFontSize.Y = 20;

		call_function( &SetCurrentConsoleFontEx, this->handle, FALSE, &cfi );
	}

	call_function( &SetConsoleTitleA, ecrypt( "premkillerv3" ) );

	auto hwnd = call_function( &GetConsoleWindow );
	if ( hwnd != NULL )
	{
		auto style = call_function( &GetWindowLong, hwnd, GWL_STYLE );
		style &= ~( WS_MAXIMIZEBOX | WS_SIZEBOX );

		call_function( &SetWindowLong, hwnd, GWL_STYLE, style );
		call_function( &SetWindowLong, hwnd, GWL_EXSTYLE, call_function( &GetWindowLong, hwnd, GWL_EXSTYLE ) | WS_EX_LAYERED );

		call_function( &SetLayeredWindowAttributes, hwnd, 0, 240, LWA_ALPHA );
	}

	CONSOLE_CURSOR_INFO cursor_info{ 0 };
	call_function( &GetConsoleCursorInfo, this->handle, &cursor_info );

	cursor_info.bVisible = FALSE;
	call_function( &SetConsoleCursorInfo, this->handle, &cursor_info );

	DWORD mode{ 0 };
	if ( call_function( &GetConsoleMode, this->input_handle, &mode ) )
	{
		mode |= ENABLE_QUICK_EDIT_MODE | ENABLE_MOUSE_INPUT;
		call_function( &SetConsoleMode, this->input_handle, mode );
	}

	call_function( &FlushConsoleInputBuffer, this->input_handle );

	DWORD cm{ 0 };
	if ( call_function( &GetConsoleMode, this->handle, &cm ) )
	{
		call_function( &SetConsoleMode, this->handle, cm | ENABLE_VIRTUAL_TERMINAL_PROCESSING );
	}

	return this->handle != 0;
}

void util_c::console_c::print( const char* text, ... )
{
	va_list args;
	va_start( args, text );

	constexpr auto label = "premkillerv3";

	constexpr auto reset = "\033[0m";
	constexpr auto white = "\033[38;2;255;255;255m";
	constexpr auto blue = "\033[38;2;160;200;255m";
	constexpr auto brackets = "\033[38;2;220;220;220m";

	call_function( &printf, "%s[%s %s %s]%s -> %s", brackets, blue, label, brackets, reset, white );
	call_function( &vfprintf, stdout, text, args );
	call_function( &printf, "%s\n", reset );

	va_end( args );
}

void util_c::console_c::space( )
{
	call_function( &printf, "\n" );
}

void util_c::console_c::sleep_ms( int milliseconds )
{
	std::this_thread::sleep_for( std::chrono::milliseconds( milliseconds ) );
}

void util_c::console_c::wait_for_input( )
{
	call_function( &fflush, stdout );
	call_function( &getchar );
}