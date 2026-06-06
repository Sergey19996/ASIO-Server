#pragma once
#include "SpellId.h"
#include "ActionSlot.h"
#include "StatusEffect.h" // Для маски эффектов
#include <cmath>

namespace GameplayRules
{
//    // --- МАГ ---
//    inline SpellId GetMageSpell(ActionSlot slot, float balance) {
//        bool isFire = balance > 0.0f;
//        switch (slot) {
//        case ActionSlot::Slot4: return isFire ? SpellId::FireDash : SpellId::IceShield;
//        case ActionSlot::Slot5: return isFire ? SpellId::BlastWave : SpellId::IceRoots;
//        case ActionSlot::Slot6: return isFire ? SpellId::SunLance : SpellId::StygianSpike;
//        case ActionSlot::Slot3: return SpellId::MagicConservation;
//           
//        default: return SpellId::None;
//        }
//    }
//
//    // --- ВОИН ---
//    inline SpellId GetWarriorSpell(ActionSlot slot, uint32_t effectsMask) {
//        // Проверяем бит эффекта Щита (StatusEffectType::Shield)
//        bool hasShield = (effectsMask & (1 << (int)StatusEffectType::Shield));
//        if (hasShield) {
//            if (slot == ActionSlot::Slot1) return SpellId::ShieldBurst;
//            if (slot == ActionSlot::Slot2) return SpellId::ShieldThrow;
//        }
//        return SpellId::None;
//    }
//
//    // --- ОХОТНИК ---
//    // Для Хантера передаем распакованные данные стрелы (из того самого uint32_t ExtraData)
//    inline SpellId GetHunterSpell(ActionSlot slot, bool arrowActive, bool arrowInFlight, int arrowType) {
//        if (slot == ActionSlot::Slot1) return SpellId::Shoot;
//        if (slot == ActionSlot::Slot2) {
//            // Если стрела в полете и не активна в колчане - это детонация/активация
//            if (!arrowActive && arrowInFlight) {
//                if (arrowType == 1 /*Explosive*/) return SpellId::StickyBomb;
//                if (arrowType == 2 /*Binding*/)   return SpellId::BindArrow;
//            }
//            // Иначе - зарядка
//            return SpellId::InfuseArrow;
//        }
//        return SpellId::None;
//    }

    // --- ЕДИНЫЙ РЕЕСТР КУЛДАУНОВ ---
    inline float GetDefaultCooldown(SpellId id) {
        switch (id)
        {
        case SpellId::Shoot: return 0.55f;
        case SpellId::SmashHit: return 0.65f;
        case SpellId::Fireball: return 1.5f;
        case SpellId::Wall: return 8.0f;
        case SpellId::Shield: return 5.0f;
        case SpellId::StickyBomb: return 5.0f;
        case SpellId::BindArrow: return 5.0f;
        case SpellId::Teleport: return 5.0f;
        case SpellId::Grapple: return 5.0f;
        case SpellId::Hook: return 5.0f;
        case SpellId::Iceball: return 1.5f;
        case SpellId::ResonanceCrystal: return 5.0f;
        case SpellId::BlastWave: return 5.0f;
        case SpellId::ShieldThrow: return 5.0f;
        case SpellId::ShieldBurst: return 5.0f;
        case SpellId::InfuseArrow: return 2.5f;
        case SpellId::ChainHarvest: return 3.0f;
        case SpellId::GhostArrow: return 3.0f;
        case SpellId::FireDash: return 5.0f;
        case SpellId::IceShield: return 15.0f;
        case SpellId::IceRoots: return 5.0f;
        case SpellId::StygianSpike: return 5.0f;
        case SpellId::SunLance: return 5.0f;
        case SpellId::SwitchSpell: return 5.0f;
        case SpellId::MagicConservation: return 5.0f;
        case SpellId::ReloadQuiver: return 2.5f;
        case SpellId::MeleeHit: return 4.0f;
        case SpellId::ArrowShot: return 4.0f;
        case SpellId::CommandDefend: return 0.5f;
        case SpellId::CommandMove: return 0.5f;
        default: return 0.0f;
        }
    }
}