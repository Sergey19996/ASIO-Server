#ifndef GAME_LEVEL
#define GAME_LEVEL
#include <string>
#include <glm/glm.hpp>

//#include "../SpriteRenderer.h"

class gameLevel {
public:
	gameLevel(const int& Width, const int& Height);
	~gameLevel();
	//void Render(SpriteRenderer& renderer);

	std::string LevelData;
	glm::ivec2 ScreenSize;
private:





};

#endif // !GAME_LEVEL
