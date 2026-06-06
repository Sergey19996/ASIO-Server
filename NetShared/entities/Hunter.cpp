#include "Hunter.h"

#ifdef GAME_SERVER
#include "../../NetServer/Match.h"
#endif // GAME_SERVER


Hunter::Hunter(uint32_t id, const std::string& name, Match* server) : Character(id, name, server)
{

 

    // Начальные статы класса
    this->maxHealth = 25;
    this->health = 25;
    
    this->radius = 0.5f;
   


    boundSpells[ActionSlot::Slot1] = SpellId::Shoot;
    boundSpells[ActionSlot::Slot2] = SpellId::InfuseArrow; // По умолчанию две кнопки стреляют
    boundSpells[ActionSlot::Slot3] = SpellId::Grapple;

    boundSpells[ActionSlot::Slot4] = SpellId::CraftGhost;
    boundSpells[ActionSlot::Slot5] = SpellId::CraftBind;
    boundSpells[ActionSlot::Slot6] = SpellId::CraftExplosive;


    boundSpells[ActionSlot::Slot7] = SpellId::ChainHarvest;
    boundSpells[ActionSlot::Slot8] = SpellId::ReloadQuiver;

    boundSpells[ActionSlot::Slot9] = SpellId::CommandMove;
    boundSpells[ActionSlot::Slot10] = SpellId::CommandDefend;


    entityType = ArchetypeId::Player_Hunter;
    m_classId = "Hunter"_sid;

#ifdef GAME_SERVER
    this->damageMultiplier = 1.0f;
    this->baseSpeed = 1.05f;

    //// Спавним 3-х спутников для Воина (например, 2 лучника для прикрытия и 1 маг)
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
#endif

}
int Hunter::FindArrowInQuiver(ArrowType type)const
{

    for (int i = 0; i < 5; ++i) {
        if (quiver[i].active && quiver[i].linkedProjectileId == -1) {
            if (type == ArrowType::Explosive && quiver[i].hasExplosion) return i;
            if (type == ArrowType::Binding && quiver[i].hasBinding) return i;
            if (type == ArrowType::Ghost && quiver[i].hasGhost) return i;
        }
    }
    return -1;
}
int Hunter::FindArrowInType(ArrowType type) const
{
    for (int i = 0; i < 5; ++i) {
      
            if (type == ArrowType::Explosive && quiver[i].hasExplosion) return i;
            if (type == ArrowType::Binding && quiver[i].hasBinding) return i;
            if (type == ArrowType::Ghost && quiver[i].hasGhost) return i;
        
    }
    return -1;
}
bool Hunter::HasArrowInWorld(ArrowType type)const
{
    for (int i = 0; i < 5; ++i) {
        // active == false значит стрела вылетела, linkedProjectileId != -1 значит она еще существует в игре
        if (!quiver[i].active && quiver[i].linkedProjectileId != -1) {
            if (type == ArrowType::Explosive && quiver[i].hasExplosion) return true;
            if (type == ArrowType::Binding && quiver[i].hasBinding) return true;
            if (type == ArrowType::Ghost && quiver[i].hasGhost) return true;
        }
    }
    return false;
}
#ifdef GAME_SERVER

void Hunter::OnUpdate(const float dt, const float lightIntensity)
{
    if (IsDead()) return;
    if (isReloading) {
        reloadTimer -= dt;
        if (reloadTimer <= 0.0f) {
            isReloading = false;
            arrowsInQuiver = MAX_ARROWS;
            for (int i = 0; i < arrowsInQuiver; ++i) {
                if (!quiver[i].active && quiver[i].linkedProjectileId == -1 && !quiver[i].IsSpecArrow()) {
                    quiver[i].ResetArrow();

                }
            }
            UnlockActions(); // Возвращаем возможность стрелять
        }
    }
   
    //else if (arrowsInQuiver < MAX_ARROWS && CanMove()) {
    //    passiveRefillTimer += dt;

    //    float timePerArrow = 5.0f; // Время восстановления одной стрелы
    //    if (passiveRefillTimer >= timePerArrow) {
    //        IncrementArrows(); // Наш «умный» метод, который найдет слот
    //        passiveRefillTimer = 0.0f;
    //    }
    //}
    Character::OnUpdate(dt, lightIntensity);
}

void Hunter::AddInfoPoint()
{
    if (infoPoints < MAX_INFO) {
        infoPoints++;
    }
}

void Hunter::OnArrowFired(int index)
{
    if (index < 0 || index >= 5 || isReloading) return;

    quiver[index].active = false; // 1 этап - стрела выпущены 

    // Если это обычная стрела (Normal), сразу списываем её
    if (!quiver[index].IsSpecArrow()) {
        DecrementArrows();
    }
}

void Hunter::DischargeAmmunition(int power, bool isRightClick)
{
    for (int i = 0; i < power; ++i) {
        int idx = GetNextArrowIndex(isRightClick);
        if (idx != -1) {
            // Очищаем слот полностью перед выстрелом
            quiver[idx].ResetArrow();
         //   quiver[idx].active = false;
        //    quiver[idx].linkedProjectileId = -1; // Больше не привязываем к спеллу
           // quiver[idx].type = ArrowType::Normal;
         //   quiver[idx].hasBinding = false;
        //    quiver[idx].hasExplosion = false;
        //    quiver[idx].hasGhost = false;
            // Уменьшаем физический счетчик стрел
            DecrementArrows();
        }
    }
}

