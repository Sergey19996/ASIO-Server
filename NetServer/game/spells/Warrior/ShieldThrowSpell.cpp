#include "ShieldThrowSpell.h"
#include "../NetShared/entities/Warrior.h"
#include "../../../Match.h"
#include "../NetShared/managers/EffectManager.h"

bool ShieldThrowSpell::Cast(Character& caster, Match* server)
{
    if (caster.GetEffects()->HasEffect(StatusEffectType::Shield))
    {
       
        Warrior* war = dynamic_cast<Warrior*>(&caster);
      
        // Расход ресурса при броске
        powerMultiplier = war->GetAdaptationLevel(); // Запоминаем текущую мощь
        war->ResetAdaptation();        // Тратим всё сопротивление
        // caster.RemoveStatusEffect(StatusEffectType::Shield); // Убираем защиту

        caster.GetEffects()->Remove(StatusEffectType::Shield); // убирает эффект щита - это принудить щит исчезнуть 
        // Направление полета (куда смотрит игрок или мышь)
        glm::vec2 direction = caster.direction;
        glm::vec2 startPos = caster.position + direction * war->radius;
        glm::vec2 velocity = direction * 15.0f; // Скорость полета

        ProjectileParams p;
        p.ownerId = caster.GetId();
        p.damageMod = 1.0f + powerMultiplier / 5; // тот самый 1.0 - 2.2x
        p.effectMod = powerMultiplier / 2.5f; // 5 + 2.5 = 7.5 / 5 = 1.5 - время стана
        //p.damage = 20.0f + (power * 0.3f); // Урон зависит от ресурса
        //p.radius = 1.0f + (power * 0.02f); // ОБЛАСТЬ взрыва зависит от ресурса!
        float radius = 0.5f;
        ProjectileId = server->CreateProjectile(ProjectileType::ShieldThrow, p, startPos, velocity, radius);

        if (ProjectileId != 0xFFFF) {
            auto& slot = server->projectiles[ProjectileId];
            //  slot.data.fLifeTime = 2.0f; // Исчезнет через 2 сек, если никого не встретит
            slot.data.fLifetime = 2.0f;
            // Добавляем флаг, что этот снаряд станит при попадании
            slot.data.fRadius = 0.5f;
            slot.data.vVel = velocity;
            slot.collisionEnabled = true;
            //  slot.rules.stuns = true;
            //  slot.rules.explodeOnHit = true;

               // 3. Публичное сообщение: шлем ВСЕМ в мире, чтобы у них переключилась анимация этого мага
            olc::net::message<GameMsg> msgLaunch;
            msgLaunch.header.id = GameMsg::Server_CastLaunch;
            msgLaunch << sCastLaunch{ caster.GetId(), (uint8_t)id };
            //server->BroadcastMessage(msgLaunch);
            server->MessageAllMatchClients(msgLaunch);

        }

        return true;
    }

    return false;
}

void ShieldThrowSpell::ForceFinish(int uid, Match* server)
{
    state = SpellState::Finished;

}

bool ShieldThrowSpell::OwnsProjectile(int uid)
{
    return  ProjectileId == (uint16_t)uid;
}

void ShieldThrowSpell::UpdateAppear(float dt, Match* server)
{
}

void ShieldThrowSpell::UpdateActive(float dt, Match* server)
{
    if (lifeTimer >= activeDuration)
    {
        state = SpellState::Disappear;
        lifeTimer = 0;
    }
}

void ShieldThrowSpell::UpdateDisappear(float dt, Match* server)
{
    server->MarkProjectileForRemoval(ProjectileId);
    state = SpellState::Finished;
}
