#pragma once
#include "../TimedSpell.h"

struct HarvestNode {
    glm::vec2 pos;
    int quiverIdx;
    uint32_t arrowProjId; // Индекс в колчане, чтобы занулить linkedId

};
class ChainHarvest : public TimedSpell {
public:
    uint32_t effectProjId = 0xFFFF;
    std::vector< HarvestNode> nodes;
    int currentTargetIdx = 0;
    float moveSpeed = 35.0f; // Скорость полета заряда между стрелами
    uint32_t CasterId = 0;
    ChainHarvest() {
        id = SpellId::ChainHarvest;
    }
    bool Cast(Character& caster, Match* server) override;


protected:

    void UpdateAppear(float dt, Match* server)override {};
    void UpdateActive(float dt, Match* server)override;
    void UpdateDisappear(float dt, Match* server)override;
    void FinishHarvest(Match* server);
};