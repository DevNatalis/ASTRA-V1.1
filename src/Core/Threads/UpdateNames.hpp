#include <Includes/Includes.hpp>
#include <Includes/Utils.hpp>
#include <Core/Offsets.hpp>
#include <Core/Core.hpp>


#include <fstream>
#include <string>
#include <regex>
#include <iostream>
#include <shared_mutex>
#include <unordered_map>
#include <chrono>

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
		mutable std::shared_mutex NetworkMapMutex;

		// Leitura thread-safe para a EntityList (ela roda a cada 1ms).
		bool GetById(int id, Core::SDK::Game::NetworkInfo& out) const
		{
			std::shared_lock<std::shared_mutex> lock(NetworkMapMutex);
			auto it = NetworkMap.find(id);
			if (it == NetworkMap.end())
				return false;
			out = it->second;
			return true;
		}
		private:
			static size_t WriteCallBack( void * contents, size_t size, size_t nmemb, void * userp )
			{
				constexpr size_t limit = 2 * 1024 * 1024;
				if (!userp || !contents || !size || nmemb > limit / size) return 0;
				const size_t bytes = size * nmemb;
				auto& body = *static_cast<std::string*>(userp);
				if (body.size() > limit || bytes > limit - body.size()) return 0;
				try { body.append(static_cast<char*>(contents), bytes); }
				catch (...) { return 0; }
				return bytes;
			}

			static bool ConfigureRequest(CURL* hnd, std::string& body)
			{
				return curl_easy_setopt(hnd, CURLOPT_WRITEFUNCTION, WriteCallBack) == CURLE_OK &&
					curl_easy_setopt(hnd, CURLOPT_WRITEDATA, &body) == CURLE_OK &&
					curl_easy_setopt(hnd, CURLOPT_TIMEOUT, 10L) == CURLE_OK &&
					curl_easy_setopt(hnd, CURLOPT_CONNECTTIMEOUT, 3L) == CURLE_OK &&
					curl_easy_setopt(hnd, CURLOPT_NOSIGNAL, 1L) == CURLE_OK &&
					curl_easy_setopt(hnd, CURLOPT_PROTOCOLS_STR, "https") == CURLE_OK &&
					curl_easy_setopt(hnd, CURLOPT_REDIR_PROTOCOLS_STR, "https") == CURLE_OK &&
					curl_easy_setopt(hnd, CURLOPT_FOLLOWLOCATION, 0L) == CURLE_OK &&
					curl_easy_setopt(hnd, CURLOPT_MAXREDIRS, 3L) == CURLE_OK &&
					curl_easy_setopt(hnd, CURLOPT_SSL_VERIFYPEER, 1L) == CURLE_OK &&
					curl_easy_setopt(hnd, CURLOPT_SSL_VERIFYHOST, 2L) == CURLE_OK;
			}

			static bool IsDiscoveryDestination(const std::string& url)
			{
				return url.rfind("https://cfx.re/join/", 0) == 0 ||
					url.rfind("https://servers.fivem.net/servers/detail/", 0) == 0;
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
				RedirectUrl.clear();
				ServerIp.clear();

				if ( DirFiveM.empty( ) )
				{
					char value[ 255 ] = {};
					DWORD BufferSize = sizeof(value);

					auto GetDirFiveM = RegGetValueA( HKEY_CURRENT_USER, xorstr( "Software\\CitizenFX\\FiveM" ), xorstr( "Last Run Location" ), RRF_RT_REG_SZ, NULL, value, &BufferSize );

					if ( GetDirFiveM != ERROR_SUCCESS ) 
						return xorstr( "" );

					DirFiveM = ( std::string ) value;
				}


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

				// Discovery requires valid TLS. HTTP-only servers are not queried.
				std::string ReqUrl = xorstr( "https://" ) + ServerIp;

				CURL * hnd;
				CURLcode res = CURLE_FAILED_INIT;
				hnd = curl_easy_init( );
				if ( hnd ) {
					if (!ConfigureRequest(hnd, ResponseStr)) { curl_easy_cleanup(hnd); return {}; }
					const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
					// Inspect every redirect before connecting, including its host.
					for (int redirects = 0; redirects <= 3; ++redirects) {
						const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
							deadline - std::chrono::steady_clock::now()).count();
						if (remaining <= 0) { res = CURLE_OPERATION_TIMEDOUT; break; }
						if (curl_easy_setopt(hnd, CURLOPT_URL, ReqUrl.c_str()) != CURLE_OK ||
							curl_easy_setopt(hnd, CURLOPT_TIMEOUT_MS, static_cast<long>(remaining)) != CURLE_OK) break;
						ResponseStr.clear();
						res = curl_easy_perform(hnd);
						if (res != CURLE_OK) break;
						long status = 0;
						if (curl_easy_getinfo(hnd, CURLINFO_RESPONSE_CODE, &status) != CURLE_OK) break;
						if (status >= 200 && status < 300) {
							RedirectUrl = ReqUrl;
							break;
						}
						char* nextUrl = nullptr;
						if (status < 300 || status >= 400 || redirects == 3 ||
							curl_easy_getinfo(hnd, CURLINFO_REDIRECT_URL, &nextUrl) != CURLE_OK ||
							!nextUrl || !IsDiscoveryDestination(nextUrl)) break;
						ReqUrl = nextUrl;
					}

					curl_easy_cleanup( hnd );
				}

				if (res != CURLE_OK || ResponseStr.empty()) {
					return xorstr("");
				}

				// Accept only the public join-link destinations used for discovery.
				if (!IsDiscoveryDestination(RedirectUrl)) return {};
				auto pos = RedirectUrl.find_last_of( '/' );
				if ( pos == std::string::npos )
					return xorstr( "" );

				std::string Token = RedirectUrl.substr( pos + 1 );
				if (Token.empty() || Token.size() > 64 ||
					Token.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789") != std::string::npos) return {};

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
				hnd = curl_easy_init( );
				if ( hnd ) {
					if (!ConfigureRequest(hnd, ResponseStr)) { curl_easy_cleanup(hnd); return nullptr; }
					curl_easy_setopt( hnd, CURLOPT_CUSTOMREQUEST, xorstr("GET") );
					curl_easy_setopt( hnd, CURLOPT_URL, ApiUrl.c_str( ) );

					struct curl_slist * headers = NULL;
					headers = curl_slist_append( headers, xorstr("User-Agent: Nome do teu cheatV2") );
					curl_easy_setopt( hnd, CURLOPT_HTTPHEADER, headers );

					curl_easy_setopt( hnd, CURLOPT_WRITEFUNCTION, WriteCallBack );
					curl_easy_setopt( hnd, CURLOPT_WRITEDATA, &ResponseStr );

					res = curl_easy_perform( hnd );
					long status = 0;
					curl_easy_getinfo(hnd, CURLINFO_RESPONSE_CODE, &status);
					curl_slist_free_all(headers);
					curl_easy_cleanup( hnd );
					if (status != 200) return nullptr;
				}

				if (res != CURLE_OK || ResponseStr.empty())
					return NULL;

				nlohmann::json ResponseJson;
				try { ResponseJson = json::parse( ResponseStr ); }
				catch ( ... ) { return NULL; }
				if ( !ResponseJson.is_object( ) || !ResponseJson.contains( xorstr( "Data" ) ) )
					return NULL;
				nlohmann::json ServerData = ResponseJson[ xorstr( "Data" ) ];
				if ( !ServerData.is_object( ) || !ServerData.contains( xorstr( "players" ) ) )
					return NULL;
				nlohmann::json PlayersArray = ServerData[ xorstr( "players" ) ];
				if ( !PlayersArray.is_array( ) )
					return NULL;

				return PlayersArray;
			}

			void GetPlayerNames( )
			{
				nlohmann::json PlayersArr = GetPlayerData( );

				if ( PlayersArr == NULL )
					return;

				std::unordered_map<int, Core::SDK::Game::NetworkInfo> fresh;
				for ( const auto & PlayerJson : PlayersArr )
				{
					// Um jogador malformado nao pode derrubar o refresh inteiro.
					try {
						if ( !PlayerJson.is_object( ) )
							continue;
						if ( !PlayerJson.contains( xorstr( "id" ) ) || !PlayerJson.contains( xorstr( "name" ) ) )
							continue;
						if ( !PlayerJson[ xorstr( "id" ) ].is_number_integer( ) || !PlayerJson[ xorstr( "name" ) ].is_string( ) )
							continue;
						int PlayerId = PlayerJson[ xorstr( "id" ) ].get<int>( );
						std::string PlayerName = PlayerJson[ xorstr( "name" ) ].get<std::string>( );
						if ( PlayerId < 0 || PlayerName.empty( ) )
							continue;

						std::string Discord, SteamId;

						auto IdIt = PlayerJson.find( xorstr( "identifiers" ) );
						if ( IdIt != PlayerJson.end( ) && IdIt->is_array( ) )
						{
							for ( const auto & Identifier : *IdIt )
							{

								if ( !Identifier.is_string( ) )
									continue;

								std::string IdentifierVal = Identifier.get<std::string>( );

								if ( IdentifierVal.rfind( "discord:", 0 ) == 0 )
									Discord = IdentifierVal.substr( 8 );
								if ( IdentifierVal.rfind( "steam:", 0 ) == 0 )
									SteamId = IdentifierVal.substr( 6 );

							}
						}

						fresh[ PlayerId ] =
						{
							PlayerName, Discord, SteamId
						};
					}
					catch ( ... ) {
						continue;
					}

				}

				// Troca atomica: leitores (EntityList) nunca veem mapa pela metade.
				std::unique_lock<std::shared_mutex> lock(NetworkMapMutex);
				NetworkMap.swap(fresh);
			}

			void Update( ) 
			{
				while ( true ) 
				{
					std::this_thread::sleep_for( std::chrono::milliseconds( 6000 ) );
					try {

						GetPlayerNames( );
					}
					catch ( const std::exception & e ) {
						// Falha de rede/parse esperada (servidor fora do ar, resposta nao-JSON).
						// So registra e tenta de novo em 6s — nunca popup, nunca mata a thread.
						OutputDebugStringA( ( std::string( "[UpdateNames] refresh falhou: " ) + e.what( ) + "\n" ).c_str( ) );
					}
					catch ( ... ) {
						OutputDebugStringA( "[UpdateNames] refresh falhou (excecao desconhecida).\n" );
					}

				}
			}
		};

		inline cUpdateNames g_UpdateNames;
	}

}
