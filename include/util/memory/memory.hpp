#ifndef MEMORY_HPP
#define MEMORY_HPP

// JUST A PLACEHOLDER

class memory_c
{
public:
    bool attach( std::uint32_t process_id );
    void detach( );

    std::uint32_t get_process_id( const std::wstring& process_name );
    std::uintptr_t get_module_base( std::uint32_t process_id, const std::wstring& module_name );
    bool is_process_valid( ); // Check if current process connection is still valid


    bool read_process_memory( std::uintptr_t address, void* buffer, std::uintptr_t size );
    bool write_process_memory( std::uintptr_t address, const void* buffer, std::uintptr_t size );

    void inject_mouse( int x, int y, std::uint8_t button_flags = 0 );

public:
    template <typename t>
    t read( std::uintptr_t address )
    {
        t response{};
        this->read_process_memory( address, &response, sizeof( t ) );
        return response;
    }

    template <typename t>
    t read_safe( std::uintptr_t address, t default_value = t{} )
    {
        if (address < 0x10000) return default_value; // Invalid address
        
        t response{};
        if (this->read_process_memory( address, &response, sizeof( t ) )) {
            return response;
        }
        return default_value;
    }

    template <typename t>
    std::vector<t> read_array( std::uintptr_t address, std::size_t count )
    {
        std::vector<t> result( count );
        if ( this->read_process_memory( address, result.data( ), count * sizeof( t ) ) )
        {
            return result;
        }

        return {};
    }

    template <typename t>
    bool write( std::uintptr_t address, t value )
    {
        return this->write_process_memory( address, &value, sizeof( t ) );
    }

    template <typename t, std::size_t N>
    bool write_array( std::uintptr_t address, const std::array<t, N>& data )
    {
        return this->write_process_memory( address, data.data( ), data.size( ) * sizeof( t ) );
    }

private:
    void* process_handle;
};

#endif // !MEMORY_HPP