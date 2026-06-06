#include "RangerMob.h"
#ifdef GAME_SERVER
#include "../../NetServer/Match.h"
#endif
RangerMob::RangerMob(uint32_t id, const std::string& name, glm::vec2 spawn, Match* server) : AIBase(id, name, server)
{
    this->position = spawn;
#ifdef GAME_SERVER
    this->attackRange = 3.0f;
    this->stopRange = 1.25f; // Держим дистанцию
    this->defaultSpell = SpellId::ArrowShot;

    this->spawnPoint = spawn;
    this->baseSpeed = 0.5f;
    chaseRange = 5.0f;
#endif
    this->maxHealth =25;
    this->health = maxHealth;
    this->entityType = ArchetypeId::Mob_Ranged;
    this->radius = 0.25f;

    m_classId = "Mob_Range"_sid;

}
#ifdef GAME_SERVER
void RangerMob::UpdateAI(float dt)
{
    float timeNow = server->matchTime;

    // Логика почти как у Melee, но с проверкой stopRange
   

    switch (currentState) {
    case AIState::Idle:
        // Ищем цель в радиусе 10 единиц
        targetIdx = server->FindNearestEnemy(position, this->id, chaseRange);
        if (targetIdx !=-1) {
            currentState = AIState::Chase;
        }
        inputVel = { 0, 0 }; // Стоим
        break;

    case AIState::Chase: {
        Character* target = server->GetCharacterByIdx(targetIdx);
        if (!target || target->IsDead()) {
            targetIdx = -1;
            currentState = AIState::Return;
            return;
        }

        float dist = glm::distance(position, target->position);
        float minPossibleDist = this->radius + target->radius + 0.05f;

        float distToSpawn = glm::distance(position, spawnPoint);
        // Если подошли вплотную — атакуем
        if (dist < attackRange && dist > stopRange) {
            inputVel = { 0, 0 }; // Идеальная позиция для стрельбы
            currentState = AIState::Attack;
        }
        else if (dist <= stopRange || dist < minPossibleDist) {
            // Если игрок подошел слишком близко — моб должен отступить или стоять
            inputVel = glm::normalize(position - target->position) * baseSpeed; // по сути это вектор самого моба - которыи к нам идет
            // Можно добавить: inputVel = glm::normalize(position - target->position) * baseSpeed; // Кайтинг
        }
        // Если моб убежал далеко от дома
        else if (distToSpawn > 5.0f) {
            currentState = AIState::Return;
        }
        else {
            // Идем к цели
          // Обычное сближение
            inputVel = glm::normalize(target->position - position);
            direction = inputVel;
        }
        break;
    }

    case AIState::Attack: {
        Character* target = server->GetCharacterByIdx(targetIdx);
        if (!target || target->IsDead()) {
            currentState = AIState::Idle;
            return;
        }
        // Логика атаки
        attackTimer += dt;
        if (attackTimer > 0.5f) {
            if (CanCast(defaultSpell, timeNow)) {
                server->HandleCastSpell(this->id, defaultSpell, 0); // 0 если нет connectionId
                attackTimer = 0.0f;
            }
        }
        // После удара проверяем, не убежал ли игрок
        float dist = glm::distance(position, target->position);
        if (dist < stopRange || dist > attackRange) {
            currentState = AIState::Chase;
        }
        inputVel = { 0, 0 }; // Во время атаки не идем
        direction = glm::normalize(target->position - position);
        break;
    }

    case AIState::Return: {
        float dist = glm::distance(position, spawnPoint);
        if (dist < 1.5f) {
            currentState = AIState::Idle;
        }
        else {
            inputVel = glm::normalize(spawnPoint - position);
            direction = inputVel;
        }
        break;
    }
    }
}

#endif