#pragma once
#include <cstdint> // Добавляет поддержку uint8_t, uint32_t и др.

enum class ProjectileType : uint8_t { // Вопрос !

	Fireball = 0,
	Wall = 1,
	SmashHit = 2,
	Shield = 3,
	Shoot = 4,
	StickyBomb = 5,
	Explosion = 6,
	TeleportPortal = 7,
	Hook = 8,
	Iceball = 9,
	BlastWave = 10,
	MageCrystal = 11,
	BindShot =12,
	ShieldThrow = 13,
	HarvestEnergy = 14,
	GhostArrow = 15,
	IceRoots = 16,
	StygianSpike = 17,
	SunLance = 18,
	SunMarker = 19,

};
enum class DamageType : uint8_t
{
	Physical,
	Magical,
	Pure
};
struct ProjectileParams { // для create projectiles расчёт сразу в момент создания а не нанесения 
	uint32_t ownerId;
	float damageMod = 1.0f;
	float effectMod = 1.0f;
	// Сюда можно добавить специфичные флаги
};
enum class Element : uint8_t { None, Fire, Ice, Shadow };

enum class DamageSource : uint8_t
{
	none,
	Projectile,
	Environment,
	LinkSecondary
};

struct sSpellCooldown {
	uint8_t spellId;
	float cooldown;
	float serverTime;
};

struct sMatchmakingStatus {
	uint32_t playersInQueue;
	// Сюда можно добавить: uint32_t estimatedWaitTime;
};

struct sEnvironmentData {
	float windAngle;  // Направление в радианах
	uint8_t windForce;  // Сила ветра (если понадобится для сноса пуль)
	float dayProgress;  // Передаем 0.0 - 1.0 (время суток)
};


enum class ArrowType : uint8_t {
	Normal = 0,
	Binding,
	Ghost,
	Explosive
};
const int BRICK_SIZE = 32;
const float MIN_RADIUS = 6.0f * BRICK_SIZE; // Ночью (тускло)
const float MAX_RADIUS = 12.0f * BRICK_SIZE; // Днем (далеко)
