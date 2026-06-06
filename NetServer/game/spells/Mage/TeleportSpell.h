#pragma once
#include "../TimedSpell.h"


class TeleportSpell : public TimedSpell {
public:
  


    TeleportSpell() {
        id = SpellId::Teleport; // ќЅя«ј“≈Ћ№Ќќ дл€ FindActive
        appearDuration = 0.3f;
        activeDuration = 10.0f; // ѕорталы сто€т 10 секунд
        disappearDuration = 0.3f;
    }

    bool Cast(Character& caster, Match* server) override;

    // ѕереопредел€ем Update, чтобы добавить логику телепортации
  
    void ForceFinish(int uid, Match* server) override;
protected:
    void UpdateAppear(float dt, Match* server) override;
    void UpdateActive(float dt, Match* server) override;
    void UpdateDisappear(float dt, Match* server) override;

private:

    struct PortalPart {
        uint32_t uid;
        glm::vec2 pos;
    };
    std::vector<PortalPart> portals;
    std::vector<uint16_t> processedInThisFrameProj;
    std::vector<uint32_t> processedInThisFrameChar;
    bool secondPortalPlaced = false;
    float casterIdx;
    void TeleportObject(uint32_t objId, glm::vec2 fromPos, glm::vec2 toPos, Match* server);
};