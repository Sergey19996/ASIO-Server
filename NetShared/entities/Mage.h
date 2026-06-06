#pragma once
#include "Character.h"

class Mage : public Character
{
public:

	// Конструктор остается общим (клиент передаст nullptr в server)
	Mage(uint32_t id, const std::string& name, Match* server = nullptr);

	// Это ОБЯЗАТЕЛЬНО оставляем открытым (для иконок в UI)
	SpellId GetBoundSpell(ActionSlot slot) const override;

	// Это оставляем открытым (для полоски ресурса в UI)
	float GetRawBalance() const { return balance; } 
	// Метод для синхронизации: клиент будет вызывать его, получая данные из пакета
	void SetBalance(float newBalance) { balance = newBalance; }
	 bool CanCast(SpellId id, float timeNow) const override {

		 bool isFire = balance > 0; // Определяем сторону
		 float absBalance = std::abs(balance);

		
		 if (absBalance < 1.0f)
		 {
			 if (id == SpellId::FireDash || id == SpellId::IceShield) {
				 return false;
			 }
		 }
		 if (absBalance < 2.0f)
		 {
			 if (id == SpellId::IceRoots || id == SpellId::BlastWave) {
				 return false;
			 }
		 }
		 if (absBalance < 2.9f) {
			 if (id == SpellId::SunLance || id == SpellId::StygianSpike) {
				 return false;
			 }

		 }

		 return Character::CanCast(id, timeNow);
	//	return GetCooldownRemaining(id, timeNow) <= 0.0f && !isDead;
	}
	
#ifdef GAME_SERVER
	float GetResourceValue() const override { return (balance + 3.0f) / 6.0f; }
	void OnUpdate(const float dt, const float lightIntensity) override;
	void OnProcessIncomingDamage(DamageContext& ctx) override;
	void  FullReset();
	// Вызывается при касте заклинаний
	void ModifyBalance(float amount);
	// Для UI: возвращает 0.0 (лёд), 0.5 (Нейтрал), 1.0 (огонь)
	float GetMaxChargeTime() const override;
	//	uint32_t GetExtraData() const override { return adaptationLevel; }
	float GetSpellPowerBonus(SpellId spell) const override;
	void OnSpellCast(SpellId spell, float timeprop) override;
	float GetDamageMultiplier() const override;
	void CastSwitch();
	void CastConservation();

private:
	const float MAX_ABS_BALANCE = 3.0f;
	const float RECOVERY_RATE = 0.05f; // Скорость возврата к 0

	float overheatRecoverPool = 0.0f; // Накопленный урон от перегрева
	float healTick = 1.0f;

	float conservedBalance = 0.0f;  // Хранилище для "Консервации"
	bool hasConserved = false;

	float selfDamageTimer = 0.0f;     // Для урона от перегрева

#endif
private:
	float balance = 0.0f;             // Текущее состояние
	
};