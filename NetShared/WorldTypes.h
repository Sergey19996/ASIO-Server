#pragma once
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include "GameplayTypes.h"

//#define PACK_TypeAndSub(type, sub) (uint8_t)(((uint8_t)(sub) << 4) | ((uint8_t)(type) & 0x0F))
//#define GET_ENTITY_TYPE(avatar) (uint8_t)((avatar) & 0x0F)
//#define GET_SUB_TYPE(avatar)    (uint8_t)((avatar) >> 4)

#pragma pack(push, 1) // Гарантирует, что размер структуры будет ровно 24 байта
struct sEntityDescription
{
	uint32_t nUniqueID;    // 4б
	uint8_t  nArchetypeId;    // 1б
	uint8_t  nHealth;      // 1б
	int8_t   radius;       // 1б
	uint8_t nTeamID; // <-- Добавляем (1 байт)
	uint8_t  nDirection;   // 1б <-- НОВОЕ ПОЛЕ (угол взгляда
	int16_t  posX, posY;   // 4б
	int8_t   velX, velY;   // 2б
	uint8_t  fSpecialBar;  // 1б
	uint8_t  fChargeRatio; // 1б
	uint32_t nClassParam;  // 4б
	uint32_t nEffectsMask; // 4б
	uint8_t  nShieldH;     // 1б
};
#pragma pack(pop)

struct sProjectileDescription {

	uint32_t nOwnerID = 0;       // ID владельца (игрока)
	uint32_t nUniqueID = 0;      // ID снаряда
	uint32_t nVictimId = 0;
	float fLifetime;
	float fRadius = 0.5f;
	ProjectileType type;
	bool bStuck = false;
	bool bPendingDestroy = false;
	int durability = 0;
	glm::vec2 vPos;
	glm::vec2 vVel;
	// можно добавить тип: enum ProjectileType { BULLET, ROCKET, ARROW }
};


struct sDamageReport {

	uint32_t nVictimId;
	uint32_t nAttackerId;
	float Amount;
	float ExperienceGained; // <-- Добавляем поле
	uint8_t nType;
	bool bResisted;



};
#pragma pack(push, 1)
struct sCastStart {
	uint32_t casterId;
	uint8_t  spellId;
//	float    prepDuration; // Сколько будет длиться замах (с учетом баланса/льда)
};

struct sCastLaunch {
	uint32_t casterId;
	uint8_t  spellId;
};

struct sCastInterrupt {
	uint32_t casterId;
};
#pragma pack(pop)