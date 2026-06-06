#pragma once


#include "../TimedSpell.h"
#include <iostream>


class InfuseArrow : public TimedSpell {
public:
    InfuseArrow() {
        id = SpellId::InfuseArrow; state = SpellState::Appear;
        appearDuration = 0.1f;
        activeDuration = 1.0f;
        disappearDuration = 0.1f;
    }

    bool Cast(Character& caster, Match* server) override; // только объ€вление





    uint16_t ProjectileId;
    uint32_t CasterId;








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
  
 
   // ArrowType finalType = ArrowType::Normal;
};