#pragma once
#include "CompanionBase.h"

class RangerCompanion : public CompanionBase {
public:
    RangerCompanion(uint32_t id, Character* owner = nullptr, Match* s = nullptr);
#ifdef GAME_SERVER
protected:
    void UpdateChase(float dt) override;
#endif
};