#include <include/global.hpp>

#include <wininet.h>
#include <sstream>

bool skin_db_c::load_all( )
{
    g->util.console.print( ecrypt( "loading skin database..." ) );

    const std::string url = ecrypt( "https://raw.githubusercontent.com/qwkdev/csapi/main/data.json" );
    std::string json_text;

    if ( !this->fetch_url_text( url, json_text ) )
    {
        g->util.console.print( ecrypt( "failed to load skin database." ) );
        return false;
    }

    try {
        auto j = nlohmann::json::parse( json_text );

        for ( auto& [key, value] : j.items( ) )
        {
            if ( !value.is_object( ) )
            {
                continue;
            }

            int paint_kit = 0;

            if ( value.contains( "finish-catalog" ) )
            {
                const auto& finish_catalog = value[ "finish-catalog" ];
                if ( finish_catalog.is_number_integer( ) ) 
                {
                    paint_kit = finish_catalog.get<int>( );
                }
            }

            if ( paint_kit > 0 )
            {
                const std::string compound = value.value( "weapon", "" ) + " | " + value.value( "finish", "" );
                this->skins[ key ] = paint_kit;
                this->skins[ compound ] = paint_kit;
            }
        }
    }
    catch ( const std::exception& )
    {
        return false;
    }

    g->util.console.print( ecrypt( "loaded %zu skins total" ), this->skins.size( ) );
    g->util.console.space( );
    return true;
}

int skin_db_c::get( const std::string& skin_name ) const
{
	if ( auto it = skins.find( skin_name ); it != skins.end( ) )
	{
		return it->second;
	}

	return 0;
}    

const ankerl::unordered_dense::map<std::string, int>& skin_db_c::get_all_skins( ) const
{
    return this->skins;
}

bool skin_db_c::fetch_url_text( const std::string& url, std::string& out_text )
{
	const auto h_session = call_function( &InternetOpenA, ecrypt( "skin_db" ), INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0 );
	if ( !h_session )
	{
		return false;
	}

	const auto h_file = call_function( &InternetOpenUrlA, h_session, url.c_str( ), nullptr, 0, INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_SECURE, 0 );
	if ( !h_file )
	{
		call_function( &InternetCloseHandle, h_session );
		return false;
	}

	char buffer[ 4096 ]{ };
	DWORD bytes_read = 0;
	out_text.clear( );

	while ( call_function( &InternetReadFile, h_file, buffer, sizeof( buffer ), &bytes_read ) && bytes_read )
	{
		out_text.append( buffer, bytes_read );
	}

	call_function( &InternetCloseHandle, h_file );
	call_function( &InternetCloseHandle, h_session );

	return true;
}