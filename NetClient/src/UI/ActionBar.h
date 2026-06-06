#pragma once
#include "BaseUI.h"
#include "../db/SpellDatabase.h" // Тот класс, что мы написали ранее
#include <vector>

struct SlotRect {
    float x, y, size;
    ActionSlot slotType;
    bool hovered = false;
};

class ActionBar : public BaseUI {
private:
    std::vector<SlotRect> computedSlots;
    Game* game; // Указатель на основной класс игры для доступа к данным

    // Константы теперь живут внутри класса (или можно вынести в конфиг)
    const float MOUSE_SIZE = 55.0f;
    const float SKILL_SIZE = 45.0f;
    const float PADDING = 8.0f;
    const float GROUP_GAP = 25.0f;

public:
    ActionBar(Game* gamePtr) : game(gamePtr) {}

    // Реализуем расчет координат (можно вызывать в OnActivate или при ресайзе)
    void RefreshLayout(int levelWidth, int levelHeight);

    // Переопределяем методы BaseUI
    void Render(TextRenderer* r, SpriteRenderer* sr, int levelWidth, int levelHeight) override;
    void Update(float dt, int mouseX, int mouseY) override;

    // Вспомогательный метод для отрисовки самого тултипа
    void DrawTooltip(TextRenderer* r, SpriteRenderer* sr, const SpellInfo& info, float x, float y);
};