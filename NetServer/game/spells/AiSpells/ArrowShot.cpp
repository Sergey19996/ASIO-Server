#include "ArrowShot.h"
#include "../../../Match.h"  // здесь уже можно подключать

bool ArrowShot::Cast(Character& caster, Match* server)
{

    glm::vec2 HitCenter = caster.position + caster.direction;

    // 1. ИНТЕГРАЦИЯ: Создаем снаряд через фабрику сервера
    // Метод сам выделит ID, обнулит данные, назначит правила (rules) 
    // и добавит индекс в список активных для обработки физики.
    glm::vec2 vel = { 0.0f,0.0f };
    float radius = 0.0f;
    //// 1. Создаем снаряд с базовым уроном мага
    ProjectileParams p;

    p.ownerId = caster.GetId();
    p.damageMod = caster.GetDamageMultiplier(); // тот самый 1.0 - 2.2x
    // p.effectMod = balanceBonus;

    projectileId = server->CreateProjectile(ProjectileType::Shoot, p, HitCenter, vel, radius);

    // 2. Проверка на свободные места
    if (projectileId == 0xFFFF) {
        std::cout << "[WARN] Projectile Pool Empty for SmashHit!" << std::endl;
        return false;
    }

    // 3. Тюнинг параметров, специфичных для этого удара
    auto& slot = server->projectiles[projectileId];
    slot.data.vVel = { 0.0f, 0.0f };
    slot.data.fRadius = 0.25f;

    casterId = server->GetPlayerIndex(caster.GetId());
    // 4. Регистрация части заклинания в локальном списке
   // parts.push_back({ (uint32_t)idx,  playerIdx, 0.0f });

      // ОТПРАВЛЯЕМ ОДИН РАЗ ВСЕМ КЛИЕНТАМ В ЗОНЕ ВИДИМОСТИ
    olc::net::message<GameMsg> msg;
    msg.header.id = GameMsg::Server_CastStart;
    msg << sCastStart{ caster.GetId(), (uint8_t)SpellId::Fireball };
    // server->BroadcastMessage(msg); // Или BroadcastToVisible
    server->MessageAllMatchClients(msg); //анимация старта каста


    return true;
}

void ArrowShot::ForceFinish(int uid, Match* server)
{
    //  parts.clear();  // не надо пытаться удалять уже удалённые projectiles
    state = SpellState::Finished;
}

bool ArrowShot::OwnsProjectile(int uid)
{
    return projectileId == (uint16_t)uid;
}

void ArrowShot::UpdateAppear(float dt, Match* server)
{
    float t = std::min(lifeTimer / appearDuration, 1.0f);
    auto& entities = server->GetEntities();
    //  for (auto& p : parts) {
          // Проверка существования игрока и снаряда
    if (casterId >= entities.size() || !entities[casterId].active) return;
    auto& slot = server->projectiles[projectileId];
    if (!slot.active) return;

    auto& caster = entities[casterId].character;
    //  p.scale = t;

    slot.data.fRadius = t * 0.15f; // Увеличиваем радиус удара
    slot.data.vPos = caster->position + caster->direction;
    //    }

    if (lifeTimer >= appearDuration) {
        state = SpellState::Active;
        slot.data.vVel = caster->direction * 10.0f;
        slot.collisionEnabled = true;
        lifeTimer = 0;


        // 3. Публичное сообщение: шлем ВСЕМ в мире, чтобы у них переключилась анимация этого мага
        olc::net::message<GameMsg> msgLaunch;
        msgLaunch.header.id = GameMsg::Server_CastLaunch;
        msgLaunch << sCastLaunch{ caster->GetId(), (uint8_t)id };
        //server->BroadcastMessage(msgLaunch);
        server->MessageAllMatchClients(msgLaunch);
    }
}

void ArrowShot::UpdateActive(float dt, Match* server)
{
    //auto& entities = server->GetEntities();
    //// for (auto& p : parts) {
    //if (casterId >= entities.size() || !entities[casterId].active) return;
    //auto& slot = server->projectiles[projectileId];
    //if (!slot.active) return;

    //auto& caster = entities[casterId].character;
    //// Удар «приклеен» к направлению взгляда игрока
    //slot.data.vPos = caster->position + caster->direction;
    ////}

    //if (lifeTimer >= activeDuration) {
    //    state = SpellState::Disappear;
    //  
    //    lifeTimer = 0;
    //}




    auto& slot = server->projectiles[projectileId];
    if (!slot.active) return;




    if (lifeTimer >= activeDuration) {
        state = SpellState::Disappear;
        lifeTimer = 0;
    }


}

void ArrowShot::UpdateDisappear(float dt, Match* server)
{

    float t = 1.0f - std::min(lifeTimer / disappearDuration, 1.0f);

    //  for (auto& p : parts) {
    auto& slot = server->projectiles[projectileId];
    if (slot.active) {
        slot.data.fRadius = t * 0.15f;
    }
    //}

    if (t <= 0.0f) {
        //   for (auto& p : parts) {
        server->MarkProjectileForRemoval(projectileId);
        // }
        state = SpellState::Finished;
    }
}
