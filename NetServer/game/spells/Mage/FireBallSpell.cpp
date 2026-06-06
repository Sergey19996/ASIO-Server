#include "FireBallSpell.h"
#include "../../../Match.h"  // здесь уже можно подключать
//#include "../../entities/Mage.h"
#include "../NetShared/entities/Mage.h"
#include "../NetShared/managers/CooldownManager.h"

bool FireballSpell::Cast(Character& caster, Match* server)
{
    Mage* mage = dynamic_cast<Mage*>(&caster);
    //// 1. СНАЧАЛА ПРОВЕРЯЕМ: может ли маг вообще совершить действие?
    //if (!caster.CanCastSpell()) {
    //    return false;
    //}

   
    basePos = caster.position + caster.direction * 0.35f;
    direction = caster.direction * 12.0f; // Базовая скорость
  

    caster.LockMovement();
    caster.LockActions(); // Теперь CanCastSpell() для него будет false
    float b = mage->GetRawBalance();
    if (b > 0.0f)
    {
    powerMultiplier = caster.GetDamageMultiplier(); // от него ещё и кд зависит до 1.5
    }
    else {
    powerMultiplier = 0.25f; // если шкала во льду - у нас быстрый каст и маленький урон

    }

    float balanceBonus = caster.GetSpellPowerBonus(SpellId::Fireball);
    //// 1. Создаем снаряд с базовым уроном мага
    ProjectileParams p;
    p.ownerId = caster.GetId();
    p.damageMod = powerMultiplier; // тот самый 1.0 - 2.2x
    p.effectMod = balanceBonus;

   

    // Передаем параметры по ссылке. Метод изменит direction и initialRadius, если дует ветер
    projectileUid = server->CreateProjectile(ProjectileType::Fireball, p, basePos, direction, targetRadius);

    if (projectileUid == 0xFFFF) {
      
        ReleaseCaster(server);
        return false;
    }
   
    float now = server->matchTime;
    float prepDuration = (mage->GetRawBalance() > 0.0f) ? 0.4f : 0.15f;

    

    // ОТПРАВЛЯЕМ ОДИН РАЗ ВСЕМ КЛИЕНТАМ В ЗОНЕ ВИДИМОСТИ
    olc::net::message<GameMsg> msg;
    msg.header.id = GameMsg::Server_CastStart;
    msg << sCastStart{ caster.GetId(), (uint8_t)SpellId::Fireball };
   // server->BroadcastMessage(msg); // Или BroadcastToVisible
    server->MessageAllMatchClients(msg); //анимация старта каста
  
   
    casterIdx = server->GetPlayerIndex(p.ownerId);
    //server->BroadcastProjectile(idx);
    return true;
}

void FireballSpell::NotifyWorldHit(glm::ivec2 cell, Match* server)
{
    auto& proj = server->projectiles[projectileUid]; // Получаем доступ к слоту снаряда
 
    // 1. Уменьшаем радиус снаряда при каждом контакте
    // Чем больше цель, тем сильнее "стирается" шар
  
    proj.data.bStuck = false;
    proj.data.vVel = direction * 0.85f;
   
    proj.data.fRadius *= 0.9f;
    // 2. Уменьшаем урон для следующего кадра (опционально)
    // Чтобы он не выжигал всю толпу одинаково больно
    proj.cachedRules.damageToPlayers *= 0.9f;
    activeDuration *= 0.9f;
    // 3. Если шар еще достаточно велик — мы ОТМЕНЯЕМ его удаление
    if (proj.data.fRadius > 0.25f) {
        // Мы НЕ вызываем MarkProjectileForRemoval. 
        // Шар просто пролетает "сквозь" или продолжает тереться.

        // Чтобы шар не наносил урон КАЖДЫЙ кадр одной и той же цели (имба):
        // Можно добавить маленькое КД на урон по конкретному targetId в рамках этого снаряда
    }
    else {
        // Шар истаял — удаляем
        server->MarkProjectileForRemoval(projectileUid);
        this->ForceFinish(projectileUid, server);
    }
}

void FireballSpell::NotifyCharacterHit(uint32_t ProjID, uint32_t TargetID, Match* server)
{
    auto& proj = server->projectiles[projectileUid]; // Получаем доступ к слоту снаряда

    // 1. Уменьшаем радиус снаряда при каждом контакте
    // Чем больше цель, тем сильнее "стирается" шар
  
    proj.data.bStuck = false;
    proj.data.vVel = direction * 0.85f;
    // 2. Уменьшаем урон для следующего кадра (опционально)
    // Чтобы он не выжигал всю толпу одинаково больно
    proj.cachedRules.damageToPlayers *= 0.9f;
    proj.data.fRadius *= 0.9f;
    // 3. Если шар еще достаточно велик — мы ОТМЕНЯЕМ его удаление
    if (proj.data.fRadius > 0.25f) {
        // Мы НЕ вызываем MarkProjectileForRemoval. 
        // Шар просто пролетает "сквозь" или продолжает тереться.

        // Чтобы шар не наносил урон КАЖДЫЙ кадр одной и той же цели (имба):
        // Можно добавить маленькое КД на урон по конкретному targetId в рамках этого снаряда
    }
    else {
        // Шар истаял — удаляем
        server->MarkProjectileForRemoval(projectileUid);
        this->ForceFinish(projectileUid, server);
    }
}