void Hunter::OnProcessIncomingDamage(DamageContext& ctx)
{
}

// 1. Ищем КУДА вливать (всегда самая левая активная стрела)
int Hunter::GetInfuseTargetIndex() {
    for (int i = 0; i < 5; ++i) {
        // Ищем стрелу, которая активна И еще не достигла лимита (bonusPower < 4)
        if (quiver[i].active&& quiver[i].bonusPower < 3) {
            return i;
        }
    }
    return -1; // Весь колчан "перегружен"
}

// 2. Ищем ЧТО поглощать (любая активная стрела, КРОМЕ целевой, желательно справа)
int Hunter::GetInfuseSourceIndex(int targetIdx) {
    if (targetIdx == -1) return -1;

    // Ищем жертву ТОЛЬКО среди тех, кто правее цели, 
    // чтобы не "перескакивать" через уже заряженную голову
    for (int i = 4; i > targetIdx; i--) {
        if (quiver[i].active) return i;
    }
    return -1;
}

const ArrowSlot& Hunter::GetQuiverSlot(int index) const
{
    // Безопасная проверка индекса
    if (index < 0 || index >= 5) return quiver[0];
    return quiver[index];
}

bool Hunter::CraftArrow(ArrowType newType)
{
   // if (infoPoints < 3) return false;

    // Ищем случайную или конкретную активную стрелу
    // Но круче сделать так: игрок сам выбирает в какую "позицию" вставить стрелу
    // Или, как ты сказал, случайную из доступных:
    std::vector<int> activeIndices;
    for (int i = 0; i < 5; ++i) {
        if (quiver[i].active && !quiver[i].IsSpecArrow())
            activeIndices.push_back(i);
    }
    int cost = 0;
    if (!activeIndices.empty()) {
        int target = activeIndices[rand() % activeIndices.size()];

        switch (newType)
        {
        
        case ArrowType::Binding:
            if (infoPoints < 2) return false;
            quiver[target].hasBinding = true;
            cost = 2;
            break;
        case ArrowType::Ghost:
            if (infoPoints < 1) return false;
            quiver[target].hasGhost = true;
            cost = 1;
            break;
        case ArrowType::Explosive:
            if (infoPoints < 3) return false;
            quiver[target].hasExplosion = true;
            cost = 3;
            break;
        default:
            break;
        }

      // quiver[target].type = newType;
      

        infoPoints -= cost;
        return true;
    }
    return false;
}

int Hunter::GetMaxChargeLimit(bool isRightClick)
{
    int count = 0;
    // Начинаем искать с той стороны колчана, с которой стреляем
    if (isRightClick) {
        for (int i = 4; i >= 0; --i) {
            if (!quiver[i].active) continue; // Пропускаем пустые
            //  if (quiver[i].type != ArrowType::Normal) return std::max(1, count + 1); // Спец-стрела!
            count++;
        }
    }
    else {
        for (int i = 0; i < 5; ++i) {
            if (!quiver[i].active) continue;
            // if (quiver[i].type != ArrowType::Normal) return std::max(1, count + 1);
            count++;
        }
    }
    return std::max(1, count); // Если спец-стрел нет, лими
}

float Hunter::GetMaxChargeTime() const
{
    // Каждая стрела дает 0.25 сек заряда (5 стрел = 1.25 сек)
    bool isRightClick = (lastCastSlot == ActionSlot::Slot2);
    int arrowLimit = const_cast<Hunter*>(this)->GetMaxChargeLimit(isRightClick);
    return arrowLimit * 0.25f;
}

void Hunter::ReloadQuiver()
{
    //if (isReloading) {
       
       //     isReloading = false;
            arrowsInQuiver = MAX_ARROWS;
            for (int i = 0; i < arrowsInQuiver; ++i) {
                if (!quiver[i].active && quiver[i].linkedProjectileId == -1 && !quiver[i].IsSpecArrow()) {
                 //   quiver[i].active = true;
                 //   quiver[i].hasBinding = false;
                 //   quiver[i].hasExplosion = false;
                  //  quiver[i].hasGhost = false;
                   //quiver[i].bonusPower = 0;
                  //  quiver[i].linkedProjectileId = -1;
                    quiver[i].ResetArrow();

                }
            }
            UnlockActions(); // Возвращаем возможность стрелять
        
   // }
}

void Hunter::RegisterLiveArrow(uint32_t projId)
{

    liveArrowIds.push_back(projId);
    liveArrowsCount++;

    if (liveArrowsCount > MAX_ARROWS) liveArrowsCount = MAX_ARROWS;
}

void Hunter::UnregisterLiveArrow(uint32_t projId)
{
    auto it = std::find(liveArrowIds.begin(), liveArrowIds.end(), projId);
    if (it != liveArrowIds.end()) liveArrowIds.erase(it);

    if (liveArrowsCount <= 0) liveArrowsCount = 0;
    liveArrowsCount--;

}

