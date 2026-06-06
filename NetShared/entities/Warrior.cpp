#include "Warrior.h"
#include "../managers/EffectManager.h"
#ifdef GAME_SERVER
#include "../../NetServer/Match.h"
#include "../AI/Companions/CompanionBase.h"
#endif // !GAME_SERVER
#include "../managers/SquadManager.h"
#include "../../NetServer/DamageContext.h"



Warrior::Warrior(uint32_t id, const std::string& name, Match* server)
    : Character(id, name, server) // Вызов родителя
{

    boundSpells[ActionSlot::Slot1] = SpellId::SmashHit;
    boundSpells[ActionSlot::Slot2] = SpellId::Shield;
    boundSpells[ActionSlot::Slot3] = SpellId::Hook;

    boundSpells[ActionSlot::Slot9] = SpellId::CommandMove;
    boundSpells[ActionSlot::Slot10] = SpellId::CommandDefend;
    // Инициализация специфичных полей Воина

    this->adaptationLevel = 0;
   

    // Начальные статы класса
    this->maxHealth = 25;
    this->health = 25;
    this->radius = 0.5f;
    entityType = ArchetypeId::Player_Warrior;

    m_classId = "Warrior"_sid;


#ifdef GAME_SERVER
    this->damageMultiplier = 1.0f;
    this->adaptationProgress = 0.0f;
    this->currentAdaptation = AdaptationType::None;


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
#endif // !GAME_SERVER
}
#ifdef GAME_SERVER
void Warrior::OnUpdate(const float dt, const  float lightIntensity)
{
    if (IsDead()) return;

    UpdateLightSize(dt, lightIntensity);

    // --- 2. ЗДОРОВЬЕ (MAX HP) ---
    // Днем +100 к макс HP. Базовое 150.
    int dayBonusHP = (int)(lightIntensity * 100.0f);
    int newMaxHP = 50 + dayBonusHP;

    if (newMaxHP != maxHealth) {
        // Если HP выросло (наступил день) — подлечиваем на разницу
        if (newMaxHP > maxHealth) health += (newMaxHP - maxHealth);
        maxHealth = newMaxHP;
        health = std::min(health, maxHealth); // На всякий случай
    }



    // Урон: днем (1.0) бьет на 40% сильнее
    this->damageMultiplier = 1.00f + lightIntensity * 0.1;
    Character::OnUpdate(dt, lightIntensity);

   // UpdateSquad(dt, lightIntensity);
}

void Warrior::OnProcessIncomingDamage(DamageContext& ctx)
{
    
    if (!squad->GetSquad().empty()) {

        for (auto& comNetId : squad->GetSquad()) {
            uint32_t idx = server->GetPlayerIndex(comNetId.first);
            if (idx != -1) {
                auto& comp = server->GetEntities()[idx].character;
                if (comp) {
                    comp->OnUnderAttack(ctx.attackerId);
                }
            }
        }
    }


    if (ctx.type == DamageType::Pure) return;

    // 1. ОПРЕДЕЛЯЕМ ТИП ВХОДЯЩЕГО УРОНА
    // (Physical -> Physical, Magical -> Magical)
    float adaptMutiplier = 0.0f;
    AdaptationType incomingType = (ctx.type == DamageType::Physical) ?
        AdaptationType::Physical : AdaptationType::Magical;
    if (ctx.type == DamageType::Physical) {
        incomingType = AdaptationType::Physical;
        adaptMutiplier = 4.0f;
    }
    else
    {
        incomingType = AdaptationType::Magical;
        adaptMutiplier = 2.0f;
    }

    // 2. ЛОГИКА НАКОПЛЕНИЯ / СМЕНЫ ФАЗЫ
    if (currentAdaptation == incomingType)
    {


        // Если тип урона совпадает — копим очки для следующей фазы
        adaptationProgress += (float)ctx.finalDamage * adaptMutiplier;




        // Если накопили достаточно (например, 100 единиц) — повышаем уровень
        if (adaptationProgress >= 100.0f)
        {
            if (adaptationLevel < 5) {
                adaptationLevel++;
                adaptationProgress = 0.0f; // Сброс прогресса внутри фазы

            }
            else {
                adaptationProgress = 100.0f;
            }
        }
    }
    else
    {
        // Если прилетел другой тип урона — "переучиваемся"
        // Сначала падают уровни старой адаптации
        if (adaptationLevel > 0)
        {
            adaptationLevel--;
            adaptationProgress = 0.0f;
        }
        else
        {
            // Если уровней не осталось — меняем тип на новый
            currentAdaptation = incomingType;
            adaptationProgress = (float)ctx.finalDamage;
        }
    }

    // 3. РАСЧЕТ СОПРОТИВЛЕНИЯ (Твоя логика с lightIntensity)
    float baseResist = adaptationLevel * 0.12f; // Каждая фаза дает 12% (макс 60%)
    float effectiveResist = baseResist * this->lightIntensity; // Ночью свет=0, резист=0

    ctx.finalDamage *= (1.0f - effectiveResist);
}

void Warrior::FullReset()
{
    Character::FullReset();
    // 2. Сбрасываем уникальные механики воина
    this->adaptationLevel = 0;
    this->adaptationProgress = 0.0f;
    this->currentAdaptation = AdaptationType::None;
    this->damageMultiplier = 1.0f;
}

float Warrior::GetSpellPowerBonus(SpellId spell) const
{

    //if (spell ==SpellId::SmashHit) {

    //    return  std::max(0.25f, balance); // Бонус огня
    //}
    //if (spell == SpellId::Iceball) {

    //    return std::max(0.25f, -balance * 0.8f); // Бонус льда
    //}
    return 1.0f;
}
//
//void Warrior::RefreshStatusModifiers()
//{   // 1. Базовая скорость зависит от времени суток
//    // Днем (1.0) -> 0.6x, Ночью (0.0) -> 1.1x (подправьте формулу под свои нужды)
//    currentSpeedModifier = 1.10f - (lightIntensity * 0.5f);
//
// 
//    bIsStunned = false;
//
//    for (const auto& e : effects) {
//        if (e.type == StatusEffectType::Slow) {
//            currentSpeedModifier *= e.value; // Замедление накладывается поверх суточного
//        }
//        if (e.type == StatusEffectType::Stun) {
//            bIsStunned = true;
//            currentSpeedModifier = 0.0f;
//        }
//    }
//}

#endif // !GAME_SERVER

SpellId Warrior::GetBoundSpell(ActionSlot slot) const
{
    
    if (effects->HasEffect(StatusEffectType::Shield)) {
        switch (slot)
        {
        case ActionSlot::Slot1: // lkm
           
            return SpellId::ShieldBurst;
            break;
        case ActionSlot::Slot2: // rkm
         
            return SpellId::ShieldThrow;
            break;

        default:
            break;
        }
    }

    return Character::GetBoundSpell(slot);
}

void Warrior::UpdateLightSize(float dt, const float& lightIntensity)
{
    this->lightIntensity = lightIntensity;
    // --- 1. РАЗМЕР (RADIUS) ---
   
    float targetRadius = 0.2f + (lightIntensity * 0.6f);
    this->radius = targetRadius;
}
