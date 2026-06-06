#include "ActionBar.h"
#include "../Game.h"
#include "../ResourceManager.h"

void ActionBar::RefreshLayout(int levelWidth, int levelHeight)
{
  
    computedSlots.clear();
    float totalWidth = (3 * MOUSE_SIZE + 2 * PADDING) + GROUP_GAP + (5 * SKILL_SIZE + 4 * PADDING);
    float startX = (levelWidth - totalWidth) * 0.5f;
    float baseY = levelHeight - MOUSE_SIZE - 35.0f;

    for (int i = 0; i < 8; ++i) {
        bool isMouse = (i < 3);
        float s = isMouse ? MOUSE_SIZE : SKILL_SIZE;
        float x = isMouse ? (startX + i * (MOUSE_SIZE + PADDING)) :
            (startX + (3 * MOUSE_SIZE + 2 * PADDING) + GROUP_GAP + (i - 3) * (SKILL_SIZE + PADDING));
        float y = isMouse ? baseY : baseY + (MOUSE_SIZE - SKILL_SIZE) * 0.5f;

        computedSlots.push_back({ x, y, s, static_cast<ActionSlot>(i), false });
    }
    // --- НОВОЕ: Добавляем слоты 9, 10, 11 (клавиши 1, 2, 3) ---
   // Разместим их ровно над слотами Q, E, R (индексы 3, 4, 5)
    for (int i = 8; i < 11; ++i) {
        int targetBaseIdx = i - 8; // Смещение относительно Q, E, R
        auto& baseSlot = computedSlots[targetBaseIdx];

        float s = SKILL_SIZE;
        float x = baseSlot.x; // Выравниваем по X с нижними кнопками
        float y = baseSlot.y - SKILL_SIZE - 20.0f; // Поднимаем выше на размер кнопки + отступ

        computedSlots.push_back({ x, y, s, static_cast<ActionSlot>(i), false });
    }
}

void ActionBar::Update(float dt, int mouseX, int mouseY) {
    // Проверяем наведение на каждый слот
    for (auto& slot : computedSlots) {
        slot.hovered = (mouseX >= slot.x && mouseX <= slot.x + slot.size && mouseY >= slot.y && mouseY <= slot.y + slot.size);
    }
}

void ActionBar::DrawTooltip(TextRenderer* r, SpriteRenderer* sr, const SpellInfo& info, float x, float y)
{
    glm::vec4 gold = glm::vec4(1.0f, 0.84f, 0.0f, 0.8f);
    glm::vec4 frameBg = glm::vec4(0.05f, 0.05f, 0.05f, 0.95f);

    float titleScale = 0.45f;
    float descScale = 0.35f;
    float padding = 12.0f;

    // Считаем ширину по самому длинному тексту
    float tw = r->MeasureTextWidth(info.name, titleScale);
    float dw = r->MeasureTextWidth(info.description, descScale);
    float boxW = std::max(tw, dw) + padding * 2.0f;
    float boxH = 75.0f; // Фиксированная или расчетная высота

    // Корректировка Y, чтобы тултип был НАД слотом
    float drawY = y - boxH - 10.0f;

    // 1. Внешняя золотая рамка
    sr->DrawSprite(ResourceManager::GetShader(ShaderID::UIGlyph), '#',
        glm::vec2(x - 2, drawY - 2), glm::vec2(boxW + 4, boxH + 4), 0.0f, gold);

    // 2. Внутренний темный фон
    sr->DrawSprite(ResourceManager::GetShader(ShaderID::UIGlyph), '#',
        glm::vec2(x, drawY), glm::vec2(boxW, boxH), 0.0f, frameBg);

    // 3. Текст: Название
    r->RenderText(info.name, x + padding, drawY + padding, titleScale, glm::vec3(1.0f, 0.84f, 0.0f));

    // 4. Текст: Описание
    r->RenderText(info.description, x + padding, drawY + padding + 25.0f, descScale, glm::vec3(0.9f));

    // 5. Текст: Стоимость (выделяем синим/голубым)
    std::string costStr = "Cost: " + std::to_string(info.cost.amount);
    r->RenderText(costStr, x + padding, drawY + boxH - 20.0f, 0.3f, glm::vec3(0.0f, 0.8f, 1.0f));
}

