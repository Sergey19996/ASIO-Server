#pragma once
#include "CompanionBase.h"

class PriestCompanion : public CompanionBase {
public:
    PriestCompanion(uint32_t id, Character* owner = nullptr, Match* s = nullptr);
      
#ifdef GAME_SERVER
protected:
    void UpdateChase(float dt) override;
    void UpdateTarget(float dt) override;
    void ExecuteAttack(Character* target);
#endif
};