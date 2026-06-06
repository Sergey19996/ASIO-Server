#include "PullHookSpell.h"
#include "../../../Match.h"  // здесь уже можно подключать
#include "../../utils/WorldGrid.h"
#include "../../../DamageContext.h"

bool PullHookSpell::Cast(Character& caster, Match* server)
{
    direction = caster.direction * 16.0f; // Быстрый полет снаряда
    casterIdx = server->GetPlayerIndex(caster.GetId());

    float baseRadius = 0.4f;

   

    //// 1. Создаем снаряд с базовым уроном мага
    ProjectileParams p;
    p.ownerId = caster.GetId();
  //  p.damageMod = caster.GetDamageMultiplier(); // тот самый 1.0 - 2.2x
  //  p.effectMod = balanceBonus;

    projectileUid = server->CreateProjectile(ProjectileType::Hook, p, caster.position + caster.direction, direction, baseRadius);

    if (projectileUid == 0xFFFF) {
        state = SpellState::Finished;
        return false;
    }

    appearDuration = (maxDistanceCells * 1.0f) / 10.0f;
    state = SpellState::Appear;
    auto& slot = server->projectiles[projectileUid];
    slot.data.fRadius = 0.3f;
    slot.data.vVel = direction;
    slot.collisionEnabled = true;


    // 3. Публичное сообщение: шлем ВСЕМ в мире, чтобы у них переключилась анимация этого мага
    olc::net::message<GameMsg> msgLaunch;
    msgLaunch.header.id = GameMsg::Server_CastLaunch;
    msgLaunch << sCastLaunch{ caster.GetId(), (uint8_t)id };
    //server->BroadcastMessage(msgLaunch);
    server->MessageAllMatchClients(msgLaunch);

    return true;
}

void PullHookSpell::ForceFinish(int uid, Match* server)
{
    state = SpellState::Disappear;
}

void PullHookSpell::NotifyWorldHit(glm::ivec2 cell, Match* server)
{
    if (state == SpellState::Appear) {
        auto& chr = server->GetEntities()[casterIdx].character;
        glm::ivec2 casterCell = WorldToCell(chr->position);

        // 1. Считаем направление шага по каждой оси отдельно
        // Если блок левее игрока, dir.x = 1, если правее = -1, если на одной линии = 0
        glm::ivec2 dir;
        dir.x = (casterCell.x > cell.x) ? 1 : (casterCell.x < cell.x ? -1 : 0);
        dir.y = (casterCell.y > cell.y) ? 1 : (casterCell.y < cell.y ? -1 : 0);

        // 2. ФИНАЛЬНАЯ ТОЧКА: клетка игрока минус этот вектор
        // Если блок был в 1(0,0), а игрок в 5(1,1), то dir = (1,1).
        // targetDestination = (1,1) - (1,1) = (0,0). Блок останется в 1.
        // НО: если блок был далеко, например в (0,0) при игроке в (5,5),
        // то dir = (1,1), и блок прилетит в (4,4) — это "угол" относительно игрока.

        int cellOffset = 1;
        if (chr->radius > 0.5f) { // Пример логики: если радиус больше пол-клетки
            cellOffset = static_cast<int>(std::ceil(chr->radius));
        }

        // ФИНАЛЬНАЯ ТОЧКА: смещаем целевую клетку от центра игрока на величину отступа
        glm::ivec2 targetDestination = casterCell - (dir * cellOffset);

        if (targetDestination == cell) {
            state = SpellState::Disappear; // Уже на месте
            return;
        }

        targetCellPos = targetDestination;
        pullType = PullType::Wall;
        state = SpellState::Active;



        // 3. Запускаем движение
        // Убедитесь, что StartTileMovement поддерживает диагональное движение (dist > 1.0)
        server->level.MoveTileInstant(cell, targetDestination);
   
    }
}

void PullHookSpell::NotifyCharacterHit(uint32_t ProjID, uint32_t TargetID, Match* server)
{
    // 1. ПРОВЕРКА ПОПАДАНИЯ В ИГРОКА (через систему коллизий Match)
 // Предполагаем, что Match помечает nVictimID, если снаряд коснулся кого-то
  //  if (slot.data.nVictimId != 0 && slot.data.nVictimId != server->GetPlayers()[casterIdx].netId) {
        targetPlayerIdx = TargetID;
        pullType = PullType::Player;
        state = SpellState::Active;
        lifeTimer = 0.0f;
        return;
 //   }
}

