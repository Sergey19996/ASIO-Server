#pragma once



#include "../TimedSpell.h"
#include <iostream>


class ShieldThrowSpell : public TimedSpell {
public:


    ShieldThrowSpell() {
        id = SpellId::ShieldThrow; state = SpellState::Active;
        appearDuration = 0.01f;
        activeDuration = 5.0f;
        disappearDuration = 0.1f;
    }
   

    uint16_t ProjectileId;
    uint32_t CasterId;
  

    bool Cast(Character& caster, Match* server) override;


    void ForceFinish(int uid, Match* server) override;
    bool OwnsProjectile(int uid) override;
 

protected:

    void UpdateAppear(float dt, Match* server) override;
    void UpdateActive(float dt, Match* server)override;
    void UpdateDisappear(float dt, Match* server)override;



};

