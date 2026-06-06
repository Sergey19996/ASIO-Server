#include "Shoot.h"
#include "../../../Match.h"  // здесь уже можно подключать
#include "../../utils/WorldGrid.h"
#include "../NetShared/entities/Hunter.h"
#include "../NetShared/ProjectileRules.h"
#include "../NetShared/managers/CooldownManager.h"
#include "../NetShared/managers/EffectManager.h"

bool Shoot::Cast(Character& caster, Match* server)
{
    auto* hunter = dynamic_cast<Hunter*>(&caster);

    // Если это охотник и он в релоаде или пуст — отменяем каст
    if (hunter && (hunter->IsReloading())) {
        return false;
    }
    // Определяем сторону: Slot2 - это ПКМ (справа), Slot1 - ЛКМ (слева)
   // bool isRightClick = (caster.GetLastCastSlot() == ActionSlot::Slot2);
    // ВАЖНО: Сначала находим реальный индекс палочки (0..4)
    //int arrowIdx = hunter->GetNextArrowIndex(isRightClick);


    basePos = caster.position + caster.direction;
    direction = caster.direction * 10.0f;
    caster.LockMovement();
    caster.LockActions();

    // 1. ИНТЕГРАЦИЯ: Используем фабрику сервера
    // Метод сам проверит freeProjectileIds, выделит индекс, 
    // настроит базовые флаги, правила (rules) и добавит в activeProjectileIndices.

    ProjectileParams p;
    p.ownerId = caster.GetId();
    ProjectileId = server->CreateProjectile(ProjectileType::Shoot, p, basePos, direction, targetRadius);

    // 2. Проверка на переполнение пула
    if (ProjectileId == 0xFFFF) {
        ReleaseCaster(server);
        return false;
    }

    // 3. Тюнинг специфических параметров для этого заклинания
    auto& slot = server->projectiles[ProjectileId];
    slot.data.vVel = { 0.0f, 0.0f }; // В данном спелле скорость задается позже (в UpdateAppear)
    slot.data.fRadius = 0.25f;

      CasterId= server->GetPlayerIndex(caster.GetId());
      // ПРИВЯЗКА ПЕРВОЙ СТРЕЛЫ
     
          // ОТПРАВЛЯЕМ ОДИН РАЗ ВСЕМ КЛИЕНТАМ В ЗОНЕ ВИДИМОСТИ
      olc::net::message<GameMsg> msg;
      msg.header.id = GameMsg::Server_CastStart;
      msg << sCastStart{ caster.GetId(), (uint8_t)SpellId::Shoot };
      // server->BroadcastMessage(msg); // Или BroadcastToVisible
      server->MessageAllMatchClients(msg); //анимация старта каста
      
       //   hunter->OnArrowFired(arrowIdx);
      
      return true;
}


bool Shoot::OwnsProjectile(int uid)
{
    return ProjectileId == (uint16_t)uid;
}

void Shoot::ForceFinish(int uid, Match* server)
{


    ReleaseCaster(server);
    state = SpellState::Finished;
}

void Shoot::NotifyWorldHit(glm::ivec2 cell, Match* server)
{
    if (isStuck) return; // Чтобы не срабатывало повторно
    auto& proj = server->projectiles[ProjectileId];
   
    proj.data.bStuck = false;
    if (power <= 1) {
    isStuck = true;

   
    if (proj.active) {

        if (hasExploded) {
            server->ProcessAreaDamage(proj.data.vPos, 3.0f, 35, server->GetEntities()[CasterId].character->GetId(), false, GetProjectileRules(ProjectileType::Explosion));
            server->MarkProjectileForRemoval(ProjectileId);
            state = SpellState::Disappear;
         //   return;
        }

        if (hasBound) {
            // 1. Находим всех врагов в радиусе
            float bindRadius = 3.5f;
            auto& entities = server->GetEntities();
            for (auto& pl : entities) {
                if (!pl.active ||pl.character == nullptr || pl.character->IsDead()) continue;

                float dist = glm::distance(pl.character->position, proj.data.vPos);
                if (dist <= bindRadius) {
                    // 2. Накладываем эффект "Нити судьбы"
                    StatusEffect bindEff;
                    bindEff.type = StatusEffectType::BindingChain; // Нужно добавить в enum
                    bindEff.timeLeft = 2.5f;                      // Длительность связки
                    bindEff.vCenterPos = proj.data.vPos;               // Центр притяжения (наша стрела)
                    bindEff.value = 150.0f;                       // Сила натяжения цепи
                    bindEff.nOwnerNetID = server->GetEntities()[CasterId].character->GetId();
                    server->MarkProjectileForRemoval(ProjectileId);
                    pl.character->GetEffects()->Add(bindEff);
                }
            }
            state = SpellState::Disappear;
         //   return;
        }
        if (hasExploded || hasBound) return;
        // 1. Увеличиваем радиус до зоны сбора информации
        proj.data.fRadius = 1.5f;
        proj.data.bStuck = true;
        // 2. МЕНЯЕМ ПРАВИЛА снаряда на лету
        // Отключаем урон, чтобы "зона информации" не убивала врагов
       // slot.cachedRules.dealsDamage = false;
        proj.cachedRules.diesOnCharacterCollision = false; // Оставляем true, чтобы снаряд исчез при "сборе"
        proj.cachedRules.damageToPlayers = 0;
        proj.cachedRules.knockbackForce = 0;
      
        // Обнуляем скорость на всякий случай (хотя bStuck это уже сделал)
     //   slot.data.vVel = { 0, 0 };
    }

    activeDuration = 6.0f;
    lifeTimer = 0.0f;

    proj.data.vVel = { 0.0f,0.0f };
    return;
    }
    // Если супер-стрела (заряд 3-5) — она пробивает
   

    power--; // Тратим мощь на пробитие
    proj.data.vVel = direction * 0.9f; // Немного замедляем
    proj.cachedRules.damageToPlayers *= 0.8f; // Теряем урон при прошивании
    
    if (power <= 0) {
        server->MarkProjectileForRemoval(ProjectileId);
        state = SpellState::Disappear;
    }
}

