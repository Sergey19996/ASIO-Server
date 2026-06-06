#pragma once
#include "../Spell.h"


class StickyBombSpell : public Spell
{
public:
    StickyBombSpell() {
        id = SpellId::StickyBomb; //
    }


        bool Cast(Character& caster, Match* server) override;


        void Update(float dt, Match* server) override;


        bool OwnsProjectile(int uid) override;

        void ForceFinish(int uid, Match* server) override;
        void NotifyWorldHit(glm::ivec2 cell, Match* server) override;
        void ApplyHunterFlags(bool hasExplosion, bool hasBinding, bool hasGhost, bool bonusPower)override;

    int memberArrowIdx = -1;
private:
    uint32_t projectileId = -1;

    bool stuck = false;
    int chargePower = 0;
    bool HasBind = false;
    bool HasGhost = false;
    void Spawn(Character& caster, Match* server);


        void Explode(Match* server);
   
};