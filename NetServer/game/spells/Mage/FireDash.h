#pragma once
#include "../TimedSpell.h"
class Match; // forward declaration

class FireDash : public TimedSpell {
public:
    FireDash();

    bool Cast(Character& caster, Match* server) override;

protected:
    void UpdateAppear(float dt, Match* server) override;
    void UpdateActive(float dt, Match* server) override;
    void UpdateDisappear(float dt, Match* server) override;

private:
    glm::vec2 dashDirection;
    uint32_t casterIdx = 0;
    float dashSpeed = 800.0f; // Скорость рывка
    float tickTimer = 0.0f;
    const float damageTickRate = 0.1f; // Как часто оставляем след/наносим урон
};