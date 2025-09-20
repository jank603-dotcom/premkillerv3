#include <include/global.hpp>

bool memory_c::attach( std::uint32_t process_id )
{
    if ( this->process_handle )
    {
        CloseHandle( this->process_handle );
    }

    this->process_handle = OpenProcess( PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION, FALSE, process_id );
    if (!this->process_handle) {
        printf("Failed to attach to process %d. Error: %d\n", process_id, GetLastError());
        return false;
    }
    return true;
}

void memory_c::detach( )
{
    if ( this->process_handle )
    {
        CloseHandle( this->process_handle );
        this->process_handle = nullptr;
    }
}

std::uint32_t memory_c::get_process_id( const std::wstring& process_name )
{
    PROCESSENTRY32 pe{ sizeof( pe ) };

    auto snap = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
    if ( snap == INVALID_HANDLE_VALUE )
    {
        return 0;
    }

    if ( !Process32First( snap, &pe ) )
    {
        CloseHandle( snap );
        return 0;
    }

    do
    {
        if ( !std::wcscmp( pe.szExeFile, process_name.c_str( ) ) )
        {
            CloseHandle( snap );
            return pe.th32ProcessID;
        }
    } while ( Process32Next( snap, &pe ) );

    CloseHandle( snap );
    return 0;
}

std::uintptr_t memory_c::get_module_base( std::uint32_t process_id, const std::wstring& module_name )
{
    MODULEENTRY32 me{ sizeof( me ) };

    auto snap = CreateToolhelp32Snapshot( TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, process_id );
    if ( snap == INVALID_HANDLE_VALUE )
    {
        return 0;
    }

    if ( !Module32First( snap, &me ) )
    {
        CloseHandle( snap );
        return 0;
    }

    do
    {
        if ( !std::wcscmp( me.szModule, module_name.c_str( ) ) )
        {
            CloseHandle( snap );
            return reinterpret_cast< std::uintptr_t >( me.modBaseAddr );
        }
    } while ( Module32Next( snap, &me ) );

    CloseHandle( snap );
    return 0;
}

bool memory_c::is_process_valid( )
{
    if ( !this->process_handle ) return false;
    
    DWORD exit_code;
    if ( !GetExitCodeProcess( this->process_handle, &exit_code ) ) {
        return false; // Process handle invalid
    }
    
    if ( exit_code != STILL_ACTIVE ) {
        return false; // Process has exited
    }
    
    return true;
}

bool memory_c::read_process_memory( std::uintptr_t address, void* buffer, std::uintptr_t size )
{
    SIZE_T bytes_read = 0;
    return ReadProcessMemory( this->process_handle, reinterpret_cast< LPCVOID >( address ), buffer, size, &bytes_read ) && bytes_read == size;
}

bool memory_c::write_process_memory( std::uintptr_t address, const void* buffer, std::uintptr_t size )
{
    SIZE_T bytes_written = 0;
    return WriteProcessMemory( this->process_handle, reinterpret_cast< LPVOID >( address ), buffer, size, &bytes_written ) && bytes_written == size;
}

void memory_c::inject_mouse( int x, int y, std::uint8_t button_flags )
{
    // Optionally move the mouse if non-zero delta provided
    if ( x != 0 || y != 0 )
    {
        INPUT move = { 0 };
        move.type = INPUT_MOUSE;
        move.mi.dx = x;
        move.mi.dy = y;
        move.mi.dwFlags = MOUSEEVENTF_MOVE;
        SendInput( 1, &move, sizeof( INPUT ) );
    }

    // Bit 0 = LEFTDOWN, Bit 1 = LEFTUP
    const bool want_down = (button_flags & 1) != 0;
    const bool want_up   = (button_flags & 2) != 0;
    if ( want_down )
    {
        INPUT down = { 0 };
        down.type = INPUT_MOUSE;
        down.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
        SendInput( 1, &down, sizeof( INPUT ) );
    }

    if ( want_down && want_up )
    {
        // Short pause improves reliability in some games
        Sleep( 5 );
    }

    if ( want_up )
    {
        INPUT up = { 0 };
        up.type = INPUT_MOUSE;
        up.mi.dwFlags = MOUSEEVENTF_LEFTUP;
        SendInput( 1, &up, sizeof( INPUT ) );
    }
}