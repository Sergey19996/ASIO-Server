#include "IceShield.h"
#include "../../../Match.h"
#include "../NetShared/entities/Mage.h"
#include "../NetShared/managers/CooldownManager.h"
#include "../NetShared/managers/EffectManager.h"

bool IceShield::Cast(Character& caster, Match* server)
{
    if (!caster.CanCastSpell()) return false;

    Mage* mage = dynamic_cast<Mage*>(&caster);
    float b = mage->GetRawBalance();
    if (b > -1) return false;

    /// Расчет прочности: 30% от максимального здоровья мага
    float hpRatio = 0.3f;
    int shieldCapacity = static_cast<int>(caster.maxHealth * hpRatio);

    StatusEffect iceShield;
    iceShield.type = StatusEffectType::IceShield; // Используем общую логику щитов
    iceShield.timeLeft = 5.0f;
    iceShield.shieldHP = shieldCapacity; // Тот самый пул в 30% от ХП

    caster.GetEffects()->Add(iceShield);
   

    // Сдвигаем баланс
    mage->ModifyBalance(1.0f);

    // Кулдаун
    float cd = GetAdjustedCooldown();
    caster.GetCooldown()->Set(id, cd, server->matchTime);
    server->SendCooldownToClient(ownerId, id, cd);

    state = SpellState::Finished; // Спелл мгновенный, логика живет в эффекте
    return true;
}
