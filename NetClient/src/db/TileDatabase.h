#pragma once
#include <map>
#include <string>
#include <glm/glm.hpp>
#include "../../../NetShared/TileType.h"
#include <vector>
#include <unordered_map>

struct TileInfo {
    std::string name;
    glm::vec2 uvOffset;
    glm::vec2 uvScale;
    int framesCount = 1; // По умолчанию 1 кадр (статичный)
    float animSpeed = 0.0f;
    std::string textureTag; // Например, "tileset", "characters" или "ui"

    int sizeX = 1; // Размер в клетках по горизонтали
    int sizeY = 1; // Размер в клетках по вертикали

    // СЮДА ДОБАВЛЯЕМ: связь с тенью (пустая строка = тени нет)
    std::string shadowTag = "";
};
struct MapDecoration {
    int id;               // ID из вашей базы данных (например, трава = 5)
    glm::vec2 position;   // Свободные float-координаты на карте (хоть 2.5f, 3.1f)
    float scale = 1.0f;   // Можно добавить легкий рандом масштаба
};


class TileDatabase {
public:
    // По-прежнему храним базовые тайлы по Enum
    std::unordered_map<TileType, TileInfo> tiles;

    // СЮДА ДОБАВЛЯЕМ: Декали по числовому ID
    std::unordered_map<int, TileInfo> decals;

    // СЮДА ДОБАВЛЯЕМ: словарь для теней по их строковому имени
    std::unordered_map<std::string, TileInfo> shadows;

    // Список декораций на текущей карте
    std::vector<MapDecoration> decorations;

    void Load(const std::string& path);
       
};