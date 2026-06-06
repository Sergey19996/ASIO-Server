#pragma once

#include "Rendering/Shader.h"
#include "Rendering/Texture.h"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

// —труктура, описывающа€ один спрайт в массиве
struct SpriteInstance {
    glm::vec2 position;
    glm::vec2 size;
    float rotation;
    glm::vec4 color;
    glm::vec2 uvOffset;
    glm::vec2 uvScale;
};

class InstancedSpriteRenderer {
public:
    InstancedSpriteRenderer();
    ~InstancedSpriteRenderer();

    // ќсновной метод: принимает шейдер, текстурный атлас и массив собранных спрайтов
    void DrawInstances(Shader& shader, Texture2D& texture, const std::vector<SpriteInstance>& instances);

private:
    unsigned int quadVAO;
    unsigned int geometryVBO;
    unsigned int instanceVBO;
    unsigned int EBO;

    const size_t MAX_INSTANCES = 20000; // ћаксимальный лимит спрайтов в буфере за один вызов

    void initRendererData();
};
