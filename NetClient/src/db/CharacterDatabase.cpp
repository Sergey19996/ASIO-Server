#include "CharacterDatabase.h"
#include <fstream>

void CharacterDatabase::Load(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return;

    nlohmann::json data;
    try { file >> data; }
    catch (...) { return; }

    if (!data.contains("characters")) return;

    std::unordered_map<std::string, LayerRotationType> rotMap = {
        {"velocity",   LayerRotationType::Velocity},
        {"mouse",      LayerRotationType::Mouse},
        {"weapon_lag", LayerRotationType::WeaponLag}
    };

    // Новый Z-index порядок слоев на основе ваших изменений:
    // 0: legs, 1: head_and_weapons (ручки + голова вместе), 2: eyes
    std::unordered_map<std::string, int> layerOrder = {
        {"legs", 0},
        {"head_and_weapons", 1},
        {"eyes", 2}
    };

    for (auto& [charKey, charValue] : data["characters"].items()) {
        CharacterInfo charInfo;
        charInfo.name = charValue.value("name", "Unknown Character");
        charInfo.textureTag = charValue.value("texture_tag", "Character_atlas");

        if (charValue.contains("layers")) {
            for (auto& [layerKey, layerValue] : charValue["layers"].items()) {
                if (layerOrder.count(layerKey) == 0) continue;

                int zIndex = layerOrder[layerKey];
                LayerInfo lInfo;

                std::string rotStr = layerValue.value("rotation_type", "velocity");
                lInfo.rotationType = rotMap.count(rotStr) ? rotMap[rotStr] : LayerRotationType::Velocity;

                // --- ДОБАВЛЕНО: ПАРСИНГ РАЗМЕРА СЛОЯ В КОЭФФИЦИЕНТАХ ПЛИТОК ---
                if (layerValue.contains("size") && layerValue["size"].is_array() && layerValue["size"].size() >= 2) {
                    lInfo.size.x = layerValue["size"][0];
                    lInfo.size.y = layerValue["size"][1];
                }
                else {
                    lInfo.size = glm::vec2(1.0f, 1.0f); // Дефолт: 1х1 плитка (32х32 пикселя)
                }

                // 1. ПАРСИНГ КАРТЫ АНИМАЦИЙ СЛОЯ
                if (layerValue.contains("animations")) {
                    for (auto& [animKey, animValue] : layerValue["animations"].items()) {
                        AnimationData anim;

                        if (animValue.contains("uv") && animValue["uv"].is_array() && animValue["uv"].size() >= 2) {
                            anim.uvOffset.x = animValue["uv"][0];
                            anim.uvOffset.y = animValue["uv"][1];
                        }
                        if (animValue.contains("uv_scale") && animValue["uv_scale"].is_array() && animValue["uv_scale"].size() >= 2) {
                            anim.uvScale.x = animValue["uv_scale"][0];
                            anim.uvScale.y = animValue["uv_scale"][1];
                        }

                        anim.framesCount = animValue.value("frames", 1);
                        anim.animSpeed = animValue.value("anim_speed", 0.0f);

                        // ИЗМЕНЕНО: Хэшируем имя анимации ("idle" -> число) при сохранении
                        lInfo.animations[RuntimeStringId(animKey)] = anim;
                    }
                }

                // 2. ПАРСИНГ ПАРАМЕТРОВ ДЛЯ ОРУЖИЯ И ДРУГИХ СЛОЕВ
                lInfo.lagSpeed = layerValue.value("lag_speed", 10.0f);

                // 3. ПАРСИНГ СПЕЦИФИЧНЫХ ПАРАМЕТРОВ ДЛЯ ГЛАЗ
                lInfo.lookDistance = layerValue.value("look_distance", 0.0f);

                if (layerValue.contains("eye_offsets") && layerValue["eye_offsets"].is_array()) {
                    for (auto& offsetVal : layerValue["eye_offsets"]) {
                        if (offsetVal.is_array() && offsetVal.size() >= 2) {
                            lInfo.localPositions.push_back(glm::vec2(offsetVal[0], offsetVal[1]));
                        }
                    }
                }

                charInfo.layers[zIndex] = lInfo;
            }
        }
        // ИЗМЕНЕНО: Хэшируем имя класса ("Mage" -> число) при сохранении в базу
        characters[RuntimeStringId(charKey)] = charInfo;
    }
}

const CharacterInfo* CharacterDatabase::GetCharacter(StringId id) const {
    auto it = characters.find(id);
    return it != characters.end() ? &it->second : nullptr;
}