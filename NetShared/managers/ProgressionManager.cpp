#include "ProgressionManager.h"
#include "../entities/Character.h"

#ifdef GAME_SERVER
#include "../../NetServer/Match.h"
#endif // GAME_SERVER


ProgressionManager::ProgressionManager(Character* owner) : owner(owner) {}


void ProgressionManager::AddExperience(float amount)
{
    currentXP += amount;

    // ѕока опыта хватает на следующий уровень и мы не достигли капа таблицы
    while (level < xpTable.size() - 1 && currentXP >= xpTable[level])
    {
        // ≈сли вы хотите, чтобы лишний опыт перетекал в следующий уровень:
         currentXP -= xpTable[level]; 

        // ≈сли вы обнул€ете (как в вашем коде):
       // currentXP = 0.0f;


#ifdef GAME_SERVER
        level++;
        OnLevelUp();
#endif

        // ≈сли обнулили опыт, выходим из цикла, так как на еще один уровень точно не хватит
        if (currentXP == 0.0f) break;
    }


#ifdef GAME_SERVER
    if (owner->server) {

        owner->server->SyncEntityStats(owner->GetId());

    }
#endif
}
void ProgressionManager::AddLevel(uint32_t lvl) {

    level = lvl;

}
void ProgressionManager::SetExp(float exp)
{
    currentXP = exp;
}
#ifdef GAME_SERVER
void ProgressionManager::OnLevelUp()
{
    // 1. ”лучшаем статы
    owner->maxHealth += 20;
    owner->Heal(owner->maxHealth); // ѕолный хил при лвлапе Ч это классика
   
   
   
}
#endif