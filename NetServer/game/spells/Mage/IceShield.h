#pragma once 
#include "../spell.h"
#include <iostream>


class IceShield : public Spell {
public:


        IceShield() {
        id = SpellId::IceShield; state = SpellState::Appear;
        
    }
    /*  struct WallPart
      {
          uint32_t uid;
          uint32_t Casteruid;
          float scale = 0.0f;
      };*/

      //  std::vector<WallPart> parts;

   // uint16_t ProjectileId;
    uint32_t CasterId;
    //  glm::vec2 basePos;
   //   glm::vec2 direction;


    bool Cast(Character& caster, Match* server) override;

    void Update(float dt, Match* server) override {}
protected:

 //   void UpdateAppear(float dt, Match* server) override;
 //   void UpdateActive(float dt, Match* server)override;
 //   void UpdateDisappear(float dt, Match* server)override;



};