void ActionBar::Render(TextRenderer* r, SpriteRenderer* sr, int levelWidth, int levelHeight) {
    auto it = game->playerEntities.find(game->nPlayerID);
    if (it == game->playerEntities.end()) return;
    Character* myChar = it->second.get();
    float now = game->GetServerTime();
    for (int i = 0; i < 11; ++i) {
        auto& slotRect = computedSlots[i];

        // 1. Получаем действие и имя клавиши из KeyConfig
        GameAction action = static_cast<GameAction>((int)GameAction::Slot1 + i);
        int boundKey = game->config.bindings[action];
        std::string keyLabel = KeyBindUI::GetKeyName(boundKey);

        // 2. Получаем ID спелла, который у игрока в этом слоте
        SpellId spellId = myChar->GetBoundSpell(slotRect.slotType);
        if (spellId == SpellId::None) continue;


        bool isAvailable = myChar->CanCast(spellId, now);


        // 3. Ищем данные в SpellDatabase (по индексу Enum)
        int idInt = static_cast<int>(spellId);
        if (game->spellDb.spells.count(idInt)) {
            const SpellInfo& info = game->spellDb.spells[idInt];

            // --- ОТРИСОВКА ---
            // Фон слота
            sr->DrawSprite(ResourceManager::GetShader(ShaderID::UIGlyph), '#',
                { slotRect.x, slotRect.y }, { slotRect.size, slotRect.size }, 0.0f, glm::vec4(0.1f, 0.1f, 0.1f, 0.8f));

   
            // Иконка спелла (используем UV из JSON)
            sr->DrawSprite(ResourceManager::GetShader(ShaderID::Sprite),ResourceManager::GetTexture("spell_icons"),
                { slotRect.x + 2, slotRect.y + 2 }, { slotRect.size - 4, slotRect.size - 4 },
                0.0f, glm::vec4(1.0f), info.uvOffset, info.uvScale);
          

            if (!isAvailable) {
                // Рисуем темно-красный или серый фильтр на всю иконку
                sr->DrawSprite(ResourceManager::GetShader(ShaderID::UIGlyph), '#',
                    { slotRect.x, slotRect.y }, { slotRect.size, slotRect.size },
                    0.0f, glm::vec4(0.2f, 0.0f, 0.0f, 0.5f)); // Полупрозрачный красный "замок"
            }

            // 2. ОТРИСОВКА КУЛДАУНА (ЗАТЕМНЕНИЕ)
            const CooldownInfo* cd = game->GetCooldownBySpellId(spellId);
            if (cd && cd->endTime > now) {
                float progress = glm::clamp((cd->endTime - now) / cd->duration, 0.0f, 1.0f);

                // Рисуем полупрозрачный черный прямоугольник, который уменьшается сверху вниз
                sr->DrawSprite(ResourceManager::GetShader(ShaderID::UIGlyph), '#',
                    glm::vec2(slotRect.x, slotRect.y + slotRect.size * (1.0f - progress)),
                    glm::vec2(slotRect.size, slotRect.size * progress),
                    0.0f, glm::vec4(0.0f, 0.0f, 0.0f, 0.6f));

                // Текст секунд кулдауна
                int seconds = (int)std::ceil(cd->endTime - now);
                r->RenderText(std::to_string(seconds), slotRect.x + slotRect.size * 0.3f, slotRect.y + slotRect.size * 0.3f, 0.4f, glm::vec3(1, 1, 0));
            }



                // Подпись клавиши (LMB, 1, 2 и т.д.)
            r->RenderText(keyLabel, slotRect.x, slotRect.y + slotRect.size + 5, 1.0f, glm::vec3(0.7f));

            // 4. Тултип (если мышь над слотом)
            if (slotRect.hovered) {
                DrawTooltip(r, sr, info, slotRect.x, slotRect.y);
            }
        }
    }
}