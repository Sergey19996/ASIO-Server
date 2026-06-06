#include "BindArrowSpell.h"
#include "../../../Match.h"  // здесь уже можно подключать
#include "../NetShared/entities/Hunter.h"
#include "../../utils/WorldGrid.h"
#include "../NetShared/managers/EffectManager.h"
#include "../NetShared/managers/CooldownManager.h"


static const int LINKED_PENDING = -2;
bool BindArrowSpell::Cast(Character& caster, Match* server)
{

    auto* hunter = dynamic_cast<Hunter*>(&caster);
   /* bool isRightClick = (caster.GetLastCastSlot() == ActionSlot::Slot2);
    int arrowIdx = hunter->GetNextArrowIndex(isRightClick);*/
    // 1. Ищем ЛЮБУЮ подходящую стрелу (сначала смотрим, нет ли уже готовой взрывной)
   
    bool hasInWorld = hunter->HasArrowInWorld(ArrowType::Binding);
  

    ownerId = caster.GetId();

    // ВТОРОЕ НАЖАТИЕ (Один узел уже летит или застрял)
    if (shotCount < 2 && hasInWorld)
    {
        Spawn(caster, server); // Выстрел второго узла

        if (memberArrowIdx >= 0 && memberArrowIdx < 5) {

            // Теперь, когда обе выпущены, полностью освобождаем слот колчана
            hunter->quiver[memberArrowIdx].linkedProjectileId = -1;
            //  hunter->quiver[arrowIdx].type = ArrowType::Normal;
            hunter->quiver[memberArrowIdx].ResetArrow();
            // Теперь HasPendingActivations() вернет false и начнется релоад (если стрел 0)
            hunter->DecrementArrows();

            // Сброс и КД
            float cd = GetAdjustedCooldown(); // Мощный каст чуть дольше откатывается
            caster.GetCooldown()->Set(id, cd, server->matchTime);
            server->SendCooldownToClient(ownerId, id, cd);

        }
        return true;
    }

  
    // ЭТАП 0: КРАФТ (Если нет ни в руках, ни в полете)
    if (memberArrowIdx == -1 && !hasInWorld) {

        if (hunter->CraftArrow(ArrowType::Binding)) {
            this->memberArrowIdx = hunter->FindArrowInQuiver(ArrowType::Binding); // так как мы скравтили стрелу - мы её находим 
            return true; // Недостаточно очков для крафта
        }
        return false;
    }

    // ПЕРВОЕ НАЖАТИЕ (Стрела еще в колчане или только что выбрана)
    memberArrowIdx = hunter->FindArrowInQuiver(ArrowType::Binding);
    if (shotCount == 0 )
    {
        Spawn(caster, server); // Выстрел первого узла
        if (memberArrowIdx != -1) {
            // Помечаем стрелу как "в процессе" (неактивна, но привязана к спеллу)
            hunter->quiver[memberArrowIdx].linkedProjectileId = LINKED_PENDING;
            hunter->OnArrowFired(memberArrowIdx);
        }
        state = SpellState::Appear;
        return true;
    }

 //   this->savedArrowIdx = hunter->FindArrowInQuiver(ArrowType::Binding); // так как мы скравтили стрелу - мы её находим 

   

    return false;
}

// ФАЗА APPEAR: Пока мы ищем цели (ждем попадания обеих стрел)
void BindArrowSpell::UpdateAppear(float dt, Match* server) {
    if (nodesCount == 2) {
        state = SpellState::Active;
        lifeTimer = 0; // Сбрасываем таймер для фазы Active
    }
}

// ФАЗА ACTIVE: Связь установлена
void BindArrowSpell::UpdateActive(float dt, Match* server) {
    // Если время вышло — удаляем спелл
    if (lifeTimer > activeDuration) {
        state = SpellState::Disappear;
        return;
    }

    // Здесь можно оставить только визуальные эффекты (проверка дистанции для разрыва цепи)
    glm::vec2 p1 = GetNodePos(0, server);
    glm::vec2 p2 = GetNodePos(1, server);

    if (glm::distance(p1, p2) > 10.0f) { // Лимит разрыва
        state = SpellState::Disappear;
    }
}

