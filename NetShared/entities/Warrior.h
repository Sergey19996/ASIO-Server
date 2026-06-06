#pragma once
#include "Character.h"



enum class AdaptationType { None, Physical, Magical };
class Warrior : public Character
{
public:

	Warrior(uint32_t id, const std::string& name, Match* server = nullptr);
	SpellId GetBoundSpell(ActionSlot slot) const override;
	float GetLightIntensity() { return lightIntensity; };
	uint8_t GetAdaptationLevel() { return adaptationLevel; }
	float GetAdaptationProgress() { return adaptationProgress; };
	void SetResourceValue(float adaptint) { adaptationProgress = adaptint; }
	void SetExtraData(uint8_t adaptationLvl) {adaptationLevel = adaptationLvl;}
	void UpdateLightSize(float dt, const  float& lightIntensity);

private:



	float lightIntensity = 0.0f;
	uint8_t adaptationLevel = 0; // 0..5
	float adaptationProgress = 0.0f; // 0 -> 100
#ifdef GAME_SERVER
public:
	void OnUpdate(const float dt, const float lightIntensity) override;
	void OnProcessIncomingDamage(DamageContext& ctx) override;
	void  FullReset();
	float GetResourceValue() const override { return adaptationProgress / 100.0f; } // шкала от 0 до 100
	float GetSpellPowerBonus(SpellId spell) const override;
	//void RefreshStatusModifiers()override;
	uint32_t GetExtraData() const override { return adaptationLevel; } // Шарики 0 - 5

	void ResetAdaptation() { adaptationLevel = 0; adaptationProgress = 0.0f; }


private:
	
	AdaptationType currentAdaptation = AdaptationType::None;
#endif // !
};