void Shoot::NotifyCharacterHit(uint32_t ProjID, uint32_t TargetID, Match* server) //обработку попадания в персонажа: targetid - idtoinedx
{
    if (!Contacted) {

    // Это был сбор информации врагом
    if (CasterId < server->GetEntities().size()) {
        auto* hunter = static_cast<Hunter*>(server->GetEntities()[CasterId].character.get());
        auto& proj = server->projectiles[ProjectileId];

        hunter->AddInfoPoint();
      //  hunter->IncrementArrows();
    
        Contacted = true;
        if (hasExploded) {
            server->ProcessAreaDamage(proj.data.vPos, 3.0f, 35, server->GetEntities()[CasterId].character->GetId(), false, GetProjectileRules(ProjectileType::Explosion));
            state = SpellState::Disappear;
          //  return;
        }
        if (hasBound) {
            // 1. Находим всех врагов в радиусе
            float bindRadius = 3.5f;
            auto& entities = server->GetEntities();
            uint32_t ownerId = server->GetEntities()[CasterId].character->GetId();
            for (auto& pl : entities) {
              

                if (!pl.active || pl.character == nullptr || pl.character->IsDead() || pl.character->GetId() == ownerId) continue;

                float dist = glm::distance(pl.character->position, proj.data.vPos);
                if (dist <= bindRadius) {
                   
                 
                  
                    server->ApplyBindEffect(pl.character.get(), proj.data.vPos, -1, false);
                  
                }
            }

            state = SpellState::Disappear;
          //  return;
        }

    }
    
        // Завершаем спелл, так как информация собрана
     //   state = SpellState::Disappear;
    //    lifeTimer = 0;

    }
     
}

