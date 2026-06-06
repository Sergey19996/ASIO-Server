#include "FontAtlas.h"
#include <glad/glad.h>
#include <iostream>
const Glyph* FontAtlas::GetChar(uint32_t c) const
{
    auto it = Glyphs.find(c);
    if (it == Glyphs.end())
        return nullptr;
    return &it->second;
}
FontAtlas::FontAtlas(const std::string& fontPath, unsigned int fontSize)
{
    // Создаем список нужных диапазонов
    std::vector<std::pair<uint32_t, uint32_t>> ranges = {
        {32, 128},    // ASCII
        {0x0400, 0x04FF} // Кириллица (Русский, Украинский и т.д.)
    };

    FontSize = fontSize;



    FT_Library ft;
    FT_Init_FreeType(&ft);

    FT_Face face;
    FT_New_Face(ft, fontPath.c_str(), 0, &face);
    FT_Set_Pixel_Sizes(face, 0, fontSize);

    FT_Size_Metrics m = face->size->metrics;
    Ascent = m.ascender >> 6; // Сколько пикселей вверх от baseline
    Descent = m.descender >> 6; // Сколько пикселей вниз от baseline
    LineHeight = m.height >> 6; // Полная высота строки

    // ---------- 1. считаем размер атласа ----------
    int atlasWidth = 0;
    int atlasHeight = 0;

    for (auto& range : ranges) {
        for (uint32_t c = range.first; c < range.second; c++) {
            if (FT_Load_Char(face, c, FT_LOAD_RENDER)) continue;
            atlasWidth += face->glyph->bitmap.width + 1;
            atlasHeight = LineHeight; //Высота атласа = высота строки Именно это позволяет сделать единый baseline
        }
    }

    // ---------- 2. создаём атлас ----------
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); //Bitmap — 1 байт на пиксель

    glGenTextures(1, &TextureID);
    glBindTexture(GL_TEXTURE_2D, TextureID);

   

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED,  // Создаём пустую текстуру
        atlasWidth, atlasHeight, 0,
        GL_RED, GL_UNSIGNED_BYTE, nullptr); //Один канал (GL_RED) — идеально для шрифтов

    int xOffset = 0; // 🔹 Горизонтальная «ручка пера»
    int yOffset = 0;
      
    // ---------- 3. заполняем ----------   Укладка glyph’ов в атлас
    for (auto& range : ranges) {
        for (uint32_t c = range.first; c < range.second; c++) {
            if (FT_Load_Char(face, c, FT_LOAD_RENDER)) continue;




            glTexSubImage2D( // Копируем bitmap символа в атлас
                GL_TEXTURE_2D,
                0,
                xOffset,
                yOffset,
                face->glyph->bitmap.width,
                face->glyph->bitmap.rows,
                GL_RED,
                GL_UNSIGNED_BYTE,
                face->glyph->bitmap.buffer
            );

            Glyph glyph;
            glyph.Size = {
                face->glyph->bitmap.width,
                face->glyph->bitmap.rows
            };
            glyph.Bearing = { // 📍 Смещение bitmap’а относительно baseline
                face->glyph->bitmap_left,
                face->glyph->bitmap_top
            };
            glyph.Advance = face->glyph->advance.x;

            glyph.UVMin = {
           (float)xOffset / atlasWidth,
           (float)yOffset / atlasHeight
            };

            glyph.UVMax = {
                (float)(xOffset + glyph.Size.x) / atlasWidth,
                (float)(yOffset + glyph.Size.y) / atlasHeight
            };
            Glyphs[c] = glyph;
            xOffset += glyph.Size.x + 1;
        }
    }
    // Параметры текстуры выносим СЮДА (после всех циклов)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    FT_Done_Face(face);
    FT_Done_FreeType(ft);
}

FontAtlas::~FontAtlas()
{
    glDeleteTextures(1, &TextureID);
}
