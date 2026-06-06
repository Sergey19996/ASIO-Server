#pragma once
#include <unordered_map>
#include "../Text/TextRender.h"
#include "../SpriteRenderer.h"
#include "../NetShared/WorldTypes.h"
struct HealthBar {
    int current = 100;
    int max = 100;

    float healthBarWidth = 32.0f;
    float healthBarHeight = 5.0f;
    float offsetY = -10.0f;

    float Percent() const {
        return (float)current / (float)max;
    }
};


class HealthBarRenderer {
public:
   /* void Render(SpriteRenderer* sr,
        const std::unordered_map<uint32_t, sPlayerDescription>& players,
        const std::map<uint32_t, VisualObject>& visual,
        float tileSize, uint32_t mask
    );*/
    void DrawBar(SpriteRenderer* sr,
        const glm::vec2& worldPos,
        const float& radius,
        const HealthBar& bar,
        float tileSize,
        uint32_t mask,
        uint32_t nShieldHP);

    void DrawStatusEffects(SpriteRenderer* renderer, glm::vec2 charPos, float radius, uint32_t mask, float tileSize);

private:
  
};