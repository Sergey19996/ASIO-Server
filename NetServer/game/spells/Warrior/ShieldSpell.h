#ifndef SHIELDSPELL_H
#define SHIELDSPELL_H


#include "../TimedSpell.h"
#include <iostream>


class ShieldSpell : public TimedSpell {
public:


    ShieldSpell() { id = SpellId::Shield; state = SpellState::Appear;
    appearDuration = 0.5f;
    activeDuration = 5.0f;
    disappearDuration = 0.25f;
    }
  /*  struct WallPart
    {
        uint32_t uid;
        uint32_t Casteruid;
        float scale = 0.0f;
    };*/

  //  std::vector<WallPart> parts;
    
    uint16_t ProjectileId;
    uint32_t CasterId;
    //  glm::vec2 basePos;
   //   glm::vec2 direction;


    bool Cast(Character& caster, Match* server) override;
  

    void ForceFinish(int uid, Match* server) override;
    bool OwnsProjectile(int uid) override;
    bool ThrowShield(Character& caster, Match* server);

protected:

    void UpdateAppear(float dt, Match* server) override;
    void UpdateActive(float dt, Match* server)override;
    void UpdateDisappear(float dt, Match* server)override;

  

};
#endif // !SHIELDSPELL_H
