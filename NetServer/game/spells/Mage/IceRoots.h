#pragma once 
#include "../TimedSpell.h"
class Match; // forward declaration
class IceRoots : public TimedSpell {
public:
    IceRoots() {
        id = SpellId::IceRoots; state = SpellState::Appear;

    }
    bool Cast(Character& caster, Match* server) override;



protected:
    uint32_t casterIdx;
   
    void UpdateAppear(float dt, Match* server) override;
    void UpdateActive(float dt, Match* server) override {};
    void UpdateDisappear(float dt, Match* server) override {};
};