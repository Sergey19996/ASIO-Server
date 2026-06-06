#include "InfuseArrow.h"
//#include "../../entities/Hunter.h"
#include "../NetShared/entities/Hunter.h"
#include "../../../Match.h"
#include "../NetShared/managers/CooldownManager.h"

bool InfuseArrow::Cast(Character& caster, Match* server)
{
    auto* hunter = dynamic_cast<Hunter*>(&caster);
    if (!hunter || hunter->IsReloading()) return false;

    CasterId = server->GetPlayerIndex(caster.GetId());
    //ownerId = caster.GetId();

    // Блокируем передвижение на время концентрации (по желанию)
    caster.LockMovement();
    caster.LockActions();
   
    state = SpellState::Appear;
    lifeTimer = 0.0f;
    power = 0; // В данном случае power — это количество поглощенных стрел

 
    return true;;
}

void InfuseArrow::UpdateAppear(float dt, Match* server)
{
  
    auto& entities = server->GetEntities();
    if (CasterId >= entities.size() || !entities[CasterId].active) {
        state = SpellState::Finished;
        return;
    }

        auto* hunter = static_cast<Hunter*>(entities[CasterId].character.get());
       
        if (hunter->IsCharging()) {
            // Каждые 0.4 сек зажатия поглощаем одну стрелу
            // Используем lifeTimer для отсчета тиков поглощения

            float timeStep = 0.1f;
            int expectedAbsorbed = (int)(lifeTimer / timeStep);
           
            if (expectedAbsorbed > power) {
                int targetIdx = hunter->GetInfuseTargetIndex(); // cмотрит на active и заряд <3 то есть и спец стрелы 

                // ПРОВЕРКА ЛИМИТА: Если в целевой стреле уже 4 влитых (итого 5), стоп.
                if (targetIdx != -1 && hunter->quiver[targetIdx].bonusPower >= 3) {
                    hunter->StopCharging();// Можно добавить эффект "переполнения" или тряску
                    state = SpellState::Finished;
                    lifeTimer = 0.0f;
                    ReleaseCaster(server); // Разблокируем движение охотника

                 
                  
                    return;
                }

                int sourceIdx = hunter->GetInfuseSourceIndex(targetIdx); // то же берёт любую active 
               
                // Условие: есть что поглощать, есть куда вливать, и это не одна и та же стрела
                if (sourceIdx != -1 && targetIdx != -1 && sourceIdx != targetIdx) {

                    auto& source = hunter->quiver[sourceIdx];
                    auto& target = hunter->quiver[targetIdx];

                    // 1. Переносим спец-эффекты (накапливаем их в целевой стреле)
                    if (source.hasExplosion) target.hasExplosion = true;
                    if (source.hasBinding)   target.hasBinding = true;
                    if (source.hasGhost)     target.hasGhost = true;

                    // 2. Увеличиваем BonusPower цели
                    target.bonusPower++;

                    // 3. Уничтожаем поглощенную стрелу
                  //  source.active = false;
                  //  source.hasExplosion = source.hasBinding = source.hasGhost = false;
                  //  source.bonusPower = 0;           // И её накопленную мощь
                    source.ResetArrow();
                    hunter->DecrementArrows();
                    source.active = false;
                    power++; // Увеличиваем локальный счетчик поглощений
                    
                    // Здесь можно вызвать server->SendEffect(...), чтобы проиграть звук "вжик"
                }
            }
            return; // Продолжаем висеть в Appear, пока зажата кнопка
        }
        //if (power == 0) {
        // 
        //    int idx = hunter->GetNextArrowIndex(true); // Ищем спецуху справа
        //    if (idx != -1) { // мы нашли особенную стрелу 
        //       // ArrowType type = hunter->quiver[idx].type;
        //        auto& arrow = hunter->quiver[idx];
        //       
        //       
        //        SpellId sid = SpellId::None;
        //      
        //        if (arrow.hasExplosion) {

        //            sid = SpellId::StickyBomb; 
        //        }
        //        else if (arrow.hasBinding) {

        //            sid = SpellId::BindArrow; 
        //        } else if (arrow.hasGhost) {

        //            sid = SpellId::GhostArrow; 
        //        }


        //        if (sid != SpellId::None) {
        //            // Прямо вызываем глобальный обработчик!
        //            // Это создаст честную StickyBomb со всей её логикой
        //           

        //            Spell* spawnedSpell = server->HandleCastSpell(ownerId, sid, 0);
        //            
        //            if (spawnedSpell) {
        //                // Тебе нужно добавить метод в базовый класс Spell или кастить к HunterProjectile
        //                // чтобы он знал: я не просто бомба, я еще и призрачная связка.
        //                spawnedSpell->ApplyHunterFlags(arrow.hasExplosion, arrow.hasBinding, arrow.hasGhost, arrow.bonusPower);
        //            }

        //            lifeTimer = 0.0f;
        //            ReleaseCaster(server); // Разблокируем движение охотника
        //            state = SpellState::Finished; // Закрываем инфузию
        //            // Сброс и КД
        //            float cd = GetAdjustedCooldown(); // Мощный каст чуть дольше откатывается
        //            hunter->SetCooldown(id, cd, server->matchTime);
        //            server->SendCooldownToClient(ownerId, id, cd);
        //            ReleaseCaster(server);
        //            return;
        //        }
        //    }
        //}
   
        // Кнопка отпущена — переходим в Active (короткая фаза завершения)
        state = SpellState::Finished;
        lifeTimer = 0.0f;
        float cd = GetAdjustedCooldown(); // Мощный каст чуть дольше откатывается
        hunter->GetCooldown()->Set(id, cd, server->matchTime);
        server->SendCooldownToClient(ownerId, id, cd);
        ReleaseCaster(server); // Разблокируем движение охотника
}

void InfuseArrow::UpdateActive(float dt, Match* server)
{
    // Просто визуальная пауза после завершения инфузии
    if (lifeTimer >= activeDuration) {
        state = SpellState::Finished;
    }
}

    void InfuseArrow::UpdateDisappear(float dt, Match * server)
    {
        state = SpellState::Finished;
    }

    // Заглушки, так как физического снаряда у этого спелла нет
    bool InfuseArrow::OwnsProjectile(int uid) { return false; }
    void InfuseArrow::ForceFinish(int uid, Match* server) { state = SpellState::Finished;   ReleaseCaster(server);}
    void InfuseArrow::NotifyWorldHit(glm::ivec2 cell, Match* server) {}
    void InfuseArrow::NotifyCharacterHit(uint32_t ProjID, uint32_t TargetID, Match* server) {}