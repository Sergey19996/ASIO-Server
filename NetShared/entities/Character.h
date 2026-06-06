#pragma once

#include <string>
#include <glm/glm.hpp>
#include <vector>
#include <unordered_map>
#include <algorithm>

#include "iostream"

// Эти заголовки тоже должны быть в NetShared
#include "../StatusEffect.h"
#include "../SpellId.h"
#include "../ActionSlot.h"
#include "../PlayerClass.h" 
#include <functional>
#include "../GameplayTypes.h"
#include "../StringId.h"

//#ifdef GAME_SERVER

//#endif


// Упреждающие объявления (Forward Declarations)
// Это позволяет не инклудить серверные файлы в клиент

#ifdef GAME_SERVER
constexpr float RESPAWN_TIME = 3.0f;


#endif
struct DamageContext;
class Match;
// Один единственный enum для ВСЕГО в игре. Никаких подтипов!
enum class ArchetypeId : uint8_t {
	None = 0,

	// Игроки (1 - 9)
	Player_Warrior = 1,
	Player_Mage = 2,
	Player_Hunter = 3,

	// Мобы (10 - 49)
	Mob_Melee = 10,
	Mob_Ranged = 11,

	// Компаньоны (50 - 99)
	Companion_Warrior = 50,
	Companion_Priest = 51,
	Companion_Hunter = 52,

	// Боссы (100+)
	Boss_LichKing = 100
};

// Компоненты-менеджеры
class CooldownManager;
class EffectManager;
class SquadManager;
class ProgressionManager;


class Character {
public:
	virtual ~Character();
	Character(uint32_t id, const std::string& name, Match* server = nullptr);


public:
	
	//
	// Получить SpellId, который сейчас активен в слоте (учитывая динамику классов)
	virtual SpellId GetBoundSpell(ActionSlot slot) const {
		auto it = boundSpells.find(slot);
		return (it != boundSpells.end()) ? it->second : SpellId::None;
	}

	// Найти слот, в котором находится конкретное заклинание (для подсветки UI)
	virtual ActionSlot GetSlotForSpell(SpellId spell) const {
		for (const auto& pair : boundSpells) {
			if (pair.second == spell) return pair.first;
		}
		return ActionSlot::None;
	}

	// Проверка возможности каста (клиент может вызывать для "серой" иконки)
	virtual bool CanCast(SpellId id, float timeNow) const;
	virtual std::string GetClassName() const { return "Unknown"; }
	Match* server = nullptr;


	// Геттеры для менеджеров


	EffectManager* GetEffects() { return effects.get(); }
	EffectManager* GetEffects()const { return effects.get(); }
	SquadManager* GetSquad() { return squad.get(); }
	SquadManager* GetSquad() const { return squad.get(); }
	ProgressionManager* GetProgression() { return progression.get(); }
	ProgressionManager* GetProgression()const { return progression.get(); }
	CooldownManager* GetCooldown() { return cooldowns.get(); }
	CooldownManager* GetCooldown() const { return cooldowns.get(); }
	ArchetypeId GetArchetypeId() const { return entityType; }

	// Возвращаем числовой ID класса (быстро и легко копируется)
	StringId GetClassId() { return m_classId; };

	//uint32_t currentEffectsMask = 0;
	uint32_t id;
	ArchetypeId entityType = ArchetypeId::None;
	glm::vec2 position = { 0,0 };
	glm::vec2 inputVel = { 0,0 };
	glm::vec2 direction = { 0,0 };
	float radius = 0.5f;
	int maxHealth = 100;
	int health = maxHealth;
	uint32_t shieldHp = 0;
	//PlayerClass playerClass;
	float chargeClient = 0.0f;
	bool isCharging = false;

	uint32_t teamId = 0; // 0 - фракия мобов

	std::string name;
protected:

	std::unordered_map<ActionSlot, SpellId> boundSpells;  // хранит на какие кнопки и и спелы к ним привязаные
	//
	ActionSlot lastCastSlot = ActionSlot::None;
	

	bool isDead = false;

	
	// Компоненты-менеджеры
	std::unique_ptr<CooldownManager> cooldowns;
	std::unique_ptr<EffectManager> effects;
	std::unique_ptr<SquadManager> squad;
	std::unique_ptr<ProgressionManager> progression;
	StringId m_classId;
private:
	

#ifdef GAME_SERVER
public:
	

