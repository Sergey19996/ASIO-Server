#pragma once
#include <vector>
#include <cstdint>
#include <cstring>
#include "../StatusEffect.h"

// ѕередаем указатель на Character, чтобы менеджер знал, на кого вли€ют эффекты
class Character;

class EffectManager {
public:
    EffectManager(Character* owner);

    void Add(const StatusEffect& effect);
    void Update(float dt);
    void Remove(const StatusEffectType& effect);

    bool HasEffect(StatusEffectType t) const {
        return (currentEffectsMask & (1 << (uint8_t)t)) != 0;
    }
    StatusEffect* GetEffect(StatusEffectType t);
    uint32_t GetMask() const { return currentEffectsMask; };
    void RefreshModifiers();
    void setCurrentEffectMask(uint32_t mask) { currentEffectsMask = mask; };
    uint32_t getCurrEffectMask() { return currentEffectsMask; };
    void Clear();

private:
    Character* owner;
    std::vector<StatusEffect> effects;
    StatusEffect* effectCache[32];
    uint32_t currentEffectsMask = 0;
};