#pragma once
#include "../TimedSpell.h"
class Match; // forward declaration

class SunMarker : public TimedSpell {
public:
    SunMarker() {
        id = SpellId::SunMarker;
        state = SpellState::Appear;
        appearDuration = 0.2f;  // Быстрое появление (вспышка)
        activeDuration = 1.0f;  // Время жизни одного такта метки
        disappearDuration = 0.3f; // Плавное угасание
    }

    // Нам нужно знать, за кем "следовать"
    uint32_t targetId = 0;
    uint16_t projectileUid = 0xFFFF;

    bool Cast(Character& caster, Match* server) override;

protected:
    void UpdateAppear(float dt, Match* server) override;
    void UpdateActive(float dt, Match* server) override ;
    void UpdateDisappear(float dt, Match* server) override;

};