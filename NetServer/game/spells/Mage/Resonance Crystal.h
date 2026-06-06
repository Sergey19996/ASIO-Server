#pragma once
#include "../TimedSpell.h"
class ResonanceCrystal : public TimedSpell{
public:
    ResonanceCrystal() { id = SpellId::ResonanceCrystal; state = SpellState::Appear;
        appearDuration = 0.5f;
        activeDuration = 10.0f;
        disappearDuration = 0.5f;

    }
    bool Cast(Character& caster, Match* server) override;
    void ForceFinish(int uid, Match* server) override;

 //   void Update(float dt, Match* server) override {}

private:
    float icePower = 0.0f;
    void UpdateAppear(float dt, Match* server)override;
    void UpdateActive(float dt, Match* server) override;
    void UpdateDisappear(float dt, Match* server) override;
    uint16_t projectileUid = 0xFFFF;
};