#ifndef ICEBALLSPELL_h
#define ICEBALLSPELL_h

#include "../TimedSpell.h"

class IceballSpell : public TimedSpell {
public:
    IceballSpell() {
        id = SpellId::Iceball; // Убедись, что Iceball есть в SpellId
        state = SpellState::Appear;
        appearDuration = 0.1f;   // Лёд создается чуть быстрее
        activeDuration = 1.6f;
        disappearDuration = 0.3f;
    }

    bool Cast(Character& caster, Match* server) override;
    bool OwnsProjectile(int uid) override;
    void ForceFinish(int uid, Match* server) override;

    bool CanBeCancelled() const override;
    void Cancel(Match* server) override;
    float GetAdjustedCooldown() const override {
        // Спелл сам знает, что его длительность зависит от powerMultiplier
        // Допустим, база 1.5 сек + время подготовки
        return 1.5f * powerMultiplier;
    }

    glm::vec2 basePos;
    glm::vec2 direction;
    uint16_t projectileUid = 0xFFFF;
    uint32_t casterIdx = 0;
    float targetRadius = 0.4f; // Чуть меньше огненного
    float explosionPower = 0.0;
private:
    void UpdateAppear(float dt, Match* server) override;
    void UpdateActive(float dt, Match* server) override;
    void UpdateDisappear(float dt, Match* server) override;
};
#endif