bool FireballSpell::OwnsProjectile(int uid)
{
    return projectileUid == (uint16_t)uid;
}

void FireballSpell::ForceFinish(int uid, Match* server)
{
    
    ReleaseCaster(server);
    state = SpellState::Finished;

}

void FireballSpell::UpdateAppear(float dt, Match* server)
{
    auto& entities = server->GetEntities();
    auto& caster = *entities[casterIdx].character;
    auto* mage = dynamic_cast<Mage*>(&caster);
    auto& slot = server->projectiles[projectileUid]; // Получаем ссылку на слот снаряда
   // float t = std::min(lifeTimer / powerMultiplier, 1.0f);

       
       
        if (!slot.active) {
            state = SpellState::Finished;
            return;
        }

        float maxCharge = caster.GetMaxChargeTime();

        if (caster.IsCharging()) {
            // Логика "Перегрева"
            if (caster.GetChargeTimer() >= maxCharge) {
                appearDuration += dt; // Используем отдельную переменную вместо appearDuration
                if (appearDuration >= 0.5f) {
                    caster.StopCharging();
                    return;
                }
            }

            // Позиция и направление (обновляем, чтобы маг мог "довернуться" во время зарядки)
            direction = caster.direction * 6.0f;
            slot.data.vPos = caster.position + caster.direction * 0.35f;

            // РАДИУС теперь зависит от доли накопленного заряда относительно ТЕКУЩЕГО максимума
            float chargeRatio = caster.GetChargeTimer() / maxCharge;

            // Визуально увеличиваем шар. 
            // powerMultiplier (баланс) определяет финальный размер, chargeRatio — текущий рост.
            slot.data.fRadius = (0.2f + (chargeRatio * 0.4f)) * powerMultiplier;



            return;
        }
      
        // --- КНОПКА ОТПУЩЕНА (ВЫСТРЕЛ) ---
        float finalCharge = caster.GetLastChargeTime();
        float finalRatio = finalCharge / maxCharge; // Насколько эффективно зарядили (0.0 - 1.0)

        // Применяем коэффициенты к снаряду
        slot.data.vVel = direction * (0.5f + finalRatio * 0.5f); // Скорость зависит от заряда
        slot.collisionEnabled = true;

        // УРОН: Базовый урон * множитель баланса * качество зарядки
        slot.cachedRules.damageToPlayers *= finalRatio;
        slot.cachedRules.windResistance = finalRatio;
        // ОТДАЧА: Чем мощнее был "затяжной" каст, тем сильнее отбросит мага
        glm::vec2 recoilDir = -glm::normalize(direction);
        float recoilForce = 3.0f * finalRatio * powerMultiplier;
        caster.knockbackVel += recoilDir * recoilForce;

        // Сброс и КД
        float cd = GetAdjustedCooldown() * (0.8f + finalRatio * 0.4f); // Мощный каст чуть дольше откатывается
        caster.GetCooldown()->Set(id, cd, server->matchTime);
        server->SendCooldownToClient(ownerId, id, cd);
        caster.OnSpellCast(id, finalRatio ); // изменяем баланс у мага 
        ReleaseCaster(server);


        // 3. Публичное сообщение: шлем ВСЕМ в мире, чтобы у них переключилась анимация этого мага
        olc::net::message<GameMsg> msgLaunch;
        msgLaunch.header.id = GameMsg::Server_CastLaunch;
        msgLaunch << sCastLaunch{ caster.GetId(), (uint8_t)id };
        //server->BroadcastMessage(msgLaunch);
        server->MessageAllMatchClients(msgLaunch);
       

        state = SpellState::Active;
        lifeTimer = 0;
  //  }
}

void FireballSpell::UpdateActive(float dt, Match* server)
{
    if (lifeTimer >= activeDuration) {
        state = SpellState::Disappear;
        lifeTimer = 0;
    }

}

void FireballSpell::UpdateDisappear(float dt, Match* server)
{
    float t = 1.0f - std::min(lifeTimer / 0.5f, 1.0f);

   
        auto& slot = server->projectiles[projectileUid];
        if (slot.active)
            slot.data.fRadius *= t;
        
    

    if (t <= 0.0f) {
      
        
            // Помечаем на удаление (вернет индекс в freeProjectileIds)
            server->MarkProjectileForRemoval(projectileUid);
        
        state = SpellState::Finished;
    }

}
bool FireballSpell::CanBeCancelled() const
{
    return state == SpellState::Appear;
}

void FireballSpell::Cancel(Match* server)
{
    ReleaseCaster(server);

    // Если projectile уже создан — удалить
    if (projectileUid != 0xFFFF)
        server->MarkProjectileForRemoval(projectileUid);

  

    state = SpellState::Finished;
}

bool FireballSpell::IsFinished() const
{
    return state == SpellState::Finished;
}
