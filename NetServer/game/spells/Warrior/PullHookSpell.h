#pragma once 
#include "../TimedSpell.h"



class PullHookSpell : public TimedSpell {
public:

    PullHookSpell() {
        id = SpellId::Hook; state = SpellState::Appear;



    }
    bool Cast(Character& caster, Match* server) override;
    void ForceFinish(int uid, Match* server) override;
    void NotifyWorldHit(glm::ivec2 cell, Match* server) override;
    void NotifyCharacterHit(uint32_t ProjID, uint32_t TargetID, Match* server) override;
    bool OwnsProjectile(int uid) override;
private:
    void UpdateAppear(float dt, Match* server) override;
    void UpdateActive(float dt, Match* server) override;
    void UpdateDisappear(float dt, Match* server) override;



    enum PullType : uint8_t
    {
        Player,
        Wall
    };
    PullType pullType;
    uint32_t projectileUid;
    uint32_t casterIdx;
    uint32_t targetPlayerIdx;
    glm::vec2 direction;
    glm::vec2 anchorPos;
    glm::ivec2 targetCellPos;
    bool isAttached = false;
    float maxDistanceCells = 5.0f;
    float pullSpeed = 12.0f;

   

};