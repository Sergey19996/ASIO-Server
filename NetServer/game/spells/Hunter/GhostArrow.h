#pragma once
#include "../TimedSpell.h"

class GhostArrow : public TimedSpell {
public:
    GhostArrow() {
        id = SpellId::GhostArrow; // Добавь в enum SpellId
        appearDuration = 0.25f;
        activeDuration = 2.0f;   // Летит дольше, так как не стопится об стены
        disappearDuration = 0.25f;
    }

    bool Cast(Character& caster, Match* server) override;


    //// Игнорируем уведомления от сервера о попадании в стену
    //void NotifyWorldHit(glm::ivec2 cell, Match* server) override {
    //    // Ничего не делаем. Стрела летит дальше.
    //}

    //void NotifyCharacterHit(uint32_t ProjID, uint32_t TargetID, Match* server) override {
    //    if (ProjID != ProjectileId) return;

    //    // Логика урона (пронзающая стрела может бить многих или исчезать на первом)
    //    if (auto* target = server->GetCharacter(TargetID)) {
    //        if (target->GetId() != CasterId) {
    //            target->ApplyDamage(20.0f * powerMultiplier);

    //            // Если хочешь, чтобы она исчезала после ПЕРВОГО попадания в игрока:
    //            // state = SpellState::Disappear; 
    //        }
    //    }
    //}
    void ForceFinish(int uid, Match* server) override;
    bool OwnsProjectile(int uid) override { return ProjectileId == uid; }
    void ApplyHunterFlags(bool hasExplosion, bool hasBinding, bool hasGhost, bool bonusPower)override;
    void NotifyCharacterHit(uint32_t ProjID, uint32_t TargetID, Match* server) override;

protected:
    void UpdateAppear(float dt, Match* server) override;

    void UpdateActive(float dt, Match* server) override;
    void UpdateDisappear(float dt, Match* server) override;

private:
    uint32_t ProjectileId = 0;
    uint32_t CasterId = 0;
    float targetRadius = 0.25f;
    bool HasBind = false;
    bool HasExplosion = false;
    int chargePower = 0; // подумать 
    int memberArrowIdx = -1;
    glm::vec2 basePos;
    glm::vec2 direction;
};