	void SetSpeedModifier(float val) { currentSpeedModifier = val; }
	void SetStunned(bool state) { bIsStunned = state; }
	float GetBaseSpeed() const { return baseSpeed; }
	float GetCurrentSpeed() const { return currentSpeedModifier; };

	// Сигнатура колбэка: (ID игрока, новые координаты, старые координаты) - это для горящих клеток или замедляющих - эвенты
	std::function<void(uint32_t, glm::ivec2, glm::ivec2)> onTileChangedPos;

	Character() = default; // Или Character() {};
	const std::string& GetName() const {
		return name;
	}
	const uint32_t& GetId() const {
		return id;
	}
	Match* GetServer();

	
	glm::vec2 knockbackVel = { 0,0 };

	
	glm::vec2 currentMovingVel = { 0,0 };
	glm::ivec2 lastTilePos = { -1, -1 }; // Начальное значение, чтобы сработало при первом шаге

	
	
	
	

	void SetName(std::string Name) {

		name = Name;
	}
	virtual void OnBeforeDestroy() {} // Хук перед удалением/смертью
	void TakeDamage(int dmg);
	// Виртуальные хуки для уникальных механик
	virtual void OnUpdate(const float dt, const  float lightIntensity);
	virtual void OnProcessIncomingDamage(DamageContext& ctx) {}
	
	virtual float GetResourceValue() const { return 0.0f; } // Прогресс бара
	virtual uint32_t GetExtraData() const { return 0; }
	//
	virtual float GetSpellPowerBonus(SpellId spell) const { return 1.0f; }
	virtual void OnSpellCast(SpellId spell, float timeprop) { /* по умолчанию ничего */ }
	virtual float GetDamageMultiplier() const { return damageMultiplier; }
	virtual bool CraftArrow(ArrowType newType) { return false; }
	//virtual MobType GetMobType() const { return MobType::None; }
	virtual void OnUnderAttack(uint32_t attackerId) {}


	float GetCurSpeedModifier() const { return currentSpeedModifier; }
	//std::unordered_map<uint32_t, uint8_t>* GetMinions();

	bool IsDead() const {
		return isDead;
	}
	virtual void FullReset(); // Добавьте virtual
	void Revive();
	//void ClearEffects();
	//void ClearCooldowns();


	void Die();
	void Respawn(const glm::vec2& pos);


	
//	ArchetypeId& GetClass() { return playerClass; }
	// ДОБАВЛЯЕМ константную версию для упаковки в сетевые пакеты
//	ArchetypeId GetClass() const { return playerClass; }

	void LockMovement() {
		movementLockStack++;

	}
	void LockActions() { actionLockStack++; }
	void UnlockActions() {
		actionLockStack--;
		if (actionLockStack < 0) actionLockStack = 0;
	}
	void UnlockMovement() {

		movementLockStack--;
		if (movementLockStack < 0)
			movementLockStack = 0;
	}

	bool CanMove() const;
	bool CanCastSpell() const;
	void Heal(float hp) {
		// std::min выберет меньшее из двух: либо сумму, либо максимум.
   // (int) нужен, если health у вас целочисленный (uint32_t/int)
		health = std::min((float)maxHealth, health + hp);
	}
	

	//void AddStatusEffect(const StatusEffect& e);
//	void RemoveEffect(StatusEffectType t);
//	StatusEffect* GetEffect(StatusEffectType t);
	

	float respawnTimer = 0.0f;
	
	int movementLockStack = 0;
	bool bIsStunned = false;
	void SetLastCastSlot(ActionSlot slot) { lastCastSlot = slot; }
	ActionSlot GetLastCastSlot() const { return lastCastSlot; }

	void StartCharging(ActionSlot slot);
	float StopCharging();
	bool IsCharging() const { return isCharging; }
	float GetChargeTimer() const { return chargeTimer; }
	float GetLastChargeTime() const { return lastChargeTime; }
	virtual float GetMaxChargeTime() const { return 2.5f; } // Дефолт для всех
private:
	

protected:

	


	float damageMultiplier = 1.0f;
	float currentSpeedModifier = 1.0f;
	float baseSpeed = 1.0f;
	int actionLockStack = 0;
	

	float chargeTimer = 0.0f;
	float lastChargeTime = 0.0f;
	const float MAX_CHARGE_TIME = 2.5f;
	ActionSlot chargingSlot = ActionSlot::None;



#endif

};