void Shoot::UpdateAppear(float dt, Match* server)
{

    auto& entities = server->GetEntities();
    auto& caster = *entities[CasterId].character;
    auto* hunter = dynamic_cast<Hunter*>(&caster);
    auto& slot = server->projectiles[ProjectileId];

   // float t = std::min(lifeTimer / appearDuration, 1.0f);

  
        if (CasterId >= entities.size() || !entities[CasterId].active)
            return;

       
        if (!slot.active) return;

        float maxCharge = caster.GetMaxChargeTime();

       
      //  slot.data.fRadius = t * 0.25f;
     //   slot.data.vPos = caster->position + caster->direction;
    
        // Пока игрок держит кнопку (isCharging == true)
        if (caster.IsCharging()) {

            
            // Если заряд достиг максимума, начинаем "перегрев"
            if (caster.GetChargeTimer() >= maxCharge) {
               
                appearDuration += dt;
                if (appearDuration >= 0.5f) { // 1 секунда на раздумья
                    caster.StopCharging(); // Принудительно отпускаем
                    return;
                }
            }

            // 1. Привязываем снаряд к игроку
            direction = caster.direction * 6.0f;
            slot.data.vPos = caster.position + caster.direction * 0.35f;

            // 2. Визуально увеличиваем (например, радиус растет от заряда)
            float chargeRatio = caster.GetChargeTimer() / 1.25f;
            slot.data.fRadius = 0.15f + (chargeRatio * 0.15f);

            // 3. Не даем переходить в фазу Active

            return;
        }


        // --- КНОПКА ОТПУЩЕНА: РАСЧЕТ МОЩНОСТИ ---
        float finalCharge = caster.GetLastChargeTime(); // Сохраненное время перед сбросом -2.5 максимум-
         power = std::max(1, (int)std::floor(finalCharge / 0.25f + 0.01f)); 
        if (power >= 2) {
            slot.cachedRules.diesOnWorldCollision = false; // Не умирает об стены
            slot.cachedRules.diesOnCharacterCollision = false; // Прошивает врагов

            //// 3. Публичное сообщение: шлем ВСЕМ в мире, чтобы у них переключилась анимация этого мага
            //olc::net::message<GameMsg> msgLaunch;
            //msgLaunch.header.id = GameMsg::Server_CastLaunch;
            //msgLaunch << sCastLaunch{ caster.GetId(), (uint8_t)id };
            ////server->BroadcastMessage(msgLaunch);
            //server->MessageAllMatchClients(msgLaunch);

        }
       // bool isRight = (caster.GetLastCastSlot() == ActionSlot::Slot2);
         
        int arrowBP = 0;
        // Охотник тратит стрелы из колчана
        for (int i = 0; i < power; ++i) {
            int idx = hunter->GetNextArrowIndex(false);
            if (idx != -1) {
                auto& arrow = hunter->quiver[idx];

                // Итоговая мощь = база (1) + влитые стрелы (bonusPower)
                arrowBP += arrow.bonusPower;

                // Если стрела была крафтовой, превращаем её в обычную ПЕРЕД выстрелом.
           // Это гарантирует, что OnArrowFired вызовет DecrementArrows().
              //  if (hunter->quiver[idx].type != ArrowType::Normal) {
                //}

               // finalType = hunter->quiver[idx].type;  // Записываем тип стрелы - какой нашли 

                if (hunter->quiver[idx].hasExplosion) hasExploded = true;
                if (hunter->quiver[idx].hasBinding) hasBound = true;
                if (hunter->quiver[idx].hasGhost) {
                    slot.cachedRules.ignoresWorld = true;
                    slot.cachedRules.homingStrength = 50;
                }
              
                hunter->quiver[idx].ResetArrow();
                //hunter->quiver[idx].
                hunter->OnArrowFired(idx);
            



            }
        }
        hunter->RegisterLiveArrow(ProjectileId);
        power += arrowBP;
        // Урон можно записать в extraData снаряда, чтобы при попадании считать его
        slot.cachedRules.damageToPlayers = slot.cachedRules.damageToPlayers * power * 0.4f;
        slot.data.vVel  = direction * (1.0f + (finalCharge * 0.3f));
       // direction = slot.data.vVel;
        slot.collisionEnabled = true;
        ReleaseCaster(server);

        state = SpellState::Active;
        lifeTimer = 0.0f;

        float cd = GetAdjustedCooldown(); // Мощный каст чуть дольше откатывается
        
        hunter->GetCooldown()->Set(id, cd, server->matchTime);
        server->SendCooldownToClient(ownerId, id, cd);
       
        // 3. Публичное сообщение: шлем ВСЕМ в мире, чтобы у них переключилась анимация этого мага
        olc::net::message<GameMsg> msgLaunch;
        msgLaunch.header.id = GameMsg::Server_CastLaunch;
        msgLaunch << sCastLaunch{ caster.GetId(), (uint8_t)id };
        //server->BroadcastMessage(msgLaunch);
        server->MessageAllMatchClients(msgLaunch);

      
}

void Shoot::UpdateActive(float dt, Match* server)
{
    auto& slot = server->projectiles[ProjectileId];
    if (!slot.active) return;

  
   

    if (lifeTimer >= activeDuration) {
        state = SpellState::Disappear;
        lifeTimer = 0;
    }



}

void Shoot::UpdateDisappear(float dt, Match* server)
{
    float t = 1.0f - std::min(lifeTimer / disappearDuration, 1.0f);

    
        auto& slot = server->projectiles[ProjectileId];
        if (slot.active) {
            slot.data.fRadius = t * 0.25f;
        }
    

    if (t <= 0.0f) {
        // --- МОМЕНТ ВОЗВРАТА СТРЕЛЫ ---
        auto& entities = server->GetEntities();
        if (CasterId < entities.size() && entities[CasterId].active) {
            auto* hunter = dynamic_cast<Hunter*>(entities[CasterId].character.get());
            if (hunter) {
             
                hunter->UnregisterLiveArrow(ProjectileId);
            }
        }
            server->MarkProjectileForRemoval(ProjectileId);
           
        
        state = SpellState::Finished;
    }
}
