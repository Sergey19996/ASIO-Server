#include "PriestCompanion.h"
#ifdef GAME_SERVER
#include "../../../NetServer/Match.h"
#include "managers/SquadManager.h"
#endif

PriestCompanion::PriestCompanion(uint32_t id, Character* owner, Match* s): CompanionBase(id, "priest", owner, s) {
    maxHealth = 50;
    health = maxHealth;
    radius = 0.25f;
    m_classId = "Comp_Priest"_sid;
    entityType = ArchetypeId::Companion_Priest;

#ifdef GAME_SERVER
    chaseRange = 5.0f;
    this->attackRange = 3.0f;
    this->stopRange = 1.25f; // Держим дистанцию

    defaultSpell = SpellId::ArrowShot; // или ваш аналог
#endif

}
#ifdef GAME_SERVER
void PriestCompanion::UpdateChase(float dt)
{
    if (!potentialTarget) return;

    float dist = glm::distance(position, potentialTarget->position);

    // Если цель почти здорова (например, > 95%), прекращаем погоню
    if (potentialTarget->health >= potentialTarget->maxHealth * 0.95f) {
        potentialTarget = nullptr;
        currentState = AIState::Follow;
        return;
    }

    if (dist > attackRange) {
        direction = glm::normalize(potentialTarget->position - position);
        inputVel = direction;
    }
    else if (dist < stopRange) {
        inputVel = { 0, 0 }; // Остановка, если подошли вплотную
        currentState = AIState::Attack;
    }
    else {
        currentState = AIState::Attack;
    }
}
void PriestCompanion::UpdateTarget(float dt)
{
    potentialTarget = nullptr;
    // 1. Приоритет — хозяин, если он ранен
    if (owner && owner->health < owner->maxHealth) {
       potentialTarget =  owner;
       return;
    }

    // 2. Если хозяин здоров, ищем раненых союзников в радиусе (например, 8 метров)
    float searchRadius = 8.0f;
    Character* weakestAlly = nullptr;
    float minHealthRatio = 1.0f;

    // 1. Получаем список подконтрольных юнитов владельца (caster.GetId())
    auto minionMap = owner->GetSquad()->GetSquad();
    


    for (auto const& [id, type] : minionMap) { // лучше у хозяина взять список компаньонов и по нему пробежаться 

        uint32_t idx = server->GetPlayerIndex(id);
        // Проверка на -1 (uint32_t max)
        if (idx == (uint32_t)-1) continue;
        Character* c = server->GetCharacterByIdx(idx);



        if (!c || c->IsDead()) continue;

        float dist = glm::distance(this->position, c->position);
        if (dist <= searchRadius) {
            float healthRatio = c->health / (float)c->maxHealth;
            if (healthRatio < minHealthRatio) {
                minHealthRatio = healthRatio;
                weakestAlly = c;
            }
        }
    }
    potentialTarget = weakestAlly;
}

void PriestCompanion::ExecuteAttack(Character* target)
{
    if (!target) return;

    // Логика лечения
    int healAmount = 10;
    target->health = std::min(target->maxHealth, target->health + healAmount);


    // Создаем отчет о лечении
    sDamageReport healReport;
    healReport.nVictimId = target->GetId();
    healReport.nAttackerId = this->GetId();
    healReport.Amount = healAmount;
    healReport.nType = 2; // Тип 2 = Лечение
    healReport.bResisted = false;
    

  server->damageQueue.push_back(healReport); // Добавляем в общую очередь

    // Можно отправить сетевой эффект (хил-вспышка)
    // server->BroadcastEffect(VisualEffect::Heal, target->position);
}
#endif