#include "MagicConservation.h"
#include "../../../Match.h"
#include "../NetShared/entities/Mage.h"

bool MagicConservation::Cast(Character& caster, Match* server)
{
    Mage* mage = dynamic_cast<Mage*>(&caster);
    if (!mage) return false;

    // ПЕРВЫЙ КЛИК (Активация)
    if (!isSecondTime) {
        mage->CastConservation(); // Здесь баланс уходит в conservedBalance
        isSecondTime = true;
        state = SpellState::Active; // Держим спелл активным в менеджере
        return true;
    }

    // ВТОРОЙ КЛИК (Возврат)
    // ОБЯЗАТЕЛЬНО вызываем CastConservation еще раз перед завершением!
    mage->CastConservation();

    state = SpellState::Finished; // Теперь HandleCastSpell поставит КД и удалит спелл
    return true;
}
