#pragma once
#include "Character.h"
#include <vector>
#include <array>
#include"../GameplayTypes.h"

//может стрелы мисс GameplayTypes
struct ArrowSlot {
  //  ArrowType type = ArrowType::Normal;

    // Флаги эффектов
    bool hasExplosion = false;
    bool hasBinding = false;
    bool hasGhost = false;

    bool active = true; // Есть ли стрела физически
    int linkedProjectileId = -1; // -1 если стрела еще не вылетела или обычная
    int bonusPower = 0; // Сколько стрел "влито" в эту одну

    bool IsSpecArrow()const {
        if (hasExplosion || hasBinding || hasGhost)
            return true;
        return false;
    }
    void ResetArrow() {
         hasExplosion = false;
         hasBinding = false;
         hasGhost = false;
         active = true;
         linkedProjectileId = -1;
            bonusPower = 0;
    }
};
class Hunter : public Character
{
public:
    Hunter(uint32_t id, const std::string& name, Match* server = nullptr);

    SpellId GetBoundSpell(ActionSlot slot) const override;
    int GetNextArrowIndex(bool isRightClick);
   
    const float RELOAD_DURATION = 2.0f;
    std::array<ArrowSlot, 5> quiver; // Наш колчан
    int infoPoints = 0;           // Нити судьбы (0..6)
    uint8_t liveArrowsCount = 0;

    void UpdateFromNetwork(uint32_t data, float info) {

        // 1. Восстанавливаем InfoPoints (десятые доли)
   // Умножаем на 10 и прибавляем мизер (0.01), чтобы 5.999 стало 6.0
        this->infoPoints = (int)(info * 10.0f + 0.01f);
        if (this->infoPoints > 6) this->infoPoints = 6;

        // 2. Восстанавливаем стрелы (сотые доли)
        // Умножаем на 100, получаем например 69.0. 
        // Вычитаем (infoPoints * 10), остается 9.
        float totalAsInt = info * 100.0f + 0.1f;
        this->liveArrowsCount = (uint8_t)((int)totalAsInt % 10);
        // 1. Получаем infoPoints (0..6)
     // Округляем до ближайшего целого, так как мы делили на 6
        //this->infoPoints = info / 25;

        //// 2. Достаем количество стрел из "остатка"
        //float fractional = (info * 6.0f) - (float)infoPoints;
        //// Умножаем обратно (0.001 * 6 = 0.006), восстанавливаем кол-во
        //this->liveArrowsCount = (int)std::round(fractional * 1000.0f / 6.0f);

        for (int i = 0; i < 5; i++) {
            // Извлекаем 6 бит для текущей стрелы
            uint32_t bits = (data >> (i * 6)) & 0x3F;

            quiver[i].active = (bits >> 0) & 1;
            quiver[i].hasExplosion = (bits >> 1) & 1;
            quiver[i].hasBinding = (bits >> 2) & 1;
            quiver[i].hasGhost = (bits >> 3) & 1;
            quiver[i].bonusPower = (bits >> 4) & 0x03;

            // Логический вывод: если стрела не в колчане, но имеет спец-эффекты, 
            // значит она "летит" (визуальный флаг для UI)
            quiver[i].linkedProjectileId = (!quiver[i].active &&
                (quiver[i].hasExplosion || quiver[i].hasBinding || quiver[i].hasGhost))
                ? 1 : -1;
        }

        // Читаем системные флаги из последних бит
        this->lastCastSlot = ((data >> 30) & 1) ? ActionSlot::Slot2 : ActionSlot::Slot1;
        this->isCharging = (data >> 31) & 1;
    }
    void setResourcValue(float infp) { infoPoints = infp; };
    int FindArrowInQuiver(ArrowType type) const;
    int FindArrowInType(ArrowType type) const;
    bool HasArrowInWorld(ArrowType type) const;


