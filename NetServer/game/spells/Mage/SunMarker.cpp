#include "SunMarker.h"
#include "../../../Match.h"

bool SunMarker::Cast(Character& caster, Match* server)
{


   
 

    glm::vec2 basePos = { 0,0 };
    glm::vec2 direction = { 0,0 };
   

    
    //auto& slot = server->projectiles[projectileUid];
    //// Усиливаем именно горение на основе баланса
    //slot.cachedRules.effectValue *= balanceBonus;

    // Сохраняем итоговый (модифицированный ветром) радиус, чтобы знать до чего расти


   
    //server->BroadcastProjectile(idx);
    return true;
}

void SunMarker::UpdateAppear(float dt, Match* server)
{
   
    if (auto& target = server->GetEntities()[targetId].character) { // берём цель 
       
    // Передаем параметры по ссылке. Метод изменит direction и initialRadius, если дует ветер
         //// 1. Создаем снаряд с базовым уроном мага
        ProjectileParams p;
        p.ownerId = ownerId;
        float targetRadius = 0.25f;

    projectileUid = server->CreateProjectile(ProjectileType::SunMarker, p, target->position, target->direction, targetRadius);

    state = SpellState::Active;
    lifeTimer = 0;

    if (projectileUid == 0xFFFF) {

        ReleaseCaster(server);
        state = SpellState::Finished;
       
    }
         }
  
   
}

void SunMarker::UpdateActive(float dt, Match* server)
{
    // 2. МОМЕНТ ВЫСТРЕЛА
    if (lifeTimer > activeDuration) {

        state = SpellState::Disappear;
        lifeTimer = 0;

    }
}


void SunMarker::UpdateDisappear(float dt, Match* server)
{

        // Помечаем на удаление (вернет индекс в freeProjectileIds)
        server->MarkProjectileForRemoval(projectileUid);

        state = SpellState::Finished;

}
