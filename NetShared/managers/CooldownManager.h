#pragma once
#include <unordered_map>
#include <algorithm>
#include "../SpellId.h"

class CooldownManager {
    std::unordered_map<SpellId, float> cooldowns;

public:
    void Set(SpellId id, float duration, float timeNow) {
        cooldowns[id] = timeNow + duration;
    }

    float GetRemaining(SpellId id, float timeNow) const {
        auto it = cooldowns.find(id);
        if (it != cooldowns.end()) {
            return std::max(0.0f, it->second - timeNow);
        }
        return 0.0f;
    }

    bool IsReady(SpellId id, float timeNow) const {
        return GetRemaining(id, timeNow) <= 0.0f;
    }

    void Clear() { cooldowns.clear(); }
};