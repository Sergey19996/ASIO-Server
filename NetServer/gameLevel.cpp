#include "gameLevel.h"

gameLevel::gameLevel(const int& Width, const int& Heigth) : ScreenSize(Width, Heigth) {
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
