#include "IceballSpell.h"
#include "../../../Match.h"  // здесь уже можно подключать
//#include "../../entities/Mage.h"
#include "../NetShared/entities/Mage.h"
#include "../NetShared/managers/CooldownManager.h"

bool IceballSpell::Cast(Character& caster, Match* server)
{
    // 1. СНАЧАЛА ПРОВЕРЯЕМ: может ли маг вообще совершить действие?
    if (!caster.CanCastSpell()) {
        return false;
    }

    Mage* mage = static_cast<Mage*>(&caster);
    if (!mage) return false;


    // Считываем баланс (для Кристалла не важно, + или -, берем силу накопления)
    float currentBalance = mage->GetRawBalance();
   
    if (currentBalance < 0.0f) {  // если шкала во фросте
     powerMultiplier = caster.GetDamageMultiplier(); // от него ещё и кд зависит 
    }
    else
    {
       powerMultiplier = 0.25f;
    }


    
    float balanceBonus = caster.GetSpellPowerBonus(SpellId::Iceball);
    basePos = caster.position + caster.direction * 0.35f;
    direction = caster.direction * 14.0f; // Ледяной шар летит быстрее (12.0 против 8.0)

    caster.LockMovement();
    caster.LockActions(); // Теперь CanCastSpell() для него будет false
    // Используем ProjectileType::Iceball
    //// 1. Создаем снаряд с базовым уроном мага
    ProjectileParams p;
    p.ownerId = caster.GetId();
    p.damageMod = powerMultiplier; // тот самый 1.0 - 2.2x
    p.effectMod = balanceBonus; // - 0 максимальное замедление - 1 вообще нет
   

    projectileUid = server->CreateProjectile(ProjectileType::Iceball, p, basePos, direction, targetRadius);

    if (projectileUid == 0xFFFF) {
        ReleaseCaster(server);
        return false;
    }

    casterIdx = server->GetPlayerIndex(caster.GetId());

    // ОТПРАВЛЯЕМ ОДИН РАЗ ВСЕМ КЛИЕНТАМ В ЗОНЕ ВИДИМОСТИ
    olc::net::message<GameMsg> msg;
    msg.header.id = GameMsg::Server_CastStart;
    msg << sCastStart{ caster.GetId(), (uint8_t)SpellId::Iceball };
    // server->BroadcastMessage(msg); // Или BroadcastToVisible
    server->MessageAllMatchClients(msg); //анимация старта каста


    return true;
}

bool IceballSpell::OwnsProjectile(int uid)
{
	return projectileUid == (uint16_t)uid;
}

void IceballSpell::ForceFinish(int uid, Match* server)
{
    if (powerMultiplier > 0.5) {

    auto& slot = server->projectiles[projectileUid];
    auto* caster = server->GetEntities()[casterIdx].character.get();

    // РАСЧЕТ ВЗРЫВА:
    // Чем выше был finalRatio (дольше заряжали), тем больше область покрытия
    // Базовый радиус взрыва 1.5, максимальный 4.5
    float explosionRadius = 0.25f + (this->explosionPower * 1.5f) * powerMultiplier;

    // Урон может быть небольшим (лед больше про контроль)
    int damage = 5 + (int)(2 * this->explosionPower);

    // Вызываем AreaDamage с флагом замедления (если в вашей системе это часть cachedRules)
    // Передаем slot.data.vPos — точку столкновения
    server->ProcessAreaDamage(slot.data.vPos, explosionRadius, damage, ownerId, false, slot.cachedRules);
    }

    // Визуальный эффект взрыва (если есть такая функция)
    // server->BroadcastExplosion(slot.data.vPos, explosionRadius, ProjectileType::Iceball);

    ReleaseCaster(server);
    server->MarkProjectileForRemoval(projectileUid);
    state = SpellState::Finished;
}

bool IceballSpell::CanBeCancelled() const
{
    return state == SpellState::Appear;
}

void IceballSpell::Cancel(Match* server)
{
    ReleaseCaster(server);

    // Если projectile уже создан — удалить
    if (projectileUid != 0xFFFF)
        server->MarkProjectileForRemoval(projectileUid);



    state = SpellState::Finished;
}

