#include "InstancedSpriteRenderer.h"
#include <cstddef> // Для offsetof

InstancedSpriteRenderer::InstancedSpriteRenderer() {
    this->initRendererData();
}

InstancedSpriteRenderer::~InstancedSpriteRenderer() {
    glDeleteVertexArrays(1, &this->quadVAO);
    glDeleteBuffers(1, &this->geometryVBO);
    glDeleteBuffers(1, &this->instanceVBO);
    glDeleteBuffers(1, &this->EBO);
}

void InstancedSpriteRenderer::initRendererData() {
    // Геометрия базового единичного квадрата [0, 1]
    float vertices[] = {
        // координаты     // UV
        1.0f,  1.0f,      1.0f, 1.0f, // верхний правый  (0)
        1.0f,  0.0f,      1.0f, 0.0f, // нижний правый   (1)
        0.0f,  1.0f,      0.0f, 1.0f, // верхний левый   (2)
        0.0f,  0.0f,      0.0f, 0.0f  // нижний левый    (3)
    };
    unsigned int indices[] = {
        0, 3, 2,
        1, 3, 0
    };

    glGenVertexArrays(1, &this->quadVAO);
    glGenBuffers(1, &this->geometryVBO);
    glGenBuffers(1, &this->EBO);
    glGenBuffers(1, &this->instanceVBO); // Буфер для динамических данных инстансов

    glBindVertexArray(this->quadVAO);

    // 1. Привязываем статическую геометрию квадрата
    glBindBuffer(GL_ARRAY_BUFFER, this->geometryVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // layout 0: Базовые вершины и текстурные координаты самого квадрата
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // 2. Привязываем динамический буфер инстансов
    glBindBuffer(GL_ARRAY_BUFFER, this->instanceVBO);
    // Выделяем память на видеокарте под максимальный лимит спрайтов
    glBufferData(GL_ARRAY_BUFFER, MAX_INSTANCES * sizeof(SpriteInstance), nullptr, GL_DYNAMIC_DRAW);

    GLsizei stride = sizeof(SpriteInstance);

    // layout 1: position
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(SpriteInstance, position));
    glVertexAttribDivisor(1, 1); // 1 означает: обновлять аттрибут раз в 1 инстанс

    // layout 2: size
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(SpriteInstance, size));
    glVertexAttribDivisor(2, 1);

    // layout 3: rotation
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(SpriteInstance, rotation));
    glVertexAttribDivisor(3, 1);

    // layout 4: color
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(SpriteInstance, color));
    glVertexAttribDivisor(4, 1);

    // layout 5: uvOffset
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 2, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(SpriteInstance, uvOffset));
    glVertexAttribDivisor(5, 1);

    // layout 6: uvScale
    glEnableVertexAttribArray(6);
    glVertexAttribPointer(6, 2, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(SpriteInstance, uvScale));
    glVertexAttribDivisor(6, 1);

    glBindVertexArray(0);
}

void InstancedSpriteRenderer::DrawInstances(Shader& shader, Texture2D& texture, const std::vector<SpriteInstance>& instances) {
    if (instances.empty()) return;

    shader.use();

    // Загружаем собранный вектор в память видеокарты
    glBindBuffer(GL_ARRAY_BUFFER, this->instanceVBO);

    // Если вдруг объектов больше лимита, обрезаем, чтобы не выйти за границы буфера
    size_t count = instances.size();
    if (count > MAX_INSTANCES) count = MAX_INSTANCES;

    // glBufferSubData заменяет данные в уже выделенной памяти (это быстро)
    glBufferSubData(GL_ARRAY_BUFFER, 0, count * sizeof(SpriteInstance), instances.data());

    glActiveTexture(GL_TEXTURE0);
    texture.Bind();

    glBindVertexArray(this->quadVAO);

    // Отрисовка всех экземпляров за ОДИН вызов
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0, static_cast<GLsizei>(count));

    glBindVertexArray(0);
}