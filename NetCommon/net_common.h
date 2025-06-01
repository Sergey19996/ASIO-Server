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
#include <glm/glm.hpp>


enum class GameMsg : uint32_t
{
	Server_GetStatus,
	Server_GetPing,

	Client_Accepted,
	Client_AssignID,
	Client_RegisterWithServer,
	Client_UnregisterWithServer,

	Game_AddPlayer,
	Game_RemovePlayer,
	Game_UpdatePlayer,
	chat_message,

	Game_AddProjectile,
	Game_RemoveProjectile,
	Game_UpdateProjectile,
};


struct sPlayerDescription
{
	uint32_t nUniqueID = 0;
	uint32_t nAvatarID = 0;

	uint32_t nHealth = 100;
	uint32_t nAmmo = 20;
	uint32_t nKills = 0;
	uint32_t nDeaths = 0;

	float fRadius = 0.5f;

	glm::vec2 vPos;
	glm::vec2 vVel;
};
struct sChatMessage
{
	uint32_t nSenderID;            // ID отправителя (тот же, что в sPlayerDescription.nUniqueID)
	std::string sText;             // Сам текст
};

struct sProjectileDescription {

	uint32_t nOwnerID = 0;       // ID владельца (игрока)
	uint32_t nUniqueID = 0;      // ID снаряда
	glm::vec2 vPos;
	glm::vec2 vVel;
	float fLifetime;
	float fRadius = 0.5f;
	// можно добавить тип: enum ProjectileType { BULLET, ROCKET, ARROW }
};