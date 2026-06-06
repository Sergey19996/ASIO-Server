#include "HealthBarRenderer.h"
#include "../ResourceManager.h"
#include "../NetShared/StatusEffect.h"
//void HealthBarRenderer::Render(SpriteRenderer* sr, const std::unordered_map<uint32_t, sPlayerDescription>& players, const std::map<uint32_t, VisualObject>& visual, float tileSize, uint32_t mask)
//{
//    //for (const auto& [id, p] : players)
//    //{
//    //    
//
//    //    if (p.nHealth <= 0)
//    //        continue;
//
//    // 
//    //    auto it = visual.find(id);
//    //    if (it != visual.end())
//    //    {
//
//
//    //        HealthBar bar;
//    //        bar.current = p.nHealth;
//
//
//
//    //        // Используем vCurrentPos для отрисовки!
//    //        DrawBar(sr, it->second.vCurrentPos, p.fRadius, bar, tileSize,mask);
//    //    }
//    //}
//}

void HealthBarRenderer::DrawBar(SpriteRenderer* sr, const glm::vec2& worldPos, const float& radius, const HealthBar& bar, float tileSize, uint32_t mask, uint32_t nShieldHP)
{
    
    char HealthBarChar = '!'; // 33 
    // ВАЖНО: worldPos.x * tileSize — это центр персонажа.
   // Чтобы полоска была по центру, вычитаем ПОЛОВИНУ её ширины.
    float startX = (worldPos.x * tileSize) - (bar.healthBarWidth / 2.0f);

    // По Y: голова персонажа это (worldPos.y - radius). 
    // Поднимаем еще выше на высоту полоски и отступ.
    float startY = (worldPos.y - radius) * tileSize - bar.healthBarHeight - 5.0f;



    glm::vec2 screenPos = { startX, startY };

    Shader& SpriteShader = ResourceManager::GetShader(ShaderID::World);
    float percent = bar.Percent();



    // background
    sr->DrawSprite(SpriteShader,
        HealthBarChar,
        screenPos,
        { bar.healthBarWidth, bar.healthBarHeight },
        0.0f,
        { 0,0,0,0.6f });

    // fill
    sr->DrawSprite(SpriteShader,
        HealthBarChar,
        screenPos,
        { bar.healthBarWidth * percent, bar.healthBarHeight },
        0.0f,
        { 1,0,0,0.9f });



    // 3. ЩИТ (Исправленный расчет ширины!)
    if (nShieldHP > 0)
    {
        // ОШИБКА БЫЛА ТУТ: нельзя умножать ширину на количество ХП щита напрямую.
        // Нужно делить текущий щит на максимальное ХП (или макс. щит), чтобы получить процент.
        float shieldPercent = (float)nShieldHP / bar.max;

        glm::vec2 shieldStartPos = { screenPos.x + (bar.healthBarWidth * percent), screenPos.y };
        glm::vec4 shieldColor = (mask & (1 << (uint8_t)StatusEffectType::IceShield))
            ? glm::vec4(0.0f, 1.0f, 1.0f, 0.8f) : glm::vec4(1.0f, 0.8f, 0.2f, 0.8f);

        sr->DrawSprite(SpriteShader, HealthBarChar, shieldStartPos,
            { bar.healthBarWidth * shieldPercent, bar.healthBarHeight }, 0.0f, shieldColor);
    }
}

void HealthBarRenderer::DrawStatusEffects(SpriteRenderer* renderer, glm::vec2 charPos, float radius, uint32_t mask, float tileSize)
{
    if (mask == 0) return; // Экономим ресурсы, если эффектов нет
    float iconSize = tileSize * 0.4f; // Иконка примерно в пол-блока
    char HealthBarChar = '!'; // 33 
    glm::vec2 screenPos = {
     (charPos.x - radius) * tileSize,
     (charPos.y - radius) * tileSize - 10 - iconSize };

    Shader& SpriteShader = ResourceManager::GetShader(ShaderID::World);
    // Позиция: чуть выше имени персонажа
    // У вас имя на (currentPos.y + 1.2f), значит эффекты пустим на +1.5f
    //float startX = (charPos.x * tileSize);
    //float yPos = (charPos.y - 1.6f) * tileSize;

    int iconCount = 0;

    auto DrawIcon = [&](StatusEffectType type, glm::vec4 color) {
        if (mask & (1 << (uint8_t)type)) {
            // Рисуем квадрат или спрайт иконки
            renderer->DrawSprite(
                SpriteShader,
                HealthBarChar, // Или специальный символ из атласа для эффекта
                {screenPos.x + iconCount * iconSize, screenPos.y},
                { iconSize, iconSize },
                0.0f,
                color
            );
            iconCount++;
        }
        };

    // ?? Burn (Огонь): Насыщенный красный/оранжевый
    DrawIcon(StatusEffectType::Burn, { 1.0f, 0.2f, 0.0f, 1.0f });

    // ?? Slow (Замедление/Холод): Светло-голубой (лёд)
    DrawIcon(StatusEffectType::Slow, { 0.4f, 0.8f, 1.0f, 1.0f });

    // ? Stun (Оглушение): Ярко-желтый (электричество/искры из глаз)
    DrawIcon(StatusEffectType::Stun, { 1.0f, 0.9f, 0.0f, 1.0f });

    // ?? Silence (Безмолвие): Глубокий пурпурный (магия пустоты)
    DrawIcon(StatusEffectType::Silence, { 0.6f, 0.0f, 0.8f, 1.0f });

    // ?? BindingChain (Цепи): Бирюзовый/Мятный (магический захват, отличается от Slow)
    DrawIcon(StatusEffectType::BindingChain, { 0.0f, 1.0f, 0.7f, 1.0f });

    // ?? Linked (Связь): Ярко-зеленый (жизненная нить)
    DrawIcon(StatusEffectType::Linked, { 0.2f, 1.0f, 0.2f, 1.0f });

    // ??? Shield (Щит): Золотисто-коричневый или Белый (прочность)
    DrawIcon(StatusEffectType::Shield, { 1.0f, 0.8f, 0.4f, 1.0f });

    // ?? TeleportCooldown (КД Портала): Темно-синий/Индиго (пространственная магия)
    DrawIcon(StatusEffectType::TeleportCooldown, { 0.2f, 0.2f, 0.6f, 1.0f });

    // IceShield: Неоново-бирюзовый (Cyan) — выглядит как магический лёд
    DrawIcon(StatusEffectType::IceShield, { 0.0f, 1.0f, 1.0f, 1.0f });

  
   // Насыщенный "электрический" циан (Electric Cyan)
    DrawIcon(StatusEffectType::StygianChill, { 0.0f, 0.95f, 1.0f, 1.0f });
   
    DrawIcon(StatusEffectType::SunBrand, { 1.0f, 0.4f, 0.0f, 1.0f });
}
