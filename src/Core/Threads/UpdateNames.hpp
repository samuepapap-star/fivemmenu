#include <Includes/Includes.hpp>
#include <Includes/Utils.hpp>
#include <Core/Offsets.hpp>
#include <Core/Core.hpp>


#include <fstream>
#include <string>
#include <regex>
#include <iostream>

#define CURL_STATIC_LIB
#include <Security/Api/curl/curl.h>
#pragma comment( lib, "ws2_32.lib" )
#pragma comment( lib, "Normaliz.lib" )
#pragma comment( lib, "Crypt32.lib" )
#pragma comment( lib, "Wldap32.lib" )
#pragma comment( lib, "libcurl.lib" )


using json = nlohmann::json;

namespace Core
{
	namespace Threads
	{
		class cUpdateNames 
		{
		private:
			std::string ServerIp;
			std::string ServerToken;
			std::string DirFiveM;
			std::string RedirectUrl;
		public:
			std::unordered_map<int, Core::SDK::Game::NetworkInfo> NetworkMap;
		private:
			static size_t WriteCallBack( void * contents, size_t size, size_t nmemb, void * userp )
			{
				( ( std::string * ) userp )->append( ( char * ) contents, size * nmemb );
				return size * nmemb;
			}

			std::string ExtractIp( const std::string & line )
			{
				std::string ip;

				size_t fist_serv = line.rfind( xorstr( "last_server_url" ) );
				if ( fist_serv != std::string::npos )
				{
					size_t start = line.find( xorstr( "last_server" ), fist_serv + 16 - 1 );
					if ( start != std::string::npos )
					{
						size_t last_serv = line.find( xorstr( ":" ), start );
						if ( last_serv != std::string::npos )
						{
							size_t ip_start = start + 12 - 1;
							size_t ip_end = line.find( xorstr( ":" ), ip_start ) + 6;
							if ( ip_end != std::string::npos && ip_end > ip_start )
							{
								ip = line.substr( ip_start, ip_end - ip_start );
							}
						}
					}
				}

				return ip;
			}

