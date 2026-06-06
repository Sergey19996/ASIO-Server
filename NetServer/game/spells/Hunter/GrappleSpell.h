#pragma once
#include "../TimedSpell.h"
//#include <glm/gtx/vector_query.hpp>




class GrappleSpell : public TimedSpell {

  

 
    glm::vec2 direction;
    glm::vec2 anchorPos;
    bool isAttached = false;
    float maxDistanceCells = 5.0f;
    float pullSpeed = 12.0f;

public:
    GrappleSpell() {
        id = SpellId::Grapple; state = SpellState::Appear;

     

    }
    bool Cast(Character& caster, Match* server) override;
    
    void ForceFinish(int uid, Match* server) override;
    void NotifyWorldHit(glm::ivec2 cell, Match* server) override;
    bool OwnsProjectile(int uid) override;
private:
    uint32_t projectileUid;
    uint32_t casterIdx;
    void UpdateAppear(float dt, Match* server) override;

      

    void UpdateActive(float dt, Match* server) override;

      

    void UpdateDisappear(float dt, Match* server) override;

};