#pragma once
#include "../TimedSpell.h"

class BindArrowSpell : public TimedSpell
{
public:
    BindArrowSpell() {
        id = SpellId::BindArrow;
        activeDuration = 4.0f; // Длительность связи
    }

    bool Cast(Character& caster, Match* server) override;

    // Перегружаем методы жизненного цикла
    void UpdateAppear(float dt, Match* server) override;
    void UpdateActive(float dt, Match* server) override;
    void UpdateDisappear(float dt, Match* server) override;

    void NotifyCharacterHit(uint32_t ProjID, uint32_t TargetID, Match* server) override;
    void NotifyWorldHit(glm::ivec2 cell, Match* server) override;
    bool OwnsProjectile(int uid) override;
    void ApplyHunterFlags(bool hasExplosion, bool hasBinding, bool hasGhost, bool bonusPower)override;
    

    // Вспомогательный метод для Match::ApplyDamage
    uint32_t GetLinkedEntityFor(uint32_t victimId);
    void ForceFinish(int uid, Match* server) override;
    void CheckAndApplyLink(Match* server);
private:
    struct LinkNode {
        uint32_t entityId;
        uint32_t associatedProjId;
        glm::vec2 endPos; // Мировая позиция центра (cell + 0.5)
        glm::ivec2 startPos;  // Координаты в сетке (целые)
        bool isStatic;
    };

    bool HasExplosion = false;
    bool HasGhost = false;
    int chargePower = 0;
    uint32_t projectileId = -1;
    int memberArrowIdx = -1;
    LinkNode nodes[2];
    int nodesCount = 0;

    int shotCount = 0;

    const float maxDistance = 12.0f;
    std::vector<uint16_t> projectileIds;
    void Spawn(Character& caster, Match* server);
    glm::vec2 GetNodePos(int idx, Match* server);
    bool bHasLinked = false;
};