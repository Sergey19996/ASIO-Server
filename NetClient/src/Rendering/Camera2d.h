#pragma once
#include <glm/glm.hpp>

class Camera2D
{
public:
    glm::vec2 position = { 0, 0 }; // центр камеры
    float zoom = 1.0f;
    glm::vec2 visibleArea = { 0,0 };
    glm::mat4 GetMatrix(float levelWidth, float levelHeight) const;
    glm::mat4 GetViewMatrix() const;
    void UpdateViewport(float width, float height);
};
