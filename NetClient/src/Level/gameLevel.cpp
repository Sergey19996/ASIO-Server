#include "gameLevel.h"

gameLevel::gameLevel(const int& Width, const int& Heigth) : ScreenSize(Width,Heigth){
	LevelData =

        "#########################"
        "#.......................#"
        "#.......................#"
        "#.......................#"
        "#.......................#"
        "#.......................#"
        "#..####.................#"
        "###.....................#"
        "#................###....#"
        "#...................#####"
        "#.......................#"
        "#####...................#"
        "#...#...................#"
        "#...#................#..#"
        "#...#................#..#"
        "#....#.....##........#..#"
        "#.........#....#####.#..#"
        "#....#....#........#.#..#"
        "#########################";


}
gameLevel::~gameLevel()
{
	LevelData.clear();

}
void gameLevel::Render(SpriteRenderer& renderer){
	for (float y = 0; y < ScreenSize.y; y++) {
		for (float x = 0; x < ScreenSize.x; x++) {
			char tile = LevelData[y * ScreenSize.x + x];

            renderer.DrawSprite(tile, glm::ivec2(x * 32, y * 32),glm::vec2(32.0f,32.0f),0.0f, glm::vec4(0.611f, 0.3f, 0.3f,1.0f));


		}
	
	}


};
