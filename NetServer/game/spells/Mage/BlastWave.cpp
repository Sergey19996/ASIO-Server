#include "BlastWave.h"
//#include "../../entities/Mage.h"
#include "../NetShared/entities/Mage.h"
#include "../../../Match.h"

bool BlastWave::Cast(Character& caster, Match* server)
{
    Mage* mage = dynamic_cast<Mage*>(&caster);
    float b = mage->GetRawBalance();
    if (!mage || b <= 0.5f) return false; // Кастуем только при положительном балансе

    // Берем дефолтные правила и тюнингуем их балансом
    ProjectileRules rules = GetProjectileRules(ProjectileType::BlastWave);
    float power = b / 3.0f;
    rules.knockbackForce *= (1.0f + power);
    float finalDamage = rules.damageToPlayers ;
    float finalRadius = 3.5f + (power * 2.0f);

    // ВЫЗОВ: передаем false в hitAttacker, чтобы маг не бил себя
    server->ProcessAreaDamage(caster.position, finalRadius, finalDamage, caster.GetId(), false, rules);

    float pwr = caster.GetSpellPowerBonus(id); // берём силу от спела
    SetPowerMultiplier(pwr); // берём множитель который сыграет свою роль в момент попадения 
  

    mage->ModifyBalance(-b);

    //// Мы можем либо захардкодить SpellId::Shield, либо передать его динамически
    //SpellId parentId = SpellId::Shield;

    //// Получаем время отката (с учетом талантов/статов персонажа, если есть)
    //float shieldCD = 10.0f; // Дефолт, или вытяни из SpellFactory::Create(parentId)->GetAdjustedCooldown()

    //// 2. Ставим КД в логику персонажа (серверная проверка)
    //caster.SetCooldown(parentId, shieldCD, server->matchTime);

    //// 3. Отправляем пакет клиенту, чтобы иконка Щита (Slot2) начала тикать
    //server->SendCooldownToClient(caster.GetId(), parentId, shieldCD);


    state = SpellState::Finished;
    return true;
}
