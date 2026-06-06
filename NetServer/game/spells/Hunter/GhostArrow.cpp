#include "GhostArrow.h"
#include "../../../Match.h"
#include "../NetShared/entities/Hunter.h"

bool GhostArrow::Cast(Character& caster, Match* server)
{

    auto* hunter = dynamic_cast<Hunter*>(&caster);
    bool hasInWorld = hunter->HasArrowInWorld(ArrowType::Ghost);

        CasterId = server->GetPlayerIndex(caster.GetId());
        basePos = caster.position;
        direction = caster.direction;


     
        // --- 2. КРАФТ ---
        if (memberArrowIdx == -1 && !hasInWorld) {
            if (hunter->CraftArrow(ArrowType::Ghost)) {
                memberArrowIdx = hunter->FindArrowInQuiver(ArrowType::Ghost);
                return true;
            }
            return false;
        }

        caster.LockMovement();
        caster.LockActions();


        memberArrowIdx = hunter->FindArrowInQuiver(ArrowType::Ghost);
        // Параметры снаряда: 
        // 1. Игнорирует коллизии мира (WorldCollision = false)
        // 2. Проходит сквозь цели (опционально)
        if (memberArrowIdx != -1) { // ОБЯЗАТЕЛЬНАЯ ПРОВЕРКА
            ProjectileParams p;
            p.ownerId = caster.GetId();
            //   ProjectileId = server->CreateProjectile(ProjectileType::Shoot, p, basePos, direction, targetRadius);
            ProjectileId = server->CreateProjectile(ProjectileType::GhostArrow, p, basePos, direction, targetRadius);

            state = SpellState::Appear;

            hunter->quiver[memberArrowIdx].ResetArrow();
            hunter->DecrementArrows();
            memberArrowIdx = -1;
            return true;
        }
        return false;
    
}

void GhostArrow::ForceFinish(int uid, Match* server)
{
    ReleaseCaster(server);

    state = SpellState::Finished;
}

void GhostArrow::ApplyHunterFlags(bool hasExplosion, bool hasBinding, bool hasGhost, bool bonusPower)
{
    HasExplosion = hasExplosion;
    HasBind = hasBinding;
    chargePower = bonusPower;
}

void GhostArrow::NotifyCharacterHit(uint32_t ProjID, uint32_t TargetID, Match* server)
{
    // Радиус: базовый 3.0 + 0.5 за каждую стрелу
    float finalRadius = 3.0f + (chargePower * 0.5f);
    auto& slot = server->projectiles[ProjectileId];
    if (HasExplosion) {
        // Используем кешированные правила взрыва (Explosion)
        ProjectileRules rules = GetProjectileRules(ProjectileType::Explosion);
        // --- МОДИФИКАТОРЫ ОТ ЗАРЯДА ---
       // Урон: базовый + 30% за каждую влитую стрелу
        float finalDamage = (float)rules.damageToPlayers * (1.5f + chargePower * 0.3f);
       

        // Если заряд максимальный (3), меняем тип урона или добавляем эффект
        bool bigBoom = (chargePower >= 3);

        rules.type = DamageType::Magical; // сделаем магический урон  -против вара-
      
        // Вызываем общую логику (задевает всех, включая мага, рушит мир)
        server->ProcessAreaDamage(slot.data.vPos, finalRadius, (float)finalDamage, ownerId, false, rules);

    }
    if (HasBind) {
        // 1. Находим всех врагов в радиусе
        float bindRadius = finalRadius;
        auto& entities = server->GetEntities();
        for (auto& pl : entities) {
            if (!pl.active || pl.character->IsDead()) continue;

            float dist = glm::distance(pl.character->position, slot.data.vPos);
            if (dist <= bindRadius) {

               // uint32_t ownerId = server->GetPlayers()[server->GetPlayerIndex(ownerId)].character->GetId();

                server->ApplyBindEffect(pl.character.get(), slot.data.vPos, -1, false);

            }
        }
    }
}

void GhostArrow::UpdateAppear(float dt, Match* server)
{
    float t = std::min(lifeTimer / appearDuration, 1.0f);


    auto& slot = server->projectiles[ProjectileId];
    auto& entities = server->GetEntities();
    auto& caster = *entities[CasterId].character;
    if (CasterId >= entities.size() || !entities[CasterId].active) {
        ReleaseCaster(server);
        return;
    }
    if (!slot.active) return;

        slot.data.fRadius = t * 0.25f;
        direction = caster.direction * 14.0f;
        slot.data.vPos = caster.position + caster.direction;
    

    if (t >= 1.0f) {
        state = SpellState::Active; 
        lifeTimer = 0.0f; 
        slot.collisionEnabled = true;
        slot.data.vVel = direction;
        ReleaseCaster(server);

      
    }
    
}

void GhostArrow::UpdateActive(float dt, Match* server)
{if (lifeTimer >= activeDuration) { state = SpellState::Disappear; lifeTimer = 0.0f; }
    
}

void GhostArrow::UpdateDisappear(float dt, Match* server)
{
    float t = 1.0f - std::min(lifeTimer / disappearDuration, 1.0f);


    auto& slot = server->projectiles[ProjectileId];
    if (slot.active) {
        slot.data.fRadius = t * 0.25f;
    }


    if (t <= 0.0f) {
       
        server->MarkProjectileForRemoval(ProjectileId);
        ReleaseCaster(server);
        state = SpellState::Finished;
    }
}
