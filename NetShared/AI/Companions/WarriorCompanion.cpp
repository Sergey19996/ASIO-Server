#include "WarriorCompanion.h"

WarriorCompanion::WarriorCompanion(uint32_t id, Character* owner, Match* s) : CompanionBase(id, "Guardian", owner, s) {
    maxHealth = 60;
    health = maxHealth;
    radius = 0.45f;
    m_classId = "Comp_Warrior"_sid;
    entityType = ArchetypeId::Companion_Warrior;

#ifdef GAME_SERVER
    chaseRange = 2.0f;

    attackRange = 1.5f;
    defaultSpell = SpellId::MeleeHit; // или ваш аналог
    position = { 0,0 };
#endif
}

