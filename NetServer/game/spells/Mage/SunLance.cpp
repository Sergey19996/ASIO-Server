#include "SunLance.h"
#include "../NetShared/entities/Mage.h"
#include "../../../Match.h"

bool SunLance::Cast(Character& caster, Match* server)
{
    Mage* mage = dynamic_cast<Mage*>(&caster);
    // Проверка: нужно ровно -3.0 (максимальный лед)
    if (!mage || !caster.CanCastSpell() || mage->GetRawBalance() < 2.9f) {
        return false;
    }

    casterIdx = server->GetPlayerIndex(caster.GetId());
    ownerId = caster.GetId();

    caster.LockMovement();
    caster.LockActions();

    // Сразу сбрасываем баланс в 0 (тратим всю мощь льда)
    mage->ModifyBalance(-3.0f);

    state = SpellState::Appear;

    basePos = caster.position + caster.direction * 0.35f;
    direction = caster.direction * 7.0f; // Ледяной шар летит быстрее (12.0 против 8.0)

    // Подготавливаем параметры снаряда (пока он неподвижен в руках)
    ProjectileParams p;
    p.ownerId = ownerId;
    p.damageMod = powerMultiplier;
    // Создаем снаряд
    projectileUid = server->CreateProjectile(ProjectileType::SunLance, p, basePos, direction, targetRadius);
    if (projectileUid == 0xFFFF) {

        ReleaseCaster(server);
        return false;
    }

    casterIdx = server->GetPlayerIndex(caster.GetId());

    // ОТПРАВЛЯЕМ ОДИН РАЗ ВСЕМ КЛИЕНТАМ В ЗОНЕ ВИДИМОСТИ
    olc::net::message<GameMsg> msg;
    msg.header.id = GameMsg::Server_CastStart;
    msg << sCastStart{ caster.GetId(), (uint8_t)SpellId::SunLance };
    // server->BroadcastMessage(msg); // Или BroadcastToVisible
    server->MessageAllMatchClients(msg); //анимация старта каста

    return true;
}

bool SunLance::OwnsProjectile(int uid)
{
    return projectileUid == (uint16_t)uid;
}

void SunLance::ForceFinish(int uid, Match* server)
{
    ReleaseCaster(server);
    server->MarkProjectileForRemoval(projectileUid);
    state = SpellState::Finished;
}

void SunLance::UpdateAppear(float dt, Match* server)
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

    // 1. ПОДГОТОВКА (Пока тикает таймер появления)
     // Обновляем позицию, чтобы копье "росло" прямо перед магом
    slot.data.vPos = caster.position + caster.direction * 0.35f;

    // Визуальный эффект: копье сужается, становясь острой иглой
    float chargeRatio = std::min(lifeTimer / appearDuration, 1.0f); // 0 - 1 
    slot.data.fRadius = chargeRatio * 0.35f; // От 0.85 до 0.25

    // 2. МОМЕНТ ВЫСТРЕЛА
    if (lifeTimer > appearDuration) {
        ReleaseCaster(server);


        // Применяем коэффициенты к снаряду
        slot.data.vVel = direction * (2.0f); // Скорость зависит от заряда
        slot.collisionEnabled = true;


        // ОТДАЧА: Чем мощнее был "затяжной" каст, тем сильнее отбросит мага
        // Отдача (сила 6.0 — это ощутимый толчок назад)
        caster.knockbackVel -= caster.direction * 6.0f;

        state = SpellState::Active;
        lifeTimer = 0;


        // 3. Публичное сообщение: шлем ВСЕМ в мире, чтобы у них переключилась анимация этого мага
        olc::net::message<GameMsg> msgLaunch;
        msgLaunch.header.id = GameMsg::Server_CastLaunch;
        msgLaunch << sCastLaunch{ caster.GetId(), (uint8_t)id };
        //server->BroadcastMessage(msgLaunch);
        server->MessageAllMatchClients(msgLaunch);

    }

}

void SunLance::UpdateActive(float dt, Match* server)
{
    if (lifeTimer >= 1.5f) { // Копье существует недолго из-за высокой скорости
        state = SpellState::Finished;
        server->MarkProjectileForRemoval(projectileUid);
    }
}

void SunLance::UpdateDisappear(float dt, Match* server)
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
