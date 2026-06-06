#pragma once
#include "../TimedSpell.h"
#include <iostream>


class MeleeHit : public TimedSpell {
public:


    MeleeHit() {
        id = SpellId::MeleeHit; state = SpellState::Appear;
        appearDuration = 0.1f;
        activeDuration = 0.1f;
        disappearDuration = 0.1f;
    }




    uint16_t projectileId;
    uint32_t casterId;

    //  glm::vec2 basePos;
   //   glm::vec2 direction;


    bool Cast(Character& caster, Match* server) override;
    // void Update(float dt, GameServer* server) override;

    void ForceFinish(int uid, Match* server) override;
    bool OwnsProjectile(int uid) override;
protected:

    void UpdateAppear(float dt, Match* server)override;
    void UpdateActive(float dt, Match* server)override;
    void UpdateDisappear(float dt, Match* server)override;


};
