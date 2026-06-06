#include "CharacterFactory.h"
#include "../NetShared/entities/Warrior.h"
#include "../NetShared/entities/Mage.h"
#include"../NetShared/entities/Hunter.h"
#include "../NetShared/AI/MeleeMob.h"
#include "../NetShared/AI/RangerMob.h"
#include "../NetShared/AI/Companions/WarriorCompanion.h"
#include "../NetShared/AI/Companions/RangerCompanion.h"
#include "../NetShared/AI/Companions/PriestCompanion.h"
#include "../../Match.h"


std::unique_ptr<Character> CharacterFactory::Create(PlayerClass cls, uint32_t netId, const std::string& name, Match* match)
{
    std::unique_ptr<Character> result;

    switch (cls) {
    case PlayerClass::Warrior:
        result = std::make_unique<Warrior>(netId, name, match);
        break;
    case PlayerClass::Mage:
        result = std::make_unique<Mage>(netId, name, match);
        break;
    case PlayerClass::Hunter:
        result = std::make_unique<Hunter>(netId, name, match);
        break;
    default:
        result = std::make_unique<Character>(netId, name, match);
        break;
    }




    return result;
};

std::unique_ptr<Character> CharacterFactory::CreateAI(ArchetypeId type, uint32_t netId, Match* server, glm::vec2 spawnPos,int home)
{
    if (type == ArchetypeId::Mob_Melee) {
            auto zombie = std::make_unique<MeleeMob>(netId, "zombie", spawnPos, server);
            zombie->entityType = ArchetypeId::Mob_Melee;
            zombie->homeTile = home; // Внутри сервера фабрика ЗНАЕТ про AIBase
            return zombie;
        }
        else {
            auto Skeleton = std::make_unique<RangerMob>(netId, "Skeleton", spawnPos, server);
            Skeleton->entityType = ArchetypeId::Mob_Ranged;
            Skeleton->homeTile = home; // Внутри сервера фабрика ЗНАЕТ про AIBase
            return Skeleton;
        }

        //auto zombie = std::make_unique<RangerMob>(netId, "Skeleton", spawnPos, server);
        //zombie->entityType = EntityType::Mob;
        //zombie->homeTile = home; // Внутри сервера фабрика ЗНАЕТ про AIBase
        //return zombie;
    //if (type == EntityType::Boss) {
    //    // return std::make_unique<BossCharacter>(...);
    //}
    return nullptr;
}

std::unique_ptr<Character> CharacterFactory::CreateCompanion(ArchetypeId type,uint32_t netId, Character* owner, Match* server)
{

   
    //if (type == ArchetypeId::Companion) {

        if (type == ArchetypeId::Companion_Warrior) {
            auto Warrior = std::make_unique<WarriorCompanion>(netId, owner, server);
        //    Warrior->entityType = EntityType::Companion;
            //  zombie->homeTile = home; // Внутри сервера фабрика ЗНАЕТ про AIBase
            return Warrior;
        }
        else if (type == ArchetypeId::Companion_Hunter){
            auto Ranger = std::make_unique<RangerCompanion>(netId, owner, server);
         //   Ranger->entityType = EntityType::Companion;
            //  zombie->homeTile = home; // Внутри сервера фабрика ЗНАЕТ про AIBase
            return Ranger;
        }
        else if (type == ArchetypeId::Companion_Priest)
        {
            auto Priest = std::make_unique<PriestCompanion>(netId, owner, server);
        //    Priest->entityType = EntityType::Companion;
            //  zombie->homeTile = home; // Внутри сервера фабрика ЗНАЕТ про AIBase
            return Priest;
        }

    
   // }
    if (type == ArchetypeId::Boss_LichKing) {
        // return std::make_unique<BossCharacter>(...);
    }
    return nullptr;
}

