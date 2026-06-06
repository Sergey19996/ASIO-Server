#include "Mage.h"
#ifdef GAME_SERVER
#include "../../NetServer/Match.h"
#endif // !1
#include "../managers/EffectManager.h"
#include "../../NetServer/DamageContext.h"

Mage::Mage(uint32_t id, const std::string& name, Match* server) : Character(id, name, server)
{
  
    // Инициализация специфичных полей Воина
    /*this->adaptationLevel = 0;
    this->adaptationProgress = 0.0f;
    this->currentAdaptation = AdaptationType::None;*/

    boundSpells[ActionSlot::Slot1] = SpellId::Fireball;  // лкм
    boundSpells[ActionSlot::Slot3] = SpellId::MagicConservation;    //мид бласт по дефолту  SwitchSpell
    boundSpells[ActionSlot::Slot2] = SpellId::Iceball;   //ркм
    boundSpells[ActionSlot::Slot4] = SpellId::FireDash;   //1  SpellId::IceShield
    boundSpells[ActionSlot::Slot5] = SpellId::StygianSpike;    //2  SpellId::FireDash   SpellId::IceRoots
    boundSpells[ActionSlot::Slot6] = SpellId::SunLance; // 3   SpellId::StygianSpike
    boundSpells[ActionSlot::Slot7] = SpellId::SwitchSpell;
    // Начальные статы класса

    boundSpells[ActionSlot::Slot9] = SpellId::CommandMove;
    boundSpells[ActionSlot::Slot10] = SpellId::CommandDefend;

    this->maxHealth = 25;
    this->health = 25;
    
    this->radius = 0.5f;

    m_classId = "Mage"_sid;
    entityType = ArchetypeId::Player_Mage;
#ifdef GAME_SERVER
    this->damageMultiplier = 1.0f;

    // Спавним 3-х спутников для Воина (например, 2 лучника для прикрытия и 1 маг)
    //if (server) {
    //    // Просим сервер заспавнить нам помощника
    //    server->pendingSpawns.push_back({ EntityType::Companion, NpcType::WarriorCompanion, server->GetSpawnPoint(id), this });
    //    server->pendingSpawns.push_back({ EntityType::Companion, NpcType::WarriorCompanion, server->GetSpawnPoint(id), this });
    //    server->pendingSpawns.push_back({ EntityType::Companion, NpcType::WarriorCompanion, server->GetSpawnPoint(id), this });
    //    server->pendingSpawns.push_back({ EntityType::Companion, NpcType::RangerCompanion, server->GetSpawnPoint(id), this });
    //    server->pendingSpawns.push_back({ EntityType::Companion, NpcType::RangerCompanion, server->GetSpawnPoint(id), this });
    //    server->pendingSpawns.push_back({ EntityType::Companion, NpcType::PriestCompanion, server->GetSpawnPoint(id), this });
    //    server->pendingSpawns.push_back({ EntityType::Companion, NpcType::PriestCompanion, server->GetSpawnPoint(id), this });
    //    // Опционально: сохраняем указатель в squad для быстрого доступа
    //   // auto compPtr = std::dynamic_pointer_cast<CompanionBase>(server->GetCharacterById(compId));
    // //   if (compPtr) squad.push_back(compPtr);
    //}
#endif // !1

}
#ifdef GAME_SERVER
void Mage::OnUpdate(const float dt, const float lightIntensity)
{
    if (IsDead()) return;
    // 1. Естественное затухание (стремление к балансу)  // баланс от -3 до + 3 
    if (balance > 0.1f) balance -= RECOVERY_RATE * dt;
    else if (balance < -0.1f) balance += RECOVERY_RATE * dt;
    else balance = 0.0f;

    // 2. Логика перегрева (ОГОНЬ > 2.5)
    if (balance > 2.0f) {

        StatusEffect Burn;
        Burn.type = StatusEffectType::Burn;
        Burn.value = 1.0f * (balance / MAX_ABS_BALANCE);
        Burn.timeLeft = 0.2f;
        Burn.nOwnerNetID = this->id;
        effects->Add(Burn);
      //  AddStatusEffect(Burn);
    }
    else
    {
        if (overheatRecoverPool > 0.0f)
        {
            //float icePower = std::abs(balance) / 3.0f; // от 0.5 до 1.0

            // 1. Считаем желаемую скорость в СЕКУНДУ
            float speedProportion = overheatRecoverPool * 0.2f; // 20% пула в секунду
            float speedBase = 5.0f;                             // 5 HP в секунду

            // 2. Выбираем максимальную скорость и переводим её в ТЕКУЩИЙ КАДР (* dt)
            float currentSpeed = std::max(speedProportion, speedBase);
            float frameHeal = currentSpeed * dt;

            // 3. Накапливаем тик
            healTick += frameHeal;
       
            if (healTick >= 1.0f) {
                // Важно: не лечим больше, чем осталось в пуле
                float finalHeal = std::min(healTick, overheatRecoverPool);

                this->Heal(finalHeal);
                overheatRecoverPool -= finalHeal;
                healTick = 0.0f;
           
            }



        }
    }

    // 3. Логика переохлаждения (ЛЁД < -2.5)
    if (balance < -0.1f) {
        // Накладываем эффект замедления на самого себя
        // StatusEffect(Тип, Сила, Длительность)
        float abs = 1.0 - (std::abs(balance) / 3.0f) * 0.25f;

        StatusEffect slow;
        slow.type = StatusEffectType::Slow;
        slow.value = std::clamp(abs, 0.0f, 1.0f);
        slow.timeLeft = 0.2f;
        effects->Add(slow);
      //  AddStatusEffect(slow);
    }
    Character::OnUpdate(dt, lightIntensity);
}

