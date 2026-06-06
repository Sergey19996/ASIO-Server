#pragma once
#include <cstdint>
#include "../NetShared/GameplayTypes.h"

enum class DamageTargetType
{
    Character,
    WorldTile,
    Projectile
};

struct DamageContext
{
    uint32_t attackerId = 0;
 

    int baseDamage = 0;
    int finalDamage = 0;

    // === TARGET ===
    DamageTargetType targetType = DamageTargetType::Character;
    uint32_t targetId = 0;          // characterId
    glm::ivec2 targetCell = { -1,-1 }; // для мира


    DamageType type = DamageType::Physical;
    DamageSource source = DamageSource::Projectile;

    bool cancelled = false;
    bool absorbedByShield = false;
};