void IceballSpell::UpdateAppear(float dt, Match* server)
{
    auto& entities = server->GetEntities();
    auto& caster = *entities[casterIdx].character;
    auto* mage = dynamic_cast<Mage*>(&caster);
    auto& slot = server->projectiles[projectileUid]; // Получаем ссылку на слот снаряда
    // float t = std::min(lifeTimer / powerMultiplier, 1.0f);



    if (!slot.active) {
        state = SpellState::Finished;
        return;
    }

    float maxCharge = caster.GetMaxChargeTime();

    if (caster.IsCharging()) {
        // Логика "Перегрева"
        if (caster.GetChargeTimer() >= maxCharge) {
            appearDuration += dt; // Используем отдельную переменную вместо appearDuration
            if (appearDuration >= 0.5f) {
                caster.StopCharging();
                return;
            }
        }

        // Позиция и направление (обновляем, чтобы маг мог "довернуться" во время зарядки)
        direction = caster.direction * 6.0f;
        slot.data.vPos = caster.position + caster.direction * 0.35f;

        // РАДИУС теперь зависит от доли накопленного заряда относительно ТЕКУЩЕГО максимума
        float chargeRatio = 1.0f - caster.GetChargeTimer() / maxCharge; // от 1 до 0 

        // Визуально увеличиваем шар. 
        // powerMultiplier (баланс) определяет финальный размер, chargeRatio — текущий рост.
        slot.data.fRadius = (0.35f + (chargeRatio * 0.35f)) * powerMultiplier;
        this->targetRadius = slot.data.fRadius;
        return;
    }

    // --- КНОПКА ОТПУЩЕНА (ВЫСТРЕЛ) ---
    float finalCharge = caster.GetLastChargeTime();
    float finalRatio = finalCharge / maxCharge; // Насколько эффективно зарядили (0.0 - 1.0)

    // Применяем коэффициенты к снаряду
    slot.data.vVel = direction * (0.5f + finalRatio * 0.4f); // Скорость зависит от заряда
    slot.collisionEnabled = true;

    // УРОН: Базовый урон * множитель баланса * качество зарядки
    slot.cachedRules.damageToPlayers *= finalRatio;
    slot.cachedRules.windResistance = finalRatio;
    // ОТДАЧА: Чем мощнее был "затяжной" каст, тем сильнее отбросит мага
    glm::vec2 recoilDir = -glm::normalize(direction);
    float recoilForce = 3.0f * finalRatio * powerMultiplier;
    caster.knockbackVel += recoilDir * recoilForce;
    // Сброс и КД
    float cd = GetAdjustedCooldown() * (0.8f + finalRatio * 0.4f); // Мощный каст чуть дольше откатывается
    caster.GetCooldown()->Set(id, cd, server->matchTime);
    server->SendCooldownToClient(ownerId, id, cd);

    caster.OnSpellCast(id, finalRatio * -1.0f);

    this->explosionPower = finalRatio;
    ReleaseCaster(server);

    state = SpellState::Active;
    lifeTimer = 0;


    // 3. Публичное сообщение: шлем ВСЕМ в мире, чтобы у них переключилась анимация этого мага
    olc::net::message<GameMsg> msgLaunch;
    msgLaunch.header.id = GameMsg::Server_CastLaunch;
    msgLaunch << sCastLaunch{ caster.GetId(), (uint8_t)id };
    //server->BroadcastMessage(msgLaunch);
    server->MessageAllMatchClients(msgLaunch);

}

void IceballSpell::UpdateActive(float dt, Match* server)
{
    if (lifeTimer >= activeDuration) {
        state = SpellState::Disappear;
        lifeTimer = 0;
    }
}

void IceballSpell::UpdateDisappear(float dt, Match* server)
{
    float t = 1.0f - std::min(lifeTimer / powerMultiplier, 1.0f);


    auto& slot = server->projectiles[projectileUid];
    if (slot.active)
        slot.data.fRadius *= t;



    if (t <= 0.0f) {


        // Помечаем на удаление (вернет индекс в freeProjectileIds)
        server->MarkProjectileForRemoval(projectileUid);

        state = SpellState::Finished;
    }
}
