#pragma once
#include <unordered_map>
#include <cstdint>
class Match;
class Character;
class SquadManager {
   
   
public:
    SquadManager(Character* owner, Match* server);

    bool AddCompanion(Character* companion);
    void RemoveCompanion(uint32_t netId);

    size_t GetSquadSize() const { return companionSquad.size(); }
    const std::unordered_map<uint32_t, uint8_t>& GetSquad() const { return companionSquad; }
    void Clear();


#ifdef GAME_SERVER
    bool CanAddMore() const;
#endif
private:
    void ReorganizeSquad();

    Character* owner;
    Match* server;
    std::unordered_map<uint32_t, uint8_t> companionSquad; // netId -> formationIndex
    const size_t MAX_SQUAD_SIZE = 8;
};