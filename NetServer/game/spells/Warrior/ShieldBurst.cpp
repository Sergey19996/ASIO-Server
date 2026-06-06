#include "ShieldBurst.h"
//#include "../../entities/Warrior.h"
#include "../NetShared/entities/Warrior.h"
#include "../../../Match.h"
#include "../NetShared/managers/EffectManager.h"

bool ShieldBurst::Cast(Character& caster, Match* server)
{
    // 1. Проверяем, что это Воин и у него активен эффект щита
   /* if (caster.GetClass() != PlayerClass::Warrior || !caster.GetEffects()->HasEffect(StatusEffectType::Shield)) {
        return false;
    }*/

    

    Warrior* warrior = static_cast<Warrior*>(&caster);
    uint8_t power = warrior->GetAdaptationLevel(); // от 0.0 до 1.0 (adaptationProgress / 100)
   
    // 2. Настраиваем параметры взрыва на основе адаптации
    ProjectileRules rules = GetProjectileRules(ProjectileType::BlastWave); // Берем базу от взрывной волны

    rules.dealsWorldDamage = true;
    float valuePower = power / 5;
  //  rules.damageToWorld = 100; // Чтобы гарантированно ломать соседние клетки
    rules.knockbackForce = valuePower; // Сила толчка растет от адаптации

    // Эффект замедления
    rules.effectToApply = StatusEffectType::Slow;
    rules.effectDuration = power / 2;
    rules.effectValue = 0.5f; // Замедление на 50%

    float finalDamage = 5.0f + (power / 5 * 15.0f);
    float finalRadius = warrior->radius * 3; // Радиус сильно растет от адаптации

    // 3. Вызываем взрыв вокруг воина
    server->ProcessAreaDamage(caster.position, finalRadius, finalDamage, caster.GetId(), false, rules);

    // 4. Спец-эффекты: Снимаем щит и обнуляем адаптацию
    caster.GetEffects()->Remove(StatusEffectType::Shield);
    warrior->ResetAdaptation(); // Тебе нужно будет добавить этот метод в класс Warrior

    state = SpellState::Finished;


    // 3. Публичное сообщение: шлем ВСЕМ в мире, чтобы у них переключилась анимация этого мага
    olc::net::message<GameMsg> msgLaunch;
    msgLaunch.header.id = GameMsg::Server_CastLaunch;
    msgLaunch << sCastLaunch{ caster.GetId(), (uint8_t)id };
    //server->BroadcastMessage(msgLaunch);
    server->MessageAllMatchClients(msgLaunch);
    return true;
}
