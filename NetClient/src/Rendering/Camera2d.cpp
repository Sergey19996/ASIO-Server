#include "Camera2d.h"
#include <glm/gtc/matrix_transform.hpp>

glm::mat4 Camera2D::GetMatrix(float levelWidth, float levelHeight) const
{
    float halfW = levelWidth * 0.5f / zoom;
    float halfH = levelHeight * 0.5f / zoom;

    return glm::ortho(
        position.x - halfW,
        position.x + halfW,
        position.y + halfH,
        position.y - halfH,
        -1.0f, 1.0f
    );
}

glm::mat4 Camera2D::GetViewMatrix() const
{
    glm::mat4 view(1.0f);


    view = glm::scale(view, glm::vec3(zoom, zoom, 1.0f));
    view = glm::translate(view, glm::vec3(-position, 0.0f));

    return view;
}

void Camera2D::UpdateViewport(float width, float height)
{
    
        this->visibleArea.x = width / zoom;  // мы получаем реверс от размера экрана - 800
        this->visibleArea.y = height / zoom; // 600 
    
}


