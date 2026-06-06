#include "TileDatabase.h"
#include <nlohmann/json.hpp>
#include <fstream>

using json = nlohmann::json;
void TileDatabase::Load(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open()) return;

    json data;
    try {
        file >> data;  // загружаем в data json данные 
    }
    catch (json::parse_error& e) {
        return;
    }

    if (data.contains("tiles")) {

    // Карта для связи строк из JSON с вашим Enum
    std::unordered_map<std::string, TileType> stringToEnum = {
        {"Ground",      TileType::Empty},
        //   {"wall",       TileType::Wall},
           {"BigStone",      TileType::Block},
           {"Beacon",     TileType::Beacon},
           {"ice",        TileType::Ice},
           {"explosive",  TileType::Explosive},
           {"sand",       TileType::Sand},
           {"fire",       TileType::Fire},
           {"healer",     TileType::Healer},
           {"graveyard",  TileType::Graveyard},
           {"altar",      TileType::CompanionAltar}
    };

    for (auto& [key, value] : data["tiles"].items()) {
        // Проверяем, есть ли такое имя тайла в нашем Enum
        if (stringToEnum.count(key) == 0) continue;
        TileType type = stringToEnum[key];

        TileInfo info;
        info.name = value.value("name", "Unknown Tile");
        info.framesCount = value.value("frames", 1);
        info.animSpeed = value.value("anim_speed", 0.0f);
        info.textureTag = value.value("texture_tag", "Level_tiles"); // <-- Добавляем чтение тега
        info.sizeX = value.value("size_x", 1); // 1 дефолтное значение если не найдет size_x
        info.sizeY = value.value("size_y", 1);
        if (value.contains("uv") && value["uv"].is_array() && value["uv"].size() >= 2) {
            info.uvOffset.x = value["uv"][0];
            info.uvOffset.y = value["uv"][1];
        }

        // <-- UPDATE: Считываем имя тени для тайла, если оно задано
        info.shadowTag = value.value("shadow_type", "");


        if (value.contains("uv_scale") && value["uv_scale"].is_array() && value["uv_scale"].size() >= 2) {
            info.uvScale.x = value["uv_scale"][0];
            info.uvScale.y = value["uv_scale"][1];
        }
        tiles[type] = info;
         }
     }
    // 2. ПАРСИМ ДЕКАЛИ (DECALS)
    if (data.contains("decals")) {
        for (auto& [key, value] : data["decals"].items()) {
            int id = std::stoi(key); // Переводим строковый "1" в int 1

            TileInfo info;
            info.name = value.value("name", "Unknown Decal");
            // Так как атлас тот же, по умолчанию ставим "Level_tiles"
            info.textureTag = value.value("texture_tag", "Level_tiles");

            if (value.contains("uv") && value["uv"].is_array() && value["uv"].size() >= 2) {
                info.uvOffset.x = value["uv"][0];
                info.uvOffset.y = value["uv"][1];
            }
            if (value.contains("uv_scale") && value["uv_scale"].is_array() && value["uv_scale"].size() >= 2) {
                info.uvScale.x = value["uv_scale"][0];
                info.uvScale.y = value["uv_scale"][1];
            }

            decals[id] = info; // Записываем в словарь декалей
        }
    }
    // 3. ПАРСИМ ТЕНИ (SHADOWS)
    if (data.contains("shadows")) { // <-- UPDATE: Исправлена опечатка и логика парсинга
        for (auto& [key, value] : data["shadows"].items()) { // Читаем именно shadows

            TileInfo info;
            info.name = value.value("name", "Unknown Shadow");
            info.textureTag = value.value("texture_tag", "Level_tiles");

            info.sizeX = value.value("size_x", 1); // 1 дефолтное значение если не найдет size_x
            info.sizeY = value.value("size_y", 1);

            if (value.contains("uv") && value["uv"].is_array() && value["uv"].size() >= 2) {
                info.uvOffset.x = value["uv"][0];
                info.uvOffset.y = value["uv"][1];
            }
            if (value.contains("uv_scale") && value["uv_scale"].is_array() && value["uv_scale"].size() >= 2) {
                info.uvScale.x = value["uv_scale"][0];
                info.uvScale.y = value["uv_scale"][1];
            }

            shadows[key] = info; // Записываем в словарь теней по строковому ключу
        }
    }
}

