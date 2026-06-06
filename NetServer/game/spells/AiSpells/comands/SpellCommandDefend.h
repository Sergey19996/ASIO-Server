#pragma once
#include "../../Spell.h"
class SpellCommandDefend : public Spell {
public:
    SpellCommandDefend() { state = SpellState::Finished; }
    bool Cast(Character& caster, Match* server) override;
    void Update(float dt, Match* server) override {}
};