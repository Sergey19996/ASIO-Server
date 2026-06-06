#include "StickyBombSpell.h"
#include "../../../Match.h"  // здесь уже можно подключать
//#include "../../entities/Hunter.h"
#include "../NetShared/entities/Hunter.h"

bool StickyBombSpell::Cast(Character& caster, Match* server)
{
    auto* hunter = dynamic_cast<Hunter*>(&caster);
    if (!hunter) return false;

    ownerId = caster.GetId();
    bool hasInWorld = hunter->HasArrowInWorld(ArrowType::Explosive);

    // --- 1. ВЗРЫВ ---
    if (projectileId != -1 && hasInWorld)
    {
        // Проверяем индекс ПЕРЕД использованием
        if (memberArrowIdx >= 0 && memberArrowIdx < 5) {
            hunter->quiver[memberArrowIdx].linkedProjectileId = -1;
            chargePower = hunter->quiver[memberArrowIdx].bonusPower;
            Explode(server);
            hunter->quiver[memberArrowIdx].ResetArrow();
            hunter->DecrementArrows();
            memberArrowIdx = -1;
            return true;
        }
    }

    // --- 2. КРАФТ ---
    if (memberArrowIdx == -1 && !hasInWorld) {
        if (hunter->CraftArrow(ArrowType::Explosive)) {
            memberArrowIdx = hunter->FindArrowInQuiver(ArrowType::Explosive);
            return true;
        }
        return false;
    }

    // --- 3. ВЫСТРЕЛ ---
    // Сначала ищем индекс, и ТОЛЬКО ЕСЛИ нашли - работаем
    memberArrowIdx = hunter->FindArrowInQuiver(ArrowType::Explosive);

    if (memberArrowIdx != -1) { // ОБЯЗАТЕЛЬНАЯ ПРОВЕРКА
        Spawn(caster, server);
        hunter->quiver[memberArrowIdx].linkedProjectileId = this->projectileId;
        hunter->OnArrowFired(memberArrowIdx);
        state = SpellState::Active;

        olc::net::message<GameMsg> msg;
        msg.header.id = GameMsg::Server_CastStart;
        msg << sCastStart{ caster.GetId(), (uint8_t)SpellId::StickyBomb };
        // server->BroadcastMessage(msg); // Или BroadcastToVisible
        server->MessageAllMatchClients(msg); //анимация старта каста

        return true;
    }

    return false;
}

void StickyBombSpell::Update(float dt, Match* server)
{
    lifeTimer += dt;

    if (state != SpellState::Active || stuck || projectileId == 0)
        return;

    auto& slot = server->projectiles[projectileId];

    // Если снаряд был уничтожен извне (например, сбит другим снарядом)
    if (!slot.active)
    {
        state = SpellState::Finished;
        projectileId = 0;
        return;
    }

 /*   if (slot.data.bStuck)
    {
        stuck = true;
        slot.data.vVel = { 0, 0 };
    }*/
}

bool StickyBombSpell::OwnsProjectile(int uid)
{
    return uid == projectileId;
}

void StickyBombSpell::ForceFinish(int uid, Match* server)
{

    Explode(server);
    //if (uid == projectileId)
    //{
    //   
    //   // Explode(server);
    //}
}

void StickyBombSpell::NotifyWorldHit(glm::ivec2 cell, Match* server)
{
    /*if (slot.data.bStuck)
    {*/
        stuck = true;
  /*      slot.data.vVel = { 0, 0 };
    }*/
}

void StickyBombSpell::ApplyHunterFlags(bool hasExplosion, bool hasBinding, bool hasGhost, bool bonusPower)
{
    HasBind = hasBinding;
    HasGhost = hasGhost;

}

void StickyBombSpell::Spawn(Character& caster, Match* server)
{
    // 1. ИНТЕГРАЦИЯ: Используем фабрику сервера
    // Метод выделит индекс, настроит правила StickyBomb, обнулит флаги 
    // и добавит снаряд в очередь на обновление.
    float radius = 0.5f;
  //  float balanceBonus = caster.GetSpellPowerBonus(SpellId::Fireball);

    //// 1. Создаем снаряд с базовым уроном мага
    ProjectileParams p;
    p.ownerId = caster.GetId();
  //  p.damageMod = caster.GetDamageMultiplier(); // тот самый 1.0 - 2.2x
  //  p.effectMod = balanceBonus;
    uint16_t worldId = server->CreateProjectile(ProjectileType::StickyBomb, p, caster.position, caster.direction, radius);

    // 2. Проверка на свободные места в пуле (2026 стандарт)
    if (worldId == -1) {
        return;
    }

    // 3. Тюнинг параметров, специфичных для броска бомбы
    auto& slot = server->projectiles[worldId];
    slot.data.vVel = caster.direction * 10.0f;
    slot.data.fRadius = 0.125f;
    slot.data.bStuck = false; // На старте она всегда летит
    slot.collisionEnabled = true;

    // 4. Запоминаем индекс в объекте заклинания для последующего взрыва
    projectileId = worldId;
}

void StickyBombSpell::Explode(Match* server)
{
 auto& slot = server->projectiles[projectileId];
    if (!slot.active) return;

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
    
    // Вызываем общую логику (задевает всех, включая мага, рушит мир)
    server->ProcessAreaDamage(slot.data.vPos, finalRadius, (float)finalDamage, ownerId, false, rules);

    if (HasBind) {
    // 1. Находим всех врагов в радиусе
    float bindRadius = finalRadius;
    auto& entities = server->GetEntities();
    for (auto& pl : entities) {
        if (!pl.active || pl.character == nullptr || pl.character->IsDead()) continue;

        float dist = glm::distance(pl.character->position, slot.data.vPos);
        if (dist <= bindRadius) {

          //  uint32_t ownerId = server->GetPlayers()[server->GetPlayerIndex(ownerId)].character->GetId();

            server->ApplyBindEffect(pl.character.get(), slot.data.vPos, -1, false);

        }
    }
    }
    server->MarkProjectileForRemoval(projectileId);
    projectileId = 0;
    state = SpellState::Finished;
}

