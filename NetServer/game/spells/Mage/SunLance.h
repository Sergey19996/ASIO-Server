#pragma once
#include "../TimedSpell.h"
class Match; // forward declaration
class SunLance : public TimedSpell {
public:
    SunLance() {
        id = SpellId::SunLance; state = SpellState::Appear;

        appearDuration = 1.5f;
        activeDuration = 1.5f;
        disappearDuration = 0.1f;


    }
    bool Cast(Character& caster, Match* server) override;

    bool OwnsProjectile(int uid) override;
    void ForceFinish(int uid, Match* server) override;
protected:
    glm::vec2 basePos;
    glm::vec2 direction;
    uint16_t projectileUid = 0xFFFF;
    uint32_t casterIdx = 0;

    float targetRadius = 0.2f;

    void UpdateAppear(float dt, Match* server) override;
    void UpdateActive(float dt, Match* server) override;
    void UpdateDisappear(float dt, Match* server) override;
};