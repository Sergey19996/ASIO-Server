#include "ReloadQuiver.h"
#include "../../../Match.h"
#include "../NetShared/entities/Hunter.h"

bool ReloadQuiver::Cast(Character& caster, Match* server)
{
    auto* hunter = dynamic_cast<Hunter*>(&caster);
    if (!hunter ) return false;

    hunter->ReloadQuiver();
    state = SpellState::Finished;

    return true;
}
