#include "Character.h"
#include "../managers/ProgressionManager.h"
#include "../managers/SquadManager.h"
#include "../managers/CooldownManager.h"
#include "../managers/EffectManager.h"

#ifdef GAME_SERVER
#include "../../NetServer/Match.h"
#include "../../NetServer/game/spells/Mage/SunMarker.h"
#include "../AI/Companions/CompanionBase.h"


#endif

Character::~Character() = default; // Реализация в .cpp, где известны типы

Character::Character(uint32_t id, const std::string& name, Match* server) : id(id), name(name), server(server)
{
    // Инициализируем каждый менеджер, передавая нужные зависимости (this или server)
    cooldowns = std::make_unique<CooldownManager>();
    effects = std::make_unique<EffectManager>(this);
    squad = std::make_unique<SquadManager>(this, server);
    progression = std::make_unique<ProgressionManager>(this);
    m_classId = "None"_sid;
}

bool Character::CanCast(SpellId id, float timeNow) const
{
    
        return cooldowns->IsReady(id, timeNow) && !isDead;
    
}


#ifdef GAME_SERVER
Match* Character::GetServer()
{
    return server;
}
void Character::TakeDamage(int dmg)
{
    if (isDead)
        return;

    health -= dmg;
    if (health <= 0) {
        health = 0;
        Die();
    }
}

void Character::OnUpdate(const float dt, const float lightIntensity)
{
   

    if (isCharging) {
        chargeTimer += dt;
        float limit = GetMaxChargeTime();
        if (chargeTimer > limit) chargeTimer = limit; // Жесткое ограничение
    }
    

    glm::ivec2 currentTilePos = { (int)floor(position.x), (int)std::floor(position.y) };
    if (currentTilePos != lastTilePos) {
        if (onTileChangedPos) {
            onTileChangedPos(this->GetId(), currentTilePos, lastTilePos);
        }
        lastTilePos = currentTilePos;
    }
}


void Character::FullReset()
{

    // 2. Движение и физика
    inputVel = { 0.0f, 0.0f };
    knockbackVel = { 0.0f, 0.0f };
    currentMovingVel = { 0.0f, 0.0f };
    direction = { 1.0f, 0.0f }; // Взгляд по умолчанию
    movementLockStack = 0;

    // 3. Способности
   // spellCooldowns.clear(); // Сбрасываем все КД
    cooldowns->Clear();
    effects->Clear();
    squad->Clear();
    // 4. Эффекты
    //effects.clear(); // Удаляем все активные баффы/дебаффы
}

void Character::Revive()
{
    isDead = false;
    health = maxHealth;
}


void Character::Die()
{
    isDead = true;
    respawnTimer = RESPAWN_TIME;

}

void Character::Respawn(const glm::vec2& pos)
{
    isDead = false;
    health = 100;
    position = pos;
}



void Character::StartCharging(ActionSlot slot) // более характерно для игроков 
{
    isCharging = true;
    chargingSlot = slot;
    chargeTimer = 0.0f;
    lastChargeTime = 0.0f;
}

float Character::StopCharging() // более характерно для игроков 
{
    lastChargeTime = chargeTimer; // Фиксируем время перед сбросом
    isCharging = false;
    chargeTimer = 0.0f;
    return  lastChargeTime; // Возвращаем накопленное время (0.0 - 2.5)
}

bool Character::CanMove() const
{
    if (isDead) return false;
    
    // Стан блокирует движение физически
    if (effects->HasEffect(StatusEffectType::Stun)) return false;
    // Если стек блокировок не пуст (например, кастуем тяжелый спелл)
    return movementLockStack == 0;
}

bool Character::CanCastSpell() const
{
    if (isDead) return false;
    // Стан и Молчание (Silence) блокируют магию
    if (effects->HasEffect(StatusEffectType::Stun) || effects->HasEffect(StatusEffectType::Silence)) return false;
    // Если мы уже заняты другим кастом (actionLockStack > 0)
    return actionLockStack == 0;
}

#endif