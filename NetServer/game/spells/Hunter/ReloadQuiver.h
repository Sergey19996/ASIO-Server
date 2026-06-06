#pragma once
#include "../TimedSpell.h"

class ReloadQuiver : public TimedSpell {
public:
    ReloadQuiver() {
        id = SpellId::ReloadQuiver; // Добавь в enum SpellId
      
    }
    //uint32_t effectProjId = 0xFFFF;
  //  std::vector< HarvestNode> nodes;
  //  int currentTargetIdx = 0;
  //  float moveSpeed = 35.0f; // Скорость полета заряда между стрелами
  //  uint32_t CasterId = 0;

    bool Cast(Character& caster, Match* server) override;


protected:

    void UpdateAppear(float dt, Match* server)override {};
    void UpdateActive(float dt, Match* server)override {};
    void UpdateDisappear(float dt, Match* server)override {};
  //  void FinishHarvest(Match* server);
};