			std::string GetServerToken( ) {
                ServerIp.clear();
                RedirectUrl.clear();

				if ( DirFiveM.empty( ) )
				{
					char value[ 8192 ] = {};
					DWORD BufferSize = sizeof(value);

					auto GetDirFiveM = RegGetValueA( HKEY_CURRENT_USER, xorstr( "Software\\CitizenFX\\FiveM" ), xorstr( "Last Run Location" ), RRF_RT_REG_SZ, NULL, ( PVOID ) &value, &BufferSize );

					if ( GetDirFiveM != ERROR_SUCCESS ) 
						return xorstr( "" );

					DirFiveM = ( std::string ) value;
				}


				if (!DirFiveM.empty() && DirFiveM.back() != '\\' && DirFiveM.back() != '/')
                    DirFiveM += '\\';
                std::string CrashoMetryDir = DirFiveM + xorstr( "data\\cache\\crashometry" );
				std::ifstream File( CrashoMetryDir, std::ios::binary );

				if ( !File )
					return xorstr( "" );

				std::string line;
				while ( std::getline( File, line ) ) {
					size_t LastServer = line.find( xorstr( "last_server" ) );

					if ( LastServer != std::string::npos )
					{
						ServerIp = this->ExtractIp( line );
						break;
					}
				}

				File.close( );

				if ( ServerIp.empty( ) ) {
					return xorstr( "" );
				}

				g_Variables.ServerIp = ServerIp;

				std::string ResponseStr;

				std::string ReqUrl = xorstr( "http://" ) + ServerIp;

				CURL * hnd;
				CURLcode res = CURLE_FAILED_INIT;
                long HttpStatus = 0;
				hnd = curl_easy_init( );
				if ( hnd ) {
					curl_easy_setopt( hnd, CURLOPT_CUSTOMREQUEST, xorstr( "GET" ) );
					curl_easy_setopt( hnd, CURLOPT_URL, ReqUrl.c_str( ) );

					struct curl_slist * headers = NULL;
					curl_easy_setopt( hnd, CURLOPT_HTTPHEADER, headers );

					curl_easy_setopt( hnd, CURLOPT_WRITEFUNCTION, WriteCallBack );
					curl_easy_setopt( hnd, CURLOPT_WRITEDATA, &ResponseStr );

					curl_easy_setopt( hnd, CURLOPT_FOLLOWLOCATION, 1L );
					curl_easy_setopt( hnd, CURLOPT_MAXREDIRS, 10L );

					curl_easy_setopt(hnd, CURLOPT_CONNECTTIMEOUT, 5L);
                    curl_easy_setopt(hnd, CURLOPT_TIMEOUT, 10L);
                    res = curl_easy_perform( hnd );
                    curl_easy_getinfo(hnd, CURLINFO_RESPONSE_CODE, &HttpStatus);

					if ( res == CURLE_OK )
					{
						char * FinalUrl = nullptr;
						curl_easy_getinfo( hnd, CURLINFO_EFFECTIVE_URL, &FinalUrl );

						if ( FinalUrl ) {
							RedirectUrl = std::string( FinalUrl );
						}
					}

					curl_easy_cleanup( hnd );
				}

				if (res != CURLE_OK || HttpStatus < 200 || HttpStatus >= 300 || RedirectUrl.empty()) {
					return xorstr("");
				}

				auto pos = RedirectUrl.find_last_of( '/' );
				if ( pos == std::string::npos )
					return xorstr( "" );

				// Only a cfx.re join redirect can supply a server token.
                if (RedirectUrl.rfind("https://cfx.re/join/", 0) != 0 &&
                    RedirectUrl.rfind("http://cfx.re/join/", 0) != 0)
                    return "";
                std::string Token = RedirectUrl.substr( pos + 1 );
                if (Token.empty() || Token.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789") != std::string::npos)
                    return "";

				return Token;
			}
		public:

			nlohmann::json GetPlayerData( ) {
				ServerToken = GetServerToken( );

				if ( ServerToken.empty() )
					return NULL;

				std::string ApiUrl = xorstr("https://servers-frontend.fivem.net/api/servers/single/") + ServerToken;

				std::string ResponseStr;
				CURL * hnd;
				CURLcode res = CURLE_FAILED_INIT;
                long HttpStatus = 0;
				hnd = curl_easy_init( );
				if ( hnd ) {
					curl_easy_setopt( hnd, CURLOPT_CUSTOMREQUEST, xorstr("GET") );
					curl_easy_setopt( hnd, CURLOPT_URL, ApiUrl.c_str( ) );

					struct curl_slist * headers = NULL;
					headers = curl_slist_append( headers, xorstr("User-Agent: WexizeV2") );
					curl_easy_setopt( hnd, CURLOPT_HTTPHEADER, headers );

					curl_easy_setopt( hnd, CURLOPT_WRITEFUNCTION, WriteCallBack );
					curl_easy_setopt( hnd, CURLOPT_WRITEDATA, &ResponseStr );

					curl_easy_setopt(hnd, CURLOPT_CONNECTTIMEOUT, 5L);
                    curl_easy_setopt(hnd, CURLOPT_TIMEOUT, 10L);
                    res = curl_easy_perform( hnd );
                    curl_easy_getinfo(hnd, CURLINFO_RESPONSE_CODE, &HttpStatus);

					curl_slist_free_all(headers);
                    curl_easy_cleanup( hnd );
				}

				if (res != CURLE_OK || HttpStatus < 200 || HttpStatus >= 300 || ResponseStr.empty()) {
                    const std::string diagnostic = "[Wexize] Player lookup skipped: curl=" +
                        std::to_string(static_cast<int>(res)) + ", HTTP=" + std::to_string(HttpStatus);
                    OutputDebugStringA(diagnostic.c_str());
                    return nullptr;
                }

				nlohmann::json ResponseJson = json::parse(ResponseStr, nullptr, false);
                if (ResponseJson.is_discarded() || !ResponseJson.is_object() ||
                    !ResponseJson.contains("Data") || !ResponseJson["Data"].is_object() ||
                    !ResponseJson["Data"].contains("players") || !ResponseJson["Data"]["players"].is_array()) {
                    OutputDebugStringA("[Wexize] Player lookup skipped: invalid JSON/schema.\n");
                    return nullptr;
                }
				nlohmann::json ServerData = ResponseJson[ xorstr( "Data" ) ];
				nlohmann::json PlayersArray = ServerData[ xorstr( "players" ) ];

				return PlayersArray;
			}

			void GetPlayerNames( )
			{
				nlohmann::json PlayersArr = GetPlayerData( );

				if ( !PlayersArr.is_array() )
					return;

				for ( const auto & PlayerJson : PlayersArr )
				{
					const auto & Player = PlayerJson;
                    if (!Player.is_object() || !Player.contains("id") || !Player["id"].is_number_integer() ||
                        !Player.contains("name") || !Player["name"].is_string())
                        continue;
					int PlayerId = Player[ xorstr( "id" ) ].get<int>();
					std::string PlayerName = Player[ xorstr( "name" ) ].get<std::string>();
					nlohmann::json Identifiers = Player.value("identifiers", json());

					std::string Discord, SteamId;

					if ( Identifiers.is_array() )
					{
						for ( const auto & Identifier : Identifiers ) 
						{

							if ( !Identifier.is_string( ) )
								continue;

							std::string IdentifierVal = Identifier;

							if ( IdentifierVal.rfind( "discord:", 0 ) == 0 )
								Discord = IdentifierVal.substr( 8 );
							if ( IdentifierVal.rfind( "steam:", 0 ) == 0 )
								SteamId = IdentifierVal.substr( 6 );

						}
					}

					NetworkMap[ PlayerId ] =
					{ 
						PlayerName, Discord, SteamId 
					};

				}
			}

			void Update( ) 
			{
				while ( true ) 
				{
					std::this_thread::sleep_for( std::chrono::milliseconds( 6000 ) );
					try {
						//if ( !g_MenuInfo.IsLogged && !g_Variables.g_bPassedByThisVerify )
							//continue;

						GetPlayerNames( );
					}
                    catch (const std::exception & e) {
                        OutputDebugStringA("[Wexize] Player lookup failed: ");
                        OutputDebugStringA(e.what());
                    }
                    catch (...) {
                        OutputDebugStringA("[Wexize] Player lookup failed: unknown exception.");
                    }

				}
			}
		};

		inline cUpdateNames g_UpdateNames;
	}

}
