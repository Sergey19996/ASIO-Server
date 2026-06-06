#include "TextRender.h"
#include <iostream>
#include "glm/gtc/matrix_transform.hpp"

#include "ft2build.h"
#include FT_FREETYPE_H

#include "../ResourceManager.h"
#include <vector>
TextRenderer::TextRenderer(unsigned int w, unsigned int h)
{
    Shader = ResourceManager::GetShader(ShaderID::TextShader);
    Shader.use();
    Shader.setMat4("projection",glm::ortho(0.0f, (float)w, (float)h, 0.0f));
    Shader.setInt("fontAtlas", 0);

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, 10000 * sizeof(TextVertex),
        nullptr, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
        sizeof(TextVertex), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
        sizeof(TextVertex),
        (void*)(sizeof(glm::vec2)));
}

void TextRenderer::Resize(unsigned int w, unsigned int h)
{
    Shader.use();
    Shader.setMat4(
        "projection",
        glm::ortho(0.0f, (float)w, (float)h, 0.0f)
    );
}

void TextRenderer::SetFont(FontAtlas* font)
{
    Font = font;
}

void TextRenderer::RenderText(const std::string& text,float x, float y,float scale,glm::vec3 color)
{
   
    Shader.use();
    Shader.setvec3f("color", color);
   
    Shader.setInt("fontAtlas", 0); // Убедитесь, что имя uniform в вашем шейдере именно "textSampler"
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, Font->TextureID);
    glBindVertexArray(VAO);

    std::vector<TextVertex> vertices;

   // for (char c : text)
    for (size_t i = 0; i < text.length(); ) // Убрали char c
    {
        uint32_t codepoint = next_utf8(text, i); // Читаем целый символ (даже если он 2 байта)

        const Glyph* g = Font->GetChar(codepoint); // Передаем uint32_t
        if (!g) continue;

        float baseline = y + Font->Ascent * scale;



        float xpos = std::round(x + g->Bearing.x * scale);
        float ypos = std::round(baseline - g->Bearing.y * scale);

        float w = std::round(g->Size.x * scale);
        float h = std::round(g->Size.y * scale);

        vertices.push_back({ {xpos,     ypos},     {g->UVMin.x, g->UVMin.y} });
        vertices.push_back({ {xpos,     ypos + h}, {g->UVMin.x, g->UVMax.y} });
        vertices.push_back({ {xpos + w, ypos + h}, {g->UVMax.x, g->UVMax.y} });

        vertices.push_back({ {xpos,     ypos},     {g->UVMin.x, g->UVMin.y} });
        vertices.push_back({ {xpos + w, ypos + h}, {g->UVMax.x, g->UVMax.y} });
        vertices.push_back({ {xpos + w, ypos},     {g->UVMax.x, g->UVMin.y} });

        x += (g->Advance >> 6) * scale;
    }

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0,
        vertices.size() * sizeof(TextVertex),
        vertices.data());


   
    glDrawArrays(GL_TRIANGLES, 0, vertices.size());
}

uint32_t TextRenderer::next_utf8(const std::string& str, size_t& i)
{
    // Берем текущий байт и сразу двигаем индекс i на следующий
    uint32_t c = (unsigned char)str[i++];

    // Если код меньше 128 (0x80), это обычный английский символ (ASCII).
    // Он занимает всего 1 байт. Условие if пропустится, и мы просто вернем c.
    if (c >= 0x80) {

        // Если байт начинается с битов '110' ((c & 0xE0) == 0xC0),
        // значит перед нами символ из 2-х байтов (например, русская 'А').
        if ((c & 0xE0) == 0xC0) {
            // Формула склейки:
            // 1. Берем полезные биты из первого байта (c & 0x1F) и сдвигаем их влево.
            // 2. Берем полезные биты из второго байта (str[i++] & 0x3F) и добавляем через ИЛИ.
            c = ((c & 0x1F) << 6) | (str[i++] & 0x3F);
        }

        // Если байт начинается с битов '1110' ((c & 0xF0) == 0xE0),
        // значит это символ из 3-х байтов (например, некоторые спецсимволы).
        else if ((c & 0xF0) == 0xE0) {
            // Склеиваем 3 байта в одно число. 
            // Полезные биты из первого байта сдвигаем уже на 12 позиций.
            c = ((c & 0x0F) << 12) | ((str[i] & 0x3F) << 6) | (str[i + 1] & 0x3F);

            // Мы использовали i и i+1, поэтому сдвигаем общий индекс строки на 2 вперед.
            i += 2;
        }
    }

    // Возвращаем итоговый код символа (например, для 'А' это будет 1040).
    return c;
}


float TextRenderer::MeasureTextWidth(const std::string& text, float scale)
{
    float width = 0.0f;
    if (!Font) return 0.0f;

    for (size_t i = 0; i < text.length(); ) {
        uint32_t codepoint = next_utf8(text, i);
        const Glyph* ch = Font->GetChar(codepoint);
        if (ch) width += (ch->Advance >> 6) * scale;
    }
    return width;
}
