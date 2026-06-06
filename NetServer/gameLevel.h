#ifndef GAME_LEVEL
#define GAME_LEVEL
#include <string>
#include <glm/glm.hpp>
#include <vector>
#include <functional>
//#include "../SpriteRenderer.h"
#include "iostream"
#include "../NetShared/TileType.h"

struct Player;
class Match;


struct Tile
{
	TileType type;
	uint32_t ownerId;
	
	int hp = 0;
	bool destroyed = false;


	float respawnTimer = 0.0f;

	// для анимаций
	float growInterp = 1.0f; // 0..1 (появление / движение)
	glm::ivec2 offset = { 0,0 }; // смещение при "ползании"


	// 🔹 ДВИЖЕНИЕ (логика, не визуал)
	bool moving = false;
	glm::ivec2 from;
	glm::ivec2 to;
	float moveT = 0.0f;     // 0..1
	float moveSpeed = 1.0f / 0.6f; // 0.6 сек
	
	bool isUpdateActive = false; // Флаг, чтобы не добавлять в вектор дважды
	bool isSolid = false;

};

struct Node {
	glm::ivec2 pos;
	float g, h;
	glm::ivec2 parent;
	float f() const { return g + h; }
	bool operator>(const Node& other) const { return f() > other.f(); }
};
struct PathNode {
	int parentIdx = -1;
	float gCost = INFINITY;
	uint32_t lastQueryId = 0; // Магия для быстрой очистки
};
class gameLevel {
public:
	

//	std::vector<Tile> tiles;

	gameLevel(int w, int h, uint32_t seed, Match* match);
	~gameLevel();

	Tile& GetTile(const glm::ivec2& cell);
	glm::vec2 GetTileCenter(const int id);
	int FindRandomSolidTile();

	bool IsSolid(const glm::ivec2& c) const;
	void DamageTile(const glm::ivec2& c, int dmg, uint32_t AttackerId);
	void Update(float dt);
	void TryMoveTilesNearPlayers(const std::vector<glm::vec2>& playerPositions, int attempts);
	bool MoveTileInstant(const glm::ivec2& from, const glm::ivec2& to);
	void StartTileMovement(const glm::ivec2& from, const glm::ivec2& to, float speed);
	std::vector<uint8_t> GetBlocksData() const;
	// Безопасный доступ к тайлу по индексу
	const Tile& GetTileByIdx(int idx) const { return tiles[idx]; }
	// Геттер для итерации по активным индексам (только для чтения)
	const std::vector<int>& GetActiveTileIndices() const { return activeTiles; }
	std::vector<glm::vec2> FindPath(glm::vec2 startWorld, glm::vec2 endWorld);
	bool IsPathClear(glm::vec2 start, glm::vec2 end, float radius);

	std::function<void(const glm::ivec2&, char, float)> OnTileChanged;
	std::function<void(const glm::ivec2& cell, uint32_t ownerId)> OnBeaconOwnerChanged;
	std::function<void(const glm::ivec2& from,const glm::ivec2& to,float duration)> OnTileMove;
	bool IsValid(const glm::ivec2& pos) const  {
		if (pos.x >= 0 && pos.x < levelWidth && pos.y >= 0 && pos.y < levelHeight)
			return true;

		return false;
	}
	int levelWidth;
	int levelHeight;
	uint32_t seed;
	Match* match; // Указатель на владельца
	//std::string LevelData;
private:

	void MarkTileActive(int idx) {
		if (!tiles[idx].isUpdateActive) {
			tiles[idx].isUpdateActive = true;
			activeTiles.push_back(idx);
		}
		
	}
	glm::ivec2 IndexToCoords(int idx) const {
		return { idx % levelWidth, idx / levelWidth };
	}

//const LevelTemplate& base; // 🔥 ссылка, не копия
	
	std::vector<Tile> tiles; //состояние мира теперь хранится в std::vector<Tile> tiles
	std::vector<int> activeTiles;


	std::vector<PathNode> nodeCache;
	uint32_t currentQueryId = 0; // Счётчик поисков

};

#endif // !GAME_LEVEL
