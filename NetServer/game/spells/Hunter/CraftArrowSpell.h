#pragma once
#include "../TimedSpell.h"
#include "../NetShared/entities/Hunter.h"

class CraftArrowSpell : public TimedSpell {
public:
    //  онструктор теперь принимает тип стрелы
    CraftArrowSpell(SpellId spellId, ArrowType arrowType);
    //uint32_t effectProjId = 0xFFFF;
  //  std::vector< HarvestNode> nodes;
  //  int currentTargetIdx = 0;
  //  float moveSpeed = 35.0f; // —корость полета зар€да между стрелами
  //  uint32_t CasterId = 0;

    bool Cast(Character& caster, Match* server) override;


protected:
    ArrowType typeToCraft;
    void UpdateAppear(float dt, Match* server)override {};
    void UpdateActive(float dt, Match* server)override {};
    void UpdateDisappear(float dt, Match* server)override {};
    //  void FinishHarvest(Match* server);
};