void Mage::OnProcessIncomingDamage(DamageContext& ctx)
{

    // Проверяем: урон нанесен самим собой (ID атакующего == мой ID)
   // И источник урона — магия (твой эффект Burn из OnUpdate)
    if (ctx.attackerId == this->GetId() && ctx.type == DamageType::Magical)
    {
        if (health - ctx.finalDamage <= 0) {
            health += ctx.finalDamage;
            return;
        }

        // Накапливаем урон, который мы сейчас получим, в пул регенерации
        // Мы берем ctx.finalDamage, так как это то, что реально вычтется из HP
        this->overheatRecoverPool += ctx.finalDamage;
        
      

    }

}

void Mage::FullReset()
{
    Character::FullReset();
}

void Mage::ModifyBalance(float amount)
{
    //bool isFire = amount > 0; // Определяем сторону -- если кастится фаер бол то положительное число - нет - значит фрост

    //if (isFire) {
    //    balance = std::abs(balance);
    //}
    //else
    //{
    //}
   // float absBalance = std::abs(balance);

    balance = std::clamp(balance + amount, -MAX_ABS_BALANCE, MAX_ABS_BALANCE);
    balance *= -1;

}

float Mage::GetMaxChargeTime() const
{
    // 1. Определяем, какой спелл сейчас на зарядке
    SpellId currentSpell = GetBoundSpell(chargingSlot);

    // 2. Базовая проверка "родства" стихии и баланса
    // balance > 0 (Огонь), balance < 0 (Лед)
    bool isFireSpell = (currentSpell == SpellId::Fireball);
    bool isIceSpell = (currentSpell == SpellId::Iceball);

    bool isCompatible = (isFireSpell && balance > 0.0f) || (isIceSpell && balance < 0.0f);

    // 3. Если кастуем "чужую" стихию (например, лед, когда мы в огне)
    if (!isCompatible && (isFireSpell || isIceSpell)) {
        return 0.25f; // Минимальный каст без возможности усиления
    }

    // 4. Если стихия "родная", рассчитываем прогрессивное время
    // Нормализуем влияние баланса (от 0.0 до 1.0)
    float balanceWeight = std::abs(balance) / 3.0f; // если макс баланс 3.0

    // 1.3 + 0.1f = 1.4
    return 0.1f + (balanceWeight * 1.3f);
}

float Mage::GetSpellPowerBonus(SpellId spell) const  // Ignite/slow эффекты расчёт
{

    if (spell == SpellId::Fireball) {

        return  std::max(0.25f, balance); // Бонус огня  
    }
    if (spell == SpellId::Iceball) {

        float abs = 0.9f; // 10 процентов
        if (balance < 0.0f) {  // если шкала во фросте
            abs = 1.0f - std::abs(balance) / 3.0f;
            return abs;
        }
        else
        {
            return abs;

        }
    }
    return 1.0f;
}

void Mage::OnSpellCast(SpellId spell, float timeprop)
{
    if (spell == SpellId::Fireball) ModifyBalance(timeprop);
    if (spell == SpellId::Iceball) ModifyBalance(timeprop);
}

float Mage::GetDamageMultiplier() const
{
    float absBal = std::abs(balance);


    float bonus = absBal * 0.5f;  //  0;  +1.5

    return 0.1f + bonus; // 1.6
}



void Mage::CastSwitch()
{
    this->balance = -this->balance;
    // Опционально: эффект или лог
}

void Mage::CastConservation()
{
    if (!hasConserved) {
        // 1. Поглощение: забираем текущий баланс в "банку"
        conservedBalance = balance;
       
        balance = 0.0f; // Сбрасываем текущий баланс в нейтраль
        hasConserved = true;
    }
    else {
        // 2. Возврат: выливаем из "банки" обратно
        // Можно либо прибавить к текущему, либо просто заменить
        balance = conservedBalance;
       
        conservedBalance = 0.0f;
        hasConserved = false;
    }
}
#endif // !1

SpellId Mage::GetBoundSpell(ActionSlot slot) const
{
    bool isFire = balance > 0; // Определяем сторону

    switch (slot) {
    case ActionSlot::Slot4:
        return isFire ? SpellId::FireDash : SpellId::IceShield;

    case ActionSlot::Slot5:
        return isFire ? SpellId::BlastWave : SpellId::IceRoots;

    case ActionSlot::Slot6:
        return isFire ? SpellId::SunLance : SpellId::StygianSpike;

    default:
        return Character::GetBoundSpell(slot);
    }
}