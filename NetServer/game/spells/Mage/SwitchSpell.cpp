#include "SwitchSpell.h"
#include "../NetShared/entities/Mage.h"
#include "../../../Match.h"
bool SwitchSpell::Cast(Character& caster, Match* server)
{
    Mage* mage = dynamic_cast<Mage*>(&caster);
    if (mage) {
        mage->CastSwitch();
        return true;
       
    }

   
    return false;
}