// ФАЗА DISAPPEAR: Очистка эффектов
void BindArrowSpell::UpdateDisappear(float dt, Match* server) {
    auto& entities = server->GetEntities();
    for (int i = 0; i < 2; ++i) {
        if (!nodes[i].isStatic) { 
            if (nodes[i].entityId < entities.size()) {
                if (auto& c = entities[nodes[i].entityId].character) {
                    c->GetEffects()->Remove(StatusEffectType::Linked);
                }
            }
        }
    }
    for (int i = 0; i < projectileIds.size(); i++){
            server->MarkProjectileForRemoval(projectileIds[i]);

    }
    state = SpellState::Finished;
    shotCount = 0;
}

void BindArrowSpell::NotifyCharacterHit(uint32_t ProjID, uint32_t TargetID, Match* server) {

   
    // 1. Проверяем, наш ли это снаряд
    auto it = std::find(projectileIds.begin(), projectileIds.end(), (uint16_t)ProjID);
    if (it == projectileIds.end()) return;
  
    int targetIdx = -1;

   
    if (HasExplosion) {
        // Используем кешированные правила взрыва (Explosion)
        ProjectileRules rules = GetProjectileRules(ProjectileType::Explosion);
        // --- МОДИФИКАТОРЫ ОТ ЗАРЯДА ---
       // Урон: базовый + 30% за каждую влитую стрелу
        float finalDamage = (float)rules.damageToPlayers * (1.5f + chargePower * 0.3f);
        // Радиус: базовый 3.0 + 0.5 за каждую стрелу
        float finalRadius = 3.0f + (chargePower * 0.5f);

        // Если заряд максимальный (3), меняем тип урона или добавляем эффект
        bool bigBoom = (chargePower >= 3);

        rules.type = DamageType::Magical; // сделаем магический урон  -против вара-
        // Ссылка на данные снаряда в сервере для получения позиции
        auto& projSlot = server->projectiles[ProjID];
        // Вызываем общую логику (задевает всех, включая мага, рушит мир)
        server->ProcessAreaDamage(projSlot.data.vPos, finalRadius, (float)finalDamage, ownerId, false, rules);

    }

    // 1. Ищем: может этот снаряд уже занял слот как статика (ловушка)?
    for (int i = 0; i < nodesCount; i++) {
        if (nodes[i].isStatic && nodes[i].associatedProjId == ProjID) {
            targetIdx = i;
            break;
        }
    }
  
    // 2. Если это новый хит (стрела еще летела)
    if (targetIdx == -1) {
        if (nodesCount >= 2) return; // Спелл уже полон
        targetIdx = nodesCount;
        nodesCount++;
    }

    // 3. ПРЕВРАЩАЕМ УЗЕЛ В ПЕРСОНАЖА (даже если он был статикой)
    nodes[targetIdx].isStatic = false;
    nodes[targetIdx].entityId = TargetID;
    nodes[targetIdx].associatedProjId = 0; // Больше не ловушка

    // 4. ГАРАНТИРУЕМ: Персонаж всегда в nodes[0], если второй узел - стена
    if (targetIdx == 1 && nodes[0].isStatic) {
        LinkNode temp = nodes[0];
        nodes[0] = nodes[1];
        nodes[1] = temp;
     
    }

    // Удаляем снаряд из мира, так как он попал в цель
    server->MarkProjectileForRemoval(ProjID);

    // Удаляем ID из списка летящих (чтобы не обрабатывать повторно)
    projectileIds.erase(it);

    CheckAndApplyLink(server);

    // Если этот снаряд был "основным", зануляем его
    if (projectileId == (int)ProjID) projectileId = -1;
}

void BindArrowSpell::NotifyWorldHit(glm::ivec2 cell, Match* server) {
    if (nodesCount >= 2) return;

    if (HasGhost && projectileId != -1) {
        server->projectiles[projectileId].cachedRules.ignoresWorld = true;
        return;
    }

    // Сохраняем позицию (ЦЕНТР клетки для точности)
    nodes[nodesCount].startPos = glm::vec2(cell) + 0.5f;
    nodes[nodesCount].isStatic = true;
    nodes[nodesCount].associatedProjId = projectileId;

    if (HasExplosion) {
        ProjectileRules rules = GetProjectileRules(ProjectileType::Explosion);
        float finalDamage = (float)rules.damageToPlayers * (1.5f + chargePower * 0.3f);
        float finalRadius = 3.0f + (chargePower * 0.5f);
        rules.type = DamageType::Magical;

        // Взрыв в центре стены
        server->ProcessAreaDamage(nodes[nodesCount].startPos, finalRadius, finalDamage, ownerId, false, rules);
    }

    // Удаляем снаряд, так как он "воткнулся"
  //  if (projectileId != -1) server->MarkProjectileForRemoval(projectileId);

    nodesCount++;
    projectileId = -1; // Сбрасываем текущий активный ID для следующего выстрела
    CheckAndApplyLink(server);
}

