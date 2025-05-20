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

			renderer.DrawSprite(tile, glm::ivec2(x * 32,y * 32));


		}
	
	}


};
