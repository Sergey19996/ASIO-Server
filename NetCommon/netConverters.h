#ifndef NETCONVERTERS_H
#define NETCONVERTERS_H
//#include "../NetServer/game/entities/Character.h"
#include "../NetShared/entities/Character.h"
#include "../NetShared/WorldTypes.h"
#include "../NetShared/managers/EffectManager.h"

inline sEntityDescription ToNet(const Character& c)
{
    sEntityDescription d;

    d.nUniqueID = c.GetId();

    // Кастуем enum к uint8_t — это абсолютно безопасно, так как все ID меньше 255
    d.nArchetypeId = static_cast<uint8_t>(c.GetArchetypeId());
    
    // HP в процентах
    d.nHealth = (uint8_t)(std::clamp(c.health / (float)c.maxHealth, 0.0f, 1.0f) * 255.0f);

    // Радиус (точность 0.01)
    d.radius = (int8_t)(c.radius * 100.0f);

    // Позиция и скорость
    d.posX = (int16_t)(c.position.x * 100.0f);
    d.posY = (int16_t)(c.position.y * 100.0f);
    d.velX = (int8_t)(c.inputVel.x * 127.0f);
    d.velY = (int8_t)(c.inputVel.y * 127.0f);
    // Переводим вектор направления в угол [0, 255]
    float angle = std::atan2(c.direction.y, c.direction.x); // [-pi, pi]
    if (angle < 0.0f) angle += 2.0f * 3.14f; // [0, 2pi]
    d.nDirection = (uint8_t)((angle / (2.0f * 3.14f)) * 255.0f);
    // Ресурсы
    d.fSpecialBar = (uint8_t)(std::clamp(c.GetResourceValue(), 0.0f, 1.0f) * 255.0f); // тут у вара adaptation progress 0->100
    d.fChargeRatio = c.IsCharging() ? (uint8_t)((c.GetChargeTimer() / c.GetMaxChargeTime()) * 255.0f) : 0;

    // Битовые маски и параметры
    d.nClassParam = c.GetExtraData(); // шарики - 0 -5
    d.nEffectsMask = c.GetEffects()->GetMask();
    
    d.nTeamID = c.teamId; // Передаем ID команды

    // Логика щитов через менеджер эффектов
    d.nShieldH = 0;
    auto* s = c.GetEffects()->GetEffect(StatusEffectType::Shield);
    if (!s) s = c.GetEffects()->GetEffect(StatusEffectType::IceShield);

    if (s && c.maxHealth > 0) {
        d.nShieldH = (uint8_t)(std::clamp(s->shieldHP / (float)c.maxHealth, 0.0f, 1.0f) * 255.0f);
    }

    return d;


}


#endif // !NetConverters_h
