#include "CraftArrowSpell.h"

#include "../../../Match.h"
CraftArrowSpell::CraftArrowSpell(SpellId spellId, ArrowType arrowType)
{
    
        this->id = spellId;
        this->typeToCraft = arrowType;
    
}
bool CraftArrowSpell::Cast(Character& caster, Match* server)
{
    // Приводим персонажа к типу Hunter
    auto* hunter = dynamic_cast<Hunter*>(&caster);

    if (!hunter) return false;

    // Пытаемся скрафтить стрелу нужного типа
    // Метод Hunter::CraftArrow должен возвращать true, если в колчане было место
    return hunter->CraftArrow(this->typeToCraft);
}
