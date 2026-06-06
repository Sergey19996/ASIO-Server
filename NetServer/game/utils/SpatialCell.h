#pragma once 
#include <vector>

struct SpatialCell
{
    int firstCharacter = -1; // Индекс первого игрока в этой ячейке
    int firstProjectile = -1; // Индекс первого снаряда в этой ячейке

    void Clear() {
        firstCharacter = -1;
        firstProjectile = -1;
    }
};
