
#include "../header/factory.hpp"
// Local Includes
#include "../header/code_definitions.hpp"
#include "../header/game_data.hpp"
#include "../header/logger.hpp"
#include "../header/npc.hpp"
#include "../header/platform.hpp"
#include "../header/player.hpp"
#include "../header/room.hpp"

namespace Factory {
	static UInt64 g_id_next( 0 );
	const UInt64 get_next_id() {
		++g_id_next;
		return g_id_next;
	}

	void npc_create( const std::string & p_file_name ) {
		const std::string & l_file_path( "res//entities//actors//" + p_file_name );
		std::ifstream l_file( l_file_path );
		
		if( !l_file.is_open() ) {
			log_error << "Failed to open file: " << l_file_path;
			return;
		}

		log_info << "Creating actor from file: " << l_file_path;

		std::string l_name( "" );
		Int64 l_health( 0 );
		Int64 l_damage( 0 );
		Int64 l_gold( 0 );
		std::vector <std::string> l_tags;

		while( l_file.good() ) {
			std::string l_file_line;
			std::getline( l_file, l_file_line );

			if( l_file_line.substr( 0, 6 ) == "[NAME]" ) {
				l_name = l_file_line.substr( 6, l_file_line.back() );
			}
			else if( l_file_line.substr( 0, 8 ) == "[HEALTH]" ) {
				l_health = stoi( l_file_line.substr( 8, l_file_line.back() ) );
			}
			else if( l_file_line.substr( 0, 8 ) == "[DAMAGE]" ) {
				l_damage = stoi( l_file_line.substr( 8, l_file_line.back() ) );
			}
			else if( l_file_line.substr( 0, 6 ) == "[GOLD]" ) {
				l_gold = stoi( l_file_line.substr( 6, l_file_line.back() ) );
			}
			else if( l_file_line.substr( 0, 11 ) == "[TAG_START]" ) {
				while( l_file.good() &&
							 l_file_line != "[TAG_END]" ) {
					std::getline( l_file, l_file_line );
					l_tags.push_back( l_file_line );
				}
			}
		}

		actors.push_back( new NPC( get_next_id(), l_name, l_health, l_damage, l_gold, l_tags ) );
	}

	void npc_destroy( UInt64 p_npc_id ) {
		size_t l_actor_count( actors.size() );
		for( size_t l_actor_iter( 0 ); l_actor_iter < l_actor_count; ++l_actor_iter ) {
			if( ( actors[ l_actor_iter ]->get_id() == p_npc_id ) &&
					( dynamic_cast<NPC *>( actors[ l_actor_iter ] ) ) ) {
				log_info << "Deleting NPC named: " << actors[ l_actor_iter ]->get_name();

				delete actors[ l_actor_iter ];
				actors.erase( actors.begin() + l_actor_iter );
				return;
			}
		}
	}

	void player_create( const std::string & p_name )
	{
		player = new Player( get_next_id(), p_name, 25, 5, 0 );
		actors.push_back( player );
	}

	void player_destroy()
	{
		size_t l_actor_size( actors.size() );
		for( size_t l_actor_iter( 0 ); l_actor_iter < l_actor_size; ++l_actor_iter ) {
			if( dynamic_cast<Player *>( actors[ l_actor_iter ] ) ) {
				actors.erase( actors.begin() + l_actor_iter );
			}
		}

		if( player != nullptr ) {
			log_info << "Deleting Player named: " << player->get_name();

			delete player;
			player = nullptr;
		}
	}

	void room_change()
	{
		if( room_next != nullptr ) {
			room_destroy( room );

			room = room_next;
			room_next = nullptr;
		}
	}

	Room * room_create( const std::string & p_file_name ) {
		std::string l_filepath_separator( get_filepath_separator() );
		std::string l_file_path( "res" + l_filepath_separator + "rooms" + l_filepath_separator + p_file_name );

		log_progress_detailed << "Attempting to open file: " << l_file_path;
		std::ifstream l_file( l_file_path );
		if( !l_file.is_open() ) {
			log_error << "Failed to open file: " << l_file_path;
			return nullptr;
		}

		log_progress_detailed << "Succeeding in opening file";

		std::vector <std::string> l_file_lines;
		while( l_file.good() ) {
			std::string l_line;
			std::getline( l_file, l_line );

			if( !l_line.empty() ) {
				l_file_lines.push_back( l_line );
			}
		}

		std::string l_room_name( "" );
		std::string l_room_type_string( "" );
		RoomList l_room_type( RoomList::null );
		std::vector <ExitList> l_room_exits;

		size_t l_line_count( l_file_lines.size() );
		for( UInt8 l_line_iter( 0 ); l_line_iter < l_line_count; ++l_line_iter ) {
			std::string l_file_line = l_file_lines[ l_line_iter ];

			// Get name of room
			if( l_file_line.substr( 0, 6 ) == "[NAME]" ) {
				l_room_name = l_file_line.substr( 6, l_file_line.back() );
			}
			// Get type of room
			else if( l_file_line.substr( 0, 6 ) == "[TYPE]" ) {
				l_room_type_string = l_file_line.substr( 6, l_file_line.size() );
				std::map <std::string, RoomList>::iterator l_roomlist_iter( Room::g_roomlist_map.find( l_file_line.substr( 6, l_file_line.back() ) ) );
				if( l_roomlist_iter != Room::g_roomlist_map.end() ) {
					l_room_type = l_roomlist_iter->second;
				} else {
					log_error << "Invalid room type";
					return nullptr;
				}
			}
			// Get exits from room
			else if( l_file_line.substr( 0, 13 ) == "[EXITS_BEGIN]" ) {
				++l_line_iter;
				l_file_line = l_file_lines[ l_line_iter ];

				while( ( l_file_line.substr( 0, 11 ) != "[EXITS_END]" ) &&
							 ( l_line_iter <= l_line_count ) ) {
					if( l_file_line.substr( 0, 7 ) == "[NORTH]" ) {
						l_room_exits.push_back( ExitList( Vocab::NounList::north, "north", l_file_line.substr( 7, l_file_line.back() ) ) );
					}
					else if( l_file_line.substr( 0, 7 ) == "[SOUTH]" ) {
						l_room_exits.push_back( ExitList( Vocab::NounList::south, "south", l_file_line.substr( 7, l_file_line.back() ) ) );
					}
					else if( l_file_line.substr( 0, 6 ) == "[EAST]" ) {
						l_room_exits.push_back( ExitList( Vocab::NounList::east, "east", l_file_line.substr( 6, l_file_line.back() ) ) );
					}
					else if( l_file_line.substr( 0, 6 ) == "[WEST]" ) {
						l_room_exits.push_back( ExitList( Vocab::NounList::west, "west", l_file_line.substr( 6, l_file_line.back() ) ) );
					}

					++l_line_iter;
					l_file_line = l_file_lines[ l_line_iter ];
				}
			}
		}

		return new Room( p_file_name, l_room_type, l_room_name, l_room_exits );
	}

	void room_destroy( Room * p_room ) {
		if( p_room != nullptr ) {
			delete p_room;
		}
	}
}