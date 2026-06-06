#pragma once
#include "../../Spell.h"



class SpellCommandMove : public Spell {
public:
    SpellCommandMove() { state = SpellState::Finished; }
    bool Cast(Character& caster, Match* server) override;
    void Update(float dt, Match* server) override {}
};