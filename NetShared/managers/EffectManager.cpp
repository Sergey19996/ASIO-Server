#include "EffectManager.h"
#include "../entities/Character.h"
#include "../../NetServer/Match.h"
#include "../../NetServer/game/spells/Mage/SunMarker.h"
#include "../../NetServer/DamageContext.h"

EffectManager::EffectManager(Character* owner) : owner(owner) {
    memset(effectCache, 0, sizeof(effectCache));
}


void EffectManager::Add(const StatusEffect& e)
{
    for (auto& eff : effects)
    {
        if (eff.type == e.type) {
            // Обновляем силу (берем максимальную или новую)
            eff.value = e.value;
            // Обновляем время (продлеваем, а не сбрасываем)
            eff.timeLeft = std::max(eff.timeLeft, e.timeLeft);

            // ВАЖНО: МЫ НЕ ТРОГАЕМ eff.tickTimer! 
            // Он продолжает считать свои 0.5с независимо от обновлений.
            return;
        }
    }
    effects.push_back(e);
}

void EffectManager::Update(float dt)
{
#ifdef GAME_SERVER
    memset(effectCache, 0, sizeof(effectCache));
    uint32_t newMask = 0;
    Match* server = owner->GetServer(); // Предположим, у Character есть геттер
    
    for (int i = 0; i < effects.size(); ) {
        auto& effect = effects[i];
        effect.timeLeft -= dt;

        newMask |= (1 << (uint8_t)effect.type);
        effectCache[(uint8_t)effect.type] = &effect;

        // Логика Burn
        if (effect.type == StatusEffectType::Burn && server) {
            effect.tickTimer += dt;
            if (effect.tickTimer >= 0.5f) {
                effect.tickTimer = 0.0f;

                DamageContext ctx;
                ctx.baseDamage = effect.value;
                ctx.attackerId = effect.nOwnerNetID;
                ctx.targetId = server->GetPlayerIndex(owner->id);
                ctx.type = DamageType::Magical;
                ctx.source = DamageSource::Projectile;

                server->ApplyDamage(ctx);
                if (owner->IsDead()) server->OnCharacterDied(owner->GetId());
            }
        }
        if (effect.type == StatusEffectType::BindingChain) {
            glm::vec2 targetPos = effect.vCenterPos;

            // Если привязаны к игроку — обновляем цель каждый кадр
            if (effect.linkedEntityId != -1) {
                targetPos = server->GetEntities()[effect.linkedEntityId].character->position;
            }

            glm::vec2 toTarget = targetPos - owner->position;
            float dist = glm::length(toTarget);

            if (dist > 0.3f) {
                owner->knockbackVel += glm::normalize(toTarget) * (effect.value * (dist / 5.0f)) * dt;
            }

        }
        if (effect.type == StatusEffectType::StygianChill) {
            effect.tickTimer += dt;

            // Эффект "Хладной души" замедляет на 20% по умолчанию
            owner->knockbackVel *= 0.98f; // Маленькое постоянное торможение (вязкость)

            // Наносит чистый урон раз в 1.0 секунду (реже чем горение, но больнее)
            if (effect.tickTimer >= 1.0f) {
                effect.tickTimer = 0.0f;

                DamageContext ctx;
                ctx.attackerId = effect.nOwnerNetID;
                ctx.targetId = server->GetPlayerIndex(owner->id);
                ctx.targetType = DamageTargetType::Character;
                ctx.baseDamage = effect.value; // Например 15.0 Pure Damage
                ctx.type = DamageType::Pure;   // ИГНОРИРУЕТ ВСЁ
                ctx.source = DamageSource::Projectile;

                server->ApplyDamage(ctx);

                // Визуал: из персонажа вылетают ледяные искры
            }
        }
        if (effect.type == StatusEffectType::SunBrand) {
            effect.tickTimer += dt;
            if (effect.tickTimer >= 1.0f) {
                effect.tickTimer = 0.0f;

                // Создаем маркер через общую систему
                if (Spell* marker = server->HandleCastSpell(effect.nOwnerNetID, SpellId::SunMarker, 0)) {
                    // Приводим к нужному типу и передаем ID того, на ком висит эффект (this)
                    if (auto* sunMarker = dynamic_cast<SunMarker*>(marker)) {
                        sunMarker->targetId = server->GetPlayerIndex(owner->id);

                    }
                }
            }
        }
        // ... Сюда переносим BindingChain, StygianChill и SunBrand из твоего кода ...

        if (effect.timeLeft <= 0) {
            effects.erase(effects.begin() + i);
        }
        else {
            i++;
        }
    }
    currentEffectsMask = newMask;
#endif
}

void EffectManager::Remove(const StatusEffectType& t)
{
    effects.erase(
        std::remove_if(effects.begin(), effects.end(),
            [t](const StatusEffect& e) { return e.type == t; }),
        effects.end()
    );
}

StatusEffect* EffectManager::GetEffect(StatusEffectType t)
{
    if (!(currentEffectsMask & (1 << (uint8_t)t))) return nullptr;
    return effectCache[(uint8_t)t];
}

void EffectManager::Clear()
{
    effects.clear();
    currentEffectsMask = 0;

    // Обязательно обнуляем кэш, чтобы GetEffect не вернул мусор
    memset(effectCache, 0, sizeof(effectCache));
}

void EffectManager::RefreshModifiers()
{
#ifdef GAME_SERVER
    owner->SetSpeedModifier(owner->GetBaseSpeed()); // Сброс
    for (const auto& e : effects) {
    if (e.type == StatusEffectType::Slow) {
        owner->SetSpeedModifier(owner->GetCurrentSpeed() * e.value);
    }
    owner->bIsStunned = false;

   
        // Замедление
        if (e.type == StatusEffectType::Slow) {
            owner->SetSpeedModifier(owner->GetCurrentSpeed() * e.value);
        }

        // Стан
        if (e.type == StatusEffectType::Stun) {
            owner->bIsStunned = true;
            owner->SetSpeedModifier(0.0f);

        }

        // Можно добавить усиление урона (DamageMultiplier), если он есть в эффектах
    //    if (e.type == StatusEffectType::DamageBoost) {
            // owner->damageMultiplier *= e.value;
    //    }
    }
#endif

}
