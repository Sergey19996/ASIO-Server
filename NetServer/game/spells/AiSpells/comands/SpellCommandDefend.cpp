#include "SpellCommandDefend.h"
#include "../../../../Match.h"
#include "../../../../../NetShared/AI/Companions/CompanionBase.h"
#include "../../../../../NetShared/managers/SquadManager.h"



bool SpellCommandDefend::Cast(Character& caster, Match* server)
{

    // 1. Получаем список подконтрольных юнитов владельца (caster.GetId())
    auto minionMap = caster.GetSquad();

    // 2. Рассчитываем целевую точку (позиция игрока + вектор взгляда * дистанция)
    glm::vec2 targetPos = caster.position + caster.direction * 10.0f;

    for (auto const& [id, type] : minionMap->GetSquad()) {
        uint32_t idx = server->GetPlayerIndex(id);
        // Проверка на -1 (uint32_t max)
        if (idx == (uint32_t)-1) continue;

        Character* baseChar = server->GetCharacterByIdx(idx);

        if (!baseChar) continue;

        // Пытаемся привести Character к Companion
        CompanionBase* companion = dynamic_cast<CompanionBase*>(baseChar);


        if (companion && !companion->IsDead()) {

          
              

                if (companion->GetCurrentState() == AIState::Defending) {
                    companion->SetState(AIState::Follow); // Сброс в дефолт
                }
                else {
                    companion->SetDefendMode(companion->position); // Защищать текущее место
                }
            


           
        }
    }
    return true;
}
