#ifndef SpellFactory_h
#define SpellFactory_h

#include <memory>
#include "Spell.h"
#include "Mage/FireBallSpell.h"
#include "Mage/WallSpell.h"
#include "Warrior/SmashHit.h"
#include "Warrior/ShieldSpell.h"
#include "Hunter/Shoot.h"
#include "Hunter/StickyBombSpell.h"
#include "Mage/TeleportSpell.h"
#include "Hunter/GrappleSpell.h"
#include "Warrior/PullHookSpell.h"
#include "Mage/IceballSpell.h"
#include "Mage/BlastWave.h"
#include "Mage/Resonance Crystal.h"
#include "Hunter/BindArrowSpell.h"
#include "Warrior/ShieldBurst.h"
#include "Warrior/ShieldThrowSpell.h"
#include "Hunter/InfuseArrow.h"
#include "Hunter/ChainHarvest.h"
#include "Hunter/GhostArrow.h"
#include "Mage/FireDash.h"
#include "Mage/IceShield.h"
#include "Mage/IceRoots.h"
#include "Mage/StygianSpike.h"
#include "Mage/SunLance.h"
#include "Mage/SunMarker.h"
#include "Mage/SwitchSpell.h"
#include "Mage/MagicConservation.h"
#include "Hunter/ReloadQuiver.h"
#include "Hunter/CraftArrowSpell.h"
#include "AiSpells/MeleeHit.h"
#include "AiSpells/ArroWShot.h"

#include "AiSpells/comands/SpellCommandDefend.h"
#include "AiSpells/comands/SpellCommandMove.h"

class SpellFactory {
public:
    static std::unique_ptr<Spell> Create(SpellId id) {
        switch (id) {
        case SpellId::Fireball:
            return std::make_unique<FireballSpell>();
        case SpellId::Wall:
            return std::make_unique<WallSpell>();
        case SpellId::SmashHit:
            return std::make_unique<SmashHit>();
        case SpellId::Shield:
            return std::make_unique<ShieldSpell>();
        case SpellId::Shoot:
            return std::make_unique<Shoot>();
        case SpellId::StickyBomb:
            return std::make_unique<StickyBombSpell>();
        case SpellId::Teleport:
            return std::make_unique<TeleportSpell>();
        case SpellId::Grapple:
            return std::make_unique<GrappleSpell>();
        case SpellId::Hook:
            return std::make_unique<PullHookSpell>();
        case SpellId::Iceball:
            return std::make_unique<IceballSpell>();
        case SpellId::ResonanceCrystal:
            return std::make_unique<ResonanceCrystal>();
        case SpellId::BlastWave :
            return std::make_unique<BlastWave>();
        case SpellId::BindArrow:
            return std::make_unique<BindArrowSpell>();
        case SpellId::ShieldBurst:
            return std::make_unique<ShieldBurst>();
        case SpellId::ShieldThrow:
            return std::make_unique<ShieldThrowSpell>();
        case SpellId::InfuseArrow:
            return std::make_unique<InfuseArrow>();
        case SpellId::ChainHarvest:
            return std::make_unique<ChainHarvest>();
        case SpellId::GhostArrow:
            return std::make_unique<GhostArrow>();
        case SpellId::FireDash:
            return std::make_unique<FireDash>();
        case SpellId::IceShield:
            return std::make_unique<IceShield>();
        case SpellId::IceRoots :
            return std::make_unique<IceRoots>();
        case SpellId::StygianSpike:
            return std::make_unique<StygianSpike>();
        case SpellId::SunLance:
            return std::make_unique<SunLance>();
        case SpellId::SunMarker:
            return std::make_unique<SunMarker>();
        case SpellId::SwitchSpell:
            return std::make_unique<SwitchSpell>();
        case SpellId::MagicConservation:
            return std::make_unique<MagicConservation>();
        case SpellId::ReloadQuiver:
            return std::make_unique<ReloadQuiver>();
        case SpellId::MeleeHit:
            return std::make_unique<MeleeHit>();
        case SpellId::CraftGhost:
            return std::make_unique<CraftArrowSpell>(SpellId::CraftGhost, ArrowType::Ghost);
        case SpellId::CraftBind:
            return std::make_unique<CraftArrowSpell>(SpellId::CraftBind, ArrowType::Binding);
        case SpellId::CraftExplosive:
            return std::make_unique<CraftArrowSpell>(SpellId::CraftExplosive, ArrowType::Explosive);
        case SpellId::ArrowShot:
            return std::make_unique<ArrowShot>();
        case SpellId::CommandDefend:
            return std::make_unique<SpellCommandDefend>();
        case SpellId::CommandMove:
            return std::make_unique<SpellCommandMove>();


        default:
            return nullptr;
        }
    }
};


#endif // !SpellFactory_h
