#include <include/global.hpp>

shadow::shadow( HMODULE module, const std::string& name ) : syscall_number( 0 ), stub( nullptr )
{
    this->resolve( module, name );
}

shadow::~shadow( )
{
    if ( this->stub )
    {
        call_function( &VirtualFree, this->stub, 0, MEM_RELEASE );
    }
}

bool shadow::valid( ) const
{
    return this->stub != nullptr && this->syscall_number != 0;
}

void shadow::resolve( HMODULE module, const std::string& name )
{
    auto proc = call_function( &GetProcAddress, module, name.c_str( ) );
    if ( !proc )
    {
        return;
    }

    auto bytes = reinterpret_cast< std::uint8_t* >( proc );

    bool found = false;
    for ( int i = 0; i < 24; ++i )
    {
        if ( bytes[ i ] == 0xB8 && i + 4 < 32 )
        {
            for ( int j = i + 5; j < i + 20 && j + 1 < 32; ++j )
            {
                if ( bytes[ j ] == 0x0F && bytes[ j + 1 ] == 0x05 )
                {
                    this->syscall_number = *reinterpret_cast< std::uint32_t* >( bytes + i + 1 );
                    found = true;
                    break;
                }
            }

            if ( found )
            {
                break;
            }
        }
    }

    if ( !found )
    {
        return;
    }

    std::uint8_t stub_bytes[ ] = {
        0x4C, 0x8B, 0xD1,         // mov r10, rcx
        0xB8, 0, 0, 0, 0,         // mov eax, syscall_number
        0x0F, 0x05,               // syscall
        0xC3                      // ret
    };

    *reinterpret_cast< std::uint32_t* >( stub_bytes + 4 ) = this->syscall_number;

    this->stub = call_function( &VirtualAlloc, nullptr, sizeof( stub_bytes ), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE );
    if ( this->stub )
    {
        std::memcpy( this->stub, stub_bytes, sizeof( stub_bytes ) );
    }
}