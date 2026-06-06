#include "ChainHarvest.h"
#include "../../../Match.h"
//#include "../../entities/Hunter.h"
#include "../NetShared/entities/Hunter.h"

bool ChainHarvest::Cast(Character& caster, Match* server)
{
    auto* hunter = dynamic_cast<Hunter*>(&caster);
    if (!hunter || hunter->liveArrowIds.empty()) return false;

    nodes.clear();

    // Бежим по списку стрел, которые Охотник считает "своими" в мире
    for (uint32_t pid : hunter->liveArrowIds) {
        // 1. Проверяем границы массива на всякий случай
        if (pid >= MAX_PROJECTILES) continue;

        auto& slot = server->projectiles[pid];

        // 2. Проверяем, что это именно тот снаряд (по ID и активности)
        // Так как nUniqueID == idx, это проверка "в яблочко"
        if (slot.active && slot.data.nUniqueID == pid && slot.data.bStuck) {
            HarvestNode node;
            node.pos = slot.data.vPos;
            node.arrowProjId = pid;
            nodes.push_back(node);
        }
    }

    if (nodes.size() < 2) return false;

    // 2. Создаем визуальный снаряд-"жнец" в точке первой стрелы
    ProjectileParams p;
    p.ownerId = caster.GetId();
    float baseRadius = 0.35f;
    glm::vec2 direction = { 0,0 };
    effectProjId = server->CreateProjectile(ProjectileType::HarvestEnergy, p, nodes[0].pos, direction, baseRadius);

    if (effectProjId == 0xFFFF) return false;

    state = SpellState::Active;
    currentTargetIdx = 1; // Целимся во вторую точку
    auto& slot = server->projectiles[effectProjId];
    slot.collisionEnabled = true;
    slot.data.fRadius = 0.35f;
    CasterId = server->GetPlayerIndex(caster.GetId());

    //убиваем точку  выхода 
    server->MarkProjectileForRemoval(nodes[0].arrowProjId);
    hunter->UnregisterLiveArrow(nodes[0].arrowProjId);
    server->spellManager.ForceFinishProjectile(nodes[0].arrowProjId, server);
    hunter->IncrementArrows();

    olc::net::message<GameMsg> msg;
    msg.header.id = GameMsg::Server_CastStart;
    msg << sCastStart{ caster.GetId(), (uint8_t)SpellId::BindArrow };
    // server->BroadcastMessage(msg); // Или BroadcastToVisible
    server->MessageAllMatchClients(msg); //анимация старта каста

    return true;
}

void ChainHarvest::UpdateActive(float dt, Match* server)
{
    if (currentTargetIdx >= nodes.size()) {
        FinishHarvest(server);
        return;
    }

    auto& harvestProj = server->projectiles[effectProjId];
    HarvestNode node = nodes[currentTargetIdx];

    glm::vec2 toTarget = node.pos - harvestProj.data.vPos;
    float dist = glm::length(toTarget);
    float moveStep = moveSpeed * dt; // При 20 FPS это ~1.75
    if (moveStep >= dist || dist < 0.1f) {
        harvestProj.data.vPos = node.pos;
        // Достигли текущей стрелы, переходим к следующей
        server->MarkProjectileForRemoval(node.arrowProjId);
        server->spellManager.ForceFinishProjectile(node.arrowProjId, server);
        //
        currentTargetIdx++;

        auto& entities = server->GetEntities();
        if (CasterId < entities.size() && entities[CasterId].active) {
            auto* hunter = dynamic_cast<Hunter*>(entities[CasterId].character.get());
            if (hunter) {
                // Зануляем связь, чтобы HasPendingActivations() перестал её видеть
                hunter->UnregisterLiveArrow(node.arrowProjId);
                hunter->IncrementArrows();

            }
        }
        // Можно добавить микро-взрыв в точке контакта
       // server->ProcessAreaDamage(targetPos, 1.2f, 15, casterNetId, false);

    }
    else {
        // Двигаем "жнеца" к цели
        glm::vec2 dir = toTarget / dist;
        harvestProj.data.vPos += dir * moveSpeed * dt;

        // Наносим урон всем, кого задело тело летящего заряда
        // (Используем серверную проверку коллизий для точки)
     //   server->CheckCollisionsForPoint(harvestProj.data.vPos, 0.6f, casterNetId, 30);
    }
}

void ChainHarvest::UpdateDisappear(float dt, Match* server)
{
    state == SpellState::Finished;
}

void ChainHarvest::FinishHarvest(Match* server)
{
    if (effectProjId != 0xFFFF) {
        server->MarkProjectileForRemoval(effectProjId);
    }
    state = SpellState::Finished;
}
