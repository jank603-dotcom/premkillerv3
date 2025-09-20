#ifndef UTIL_HPP
#define UTIL_HPP

class util_c
{
public:
	class console_c
	{
	public:
		bool initialize( );
		void print( const char* text, ... );
		void space( );
		void sleep_ms( int milliseconds );
		void wait_for_input( );
	private:
		void* handle;
		void* input_handle;
	};

	console_c console;
};

#endif // !UTIL_HPP