void Hunter::DecrementArrows()
{
    arrowsInQuiver--;
    if (arrowsInQuiver <= 0 && !HasPendingActivations()) {
        isReloading = true;
        reloadTimer = RELOAD_DURATION;
        LockActions();
    }
}

void Hunter::IncrementArrows()
{
    if (arrowsInQuiver >= MAX_ARROWS) return;

    int targetIdx = -1;

    // Условие "Слот свободен": 
    // 1. Он не активен в колчане (!quiver[i].active)
    // 2. Он НЕ находится в полете в ожидании активации (linkedProjectileId == -1)
    auto isSlotTrulyEmpty = [&](int i) {
        return !quiver[i].active && (quiver[i].linkedProjectileId == -1);
        };

    if (arrowsInQuiver == 0) {
        // Ищем первый по-настоящему пустой слот, начиная с нуля
        for (int i = 0; i < 5; ++i) {
            if (isSlotTrulyEmpty(i)) { targetIdx = i; break; }
        }
    }
    else {
        for (int i = 0; i < 5; ++i) {
            if (isSlotTrulyEmpty(i)) {
                bool hasLeftNeighbor = (i > 0 && quiver[i - 1].active);
                bool hasRightNeighbor = (i < 4 && quiver[i + 1].active);

                if (hasLeftNeighbor || hasRightNeighbor) {
                    targetIdx = i;
                    break;
                }
            }
        }
    }

    // Финальная проверка, если кучности не вышло
    if (targetIdx == -1) {
        for (int i = 0; i < 5; ++i) {
            if (isSlotTrulyEmpty(i)) { targetIdx = i; break; }
        }
    }

    // Активируем найденный слот
    if (targetIdx != -1) {
      //  quiver[targetIdx].active = true;
      //  quiver[targetIdx].hasBinding = false;
      //  quiver[targetIdx].hasExplosion = false;
      //  quiver[targetIdx].hasGhost = false;
      //  quiver[targetIdx].bonusPower = 0;
      //  quiver[targetIdx].linkedProjectileId = -1;
        quiver[targetIdx].ResetArrow();
        arrowsInQuiver++;

        // Прерываем релоад, если он был
        if (isReloading) {
            isReloading = false;
            reloadTimer = 0.0f;
            UnlockActions();
        }
    }
}

bool Hunter::HasPendingActivations() const
{
    for (int i = 0; i < 5; ++i)
        if (quiver[i].linkedProjectileId != -1) return true;

    return false;

}

void Hunter::FullReset()
{
    Character::FullReset();
    infoPoints = 0;
    arrowsInQuiver = MAX_ARROWS;
    isReloading = false;
    reloadTimer = 0.0f;
    // Сбрасываем колчан: все стрелы активны и обычные
    for (auto& slot : quiver) {
        slot.active = true;
        slot.hasBinding = false;
        slot.hasExplosion = false;
        slot.hasGhost = false;
        slot.bonusPower = 0;
        slot.linkedProjectileId = -1;
    }
}

#endif // GAME_SERVER

int Hunter::GetNextArrowIndex(bool isRightClick)
{
    auto check = [&](int i) {

        if (isRightClick) {
            return quiver[i].IsSpecArrow(); // Только спецухи
        }
        else // лкм
        {
            // Для ПКМ ищем ТОЛЬКО спец-стрелы, для ЛКМ — любую доступную (или только Normal)
            if (!quiver[i].active || quiver[i].linkedProjectileId != -1)
                return false;
        }
        return true; // если левый клик - все 
        };

    if (isRightClick) {
        for (int i = 4; i >= 0; --i)
            if (check(i))
                return i;
    }
    else {
        for (int i = 0; i < 5; ++i)
            if (check(i)) 
                return i;
    }
    return -1;
}
SpellId Hunter::GetBoundSpell(ActionSlot slot) const
{
    bool isRightClick = (slot == ActionSlot::Slot2);
    int targetIndex = const_cast<Hunter*>(this)->GetNextArrowIndex(isRightClick);


    switch (slot)
    {
    case ActionSlot::Slot1:

        if (slot == ActionSlot::Slot1)
            return SpellId::Shoot;
        break;
    case ActionSlot::Slot2:
        if (targetIndex != -1) {
            const auto& current = quiver[targetIndex];

            // 1. АКТИВАЦИЯ: Если стрела этого типа уже в полете
            if (!current.active && current.linkedProjectileId != -1) {

                if (current.hasExplosion) return SpellId::StickyBomb;
                if (current.hasBinding) return SpellId::BindArrow;
                //  if (current.type == ArrowType::Ghost) return SpellId::GhostArrow;
            }

            // 2. ЗАРЯДКА: Если стрелы в колчане, ПКМ запускает процесс "сжатия"
            if (current.active) {

                return  SpellId::InfuseArrow; // Тот самый новый класс
            }
        }
        return SpellId::InfuseArrow;

    default:
        return Character::GetBoundSpell(slot);
        break;
    }

    return Character::GetBoundSpell(slot);
  
}