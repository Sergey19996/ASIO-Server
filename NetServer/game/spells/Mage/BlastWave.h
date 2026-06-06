#pragma once
#include "../Spell.h"

class BlastWave : public Spell {
public:
    BlastWave() {
        id = SpellId::BlastWave; state = SpellState::Appear;
   
    }
    bool Cast(Character& caster, Match* server) override;


    void Update(float dt, Match* server) override {}
};