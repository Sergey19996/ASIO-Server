#include "ShieldSpell.h"
#include "../../../Match.h"  // здесь уже можно подключать
#include "../../utils/WorldGrid.h"
#include "../NetShared/entities/Warrior.h"
#include "../NetShared/managers/EffectManager.h"



bool ShieldSpell::Cast(Character& caster, Match* server)
{

    // 1. СНАЧАЛА ПРОВЕРЯЕМ: может ли маг вообще совершить действие?
    if (!caster.CanCastSpell()) {
        return false;
    }
  

    // 1. Применяем эффект на персонажа
    StatusEffect shield;
    shield.type = StatusEffectType::Shield;
    shield.timeLeft = 5.0f;
    shield.shieldHP = 50;
    caster.GetEffects()->Add(shield);

    // 2. Определяем позицию "физического" щита
    glm::vec2 ShieldCenter = caster.position + caster.direction;

    // 3. ИНТЕГРАЦИЯ: Создаем снаряд через серверный метод-фабрику
    // Метод сам проверит очередь, достанет индекс, инициализирует .data и .rules,
    // выставит .active = true и добавит в activeProjectileIndices.
    glm::vec2 vel = { 0.0f,0.0f };
    float radius = 0.0f;
    ProjectileParams p;
    p.ownerId = caster.GetId();
    ProjectileId = server->CreateProjectile(ProjectileType::Shield, p, ShieldCenter, vel, radius);

    // Если в пуле нет мест — выходим
    if (ProjectileId == 0xFFFF) {
        std::cout << "[WARN] Projectile Pool Empty for Shield!" << std::endl;
        return false;
    }

    // 4. Тюнинг специфических параметров (то, что отличается от дефолта CreateProjectile)
    auto& slot = server->projectiles[ProjectileId];
    slot.data.fRadius = 0.0f;
    slot.data.vVel = { 0.0f, 0.0f }; // Щит обычно неподвижен относительно мира или привязан к игроку

    // 3. Используем idToIndex для получения порядкового номера в векторе players
    CasterId = server->GetPlayerIndex(caster.GetId());
  
    // ОТПРАВЛЯЕМ ОДИН РАЗ ВСЕМ КЛИЕНТАМ В ЗОНЕ ВИДИМОСТИ
    olc::net::message<GameMsg> msg;
    msg.header.id = GameMsg::Server_CastStart;
    msg << sCastStart{ caster.GetId(), (uint8_t)SpellId::Shield };
    // server->BroadcastMessage(msg); // Или BroadcastToVisible
    server->MessageAllMatchClients(msg); //анимация старта каста


    return true;
     
}

void ShieldSpell::ForceFinish(int uid, Match* server)
{
    auto& entities = server->GetEntities(); // Используем геттер
 
       // server->MarkProjectileForRemoval(ProjectileId);
 
     
 
        if (CasterId < entities.size() && entities[CasterId].active) {
            auto& caster = entities[CasterId].character;
            
            if (caster->GetEffects()->HasEffect(StatusEffectType::Shield)) {
                caster->GetEffects()->Remove(StatusEffectType::Shield);
            //caster->SetCooldown(SpellId::Shield, shieldCD, server->matchTime);
            }
          
      
        }
 
    state = SpellState::Finished;

}

bool ShieldSpell::OwnsProjectile(int uid)
{
    
    return ProjectileId == (uint16_t)uid;
}

void ShieldSpell::UpdateAppear(float dt, Match* server)
{
    float t = std::min(lifeTimer / appearDuration, 1.0f);
    auto& entities = server->GetEntities();
    if (CasterId >= entities.size() || !entities[CasterId].active)
        return;

        auto& slot = server->projectiles[ProjectileId];
        if (!slot.active) return;

        auto& caster = entities[CasterId].character;
       // p.scale = t;

        slot.data.fRadius = t * 0.5f; // Размер щита
        slot.data.vPos = caster->position + caster->direction;
        
    

    if (lifeTimer >= appearDuration) {
        state = SpellState::Active;
        slot.collisionEnabled = true;
        lifeTimer = 0;
    }
}

void ShieldSpell::UpdateActive(float dt, Match* server)
{
    auto& entities = server->GetEntities();
    auto& slot = server->projectiles[ProjectileId];
        if (CasterId >= entities.size() || !entities[CasterId].active) {
            state = SpellState::Finished;
            return;
        }

        auto& caster = entities[CasterId].character;
        
        auto* shield = caster->GetEffects()->GetEffect(StatusEffectType::Shield);

        if (!shield) // на этом правиле у меня 2 спела - shiedBurst и shieldThrow
        {
            state = SpellState::Disappear;
            lifeTimer = 0;
            slot.collisionEnabled = false;
            return;
        }
        else
        {
        shield->timeLeft -= dt;
       
        if (slot.active) {
            slot.data.vPos = caster->position + caster->direction;

        }

        // Логика timeLeft обычно обрабатывается внутри Character::UpdateStatusEffects, 
        // но если вы делаете это здесь:

        }
    

    if (lifeTimer >= activeDuration)
    {
        state = SpellState::Disappear;
        lifeTimer = 0;
    }
}

void ShieldSpell::UpdateDisappear(float dt, Match* server) // если щит просуществовал до конца он не multistage в методе 
{

    float t = 1.0f - std::min(lifeTimer / disappearDuration, 1.0f);


    auto& slot = server->projectiles[ProjectileId];
    if (slot.active) {
        slot.data.fRadius = t * 0.5f;


        if (t <= 0.0f) {
            server->MarkProjectileForRemoval(ProjectileId);
            state = SpellState::Finished;
           
        }
    }
}

