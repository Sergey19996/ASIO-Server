#ifndef FIREBALLSPELL_h
#define FIREBALLSPELL_h

#include "../TimedSpell.h"
#include <iostream>
class Match; // forward declaration

class FireballSpell : public TimedSpell {
public:
    FireballSpell() { id = SpellId::Fireball; state = SpellState::Appear;
    appearDuration = 0.1f;
    activeDuration = 1.5f;
    disappearDuration = 0.5f;
    }

    bool Cast(Character& caster, Match* server) override; // только объявление


    float GetAdjustedCooldown() const override {
        // Спелл сам знает, что его длительность зависит от powerMultiplier
        // Допустим, база 1.5 сек + время подготовки
      
        return 1.5f * powerMultiplier;
    }

    void NotifyWorldHit(glm::ivec2 cell, Match* server) override;
    void NotifyCharacterHit(uint32_t ProjID, uint32_t TargetID, Match* server) override;

    glm::vec2 basePos;
    glm::vec2 direction;
    uint16_t projectileUid = 0xFFFF;
    uint32_t casterIdx = 0;

    float targetRadius = 0.5f;


    bool OwnsProjectile(int uid) override;
    void ForceFinish(int uid, Match* server) override;
    bool CanBeCancelled() const override;
    void Cancel(Match* server) override;

    bool IsFinished() const override; 
private:

    void UpdateAppear(float dt, Match* server)override;
    void UpdateActive(float dt, Match* server)override;
    void UpdateDisappear(float dt, Match* server)override;
};
#endif