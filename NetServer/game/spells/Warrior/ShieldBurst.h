#pragma once
#include "../Spell.h"

class ShieldBurst : public Spell {
public:
    ShieldBurst() {
        id = SpellId::ShieldBurst; state = SpellState::Appear;

    }
    bool Cast(Character& caster, Match* server) override;


    void Update(float dt, Match* server) override {}
};