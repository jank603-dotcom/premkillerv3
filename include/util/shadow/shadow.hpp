#ifndef SHADOW_HPP
#define SHADOW_HPP

class shadow
{
public:
	using syscall_fn = NTSTATUS( __stdcall* )( ... );

	explicit shadow( HMODULE module, const std::string& name );
	~shadow( );

	bool valid( ) const;
	std::uint32_t get_syscall_number( ) const { return this->syscall_number; };

	template<typename... arg>
	NTSTATUS call( arg... args ) const
	{
		using fn = NTSTATUS( __stdcall* )( arg... );
		fn f = reinterpret_cast< fn >( this->stub );
		return f( args... );
	}

private:
	std::uint32_t syscall_number;
	void* stub;

	void resolve( HMODULE module, const std::string& name );
};

#endif // SHADOW_HPP