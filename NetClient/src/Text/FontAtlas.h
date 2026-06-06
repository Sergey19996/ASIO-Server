#pragma once
#include <unordered_map>
#include <glm/glm.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H
#include <string>

struct Glyph
{
    glm::vec2 Size;
    glm::vec2 Bearing;
    unsigned int Advance;
    glm::vec2 UVMin;
    glm::vec2 UVMax;
};

class FontAtlas
{
public:
    unsigned int TextureID;
    unsigned int FontSize;

    std::unordered_map<uint32_t, Glyph> Glyphs; // Вместо char

    const Glyph* GetChar(uint32_t c) const;    // Вместо char

    FontAtlas(const std::string& fontPath, unsigned int fontSize);
    ~FontAtlas();
    int Ascent;     // ↑ высота над baseline

private:
    int Descent;    // ↓ под baseline (отрицательная)
    int LineHeight; // общая высота строки
};