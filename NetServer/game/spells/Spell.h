#ifndef Spell_H
#define Spell_H

#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "../../../NetShared/SpellId.h"
//#include "../gameplay/SpellCooldowns.h"
#include "../../../NetShared/GameplayRules.h"
class Match;
class Character;

enum class SpellState {
    Appear,
    Active,
    Disappear,
    Finished
};

class Spell {
public:
    virtual ~Spell() {}
    virtual bool Cast(Character& caster, Match* server) { return true; };
    virtual void Update(float dt, Match* server) = 0;
    virtual bool IsFinished() const { return state == SpellState::Finished; }
    virtual bool OwnsProjectile(int uid) { return false; }
    virtual void ForceFinish(int uid, Match* server) { state = SpellState::Finished; }
    virtual void NotifyWorldHit(glm::ivec2 cell, Match* server) {};
    virtual void NotifyCharacterHit(uint32_t ProjID, uint32_t TargetID, Match* server) {};
    virtual void ApplyHunterFlags(bool hasExplosion, bool hasBinding, bool hasGhost, bool bonusPower) {};
    virtual bool CanBeCancelled() const { return false; }
    virtual void Cancel(Match* server) {}


    void SetPowerMultiplier(float m) { powerMultiplier = m; }
    virtual float GetAdjustedCooldown() const {
        return GameplayRules::GetDefaultCooldown(id); // По умолчанию из таблицы
    }
    void ReleaseCaster(Match* server);
       
   
    SpellId& GetSpellId() { return id; }
    SpellState& GetState() { return state; }
    uint32_t GetOwner() { return ownerId; }
    void SetOwner(uint32_t id) { ownerId = id; }

    SpellId id;
    SpellState state = SpellState::Appear;


protected:
    bool bHasUnlocked = false; // Флаг предотвращения двойного анлока
    float lifeTimer = 0.0f;
    uint32_t ownerId = 0;
    float powerMultiplier = 1.0f; // По умолчанию сила 100%
};

#endif // !Game