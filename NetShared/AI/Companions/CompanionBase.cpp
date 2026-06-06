#include "CompanionBase.h"
#ifdef GAME_SERVER
#include "../../../NetServer/Match.h"
#include "../../managers/SquadManager.h"

namespace {
    static constexpr glm::vec2 FORMATION_OFFSETS[] = {
        { 0.0f,  0.5f},  // 0: Фронт центр
        {-1.0f,  0.5f},  // 1: Фронт лево
        { 1.0f,  0.5f},  // 2: Фронт право
        {-0.5f,  -0.5f},  // 3: Фланг лево
        { 0.5f,  -0.5f},  // 4: Фланг право
        {-0.2f, -1.0f},  // 5: Тыл лево
        { 0.2f, -1.0f},  // 6: Тыл право
        { 0.0f, -1.0f}   // 7: Тыл центр
    };
}
#endif


CompanionBase::CompanionBase(uint32_t id, const std::string& name, Character* owner, Match* server) : AIBase(id, name, server)
{
       // entityType = EntityType::Companion; // Или Boss, если хотим больше HP
     
        

#ifdef GAME_SERVER
        this->owner = owner;
        this->teamId = owner->teamId;
        followRange = 0.5f;           // Дистанция комфорта рядом с игроком
        teleportRange = 10.0f;        // Если игрок слишком далеко — телепорт
        // В конструкторе компаньона
        pathUpdateTimer = (rand() % 100) / 500.0f; // Случайная задержка 0-0.2 сек
#endif
}
#ifdef GAME_SERVER
void CompanionBase::UpdateAI(float dt)
{
    if (!owner || owner->IsDead()) {
        inputVel = { 0,0 };
        return;
    }

    // Обновляем цель один раз за кадр
    UpdateTarget(dt);

    glm::vec2 offset = FORMATION_OFFSETS[this->formationIndex % 8];
    glm::vec2 forward = owner->direction;
    if ((forward.x * forward.x + forward.y * forward.y) < 0.000001f) {
        forward = { 0, 1 };
    }
    glm::vec2 right = { forward.y, -forward.x };
    // Учитываем радиус владельца, чтобы оффсет начинался от "границы" тела
    float personalSpace = owner->radius + this->radius;

    // Итоговая целевая точка
    glm::vec2 targetPos = owner->position +
        forward * (offset.y + personalSpace) +
        right * (offset.x);

    float distToPoint = glm::distance(position, targetPos);  // Дистанция до Места
    float realDistToOwner = glm::distance(position, owner->position); // Дистанция до ТЕЛА

    float distToTarget = 0.0f;
    // Глобальная проверка (прерывает любое состояние)
    if (realDistToOwner > teleportRange) {
        position = owner->position;
        currentState = AIState::Follow;
        potentialTarget = nullptr;
        return;
    }

    switch (currentState) {
        // Команда 1: Идем в точку, игнорируем формацию
    case AIState::MoveToPoint: {
        float dist = glm::distance(position, commandTargetPos);
        if (dist < 0.5f) {
            if (returnToOwnerAfterMove) {
                currentState = AIState::Follow;
            }
            else {
                currentState = AIState::Idle;
            }
        }
        else {
            MoveAlongPath(commandTargetPos, dt);
            inputVel = direction;
        }
        break;
    }

                             // Команда 2: Оборона позиции
    case AIState::Defending: {
        float distToPost = glm::distance(position, commandTargetPos);

        // Если слишком далеко от поста — возвращаемся на него
        if (distToPost > 1.0f) {
            direction = glm::normalize(commandTargetPos - position);
            inputVel = direction;
        }
        else {
            inputVel = { 0, 0 };
        }

        // Здесь можно добавить поиск цели ТОЛЬКО в радиусе поста
        // potentialTarget = FindTargetNear(commandTargetPos, 5.0f);
        if (potentialTarget) currentState = AIState::Chase;
        break;
    }
    case AIState::Idle: {

        inputVel = { 0,0 };
        // 1. Приоритет: есть ли цель от игрока или рядом?
      //  potentialTarget = FindTarget();
        if (potentialTarget) {
            currentState = AIState::Chase;
        }
        // 2. Если врагов нет, проверяем, не пора ли догонять хозяина
        else if (distToPoint > followRange) {
            currentState = AIState::Follow; // Нужно добавить такое состояние
        }
        break;

    }
    case AIState::Follow: {


        if (distToPoint <= followRange) {
            currentState = AIState::Idle;
            inputVel = { 0,0 };
        }
        else {
            MoveAlongPath(targetPos, dt);
           
        }
        // По пути проверяем врагов
     //   if (FindTarget()) currentState = AIState::Chase;
        break;

    }
    case AIState::Chase: {

        if (!potentialTarget || potentialTarget->IsDead()) {
            currentState = AIState::Follow;
            break;
        }
        UpdateChase(dt); // Вызовется версия либо базы, либо Ranger
        break;

    }

    case AIState::Attack: {

        //inputVel = { 0, 0 }; // Во время атаки не идем
        //attackTimer += dt;
        //if (attackTimer > 0.5f) {
        //    ExecuteAttack(potentialTarget);
        //    attackTimer = 0.0f;
        //}



      //  Character* target = server->GetCharacterByIdx(targetIdx);
        if (!potentialTarget || potentialTarget->IsDead()) {
            currentState = AIState::Idle;
            return;
        }
        // Логика атаки
        attackTimer += dt;
        if (attackTimer > 0.5f) {
            ExecuteAttack(potentialTarget);
            attackTimer = 0.0f;
        }
        // После удара проверяем, не убежал ли игрок
        if (glm::distance(position, potentialTarget->position) > 1.5f) {
            currentState = AIState::Chase;
        }
        inputVel = { 0, 0 }; // Во время атаки не идем
        direction = glm::normalize(potentialTarget->position - position);
        break;


        break;
    }
    }
    

}
void CompanionBase::UpdateChase(float dt)
{
   
    float distToTarget = glm::distance(position, potentialTarget->position);
    if (distToTarget <= attackRange) {
        currentState = AIState::Attack;
    }
    else {
        direction = glm::normalize(potentialTarget->position - position);
        inputVel = direction;
    }
   
}
void CompanionBase::UpdateTarget(float dt)
{
    potentialTarget = nullptr;

    // 1. Проверяем, есть ли "приоритетная" цель (обидчик)
    if (targetNetId != 0) {
        if (targetTimer > 0.0f) {
            uint32_t idx = server->GetPlayerIndex(targetNetId);
       
            if (idx != -1) {
                auto& ent = server->GetEntities()[idx];
                if (ent.active && ent.character && !ent.character->IsDead() && ent.character->teamId != this->teamId) {
                    potentialTarget = ent.character.get();
                    targetTimer -= dt;
                    return; // Приоритетная цель найдена, выходим
                }
            }
        }
        // Если таймер вышел или цель невалидна — сбрасываем
        targetNetId = 0;
        targetTimer = 0.0f;
    }

    // 2. Если приоритетной цели нет — ищем ближайшего врага (автономный поиск)
    uint32_t nearestIdx = server->FindNearestEnemy(this->position, this->id, this->chaseRange);
    if (nearestIdx != -1) {
        potentialTarget = server->GetCharacterByIdx(nearestIdx);
    }
}
//Character* CompanionBase::FindTarget()
//{
//    
//    if (!server || !owner) return nullptr;
//
//    // Ищем ближайшего врага в радиусе погони (chaseRange)
//    // Используем позицию компаньона как центр поиска
//    uint16_t targetIdx = server->FindNearestEnemy(this->position, this->id, this->chaseRange);
//
//    if (targetIdx != 0xFFFF) {
//        return server->GetCharacterByIdx(targetIdx);
//    }
//
//    return nullptr;
//    
//}
void CompanionBase::SetTarget(uint32_t id) 
{
    // Если нам передали 0 или наш собственный ID — игнорируем
    if (id == 0 || id == this->id) return;

    this->targetNetId = id;
    this->targetTimer = 2.5f; // Даем компаньону 5 секунд "ярости" на эту цель
}
void CompanionBase::ExecuteAttack(Character* target)
{
    float timeNow = server->matchTime;
    
   
   
        if (CanCast(defaultSpell, timeNow)) {
            server->HandleCastSpell(this->id, defaultSpell, 0); // 0 если нет connectionId
           
    }
}
void CompanionBase::MoveAlongPath(glm::vec2 targetPos, float dt)
{
    pathUpdateTimer += dt;

    // 1. Обновляем A*, если пора или пути нет
    if (pathUpdateTimer > 0.2f || currentPath.empty()) {
        currentPath = server->level.FindPath(position, targetPos);
        pathUpdateTimer = 0.0f;
    }

    if (currentPath.empty()) {
        inputVel = { 0, 0 };
        return;
    }

    // 2. --- RAYCAST (Срезание углов) ---
    // Проверяем, видим ли мы точку чуть дальше по пути (например, 4-ю или 5-ю)
    // Это заставит компаньона идти по прямой, а не по центрам клеток.
    int lookAhead = std::min((int)currentPath.size() - 1, 5);
    for (int i = lookAhead; i > 0; --i) {
        // Вызываем проверку видимости с учетом радиуса компаньона
        if (server->level.IsPathClear(position, currentPath[i], this->radius)) {
            // Если видим дальнюю точку — удаляем все промежуточные "костыли"
            currentPath.erase(currentPath.begin(), currentPath.begin() + i);
            break;
        }
    }

    // 3. Проверяем достижение текущей точки
    float distToWaypoint = glm::distance(position, currentPath[0]);
    if (distToWaypoint < 0.2f) {
        currentPath.erase(currentPath.begin());
        if (currentPath.empty()) {
            inputVel = { 0,0 };
            return;
        }
    }

    // 4. Движение
    direction = glm::normalize(currentPath[0] - position);
    inputVel = direction;
}
void CompanionBase::OnUnderAttack(uint32_t attackerId)
{
    SetTarget(attackerId);
}
void CompanionBase::SetFormationIndex(uint8_t idx)
{
    formationIndex = idx;
}
void CompanionBase::SetTemporaryGoal(glm::vec2 pos, bool returnAfter)
{
    commandTargetPos = pos;
    returnToOwnerAfterMove = returnAfter;
    currentState = AIState::MoveToPoint;
}
void CompanionBase::SetDefendMode(glm::vec2 pos)
{
    commandTargetPos = pos;
    currentState = AIState::Defending;
}
void CompanionBase::OnBeforeDestroy()
{
    if (owner && owner->GetSquad()) {
        owner->GetSquad()->RemoveCompanion(this->id);
    }
}
#endif
