#ifndef GAME_LEVEL
#define GAME_LEVEL
#include <string>
#include "../SpriteRenderer.h"
#include "../InstancedSpriteRenderer.h"
#include <unordered_set>
#include "../db/TileDatabase.h"

//enum class FogState : uint8_t
//{
//	Hidden,    // никогда не видел (чёрный)
//	Explored,  // видел раньше (серый)
//	Visible    // сейчас виден
//};
class Camera2D;

struct TileVisual
{
	float growInterp = 1.0f;
	uint32_t ownerId = 0xFFFFFFFF;

	bool moving = false;
	glm::vec2 from;
	glm::vec2 to;
	float t = 0.0f;
	float duration = 0.0f;

	// --- Новое ---
	float respawnTimer = 0.0f;
	bool isRespawning = false;

};

class gameLevel {
public:
	gameLevel(int Width, int Height, uint32_t seed);
	~gameLevel();
	void Update(const float& dt);
	
//	void SyncFromServer(const std::vector<uint8_t>& serverBlocks);

	void Render(SpriteRenderer& renderer, InstancedSpriteRenderer* instRenderer, Camera2D& camera, TileDatabase& tileDb);

	std::string LevelData;
	glm::ivec2 TileSize;   // (25, 18)
	std::unordered_set<int> activeTiles; // индексы тайлов
	std::vector<TileVisual> tileVisuals;   // визуал

	std::vector<uint8_t> blocks; // for fog

	// Наши 4 буфера для сбора геометрии за один проход
	std::vector<SpriteInstance> floorLayer;
	std::vector<SpriteInstance> decalLayer;
	std::vector<SpriteInstance> shadowLayer;
	std::vector<SpriteInstance> objectLayer;


	//std::vector<FogState> fog; // size = width * height
	glm::ivec2 LevelSize;
private:


	


};

#endif // !GAME_LEVEL
