#pragma once
#include <string>
#include <unordered_map>
#include <map>
#include <nlohmann/json.hpp>
#include <glm/glm.hpp> // Подключаем базовые типы GLM
#include "../../../NetShared/StringId.h"


// Типы вращения слоев
enum class LayerRotationType {
    Velocity,   // За вектором движения
    Mouse,      // За курсором мыши
    WeaponLag   // За вектором движения/мыши с отставанием
};

// Параметры конкретной анимации (один блок из JSON)
struct AnimationData {
    glm::vec2 uvOffset{ 0.0f, 0.0f };
    glm::vec2 uvScale{ 1.0f, 1.0f };
    int framesCount = 1;
    float animSpeed = 0.0f;
    
};

struct LayerInfo {
    LayerRotationType rotationType = LayerRotationType::Velocity;

    // ИЗМЕНЕНО: Ключ теперь StringId вместо std::string ("idle" станет числом)
    std::unordered_map<StringId, AnimationData> animations;

    // Специфичные данные для глаз
    float lookDistance = 0.0f;
    std::vector<glm::vec2> localPositions;
    glm::vec2  size;
    float lagSpeed = 0.0f;
};

struct CharacterInfo {
    std::string name;
    std::string textureTag;
    // Используем std::map или enum, чтобы гарантировать порядок отрисовки слоев!
    std::map<int, LayerInfo> layers;
};

class CharacterDatabase {
public:
    static CharacterDatabase& Instance() {
        static CharacterDatabase instance;
        return instance;
    }
    void Load(const std::string& path);
    const CharacterInfo* GetCharacter(StringId id) const;

    // СЛУЖЕБНЫЙ МЕТОД: Оставляем перегрузку для строк, чтобы код инициализации не падал
    const CharacterInfo* GetCharacter(const std::string& id) const {
        return GetCharacter(RuntimeStringId(id));
    }

private:
    // Ключ: хэш архетипа (например, хэш от "Mage")
    std::unordered_map<StringId, CharacterInfo> characters;
};