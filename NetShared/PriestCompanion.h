#pragma once
#include "CompanionBase.h"

class WarriorCompanion : public CompanionBase {
public:
    WarriorCompanion(uint32_t id, Character* owner, Match* s)
        : CompanionBase(id, "Guardian", owner, s) {
        maxHealth = 100;
        health = maxHealth;
        radius = 0.45f;


#ifdef GAME_SERVER
        chaseRange = 2.0f;

        attackRange = 1.5f;
        defaultSpell = SpellId::MeleeHit; // или ваш аналог
        position = { 0,0 };
#endif
    }
};