#pragma once
#include <nlohmann/json.hpp>
#include <fstream>
#include <map>
#include <string>
#include <glm/glm.hpp>
using json = nlohmann::json;

enum class ResourceType {
    None,
    Mana,
    Resist, // Воин
    Info,   // Охотник
    Fire,   // Маг (огонь)
    Frost   // Маг (лед)
};

struct ResourceCost {
    ResourceType type = ResourceType::None;
    int amount = 0;
};

struct SpellInfo {
    std::string name;
    std::string description;
    ResourceCost cost; // Теперь тут и тип, и цена
    glm::vec2 uvOffset = { 0,0 };
    glm::vec2 uvScale = { 1,1 };
};


class SpellDatabase {
public:
    std::map<int, SpellInfo> spells;

    void Load(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) return;

        json data;
        try {
            file >> data;
        }
        catch (json::parse_error& e) {
            // Ошибка в синтаксисе JSON файла
            return;
        }

        // Карта для превращения строк из JSON в Enum
        std::map<std::string, ResourceType> resMap = {
            {"none",   ResourceType::None},
            {"mana",   ResourceType::Mana},
            {"resist", ResourceType::Resist},
            {"info",   ResourceType::Info},
            {"fire",   ResourceType::Fire},
            {"frost",  ResourceType::Frost}
        };

        for (auto& [key, value] : data.items()) {
            int id = std::stoi(key);
            SpellInfo info;

            info.name = value.value("name", "Unknown");
            info.description = value.value("description", "");

            // Читаем тип ресурса и количество отдельно
            std::string resStr = value.value("resource", "none");
            info.cost.type = resMap.count(resStr) ? resMap[resStr] : ResourceType::None;
            info.cost.amount = value.value("cost", 0);

            if (value.contains("uv") && value["uv"].is_array() && value["uv"].size() >= 2) {
                info.uvOffset.x = value["uv"][0];
                info.uvOffset.y = value["uv"][1];
            }

            info.uvScale = glm::vec2(0.09375f);
            spells[id] = info;
        }
    }
};