#ifndef SHOOT_H
#define SHOOT_H

#include "../TimedSpell.h"
#include <iostream>


class Shoot : public TimedSpell {
public:
    Shoot() { id = SpellId::Shoot; state = SpellState::Appear;
    appearDuration = 0.1f;
    activeDuration = 1.0f;
    disappearDuration = 0.1f;
    }

    bool Cast(Character& caster, Match* server) override; // только объявление





    uint16_t ProjectileId;
    uint32_t CasterId;

    glm::vec2 basePos;
    glm::vec2 direction;

    float targetRadius = 0.0f;



 
    bool OwnsProjectile(int uid) override;
    void ForceFinish(int uid, Match* server) override;

    void NotifyWorldHit(glm::ivec2 cell, Match* server) override;
    void NotifyCharacterHit(uint32_t ProjID, uint32_t TargetID, Match* server) override;

protected:

    void UpdateAppear(float dt, Match* server)override;
    void UpdateActive(float dt, Match* server)override;
    void UpdateDisappear(float dt, Match* server)override;


private:
    int power = 0;
    bool isStuck = false;
    const float PICKUP_RADIUS = 1.5f; // Радиус сбора нити судьбы
   // ArrowType finalType = ArrowType::Normal;
    bool Contacted = false;

    bool hasExploded = false;
    bool hasBound = false;
};
#endif