#include "IceRoots.h"
#include "../NetShared/entities/Mage.h"
#include "../../../Match.h"
#include "../NetShared/managers/CooldownManager.h"

bool IceRoots::Cast(Character& caster, Match* server)
{
    Mage* mage = dynamic_cast<Mage*>(&caster);
    if (!mage || !caster.CanCastSpell()) return false;

    float b = mage->GetRawBalance();
    // ПРОВЕРКА: Нужно минимум 2.0 льда, чтобы вообще начать каст
    if (b > -2.0f) return false;

    // Сначала сохраняем индекс, потом блокируем
    casterIdx = server->GetPlayerIndex(caster.GetId());
    ownerId = caster.GetId();

    caster.LockMovement();
    caster.LockActions();
  

    state = SpellState::Appear;
    return true;
}

void IceRoots::UpdateAppear(float dt, Match* server)
{
    auto& entities = server->GetEntities();
    auto& caster = *entities[casterIdx].character;
    auto* mage = dynamic_cast<Mage*>(&caster);
    if (!mage) { state = SpellState::Finished; return; }

    if (caster.IsCharging()) {
        float maxCharge = caster.GetMaxChargeTime();
        if (caster.GetChargeTimer() >= maxCharge + 0.5f) {
            caster.StopCharging();
        }
        return;
    }

    // --- МОМЕНТ ВЫПУСКА ---
    float finalCharge = caster.GetLastChargeTime();
    float chargeRatio = finalCharge / caster.GetMaxChargeTime();
    float currentBalance = mage->GetRawBalance(); // Например -3.0

    // ЛОГИКА "ОСТАВИТЬ -1.0 ДЛЯ ЩИТА":
   

    // 2. Если игрок заряжал кнопку, тратим и этот последний балл пропорционально зарядке
    float extraSpent = chargeRatio * 1.0f;

    // Итоговое изменение: базовая цена возвращает к -1.0, а зарядка сжигает остатки
    mage->ModifyBalance(2 + extraSpent);

    // ПАРАМЕТРЫ ЗАКЛИНАНИЯ
    ProjectileRules rules = GetProjectileRules(ProjectileType::IceRoots);
    float icePower = std::abs(currentBalance) / 3.0f;

    float baseRadius = 3.5f;
    float finalRadius = baseRadius + (chargeRatio * 1.5f) + (icePower * 1.0f);

    rules.effectToApply = StatusEffectType::Slow;
    rules.effectValue = 0.1f + (chargeRatio * 0.15f); // Почти полная остановка (0.95)
    rules.effectDuration = 2.0f + (chargeRatio * 2.0f);
    rules.knockbackForce = 0.0f;

    server->ProcessAreaDamage(caster.position, finalRadius, 10.0f, ownerId, false, rules);

    // КУЛДАУН
    float cd = GetAdjustedCooldown();
    caster.GetCooldown()->Set(id, cd, server->matchTime);
    server->SendCooldownToClient(ownerId, id, cd);

    ReleaseCaster(server);
    state = SpellState::Finished;
}
