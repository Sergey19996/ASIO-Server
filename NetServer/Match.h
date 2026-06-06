#pragma once
#include "../NetShared/ProjectileRules.h"
#include "../NetCommon/olc_net.h" 
#include "../NetShared/GameplayRules.h"
#include "../NetServer/game/spells/SpellManager.h"

#include "game/entities/CharacterFactory.h"

#include "game/utils/SpatialCell.h"
//#include "../NetShared/ActionSlot.h"
#include "../NetServer/gameLevel.h"
#include "../NetShared/WorldTypes.h"
#include "ReconnectionSession.h"

//#include "DamageContext.h"
#include "MatchState.hpp"
#include <GLFW/glfw3.h>
#include <queue>
#include <array>
#include <set>


class GameServer; // forward


static const uint16_t MAX_PROJECTILES = 4096;

constexpr float GRID_CELL_SIZE = 2.0f; // или 3.0f
const float PLAYER_VELOCITY(2.5f);
#define SCREENWIDTH 800
#define SCREENHEIGHT 608
static const uint32_t INVALID_INDEX = 0xFFFFFFFF; // или (uint32_t)-1
struct ProjectileSlot {
	sProjectileDescription data;
	bool active = false;
    bool collisionEnabled = false;
	int nextInCell = -1;
	ProjectileRules cachedRules;
};


struct TileUpdate
{
    uint16_t index;
    char value;
    float respawnTime; // Время до восстановления
};
struct Player {
    uint32_t netId;          // ПОСТОЯННЫЙ
    std::unique_ptr<Character> character;
    PlayerSessionData session;
    PlayerState state;
    bool active = false;
    int nextInCell = -1;
};

struct SpawnerSource { // Для мобов вокруг клеток
    glm::ivec2 gridPos;
    float nextSpawnTime = 0.0f;
    int currentLimit = 1; // Макс. мобов от этого источника
    int currentCount = 0; // Добавляем это
};
struct SpawnRequest {  // Для компаньоновв
 //   EntityType type;
  //  NpcType nType; // Добавляем конкретный подтип
    ArchetypeId type;

    glm::vec2 pos;
    Character* owner;
};


class Match {
public:
    Match(uint32_t id, GameServer* server, int w, int h, uint32_t seed);

    void Update(float dt);
    uint32_t SpawnAI(ArchetypeId type, glm::vec2 pos, int home);
    uint32_t SpawnCompanion(ArchetypeId type, glm::vec2 pos, Character* owner);
    void TrySpawnCompanionForPlayer(uint32_t ownerId, glm::vec2 position);
    void OnPlayerMessage(uint32_t player,  olc::net::message<GameMsg>& msg);
    // В Match.h
    void HandlePlayerTileChange(uint32_t playerId, glm::ivec2 newPos, glm::ivec2 oldPos);
    void AddPlayer(uint32_t id);
    void RemovePlayer(uint32_t id);

    bool HasPlayer(uint32_t id) const;

    bool IsReadyToDelete() const;

    // Найти персонажа по его индексу (безопасно)
    Character* GetCharacterByIdx(uint32_t idx);
    bool HasLineOfSight(glm::vec2 start, glm::vec2 end);
    std::vector<Player>& GetEntities()  { return entities; }
    const std::vector<Player>& GetEntities() const { return entities; }
    std::vector<uint32_t>& GetPlayersIdx() { return realPlayerIds; }
    const  std::vector<uint32_t>& GetPlayersIdx() const { return realPlayerIds; }
    uint32_t GetPlayerIndex(uint32_t netId) const {  // Безопасный поиск индекса (вместо того чтобы давать менять idToIndex напрямую)
        auto it = idToIndex.find(netId);
        return (it != idToIndex.end()) ? it->second : -1;
    }
    uint32_t GetPlayerIndex(uint32_t netId) {  // Безопасный поиск индекса (вместо того чтобы давать менять idToIndex напрямую)
        auto it = idToIndex.find(netId);
        return (it != idToIndex.end()) ? it->second : -1;
    }
    const std::vector<uint16_t>& GetActiveProjectileIndices() const { return activeProjectileIndices; };

    void SendTileUpdate(const glm::ivec2& cell, char value);
    void SendTileMove(const glm::ivec2& from, const glm::ivec2& to, float duration);
    void SendCooldownToClient(uint32_t playerId, SpellId spell, float cd);
    void ApplyDamage(DamageContext ctx);
    uint16_t CreateProjectile(ProjectileType type, const ProjectileParams& params, glm::vec2 pos, glm::vec2& baseVel, float& baseRadius); // метод вызывается внутри спеллов
    void MarkProjectileForRemoval(uint32_t id);
    void MessageAllMatchClients(olc::net::message<GameMsg>& msg);
    void SendAreaDamageToClient(olc::net::message<GameMsg>& msg, glm::vec2 epicentersPos);
    void BroadcastSnapshot();
  //  void CheckProjectileCharacterCollision(uint16_t pid, ProjectileSlot& slot, float dt);
    Spell* HandleCastSpell(uint32_t playerId, SpellId spell, uint32_t ConnectionId);

