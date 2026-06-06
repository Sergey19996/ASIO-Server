#pragma once
#include "AIBase.h"
#ifdef GAME_SERVER
class Match;
#endif
class RangerMob : public AIBase {
public:

    RangerMob(uint32_t id, const std::string& name, glm::vec2 spawn = { 0,0 }, Match* server = nullptr);




protected:
#ifdef GAME_SERVER
    void UpdateAI(float dt) override;
#endif
};