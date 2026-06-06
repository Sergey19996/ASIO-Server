#include "FireDash.h"
#include "../../../Match.h"  // здесь уже можно подключать
//#include "../../entities/Mage.h"
#include "../NetShared/entities/Mage.h"
#include "../NetShared/managers/CooldownManager.h"

FireDash::FireDash()
{
  
    id = SpellId::FireDash; // Твой Enum ID

    // Тайминги: мгновенная подготовка, быстрый полет, короткий выход
    appearDuration = 0.05f;
    activeDuration = 0.25f;
    disappearDuration = 0.1f;
}

bool FireDash::Cast(Character& caster, Match* server)
{
    Mage* mage = dynamic_cast<Mage*>(&caster);
    if (!mage) return false;

    float b = mage->GetRawBalance();
    // Рывок можно кастовать всегда, но его сила зависит от огня (+1 и выше)
    if (b < 1.0f) return false;
    float power =  (b / 3.0f);

    // 1. Рассчитываем параметры рывка
    float baseDistance = 250.0f; // Базовая дистанция в пикселях/юнитах
    float finalDistance = baseDistance * (1.0f + power * 0.5f); // +50% к дальности на пике огня

  //  glm::vec2 mousePos = server->GetMouseWorldPos(ownerId);
  //  glm::vec2 dir = glm::normalize(mousePos - caster.position);

    // 2. Логика мгновенного перемещения или подготовки активной фазы
    // Если это TimedSpell, мы запоминаем параметры для UpdateActive
    this->dashDirection = caster.direction;
    this->dashSpeed = 800.0f + (power * 400.0f); // Быстрее, если много огня
    this->activeDuration = finalDistance / this->dashSpeed;

    // 3. Расход ресурса (например, рывок потребляет +1 баланса или пропорционально)
  
    powerMultiplier = b;
    // 4. Управление КД (как в твоем примере с щитом)
    // Допустим, рывок блокирует возможность использовать "Ледяную иглу" (-1)
   // SpellId iceCounterpart = SpellId::IceNeedle;
    //float cooldown = 3.0f;
    //caster.SetCooldown(id, cooldown, server->matchTime);
    //server->SendCooldownToClient(caster.GetId(), id, cooldown);

    casterIdx = server->GetPlayerIndex(caster.GetId());
    // Переходим в состояние появления (подготовка анимации)
    state = SpellState::Appear;
    return true;
}

void FireDash::UpdateAppear(float dt, Match* server)
{
    auto& entities = server->GetEntities();
    auto& caster = *entities[casterIdx].character;
    auto* mage = dynamic_cast<Mage*>(&caster);

    if (!mage) { state = SpellState::Finished; return; }

    float maxCharge = caster.GetMaxChargeTime();

    // 1. ПРОЦЕСС ЗАРЯДКИ (Кнопка держится)
    if (caster.IsCharging()) {
        // Ограничитель перегрузки (как в твоем фаерболе)
        if (caster.GetChargeTimer() >= maxCharge) {
            lifeTimer += dt;
            if (lifeTimer >= 0.5f) { caster.StopCharging(); return; }
        }

        // Визуальный эффект: маг "дрожит" или копит искры
        // Можно обновлять направление рывка вслед за мышкой/поворотом
        dashDirection = caster.direction;
        return;
    }

    // 2. МОМЕНТ ВЫПУСКА (Кнопка отпущена)
  // 2. МОМЕНТ ВЫПУСКА (Кнопка отпущена)
    float finalCharge = caster.GetLastChargeTime();
    float chargeRatio = std::max(0.1f, finalCharge / maxCharge); // Минимум 0.1, чтобы клик работал

    float currentBalance = mage->GetRawBalance();

    // Расчет затрат: теперь МИНИМУМ 1.0, но если зажали — тратим до 3.0 (весь пик огня)
    // Важно: тратим только если баланс положительный
    float potentialCost = 1.0f + (chargeRatio * 2.0f);
    float actualSpent = (currentBalance > 0) ? std::min(currentBalance, potentialCost) : 0.0f;

    if (actualSpent > 0) mage->ModifyBalance(-actualSpent);

    // --- КОРРЕКТИРОВКА СКОРОСТИ И ДИСТАНЦИИ ---
    // Теперь бонус от огня (actualSpent) работает ТОЛЬКО в связке с тем, сколько мы потратили
    // actualSpent здесь выступает как топливо.
    float speedBonus = actualSpent * 10.0f;     // Максимум +30 к скорости при полном заряде +3.0
    float distanceBonus = actualSpent * 4.0f;  // Максимум +12 клеток при полном заряде +3.0

    // Базовая скорость для "пустого" мага (или во льду)
    float baseDashSpeed = 25.0f;
    dashSpeed = baseDashSpeed + speedBonus;

    // Базовая дистанция
    float baseDistance = 5.0f;
    float distance = (baseDistance + distanceBonus) * powerMultiplier;

    // Время активной фазы
    activeDuration = distance / dashSpeed;


    // Если хотим, чтобы рывок был МГНОВЕННЫМ рывком, можно зарезать время еще сильнее:
    if (activeDuration > 0.3f) activeDuration = 0.3f;

    // КУЛДАУН: Зависит от того, насколько "тяжелым" был рывок
    float cd = GetAdjustedCooldown() * (0.7f + chargeRatio * 0.5f);
    caster.GetCooldown()->Set(id, cd, server->matchTime);
    server->SendCooldownToClient(ownerId, id, cd);

    // ОСВОБОЖДАЕМ: теперь UpdateActive будет двигать персонажа
    caster.UnlockActions(); // Разблокируем, чтобы он мог лететь
    state = SpellState::Active;
    lifeTimer = 0;
}

void FireDash::UpdateActive(float dt, Match* server)
{
    auto& entities = server->GetEntities();
    auto& caster = *entities[casterIdx].character;

    lifeTimer += dt;

    // Насильно ставим скорость КАЖДЫЙ кадр, игнорируя friction из Match.cpp
    caster.knockbackVel = dashDirection * dashSpeed;

    // На высоких скоростях проверяем столкновение жестче
    // Если Match::UpdateCharacters отработал коллизию, он изменил knockbackVel
    if (glm::length(caster.knockbackVel) < dashSpeed * 0.7f) {
        state = SpellState::Disappear;
        return;
    }

    if (lifeTimer >= activeDuration) {
        // Гасим скорость резко, чтобы не скользить после рывка слишком долго
        caster.knockbackVel *= 0.2f;
        state = SpellState::Disappear;
        lifeTimer = 0;
    }
}

void FireDash::UpdateDisappear(float dt, Match* server)
{
     if (lifeTimer >= disappearDuration) {
        // Возвращаем управление игроку
        ReleaseCaster(server); 
        state = SpellState::Finished;
    }
}
