#include "TeleportSpell.h"
#include "../../../Match.h"  // здесь уже можно подключать
#include "../../utils/WorldGrid.h"
#include "../NetShared/managers/CooldownManager.h"
#include "../NetShared/managers/EffectManager.h"


bool TeleportSpell::Cast(Character& caster, Match* server)
{
    // 1. СНАЧАЛА ПРОВЕРЯЕМ: может ли маг вообще совершить действие?
    if (!caster.CanCastSpell()) {
        return false;
    }


    // Если порталов уже 2, значит это 3-е нажатие — закрываем
    if (portals.size() >= 2) {
        state = SpellState::Disappear;
        lifeTimer = 0;
        return true;
    }

    // Логика спавна (одинаковая для 1-го и 2-го портала)
    glm::vec2 targetPos = caster.position + caster.direction;
    if (IsBlockedCell(targetPos, server->level)) return false;

    glm::vec2 dummyVel = { 0,0 };
    float currentRadius = 0.6f;
   

    //// 1. Создаем снаряд с базовым уроном мага
    ProjectileParams p;
    p.ownerId = caster.GetId();
   
    uint16_t idx = server->CreateProjectile(ProjectileType::TeleportPortal, p, targetPos, dummyVel, currentRadius);

    if (idx != 0xFFFF) {
        
        portals.push_back({ (uint32_t)idx, targetPos });
        //server->BroadcastProjectile(idx);
     
    }

    // Если поставили второй — запускаем фазу Appear (рост)
    if (portals.size() == 2) {
        std::cout << "we have 2 portals " << std::endl;
        secondPortalPlaced = true;
        state = SpellState::Appear;
        lifeTimer = 0;
    }
    return true;

    casterIdx = server->GetPlayerIndex(p.ownerId);
}



void TeleportSpell::ForceFinish(int uid, Match* server)
{
    // Переходим в стадию исчезновения вместо мгновенного удаления
   // чтобы сработал плавный скейл в 0
    if (state != SpellState::Disappear && state != SpellState::Finished) {
        state = SpellState::Disappear;
        lifeTimer = 0;
    }
}

void TeleportSpell::UpdateAppear(float dt, Match* server)
{
    if (!secondPortalPlaced) return;

   

    float t = std::min(lifeTimer / appearDuration, 1.0f);
    for (auto& p : portals) {
        auto& slot = server->projectiles[p.uid];
        if (slot.active) {
            slot.data.fRadius = t * 0.6f; // Растем до 0.6

        }
        if (t >= 1.0f) {
            state = SpellState::Active;
            lifeTimer = 0;
            slot.collisionEnabled = true;

           

        }
    }

   
}

void TeleportSpell::UpdateActive(float dt, Match* server)
{
    if (portals.size() < 2) return;

    // Очищаем, но не пересоздаем вектор
    processedInThisFrameProj.clear();
    processedInThisFrameChar.clear();

    // Сразу заносим сами порталы в список исключений, чтобы они не могли телепортировать друг друга
    for (auto& p : portals) processedInThisFrameProj.push_back(p.uid);

    for (int i = 0; i < 2; ++i) {
        auto& currentPortal = portals[i];
        auto& targetPortal = portals[1 - i];

        glm::ivec2 baseCell = server->WorldToGridCell(currentPortal.pos);

        for (const auto& n : Match::neighbors) {
            int nIdx = server->GridIndex(baseCell + n);
            if (nIdx < 0 || nIdx >= server->spatialGrid.size()) continue;

            auto& cell = server->spatialGrid[nIdx];

            // --- ПРОВЕРКА ИГРОКОВ ---
            for (int cid = cell.firstCharacter; cid != -1; cid = server->GetEntities()[cid].nextInCell) {
                auto& pl = server->GetEntities()[cid];

                // Проверяем, не обрабатывали ли мы этого игрока уже (в другом портале)
                if (std::find(processedInThisFrameChar.begin(), processedInThisFrameChar.end(), pl.netId) != processedInThisFrameChar.end()) continue;

                if (pl.active||pl.character == nullptr || pl.character->IsDead() || pl.character->GetEffects()->HasEffect(StatusEffectType::TeleportCooldown)) continue;

                if (glm::distance(pl.character->position, currentPortal.pos) < 0.7f) {
                    pl.character->position = targetPortal.pos;
                   
                    StatusEffect cd;
                    cd.type = StatusEffectType::TeleportCooldown;
                    cd.timeLeft = 1.0f;
                    pl.character->GetEffects()->Add(cd);

                    processedInThisFrameChar.push_back(pl.netId); // Помечаем как обработанного
                }
            }

            // --- ПРОВЕРКА СНАРЯДОВ ---
            for (int pid = cell.firstProjectile; pid != -1; pid = server->projectiles[pid].nextInCell) {
                if (OwnsProjectile(pid)) continue;

                // Если снаряд уже прыгнул в этом кадре — игнорируем его во втором портале
                if (std::find(processedInThisFrameProj.begin(), processedInThisFrameProj.end(), (uint32_t)pid) != processedInThisFrameProj.end()) continue;

                auto& otherSlot = server->projectiles[pid];
                if (glm::length(otherSlot.data.vVel) < 0.01f) continue;
                if (!otherSlot.active || otherSlot.data.bPendingDestroy) continue;

                if (glm::distance(otherSlot.data.vPos, currentPortal.pos) < 0.6f) {
                    // Рассчитываем направление вылета (куда летел снаряд)
                    glm::vec2 exitDir = glm::normalize(otherSlot.data.vVel);

                    // Смещаем позицию выхода на радиус портала + запас
                    otherSlot.data.vPos = targetPortal.pos + exitDir * 0.7f;

                    processedInThisFrameProj.push_back((uint32_t)pid);
                }
            }
        }
    }
  

}

void TeleportSpell::UpdateDisappear(float dt, Match* server)
{
    float t = 1.0f - std::min(lifeTimer / disappearDuration, 1.0f);

    for (auto& p : portals) {
        auto& slot = server->projectiles[p.uid];
        if (slot.active) slot.data.fRadius = t * 0.6f;
    }

    if (t <= 0.0f) {
        for (auto& p : portals) server->MarkProjectileForRemoval(p.uid);
        state = SpellState::Finished;

        auto& pl = server->GetEntities()[casterIdx];
        auto& ch = pl.character;




        float cd = GetAdjustedCooldown();
        ch->GetCooldown()->Set(id, cd, server->matchTime);
        // 2. Отправляем пакет клиенту (чтобы иконка начала тикать)
        server->SendCooldownToClient(ownerId, id, cd);

    }
}

void TeleportSpell::TeleportObject(uint32_t objId, glm::vec2 fromPos, glm::vec2 toPos, Match* server)
{
}