bool BindArrowSpell::OwnsProjectile(int uid)
{

    // Цикл по фактическому количеству элементов в векторе
    for (size_t i = 0; i < projectileIds.size(); i++)
    {
        if (projectileIds[i] == (uint16_t)uid) {
            return true;
        }
    }
    return false;
}

void BindArrowSpell::ApplyHunterFlags(bool hasExplosion, bool hasBinding, bool hasGhost, bool bonusPower)
{
    HasExplosion = hasExplosion;
    HasGhost = hasGhost;
    chargePower = bonusPower;

   
}

uint32_t BindArrowSpell::GetLinkedEntityFor(uint32_t victimId) {
    if (state != SpellState::Active) return -1;
    if (!nodes[0].isStatic && nodes[0].entityId == victimId)  return nodes[1].isStatic ? -1 : nodes[1].entityId;

    if (!nodes[1].isStatic && nodes[1].entityId == victimId)  return nodes[0].isStatic ? -1 : nodes[0].entityId;

    return UINT32_MAX;
}

void BindArrowSpell::ForceFinish(int uid, Match* server)
{
    state = SpellState::Finished;
    UpdateDisappear(0, server);
}

void BindArrowSpell::CheckAndApplyLink(Match* server)
{
    if (nodesCount != 2 || bHasLinked) return; // bHasLinked — новый флаг-предохранитель

    glm::vec2 p1 = GetNodePos(0, server);
    glm::vec2 p2 = GetNodePos(1, server);

    // 1. ОБА — Персонажи
    if (!nodes[0].isStatic && !nodes[1].isStatic) {
        auto& charA = server->GetEntities()[nodes[0].entityId].character;
        auto& charB = server->GetEntities()[nodes[1].entityId].character;
        bHasLinked = true;
        server->ApplyBindEffect(charA.get(), p2, nodes[1].entityId, true);
        server->ApplyBindEffect(charB.get(), p1, nodes[0].entityId, true);
    }
    // 2. ПЕРВЫЙ — Персонаж, ВТОРОЙ — Стена
    else if (!nodes[0].isStatic && nodes[1].isStatic) {
        bHasLinked = true;
        auto& charA = server->GetEntities()[nodes[0].entityId].character;
        server->ApplyBindEffect(charA.get(), p2, -1, false);
    }
    // 3. ПЕРВЫЙ — Стена, ВТОРОЙ — Персонаж
    else if (nodes[0].isStatic && !nodes[1].isStatic) {
        auto& charB = server->GetEntities()[nodes[1].entityId].character;
        bHasLinked = true;
        server->ApplyBindEffect(charB.get(), p1, -1, false);
    }// стена ? стена
    else if (nodes[0].isStatic && nodes[1].isStatic) {
        // НИЧЕГО НЕ ДЕЛАЕМ
        return;
    }
    
    state = SpellState::Active; // Переходим в фазу удержания
}

glm::vec2 BindArrowSpell::GetNodePos(int idx, Match* server) {
    if (nodes[idx].isStatic) return nodes[idx].startPos;
    if (auto& c = server->GetEntities()[nodes[idx].entityId].character) return c->position;
    return nodes[idx].startPos;
}

void BindArrowSpell::Spawn(Character& caster, Match* server) {
    ProjectileParams p;
    p.ownerId = caster.GetId();

    // 1. ИНТЕГРАЦИЯ: Используем фабрику сервера
    // Метод выделит индекс, настроит правила StickyBomb, обнулит флаги 
    // и добавит снаряд в очередь на обновление.
    float radius = 0.5f;
 

    projectileId = server->CreateProjectile(ProjectileType::BindShot, p, caster.position, caster.direction, radius);
   

    // 3. Тюнинг параметров, специфичных для броска бомбы
    auto& slot = server->projectiles[projectileId];
    slot.data.vVel = caster.direction * 12.0f;
    slot.data.fRadius = 1.0f;
    slot.data.bStuck = false; // На старте она всегда летит
    slot.collisionEnabled = true;
    projectileIds.push_back(projectileId);

    shotCount++;
    
}