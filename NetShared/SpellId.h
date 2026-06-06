#pragma once
#include <cstdint> // Добавляет поддержку uint8_t, uint32_t и др.
enum class SpellId : uint8_t
{
    Fireball,
    Iceball,
    Wall,
    SmashHit,
    Shoot,
    Shield,
    Heal,
    Dash,
    StickyBomb,
    Teleport,
    Grapple,
    Hook,
    ResonanceCrystal,
    BlastWave,
    BindArrow,
    ShieldBurst,
    ShieldThrow,
    InfuseArrow,
    ChainHarvest,
    GhostArrow,
    FireDash,
    IceShield,
    IceRoots,
    StygianSpike,
    SunLance,
    SunMarker,
    SwitchSpell,
    MagicConservation,
    ReloadQuiver,
    CraftGhost,
    CraftBind,
    CraftExplosive,

    MeleeHit,
    ArrowShot,

    CommandMove,
    CommandDefend,

    None
};
