#pragma once

//#ifdef GAME_SERVER
#include "GameplayTypes.h"
#include "StatusEffect.h"

struct ProjectileRules
{
    bool blocksCharacters = false;
    bool dealsDamage = false;
    bool dealsWorldDamage = false;
    bool diesOnWorldCollision = false;
    bool diesOnCharacterCollision = false;
    bool hasDurability = false;
    bool ignoresWorld = false;

    // НОВОЕ: параметры урона
    int damageToPlayers = 0;
    int damageToWorld = 0;

    float knockbackForce = 0.0f; // Сила отбрасывания
    DamageType type;
    // НОВОЕ: Какой эффект накладывает снаряд?
    StatusEffectType effectToApply = StatusEffectType::None;
    float effectDuration = 0.0f;
    float effectValue = 0.0f; // Например, сила замедления или HP щита
    float homingStrength = 0.0f; // 0.0 - нет наведения, 2.0-5.0 - легкое, 10.0 - сильное
    float windResistance = 0.0f; // - 0 ветра нет для спела - 1 полная сила с ветром 
};
inline ProjectileRules GetProjectileRules(ProjectileType type)
{
    ProjectileRules rules;

    switch (type)
    {
    case ProjectileType::SmashHit:
        rules.dealsDamage = true;
        rules.dealsWorldDamage = true;
        rules.damageToPlayers = 10;
        rules.damageToWorld = 100; // Смэш сильно бьет по стенам
        rules.knockbackForce = 0.15f;
        rules.diesOnWorldCollision = true;
        rules.diesOnCharacterCollision = true;
        rules.type = DamageType::Physical;
        rules.windResistance = 0.0f; 
        break; // ОБЯЗАТЕЛЬНО

    case ProjectileType::Shoot:
        rules.dealsDamage = true;
        rules.dealsWorldDamage = true;
        rules.damageToPlayers = 8;
        rules.damageToWorld = 100;
        rules.knockbackForce = 0.25f;
        rules.diesOnWorldCollision = false;
        rules.diesOnCharacterCollision = true;
        rules.type = DamageType::Physical;
        rules.windResistance = 0.75f;
        rules.homingStrength = 1.0f;
        break;
    case ProjectileType::GhostArrow:
        rules.dealsDamage = true;
        rules.dealsWorldDamage = false;      // Обычно призрачные стрелы не ломают стены
        rules.damageToPlayers = 12;         // Урон чуть ниже, так как от неё нельзя спрятаться
        rules.damageToWorld = 0;
        rules.knockbackForce = 0.1f;        // Слабое отбрасывание
        rules.homingStrength = 10.0f;
        // КЛЮЧЕВЫЕ НАСТРОЙКИ:
        rules.ignoresWorld = true;           // Физический движок не должен её стопить
        rules.diesOnWorldCollision = false;  // Не уничтожается при входе в стену
        rules.diesOnCharacterCollision = true; // Исчезает при попадании в игрока (или false, если пронзает насквозь)

        rules.type = DamageType::Magical;    // Логично для призрачной стрелы
        rules.windResistance = 0.1f;        // Магическая стрела почти не подвержена ветру
        break;
    case ProjectileType::Fireball:
        rules.dealsDamage = true;
        rules.dealsWorldDamage = true;
        rules.damageToPlayers = 5;
        rules.damageToWorld = 100;
        rules.knockbackForce = 0.2f;
        rules.diesOnWorldCollision = false;
        rules.diesOnCharacterCollision = false;
        rules.type = DamageType::Magical;
      //  rules.type = DamageType::Fire;
        rules.effectToApply = StatusEffectType::Burn;
        rules.effectDuration = 3.0f;
        rules.effectValue = 2.5f;   // Наносит 2.5 урона за каждый тик (раз в 0.5 сек = 30 урона всего)
        rules.windResistance = 0.6f;
        rules.homingStrength = 1.0f;
        break;
    case ProjectileType::StygianSpike: // Ледяное копье (Tier 3 Ice)
        rules.dealsDamage = true;
        rules.dealsWorldDamage = true;
        rules.damageToPlayers = 25;          // Высокий базовый урон
        rules.damageToWorld = 100;
        rules.knockbackForce = 0.4f;         // Среднее отбрасывание
        rules.type = DamageType::Pure;       // ИГНОРИРУЕТ БРОНЮ

        // ЛОГИКА ПРОНЗАНИЯ:
        rules.diesOnCharacterCollision = false; // Пролетает сквозь врагов
        rules.diesOnWorldCollision = true;      // Но разбивается о стены

        // ЭФФЕКТ: Ожог души (Stygian Chill)
        rules.effectToApply = StatusEffectType::StygianChill;
        rules.effectDuration = 5.0f;
        rules.effectValue = 3.5f;
        rules.homingStrength = 100.0f;
        rules.windResistance = 0.2f;         // Тяжелое ледяное копье почти не сносит
        break;
    case ProjectileType::SunLance:
        rules.dealsDamage = true;
        rules.damageToPlayers = 25;          // Тяжелый урон
        rules.knockbackForce = 0.5f;         // Сильный толчок
        rules.type = DamageType::Magical;    // Огонь
        rules.diesOnCharacterCollision = true; // Взрывается при контакте
        rules.diesOnWorldCollision = true;

        // ЭФФЕКТ: Солнечное клеймо
        rules.effectToApply = StatusEffectType::SunBrand;
        rules.effectDuration = 6.0f;
        rules.effectValue = 10.0f;           // Базовый урон горения клейма
        rules.windResistance = 0.3f;
        rules.homingStrength = 10.0f;
        break;

    case ProjectileType::SunMarker:
        rules.dealsDamage = false;
        rules.diesOnCharacterCollision = false;
        rules.diesOnWorldCollision = false;
        rules.ignoresWorld = true;
        rules.effectToApply = StatusEffectType::None;
        // Этот снаряд живет 0.1с, просто чтобы клиент его отрисовал в этой точке
        break;

    case ProjectileType::ShieldThrow:
        rules.dealsDamage = true;
        rules.dealsWorldDamage = true;
        rules.damageToPlayers = 10;
        rules.damageToWorld = 100;
        rules.knockbackForce = 0.5f;
        rules.diesOnWorldCollision = true;
        rules.diesOnCharacterCollision = true;
        rules.type = DamageType::Physical;
        //  rules.type = DamageType::Fire;
        rules.effectToApply = StatusEffectType::Stun;
        rules.effectDuration = 1.0f;
        rules.effectValue = 2.5f;   // Наносит 5 урона за каждый тик (раз в 0.5 сек = 30 урона всего)
        rules.windResistance = 0.8f;
        rules.homingStrength = 10.0f;
        break;
    case ProjectileType::Iceball:
        rules.dealsDamage = true;
        rules.dealsWorldDamage = true;
        rules.damageToPlayers = 4;
        rules.damageToWorld =100;
        rules.knockbackForce = 0.5f;
        rules.diesOnWorldCollision = true;
        rules.diesOnCharacterCollision = true;
        rules.type = DamageType::Magical;
        rules.effectToApply = StatusEffectType::Slow;
        rules.effectDuration = 3.0f;
        rules.effectValue = 1.0f; // там в конце мы умнажаем на 1 - если 1 -то замедления нет - если 0.1 - то замедлеие 90 процентов 
        rules.windResistance = 0.4f;
        rules.homingStrength = 1.0f;
        break;
    case ProjectileType::IceRoots:

        rules.blocksCharacters = false;
        rules.dealsDamage = true;
        rules.dealsWorldDamage = true;
        rules.diesOnWorldCollision = false;
        rules.diesOnCharacterCollision = false;
        rules.hasDurability = false;
        rules.damageToPlayers = 5;
        rules.damageToWorld = 0;
        rules.knockbackForce = 1.2f;
        rules.type = DamageType::Physical;
        rules.effectToApply = StatusEffectType::Slow;
        rules.effectDuration = 3.0f;
        rules.effectValue = 1.0f;  // Наносит 5 урона за каждый тик (раз в 0.5 сек = 30 урона всего)
        break;
    case ProjectileType::HarvestEnergy:
        rules.dealsDamage = true;
        rules.dealsWorldDamage = true; // Жнец — это чистая энергия, стены не ломает
        rules.damageToPlayers = 15;     // Базовый урон за один "пролет" сквозь врага
        rules.damageToWorld = 100;
        rules.knockbackForce = 0.4f;    // Легкое отталкивание по направлению полета

        // ВАЖНО: Он не должен исчезать при коллизиях, иначе цепочка прервется
        rules.diesOnWorldCollision = false;
        rules.diesOnCharacterCollision = false;

        rules.ignoresWorld = false;      // Позволяет лететь сквозь стены к следующей стреле
        rules.type = DamageType::Magical; // Или Physical, если это "энергия ветра/стали"

        rules.effectToApply = StatusEffectType::None; // Можно добавить Slow, если нужно
        rules.windResistance = 0.0f;    // Жнец летит по жесткой траектории, ветер не помеха
        rules.homingStrength = 10.0f;
        break;

    case ProjectileType::Wall:
        rules.blocksCharacters = true;      // Враги не пройдут сквозь него
        rules.hasDurability = true;         // Его можно разбить
        rules.dealsDamage = false;          // Сам по себе не бьет
        rules.dealsWorldDamage = false;
        rules.diesOnWorldCollision = false; // Не исчезает при касании стен
        rules.type = DamageType::Magical;
        rules.effectValue = 10.0f;         // Используем как HP кристалла
        rules.ignoresWorld = true;
        break;
    case ProjectileType::Shield:
        rules.blocksCharacters = true;
        rules.hasDurability = true;
        rules.effectValue = 10.0f;
        // Остальное по дефолту (false/0)
        break;
    case ProjectileType::Explosion:
        // Используем современный синтаксис инициализации (без break внутри {})
    
        rules.blocksCharacters = false;
        rules.dealsDamage = true;
        rules.dealsWorldDamage = true;
        rules.diesOnWorldCollision = false;
        rules.diesOnCharacterCollision = false;
        rules.hasDurability = false;
        rules.damageToPlayers = 15.0f;
        rules.damageToWorld = 100;
        rules.knockbackForce = 1.2f;
        rules.type = DamageType::Physical;
        rules.effectToApply = StatusEffectType::Burn;
        rules.effectDuration = 2.0f;
        rules.effectValue = 2.5f;   // Наносит 5 урона за каждый тик (раз в 0.5 сек = 30 урона всего)
       
            break;
        
    case ProjectileType::BlastWave:
        rules.dealsDamage = true;
        rules.dealsWorldDamage = true;
        rules.damageToPlayers = 15; // Базовый, переопределим в коде
        rules.damageToWorld = 100;
        rules.knockbackForce = 1.2f;
        rules.type = DamageType::Magical;
        rules.effectToApply = StatusEffectType::Burn;
        rules.effectDuration = 2.0f;
        rules.effectValue = 2.5f;
        break;
    case ProjectileType::StickyBomb:
       
            rules.blocksCharacters = false;
            rules.dealsDamage = false; // Сама бомба при полете не дамажит
            rules.dealsWorldDamage = false;
            rules.diesOnWorldCollision = false;
            rules.diesOnCharacterCollision = false;
            rules.hasDurability = false;
            rules.damageToPlayers = 0;
            rules.damageToWorld = 0;
            rules.windResistance = 0.4f;
               break;
    case ProjectileType::MageCrystal:
        rules.blocksCharacters = true;      // Враги не пройдут сквозь него
        rules.hasDurability = true;         // Его можно разбить
        rules.dealsDamage = false;          // Сам по себе не бьет
        rules.dealsWorldDamage = false;
        rules.diesOnWorldCollision = false; // Не исчезает при касании стен
        rules.type = DamageType::Magical;
        rules.effectValue = 100.0f;         // Используем как HP кристалла
        break;
    case ProjectileType::Hook:
        rules.dealsDamage = true;
        rules.windResistance = 0.2f;
        rules.homingStrength = 10.5f;
     //   rules.dealsWorldDamage = true;
        //rules.damageToPlayers = 5;
       // rules.damageToWorld = 25; // Смэш сильно бьет по стенам
        break; // ОБЯЗАТЕЛЬНО
    case ProjectileType::BindShot:
        rules.blocksCharacters = false;
        rules.dealsDamage = true;          // Урон наносит сама связь позже, а не стрела
        rules.dealsWorldDamage = false;
        rules.diesOnWorldCollision = true;  // Исчезает при попадании в стену, вызывая NotifyWorldHit
        rules.diesOnCharacterCollision = true; // Исчезает при попадании в игрока, вызывая NotifyCharacterHit
        rules.knockbackForce = 0.0f;
        rules.type = DamageType::Physical;
        rules.windResistance = 0.5f;
        rules.homingStrength = 10.0f;
        // Эффект Linked мы наложим вручную в NotifyCharacterHit, 
        // чтобы контролировать логику связки двух целей.
        break;
    default:
        break;
    }

    return rules;
}
//#endif