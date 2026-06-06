#include "SquadManager.h"
#include "../entities/Character.h"
#include "../AI/Companions/CompanionBase.h"
#include "ProgressionManager.h"
#include "../../NetServer/Match.h"


SquadManager::SquadManager(Character* owner, Match* server) : owner(owner), server(server)
{

}

bool SquadManager::AddCompanion(Character* companion)
{
    if (companionSquad.size() >= MAX_SQUAD_SIZE) return false; // Проверка лимита

    uint32_t netId = companion->id;
    uint8_t slot = (uint8_t)companionSquad.size();
    companionSquad[netId] = slot;

#ifdef GAME_SERVER
    auto comp = dynamic_cast<CompanionBase*>(companion);
    if (comp) comp->SetFormationIndex(slot);
#endif

    return true; // ОБЯЗАТЕЛЬНО возвращаем результат
}

void SquadManager::RemoveCompanion(uint32_t netId)
{

    if (companionSquad.erase(netId)) {
        // Пересчитываем индексы оставшихся
        uint8_t nextIdx = 0;
        for (auto& [id, slot] : companionSquad) {
            slot = nextIdx++;

#ifdef GAME_SERVER
            // Сообщаем живым компаньонам их новые позиции
            auto entityIdx = server->GetPlayerIndex(id);
            if (entityIdx != 0xFFFF) {
                auto comp = dynamic_cast<CompanionBase*>(server->GetEntities()[entityIdx].character.get());
                if (comp) comp->SetFormationIndex(slot);
            }
#endif
        }
    }
}

void SquadManager::Clear()
{
    companionSquad.clear();
}

#ifdef GAME_SERVER
bool SquadManager::CanAddMore() const
{
    // Получаем уровень через ProgressionManager владельца
    uint8_t currentLevel = owner->GetProgression()->GetLevel();
   
    // Формула лимита: например, 1 слот изначально + 1 слот за каждые 2 уровня
    // Уровень 1-2: 1 слот, Уровень 3-4: 2 слота и т.д.
    size_t allowedSize = 1 + (currentLevel / 2);

    // Ограничиваем абсолютным максимумом (8)
    if (allowedSize > MAX_SQUAD_SIZE) allowedSize = MAX_SQUAD_SIZE;

    return companionSquad.size() < allowedSize;
}
#endif

void SquadManager::ReorganizeSquad()
{
    uint8_t nextIdx = 0;
    for (auto& pair : companionSquad) {
        pair.second = nextIdx;
        nextIdx++;
    }
}
