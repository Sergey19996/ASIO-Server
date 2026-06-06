#include "SmashHit.h"
#include "../../../Match.h"  // здесь уже можно подключать
#include "../../utils/WorldGrid.h"
#include "../NetShared/entities/Warrior.h"

bool SmashHit::Cast(Character& caster, Match* server)
{
    auto* War = dynamic_cast<Warrior*>(&caster);
    if (!War) return false;

    float light = War->GetLightIntensity();
    glm::vec2 HitCenter = caster.position + caster.direction;

    // 1. ИНТЕГРАЦИЯ: Создаем снаряд через фабрику сервера
    // Метод сам выделит ID, обнулит данные, назначит правила (rules) 
    // и добавит индекс в список активных для обработки физики.
    glm::vec2 vel = { 0.0f,0.0f };
    float radius = 0.0f;
    //// 1. Создаем снаряд с базовым уроном мага
    ProjectileParams p;
    // --- НОВАЯ ЛОГИКА: Area Attack днем ---
    if (light > 0.5f)
    {
        // Настраиваем параметры АОЕ
        float areaRadius = 0.5f + caster.radius; // Увеличенный радиус для "лопания" стенок

        // Получаем дефолтные правила снаряда для эффектов (отталкивание и т.д.)
        ProjectileRules rules = GetProjectileRules(ProjectileType::SmashHit);

        float areaDamage = rules.damageToPlayers * caster.GetDamageMultiplier();
        rules.knockbackForce = War->GetLightIntensity(); // Сильнее откидываем днем

        // Вызываем круговой урон (false = не бить себя)
        server->ProcessAreaDamage(HitCenter, areaRadius, areaDamage, caster.GetId(), false, rules);
        state = SpellState::Finished;


        // 3. Публичное сообщение: шлем ВСЕМ в мире, чтобы у них переключилась анимация этого мага
        olc::net::message<GameMsg> msgLaunch;
        msgLaunch.header.id = GameMsg::Server_CastLaunch;
        msgLaunch << sCastLaunch{ caster.GetId(), (uint8_t)id };
        //server->BroadcastMessage(msgLaunch);
        server->MessageAllMatchClients(msgLaunch);

        return true;
    }


   

   
    p.ownerId = caster.GetId();
    p.damageMod = caster.GetDamageMultiplier(); // тот самый 1.0 - 2.2x
   // p.effectMod = balanceBonus;

    projectileId = server->CreateProjectile(ProjectileType::SmashHit, p, HitCenter,vel,radius);

    // 2. Проверка на свободные места
    if (projectileId == 0xFFFF) {
        std::cout << "[WARN] Projectile Pool Empty for SmashHit!" << std::endl;
        return false;
    }

    // 3. Тюнинг параметров, специфичных для этого удара
    auto& slot = server->projectiles[projectileId];
    slot.data.vVel = { 0.0f, 0.0f };
    slot.data.fRadius = 0.25f;

    slot.cachedRules.knockbackForce = War->GetLightIntensity();
    casterId = server->GetPlayerIndex(caster.GetId());
 

    // ОТПРАВЛЯЕМ ОДИН РАЗ ВСЕМ КЛИЕНТАМ В ЗОНЕ ВИДИМОСТИ
    olc::net::message<GameMsg> msg;
    msg.header.id = GameMsg::Server_CastStart;
    msg << sCastStart{ caster.GetId(), (uint8_t)SpellId::SmashHit };
    // server->BroadcastMessage(msg); // Или BroadcastToVisible
    server->MessageAllMatchClients(msg); //анимация старта каста

    return true;
    
}

bool SmashHit::OwnsProjectile(int uid)
{
    return projectileId == (uint16_t)uid;
}
void SmashHit::ForceFinish(int uid, Match* server)
{
  //  parts.clear();  // не надо пытаться удалять уже удалённые projectiles
    state = SpellState::Finished;
}



void SmashHit::UpdateAppear(float dt, Match* server)
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

        slot.data.fRadius = t * 0.5f; // Увеличиваем радиус удара
        slot.data.vPos = caster->position + caster->direction;
//    }

    if (lifeTimer >= appearDuration) {
        state = SpellState::Active;
        slot.collisionEnabled = true;
        lifeTimer = 0;


        // 3. Публичное сообщение: шлем ВСЕМ в мире, чтобы у них переключилась анимация этого мага
        olc::net::message<GameMsg> msgLaunch;
        msgLaunch.header.id = GameMsg::Server_CastLaunch;
        msgLaunch << sCastLaunch{ caster->GetId(), (uint8_t)id};
        //server->BroadcastMessage(msgLaunch);
        server->MessageAllMatchClients(msgLaunch);
    }
}

void SmashHit::UpdateActive(float dt, Match* server)
{
    auto& entities = server->GetEntities();
   // for (auto& p : parts) {
        if (casterId >= entities.size() || !entities[casterId].active) return;
        auto& slot = server->projectiles[projectileId];
        if (!slot.active) return;

        auto& caster = entities[casterId].character;
        // Удар «приклеен» к направлению взгляда игрока
        slot.data.vPos = caster->position + caster->direction;
    //}

    if (lifeTimer >= activeDuration) {
        state = SpellState::Disappear;
        lifeTimer = 0;
    }
}

void SmashHit::UpdateDisappear(float dt, Match* server)
{
    float t = 1.0f - std::min(lifeTimer / disappearDuration, 1.0f);

  //  for (auto& p : parts) {
        auto& slot = server->projectiles[projectileId];
        if (slot.active) {
            slot.data.fRadius = t * 0.5f;
        }
    //}

    if (t <= 0.0f) {
     //   for (auto& p : parts) {
            server->MarkProjectileForRemoval(projectileId);
       // }
        state = SpellState::Finished;
    }
}
