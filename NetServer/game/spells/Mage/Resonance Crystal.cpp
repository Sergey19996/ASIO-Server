#include "Resonance Crystal.h"
#include "../../../Match.h"  // здесь уже можно подключать
#include "../../utils/WorldGrid.h"
#include "../NetShared/entities/Mage.h"

bool ResonanceCrystal::Cast(Character& caster, Match* server)
{
    // 1. СНАЧАЛА ПРОВЕРЯЕМ: может ли маг вообще совершить действие?
    if (!caster.CanCastSpell()) {
        return false;
    }


    caster.LockMovement();
    caster.LockActions(); // Теперь CanCastSpell() для него будет false

    Mage* mage = static_cast<Mage*>(&caster);
    if (!mage) return false;

    
    // Считываем баланс (для Кристалла не важно, + или -, берем силу накопления)
    float currentBalance = mage->GetRawBalance();
   

    // 1. Находим позицию перед магом (например, в 1.5 метрах)
    glm::vec2 direction = glm::normalize(caster.direction); // По умолчанию вправо
    // Если у персонажа есть вектор направления взгляда, используем его:
    // direction = caster.GetLookDirection(); 

    glm::vec2 spawnPos = caster.position + direction * 1.5f;
    float radius = 0.7f;
    // 2. Создаем снаряд через сервер
    // Нам нужен свободный слот для прожектаила
      //// 1. Создаем снаряд с базовым уроном мага
    ProjectileParams p;
    p.ownerId = caster.GetId();
    projectileUid = server->CreateProjectile(ProjectileType::MageCrystal, p, spawnPos, direction, radius);
    if (projectileUid == 0) return false; // Не удалось создать

    auto& slot = server->projectiles[projectileUid];
    slot.data.vPos = spawnPos;
    slot.data.vVel = { 0.0f, 0.0f }; // Кристалл стоит на месте
    slot.data.fRadius = 0.0f;      // Довольно крупный объект

    // Используем баланс для настройки прочности
    // Базовое HP (например 50) + множитель от баланса
   
   
   
    // Можно динамически усилить HP кристалла в зависимости от баланса:
    // slot.cachedRules.effectValue = 50.0f + (absBalance * 50.0f);
   // float reverseResoruce = (absBalance * 6) - 3;
    // 3. Сбрасываем баланс мага (разрядка)
    mage->ModifyBalance(-currentBalance);

    this->ownerId = caster.GetId();
   // this->state = SpellState::Finished; // Сам спелл мгновенный, жизнь передана кристаллу
    return true;
}

void ResonanceCrystal::ForceFinish(int uid, Match* server)
{
    ReleaseCaster(server);
    state = SpellState::Finished;
}

void ResonanceCrystal::UpdateAppear(float dt, Match* server)
{

    float t = std::min(lifeTimer / appearDuration, 1.0f);

    // Получаем ссылку на слот снаряда
    auto& slot = server->projectiles[projectileUid];
    if (!slot.active) {
        state = SpellState::Finished;
        return;
    }

    //  p.scale = t;
      // Визуальный рост шара перед лицом
    slot.data.fRadius = t * 0.7f;


    if (t >= 1.0f) {

        ReleaseCaster(server);
        state = SpellState::Active;
        slot.collisionEnabled = true;
        lifeTimer = 0;
    }
}

void ResonanceCrystal::UpdateActive(float dt, Match* server) {
    auto& slot = server->projectiles[projectileUid];

    // Если кристалл разбили (враги или маг огнем), завершаем спелл
    if (!slot.active) {
        state = SpellState::Finished;
        return;
    }

    // Время жизни кристалла ограничено activeDuration
    if (lifeTimer >= activeDuration) {
        state = SpellState::Disappear;
        lifeTimer = 0;
    }
}

void ResonanceCrystal::UpdateDisappear(float dt, Match* server) {
    float t = 1.0f - std::min(lifeTimer / disappearDuration, 1.0f);
    auto& slot = server->projectiles[projectileUid];

    if (slot.active) {
        slot.data.fRadius = t * 0.7f;
    }

    if (t <= 0.0f) {
        server->MarkProjectileForRemoval(projectileUid);
        state = SpellState::Finished;
        ReleaseCaster(server);
    }
}