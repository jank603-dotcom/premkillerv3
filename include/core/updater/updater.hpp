#ifndef UPDATER_HPP
#define UPDATER_HPP

#include <atomic>
#include <thread>
#include <vector>
#include <optional>
#include <span>

class updater_c
{
public:
	void deploy( );
	void shutdown( );

	std::vector<udata_c::player_entity_t> get_player_entities( ) const;
	std::vector<udata_c::misc_entity_t> get_misc_entities( ) const;

private:
	std::uintptr_t resolve_entity_handle( std::uint32_t handle, udata_c& udata ) const;
	std::uintptr_t get_entity_at_index( int index, udata_c& udata ) const;

	void update_roots( udata_c& udata );
	void update_owning_player_entity( udata_c& udata );

	void process_player_entities( udata_c& udata );
	void process_misc_entities( udata_c& udata );

	std::optional<std::uintptr_t> resolve_player_entity( std::uintptr_t entity );
	std::optional<std::string> resolve_misc_entity_type( std::uintptr_t entity, std::span<const char*> expected_classes );

	void cleanup( );

private:
	template<typename T>
	struct triple_buffer
	{
		std::vector<T> buffers[ 3 ];
		std::atomic<int> write_index{ 0 };
		std::atomic<int> read_index{ 1 };
		std::atomic<int> spare_index{ 2 };
		std::atomic<bool> new_data_available{ false };

		void swap_write_buffer( )
		{
			const auto expected_spare = this->spare_index.load( );
			const auto current_write = this->write_index.load( );
			this->spare_index.store( current_write );
			this->write_index.store( expected_spare );
			this->new_data_available.store( true );
		}

		void update_read_buffer( )
		{
			if ( this->new_data_available.exchange( false ) )
			{
				const auto current_read = this->read_index.load( );
				const auto current_spare = this->spare_index.load( );
				this->read_index.store( current_spare );
				this->spare_index.store( current_read );
			}
		}

		std::vector<T>& get_write_buffer( )
		{
			return this->buffers[ this->write_index.load( ) ];
		}

		const std::vector<T>& get_read_buffer( )
		{
			return this->buffers[ this->read_index.load( ) ];
		}
	};

private:
	std::string last_known_map;

	mutable triple_buffer<udata_c::player_entity_t> player_entity_buffer;
	mutable triple_buffer<udata_c::misc_entity_t> misc_entity_buffer;

    // Cooperative threading
    std::atomic<bool> stop_requested{ false };
    std::thread t_maintenance;
    std::thread t_local;
    std::thread t_players;
    std::thread t_misc;
};

#endif // !UPDATER_HPP