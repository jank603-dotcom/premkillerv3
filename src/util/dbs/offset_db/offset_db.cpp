#include <include/global.hpp>

#include <wininet.h>
#include <regex>
#include <sstream>
#include <fstream>
#include <filesystem>

bool offset_db_c::load_all( )
{
	g->util.console.print( ecrypt( "loading offset database..." ) );

	this->tree.clear( );
	this->flat_map.clear( );

	const std::vector<std::pair<std::string, std::string>> files = {
		{ ecrypt( "offsets.hpp" ), ecrypt( "https://raw.githubusercontent.com/a2x/cs2-dumper/main/output/offsets.hpp" ) },
		{ ecrypt( "client_dll.hpp" ), ecrypt( "https://raw.githubusercontent.com/a2x/cs2-dumper/main/output/client_dll.hpp" ) },
		{ ecrypt( "engine2_dll.hpp" ), ecrypt( "https://raw.githubusercontent.com/a2x/cs2-dumper/main/output/engine2_dll.hpp" ) },
		{ ecrypt( "inputsystem_dll.hpp" ), ecrypt( "https://raw.githubusercontent.com/a2x/cs2-dumper/main/output/inputsystem_dll.hpp" ) },
		{ ecrypt( "matchmaking_dll.hpp" ), ecrypt( "https://raw.githubusercontent.com/a2x/cs2-dumper/main/output/matchmaking_dll.hpp" ) },
		{ ecrypt( "soundsystem_dll.hpp" ), ecrypt( "https://raw.githubusercontent.com/a2x/cs2-dumper/main/output/soundsystem_dll.hpp" ) }
	};

	bool success = true;
	for ( const auto& [filename, url] : files )
	{
		success &= this->load_file_or_url( filename, url );
	}

	if ( !success )
	{
		g->util.console.print( ecrypt( "failed to load offset database." ) );
		return false;
	}

	g->util.console.print( ecrypt( "loaded %zu offsets total" ), this->flat_map.size( ) );
	g->util.console.space( );
	return true;
}

std::uintptr_t offset_db_c::get( const std::string& klass, const std::string& field ) const
{
	for ( const auto& [module, classes] : this->tree )
	{
		if ( auto cit = classes.find( klass ); cit != classes.end( ) )
		{
			if ( auto fit = cit->second.find( field ); fit != cit->second.end( ) )
			{
				return fit->second.value;
			}
		}
	}

	return this->get_flat( field );
}

std::uintptr_t offset_db_c::get_flat( const std::string& field ) const
{
	if ( auto it = this->flat_map.find( field ); it != this->flat_map.end( ) )
	{
		return it->second.value;
	}

	return 0;
}

bool offset_db_c::load_file_or_url( const std::string& filename, const std::string& url )
{
	std::string raw_text;

	if ( this->load_local_file( filename, raw_text ) )
	{
		//g->util.console.print( ecrypt( "loaded %s from local file" ), filename.c_str( ) );
	}
	else if ( this->fetch_url_text( url, raw_text ) )
	{
		//g->util.console.print( ecrypt( "loaded %s from web" ), filename.c_str( ) );
	}
	else
	{
		g->util.console.print( ecrypt( "failed to load %s from both local and web" ), filename.c_str( ) );
		return false;
	}

	return this->parse_content( raw_text );
}

bool offset_db_c::load_local_file( const std::string& filename, std::string& out_text )
{
	try
	{
		if ( !std::filesystem::exists( filename ) )
		{
			return false;
		}

		std::ifstream file( filename );
		if ( !file.is_open( ) )
		{
			return false;
		}

		std::ostringstream buffer;
		buffer << file.rdbuf( );
		out_text = buffer.str( );

		file.close( );
		return !out_text.empty( );
	}
	catch ( const std::exception& )
	{
		return false;
	}
}

bool offset_db_c::fetch_and_parse_url( const std::string& url )
{
	std::string raw_text;
	if ( !this->fetch_url_text( url, raw_text ) )
	{
		g->util.console.print( ecrypt( "failed to fetch %s" ), url.c_str( ) );
		return false;
	}

	return this->parse_content( raw_text );
}

bool offset_db_c::parse_content( const std::string& raw_text )
{
	std::regex ns_enter( R"(\s*namespace (\w+)\s*\{)" );
	std::regex offset_entry( R"(\s*constexpr std::ptrdiff_t (\w+) = (0x[0-9A-Fa-f]+);)" );
	std::regex ns_exit( R"(\s*\})" );

	std::smatch match;
	std::istringstream stream( raw_text );
	std::string line;
	std::vector<std::string> stack;

	while ( std::getline( stream, line ) )
	{
		if ( std::regex_search( line, match, ns_enter ) )
		{
			stack.push_back( match[ 1 ] );
			continue;
		}

		if ( std::regex_search( line, match, ns_exit ) )
		{
			if ( !stack.empty( ) )
			{
				stack.pop_back( );
			}

			continue;
		}

		if ( std::regex_search( line, match, offset_entry ) )
		{
			const std::string field = match[ 1 ];
			const std::string hex = match[ 2 ];
			const std::uintptr_t value = std::stoull( hex, nullptr, 16 );

			std::string mod, cls;

			if ( stack.size( ) >= 2 )
			{
				mod = stack[ 0 ];
				for ( size_t i = 1; i < stack.size( ) - 1; ++i )
					mod += "::" + stack[ i ];
				cls = stack.back( );
			}
			else if ( stack.size( ) == 1 )
			{
				mod = "";
				cls = stack[ 0 ];
			}
			else
			{
				mod = "";
				cls = "";
			}

			std::string full_path;
			if ( !mod.empty( ) && !cls.empty( ) )
				full_path = mod + "::" + cls + "::" + field;
			else if ( !mod.empty( ) )
				full_path = mod + "::" + field;
			else if ( !cls.empty( ) )
				full_path = cls + "::" + field;
			else
				full_path = field;

			offset_entry_t entry{
				.value = value,
				.full_path = full_path
			};

			this->tree[ mod ][ cls ][ field ] = entry;

			if ( !this->flat_map.contains( field ) )
			{
				this->flat_map[ field ] = entry;
			}
		}
	}

	return true;
}

bool offset_db_c::fetch_url_text( const std::string& url, std::string& out_text )
{
	const auto h_session = call_function( &InternetOpenA, ecrypt( "offset_db" ), INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0 );
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

	char buffer[ 4096 ]{};
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