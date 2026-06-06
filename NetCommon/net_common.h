#pragma once

#include <memory>
#include <thread>
#include <mutex>
#include <deque>
#include <optional>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <cstdint>

#ifdef _WIN32
#define _WIN32_WINNT 0x0A00
#endif // _WIN32

#define ASIO_STANDALONE
#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>





enum class GameMsg : uint32_t
{
	//system
	Client_Accepted,
	//Client_AssignID,

	// login / profile
	Client_Login,          // клиент → сервер
	Server_LoginResult,    // Для входа (пароль/логин)
	Client_RegisterWithServer, // отправляет клиент на сервре 
	Server_RegisterResult,  // отправляет сервер клиенту 
	Server_SessionRestored, // Для реконнекта по токену
	// server → client
	Server_MatchmakingStatus,
	Server_RoomStarted,
	Server_GameStarted,
	Server_GameFinished,
	Server_PlayerReady,
	Server_MatchEnded,
//	Server_GetStatus,
	Server_Ping,
	Server_SpellCooldown,

	Client_CancelMatchmaking,

	Client_ReturnToLobby,
	Server_ReturnToLobby,
	// client → server
	Client_FindMatch,
	Client_SelectClass,

	Client_UnregisterWithServer,

	Game_PlayerInfo, // server → client
	Game_AddPlayer,
	Game_RemovePlayer,
	Game_UpdatePlayer,
	chat_message,

	Game_TileUpdate,
	Game_TileMove,


	Game_CastSpell,
	Game_ReleaseSpell,
//	Game_CancelSpell,

	Game_RemoveProjectile,
	
	Game_UpdateWorld,

	Game_DetonateSticky,
	Game_Explosion,
	Game_TriggerTeleport,

	Game_EnvironmentUpdate, // Погода, ветер, освещение
	Game_CraftArrow,
	Game_DamageEvent,
	Server_BeaconOwnerUpdate,
	Server_SyncPlayerProfile,
	Game_BindLink,
	Game_EntityStats,

	Server_CastStart,      // Сервер сообщает: игрок начал замах
	Server_CastLaunch,     // Сервер сообщает: снаряд вылетел (переход к фазе 2)
	Server_CastInterrupt,  // Сервер сообщает: каст прерван станом/уроном
};

struct sMatchEnd {
	uint32_t winnerID;
	float matchDuration;
};