    bool CanCast(SpellId id, float timeNow) const override {



        if (id == SpellId::CraftExplosive) {
        //    int idx = FindArrowInQuiver(ArrowType::Explosive);
        //    bool hasInWorld = HasArrowInWorld(ArrowType::Explosive);

            // Если стрелы нет ни в руках, ни в мире — проверяем инфо-очки для крафта
        //    if (idx == -1 && !hasInWorld) {
                return infoPoints >= 3;
      //      }
       //     // Если стрела уже есть — кастовать можно всегда (выстрел или взрыв)
            return true;
        }
        if (id == SpellId::CraftBind) {
       //     int idx = FindArrowInQuiver(ArrowType::Binding);
       //     bool hasInWorld = HasArrowInWorld(ArrowType::Binding);

            // Если стрелы нет ни в руках, ни в мире — проверяем инфо-очки для крафта
       //     if (idx == -1 && !hasInWorld) {
                return infoPoints >= 2;
       //     }
            // Если стрела уже есть — кастовать можно всегда (выстрел или взрыв)
            return true;
        }
        if (id == SpellId::CraftGhost) {
         //   int idx = FindArrowInQuiver(ArrowType::Ghost);
         //   bool hasInWorld = HasArrowInWorld(ArrowType::Ghost);

            // Если стрелы нет ни в руках, ни в мире — проверяем инфо-очки для крафта
         //   if (idx == -1 && !hasInWorld) {
                return infoPoints >= 1;
       //    }
            // Если стрела уже есть — кастовать можно всегда (выстрел или взрыв)
            return true;
        }

     
        if (id == SpellId::ChainHarvest) {
            if (liveArrowsCount < 2)
                return false;
        };



        return Character::CanCast(id, timeNow);
        //	return GetCooldownRemaining(id, timeNow) <= 0.0f && !isDead;
    }
#ifdef GAME_SERVER
public:

    void OnUpdate(const float dt, const float lightIntensity) override;
    void  FullReset() override;
    float GetResourceValue() const override {
        // Каждое очко инфо (0-6) дает 0.142... (всего до 0.85)
   // Каждая стрела (0-9) дает 0.01 (всего до 0.09)
   // Итого максимум 0.94 — безопасно влезает в 1.0
        return (infoPoints * 0.1f) + (std::min((int)liveArrowIds.size(), 9) * 0.01f);
    }
    // Ресурс для UI: Нити судьбы (0.0 - 1.0)
    
    // Доп. данные: количество стрел в колчане (для отрисовки в UI)
    uint32_t GetExtraData() const override {
        uint32_t data = 0;

        for (int i = 0; i < 5; i++) {
            uint32_t a = 0;
            // Упаковываем флаги (4 бита)
            if (quiver[i].active)       a |= (1 << 0);
            if (quiver[i].hasExplosion) a |= (1 << 1);
            if (quiver[i].hasBinding)   a |= (1 << 2);
            if (quiver[i].hasGhost)     a |= (1 << 3);

            // Упаковываем мощность (2 бита: 0-3)
            uint32_t pwr = (uint32_t)std::min(quiver[i].bonusPower, 3);
            a |= (pwr << 4);

            // Сдвигаем на 6 бит для каждой стрелы
            data |= (a << (i * 6));
        }

        // 31-й бит: Сторона (ЛКМ/ПКМ)
        if (lastCastSlot == ActionSlot::Slot2) data |= (1u << 30);
        // 32-й бит: Состояние зарядки
        if (isCharging) data |= (1u << 31);

        return data;
    }
    // Храним только ID снарядов, которые сейчас в мире и принадлежат нам
    std::vector<uint32_t> liveArrowIds;
    // Логика взаимодействия со стрелами на карте
    void AddInfoPoint();
    void OnArrowFired(int index);
    void DischargeAmmunition(int power, bool isrightClick);

    void OnProcessIncomingDamage(DamageContext& ctx) override;
   
    int GetInfuseTargetIndex();
    int GetInfuseSourceIndex(int targetIdx);
    const ArrowSlot& GetQuiverSlot(int index) const;
    bool IsReloading() { return isReloading; };

    void BindSpell(ActionSlot slot, SpellId spell) {
        boundSpells[slot] = spell;
    }
    bool CraftArrow(ArrowType newType) override;
    int GetMaxChargeLimit(bool isRightClick);
    float GetMaxChargeTime() const override;
    void ReloadQuiver();

    void RegisterLiveArrow(uint32_t projId);

    void UnregisterLiveArrow(uint32_t projId);
    //

    void DecrementArrows();
    void IncrementArrows();
  
private:
    bool HasPendingActivations() const;
    // Ресурсы
   
    int arrowsInQuiver = 5;       // Текущее кол-во стрел
    const int MAX_ARROWS = 5;
    const int MAX_INFO = 6;

    int leftPointer = 0;  // Индекс для ЛКМ (0 -> 4)
    int rightPointer = 4; // Индекс для ПКМ (4 -> 0)
   
    


    // Перезарядка
    bool isReloading = false;
    float reloadTimer = 0.0f;
    
    float passiveRefillTimer = 0.0f;

    // Логика отслеживания стрел в мире (ID выпущенных проджектайлов)
    // Это нужно, чтобы при перезарядке сервер мог их удалить
  //  std::vector<uint16_t> activeArrowsOnMap; 
    // Hunter.h
#endif
};