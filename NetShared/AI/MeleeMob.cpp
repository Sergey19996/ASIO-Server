#include "MeleeMob.h"
#ifdef GAME_SERVER
#include "../../NetServer/Match.h"
#endif
MeleeMob::MeleeMob(uint32_t id, const std::string& name, glm::vec2 spawn, Match* server) : AIBase(id, name, server)
{
    
        this->position = spawn;
#ifdef GAME_SERVER
        this->spawnPoint = spawn;
        this->baseSpeed = 0.35f;
        chaseRange = 2.0f;
#endif
        this->maxHealth = 25;
        this->health = maxHealth;
        this->entityType = ArchetypeId::Mob_Melee;
        this->radius = 0.25f;

        m_classId = "Mob_Melee"_sid;
    
}

#ifdef GAME_SERVER
void MeleeMob::UpdateAI(float dt)
{
    float timeNow = server->matchTime;

    switch (currentState) {
    case AIState::Idle: {
        // Ищем цель в радиусе chaseRange (например, 10 единиц)
        targetIdx = server->FindNearestEnemy(position, this->id, chaseRange);
        if (targetIdx != -1) {
            currentState = AIState::Chase;
        }
        inputVel = { 0.0f, 0.0f }; // Стоим на месте
        break;
    }

    case AIState::Chase: {
        Character* target = server->GetCharacterByIdx(targetIdx);
        if (!target || target->IsDead()) {
            targetIdx = -1;
            currentState = AIState::Return;
            break; // Заменили return на break, чтобы управление шло дальше
        }

        float dist = glm::distance(position, target->position);
        float distToSpawn = glm::distance(position, spawnPoint);

        // Если подошли вплотную — атакуем
        if (dist < 1.2f) {
            currentState = AIState::Attack;
            inputVel = { 0.0f, 0.0f };
        }
        // Лечим баг: радиус возврата (Leash Range) обязан быть больше или равен радиусу агры
        else if (distToSpawn > (chaseRange + 2.0f)) {
            targetIdx = -1;
            currentState = AIState::Return;
        }
        else {
            // Идем к цели
            inputVel = glm::normalize(target->position - position);
            direction = inputVel;
        }
        break;
    }

    case AIState::Attack: {
        Character* target = server->GetCharacterByIdx(targetIdx);
        if (!target || target->IsDead()) {
            targetIdx = -1;
            currentState = AIState::Idle;
            inputVel = { 0.0f, 0.0f };
            break;
        }

        // Поворачиваемся к цели даже во время замаха
        direction = glm::normalize(target->position - position);
        inputVel = { 0.0f, 0.0f }; // Во время атаки не движемся

        // Оставляем проверку только через глобальную систему заклинаний (кулдауны внутри CanCast)
        if (CanCast(SpellId::MeleeHit, timeNow)) {
            server->HandleCastSpell(this->id, SpellId::MeleeHit, 0); // 0 — без connectionId
        }

        // После попытки удара проверяем, не вышел ли игрок за радиус удара с небольшим запасом (хитбокс)
        if (glm::distance(position, target->position) > 1.5f) {
            currentState = AIState::Chase;
        }
        break;
    }

    case AIState::Return: {
        float distToSpawn = glm::distance(position, spawnPoint);
        if (distToSpawn < 0.5f) { // Снизили до 0.5f, чтобы моб возвращался точнее в центр
            currentState = AIState::Idle;
            inputVel = { 0.0f, 0.0f };
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