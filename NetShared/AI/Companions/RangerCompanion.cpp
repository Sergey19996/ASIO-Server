#include "RangerCompanion.h"

RangerCompanion::RangerCompanion(uint32_t id, Character* owner, Match* s) : CompanionBase(id, "Ranger", owner, s) {
    maxHealth = 40;
    health = maxHealth;
    radius = 0.3f;
    m_classId = "Comp_Hunter"_sid;
    entityType = ArchetypeId::Companion_Hunter;

#ifdef GAME_SERVER
    chaseRange = 5.0f;
    this->attackRange = 3.0f;
    this->stopRange = 1.25f; // Держим дистанцию

    defaultSpell = SpellId::ArrowShot; // или ваш аналог
#endif

}
#ifdef GAME_SERVER
void RangerCompanion::UpdateChase(float dt)
{
   

    float dist = glm::distance(position, potentialTarget->position);
    float minPossibleDist = this->radius + potentialTarget->radius + 0.05f;

    float realDistToOwner = glm::distance(position, owner->position); // Дистанция до ТЕЛА
    // Если подошли вплотную — атакуем
    if (dist < attackRange && dist > stopRange) {
        inputVel = { 0, 0 }; // Идеальная позиция для стрельбы
        currentState = AIState::Attack;
    }
    else if (dist <= stopRange || dist < minPossibleDist) {
        // Если игрок подошел слишком близко — моб должен отступить или стоять
        inputVel = glm::normalize(position - potentialTarget->position) * baseSpeed; // по сути это вектор самого моба - которыи к нам идет
        // Можно добавить: inputVel = glm::normalize(position - target->position) * baseSpeed; // Кайтинг
    }
    // Если моб убежал далеко от дома
    else if (realDistToOwner > 5.0f) {
        currentState = AIState::Follow;
    }
    else {
        // Идем к цели
      // Обычное сближение
        inputVel = glm::normalize(potentialTarget->position - position); // вектор от нашей  позиции к таргету
        direction = inputVel;
    }
   
}
#endif