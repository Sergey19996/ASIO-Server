#pragma once
#include "../Spell.h"

class MagicConservation : public Spell
{
public:
	MagicConservation() {
		id = SpellId::MagicConservation; state = SpellState::Appear;
	}


	bool Cast(Character& caster, Match* server) override;

	void Update(float dt, Match* server) override {}
protected:
	bool isSecondTime = false;

};