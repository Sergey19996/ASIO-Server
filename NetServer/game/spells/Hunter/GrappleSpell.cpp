#include "GrappleSpell.h"
#include "../../../Match.h"  // здесь уже можно подключать
#include "../../utils/WorldGrid.h"
bool GrappleSpell::Cast(Character& caster, Match* server)
{
    direction = caster.direction * 16.0f;
   // caster.LockMovement();

    float baseRadius = 0.3f;


    //// 1. Создаем снаряд с базовым уроном мага
    ProjectileParams p;
    p.ownerId = caster.GetId();
    

  

    // Создаем один снаряд
    projectileUid = server->CreateProjectile(ProjectileType::Hook, p, caster.position + caster.direction, direction, baseRadius);

    if (projectileUid == 0xFFFF) {
        caster.UnlockMovement();
        state = SpellState::Finished;
        return false;
    }

    casterIdx = server->GetPlayerIndex(caster.GetId());
    appearDuration = (maxDistanceCells * 1.0f) / 6.0f;
    state = SpellState::Appear;
    auto& slot = server->projectiles[projectileUid];
    slot.data.fRadius = 0.3f;
    slot.data.vVel = direction;
    slot.collisionEnabled = true;
    return true;
}

void GrappleSpell::ForceFinish(int uid, Match* server)
{
    state = SpellState::Disappear;
}

void GrappleSpell::NotifyWorldHit(glm::ivec2 cell, Match* server)
{
   // auto& slot = server->projectiles[projectileUid];
    // Или проверяем скорость: если снаряд ударился о стену, его скорость в Match обнулится
    if (lifeTimer > 0.05f) {
        

       

        anchorPos = { cell.x + 0.5f,cell.y + 0.5f };
        isAttached = true;
        state = SpellState::Active;
        lifeTimer = 0;
       
        return;
    }

}

bool GrappleSpell::OwnsProjectile(int uid)
{
    return projectileUid == (uint16_t)uid;
}

void GrappleSpell::UpdateAppear(float dt, Match* server)
{ // Фаза полета снаряда
    auto& slot = server->projectiles[projectileUid];
    if (!slot.active || slot.data.bPendingDestroy) {
        state = SpellState::Disappear;
        return;
    }

   
  

    if (lifeTimer >= appearDuration) {
        state = SpellState::Disappear;
    }
}

void GrappleSpell::UpdateActive(float dt, Match* server)
{
    if (!isAttached || casterIdx >= server->GetEntities().size()) {
        state = SpellState::Disappear;
        return;
    }

    auto& player = server->GetEntities()[casterIdx];
    if (!player.active) {
        state = SpellState::Disappear;
        return;
    }

    auto& character = player.character;
    glm::vec2 toAnchor = anchorPos - character->position;  // от position к anchor point
    float dist = glm::length(toAnchor);

    if(dist > 0.6f) {
        glm::vec2 moveDir = glm::normalize(toAnchor);

        // 1. ПРОВЕРКА: Проверяем несколько точек по направлению движения
        // (текущая позиция + радиус персонажа + небольшой запас)
        float checkDist = character->radius + 0.3f;
        glm::vec2 checkPos = character->position + moveDir * checkDist;
   

        if (server->level.IsSolid(checkPos)) {
            server->level.DamageTile(checkPos, 100, character->GetId());
           
            // При ударе об стену замедляем перемещение в этом кадре
            character->position += moveDir * (pullSpeed * 0.3f) * dt;
        }
        else {
            // Обычное движение только если не было удара (или всегда, если хотим пролетать быстро)
            character->position += moveDir * pullSpeed * dt;
        }

        auto& slot = server->projectiles[projectileUid];
        if (slot.active) {
            slot.data.vPos = anchorPos;
        }
    }
    else {
        state = SpellState::Disappear;
    }
}

void GrappleSpell::UpdateDisappear(float dt, Match* server)
{
    if (casterIdx < server->GetEntities().size() && server->GetEntities()[casterIdx].active) {
        server->GetEntities()[casterIdx].character->UnlockMovement();
    }

    if (projectileUid != 0xFFFF) {
        server->MarkProjectileForRemoval(projectileUid);
        projectileUid = 0xFFFF; // Чтобы не удалять повторно
    }

    state = SpellState::Finished;
}