   // void BroadcastProjectile(uint16_t idx);
    glm::ivec2 WorldToGridCell(const glm::vec2& pos) const;
    int GridIndex(const glm::ivec2& c) const;
   // void DestroyWall(const glm::vec2& cellPos);
    void ProcessAreaDamage(glm::vec2 pos, float radius, float damage, uint32_t attackerId, bool hitAttacker, ProjectileRules rule);
    // === spatial ===
    std::vector<SpatialCell> spatialGrid;
    std::array<ProjectileSlot, MAX_PROJECTILES> projectiles;
    std::unordered_map<int, SpawnerSource> activeSpawners; 
    std::vector<SpawnRequest> pendingSpawns;
    float matchTime = 0.0f;
    float dayProgress = 0.0f;
    float lightIntensity = 0.0f;
    gameLevel level;
    SpellManager spellManager;
    std::vector<sDamageReport> damageQueue;
    // Используем inline static constexpr для эффективного хранения и доступа
    inline static constexpr glm::ivec2 neighbors[9] = {
        {0, 0}, {1, 0}, {-1, 0}, {0, 1}, {0, -1}, // Центр и крест
        {1, 1}, {1, -1}, {-1, 1}, {-1, -1}        // Диагонали
    };
    // 5 векторов, покрывающих половину соседства + центр
    inline static constexpr glm::ivec2 forwardNeighbors[5] = {
        {0, 0}, {1, 0}, {1, 1}, {0, 1}, {-1, 1}
    };
    void OnCharacterDied(uint32_t id); // в паблик потому что есть персонажи которые могут бить сами себя в character
    void ApplyBindEffect(Character* target, glm::vec2 center, int linkedId, bool isDynamic);
    uint32_t FindNearestEnemy(glm::vec2 pos, uint32_t ownerNetId, float maxDist);
    glm::vec2 FindNearestEmptySpace(const gameLevel& level, glm::vec2 startPos);
    void NotifyMobDied(int  home);
    void CleanupCharacter(uint32_t netId);
    PlayerID GeneratePlayerID();
    glm::vec2 GetSpawnPoint(uint32_t id);
    void SyncEntityStats(uint32_t entityId);
    //std::unordered_map<uint32_t, uint8_t> GetMinionsOf(uint32_t casterId); // принимает вечный id - а не idtoindex
private:


    uint32_t matchID;
    MatchState state;
    uint32_t winnerID;
    std::vector<Player> entities; // плотный массив
    std::vector<uint32_t> realPlayerIds; // Только ID живых игроков
    std::unordered_map<PlayerID, uint8_t> idToIndex; // по вечному id - мы сразу находим индекс для players
  
    //роза ветров
    float windAngle = 0.0f;
    float windChangeTimer = 0.0f;
    uint8_t windStrength = 0;
    //


    std::vector<uint16_t> activeProjectileIndices;
    std::queue<uint16_t> freeProjectileIds;

    std::set<uint16_t> garbageProjectiles;
    std::vector<uint32_t> garbageEntities;

    // === spatial ===
    int gridWidth = 0;
    int gridHeight = 0;
    float tileMoveTimer = 0.0f;
   
    GameServer* server;
    std::vector<TileUpdate> pendingTileUpdates; // хранилище для тайлов - которые были уничтожены
   
    std::vector<glm::vec2> actualSpawns;
private:

   
    float endMatchTimer = 3.0f; // Таймер послематчевого экрана
    float roomTimer;
    bool bStatsProcessed = false;
  //  void BroadcastMatchmakingStatus();
    void StartRoom();
    void CheckAllReady();
    void StartGame();
    void EndMatch(uint32_t WinnerId);

    void UpdateCharacters(const float& dt, const float& lightIntensity);
    void UpdateProjectiles(float dt);
 
    void FlushTileUpdates();
    void UpdateSpawners(float dt);
//    void CollectGarbage(GameServer& server);

 //   void OnProjectileHit(...);
//    void OnCharacterDied(uint32_t id);
   
   

    void HandleTileInteractions(float dt);
    void InitSpatialGrid();
    void ClearSpatialGrid();

   

   
   
    void CalculateValidSpawns();

 
    void HandleCancelSpell(uint32_t playerId, SpellId id);
    SpellId GetSpellForSlot(PlayerClass cls, ActionSlot slot, const Character& pl);
    ActionSlot GetSlotForSpell(PlayerClass& cls, SpellId& spell, const Character& pl);

    void RemoveActiveProjectile(uint16_t id);
    void CheckVictoryCondition();
  
   // void HandleShield(DamageContext& ctx, Character& target);
    void HandleStatusModifiers(DamageContext& ctx, Character& target);

    void OnProjectileHit(uint32_t projId, uint32_t targetId, const ProjectileRules& rules);
    void OnProjectileBlocked(uint32_t projId, uint32_t blockerProjId, const ProjectileRules& rules);

    bool IsCellFree(const glm::ivec2& cell) const;
   
};