#ifndef TEXT_RENDERER_H
#define TEXT_RENDERER_H
#include <map>

#include "glm/glm.hpp"

#include "../Rendering/Texture.h"
#include "../Rendering/Shader.h"
#include "FontAtlas.h"


struct TextVertex
{
	glm::vec2 pos;
	glm::vec2 uv;
};
class TextRenderer
{
public:
    TextRenderer(unsigned int w, unsigned int h);
    void Resize(unsigned int w, unsigned int h); // 👈 НОВОЕ
    void SetFont(FontAtlas* font);
    void RenderText(const std::string& text,
        float x, float y,
        float scale,
        glm::vec3 color = glm::vec3(1.0f)
    );
    // Простая функция для чтения следующего Unicode символа из UTF-8 строки
    uint32_t next_utf8(const std::string& str, size_t& i);
    Shader& GetShader() { return Shader; };

    float MeasureTextWidth(const std::string& text, float scale);
private:
    unsigned int VAO, VBO;
    Shader Shader;
    FontAtlas* Font;
};

#endif // !TEXT_RENDERER_H
