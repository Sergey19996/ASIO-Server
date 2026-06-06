#pragma once
#include <cstdint>
#include <glm/glm.hpp> 
enum class StatusEffectType : uint8_t
{
    None,
    Stun,
    Silence,
    Slow,
    Shield,
    TeleportCooldown,
    Burn,
    Linked,
    BindingChain,
    IceShield,
    StygianChill,
    SunBrand
};

struct StatusEffect
{
    StatusEffectType type = StatusEffectType::None;
    float timeLeft = 0.0f;
    float tickTimer = 0.0f;

    // флаги поведения
    bool blocksMovement = false;
    bool blocksCasting = false;
    glm::vec2 vCenterPos = { 0,0 };
    // числовые параметры (опционально)
    float value = 0.0f; // slow %, shield hp, etc.

    // shield-specific
    int shieldHP = 0;
    uint32_t nOwnerNetID = 0;
    uint32_t linkedEntityId = -1;
};