bool PullHookSpell::OwnsProjectile(int uid)
{
    return projectileUid == (uint16_t)uid;
}

void PullHookSpell::UpdateAppear(float dt, Match* server)
{
    auto& slot = server->projectiles[projectileUid];
    if (!slot.active || slot.data.bPendingDestroy) {
        state = SpellState::Disappear;
        return;
    }

 

    //// 2. ПРОВЕРКА ПОПАДАНИЯ В СТЕНУ
    //if (slot.data.bStuck) {
    //     // ОБЯЗАТЕЛЬНО: переводим vPos (float) в ячейку сетки (int)
    //glm::ivec2 gridPos = WorldToCell(slot.data.vPos);
    //targetCellPos = gridPos; // Убедитесь, что targetCellPos в классе имеет тип glm::ivec2

    //pullType = PullType::Wall;
    //state = SpellState::Active;
    //
    //auto& chr = server->GetPlayers()[casterIdx].character;
    //// Определяем направление к игроку
    //glm::vec2 toCaster = chr.position - glm::vec2(gridPos);
    //glm::ivec2 dir = glm::ivec2(glm::round(glm::normalize(toCaster).x), 
    //                             glm::round(glm::normalize(toCaster).y));
    //
    //glm::ivec2 targetDestination = gridPos + dir;

    //// Теперь передаем чистые целочисленные координаты
    //server->level.StartTileMovement(gridPos, targetDestination, 5.0f);
    //return;
    //}

    if (lifeTimer >= appearDuration) {
        state = SpellState::Disappear;
    }
}

void PullHookSpell::UpdateActive(float dt, Match* server)
{
    auto& caster = server->GetEntities()[casterIdx].character;
    auto& hook = server->projectiles[projectileUid];

    if (pullType == PullType::Player) {
        if (targetPlayerIdx >= server->GetEntities().size() || !server->GetEntities()[targetPlayerIdx].active) {
            state = SpellState::Disappear;
            return;
        }

        auto& victim = server->GetEntities()[targetPlayerIdx].character;
        glm::vec2 toCaster = caster->position - victim->position;
        float dist = glm::length(toCaster);

        if (dist > 1.0f) {
            glm::vec2 moveDir = glm::normalize(toCaster);
            victim->position += moveDir * pullSpeed * dt;
            if (hook.active) hook.data.vPos = victim->position; // Крюк "впился" в жертву
        }
        else {
            state = SpellState::Disappear;
        }
    }
    else if (pullType == PullType::Wall) {
        auto& tile = server->level.GetTile(targetCellPos);

        if (tile.moving) {
            // РАССЧИТЫВАЕМ ТЕКУЩУЮ ПОЗИЦИЮ ТАЙЛА
            // Смешиваем начальную и конечную точки по коэффициенту moveT
            glm::vec2 currentVisualPos = glm::mix(glm::vec2(tile.from), glm::vec2(tile.to), tile.moveT);

            // ПРИВЯЗЫВАЕМ СНАРЯД
            if (hook.active) {
                hook.data.vPos = currentVisualPos + glm::vec2(0.5f, 0.5f);
            
            }

            // --- ЛОГИКА УРОНА ПРИ ПОЛЕТЕ ---
            // Проверяем игроков вокруг текущей визуальной позиции блока
            glm::ivec2 currentGridPos = server->WorldToGridCell(currentVisualPos);
            int cellIdx = server->GridIndex(currentGridPos);

            if (cellIdx >= 0 && cellIdx < server->spatialGrid.size()) {
                for (int cid = server->spatialGrid[cellIdx].firstCharacter; cid != -1; cid = server->GetEntities()[cid].nextInCell) {
                    // Не бьем самого кастера
                    if (server->GetEntities()[cid].netId == server->GetEntities()[casterIdx].netId) continue;

                    // Наносим урон (добавьте флаг, чтобы не бить одного и того же каждую итерацию)
                    DamageContext ctx;
                    ctx.attackerId = server->GetEntities()[casterIdx].netId;
                    ctx.targetId = (uint32_t)cid;
                    ctx.baseDamage = 20; // Урон при столкновении
                    server->ApplyDamage(ctx);
                }
            }
        }
        else {
            // Если движение закончилось — завершаем спелл
            state = SpellState::Disappear;
        }

    }
}

void PullHookSpell::UpdateDisappear(float dt, Match* server)
{
    if (projectileUid != 0xFFFF) {
        server->MarkProjectileForRemoval(projectileUid);
        projectileUid = 0xFFFF;
    }
    state = SpellState::Finished;
}
