#ifndef SPRITE_RENDERER_H
#define SPRITE_RENDERER_H
#include "Rendering/Shader.h"
#include "Rendering/Texture.h"


#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


class SpriteRenderer {
public:

	SpriteRenderer();
	~SpriteRenderer();


	void DrawSprite(
		Shader& shader,
		char texture, glm::vec2 position,
		glm::vec2 size = glm::vec2(32.0f, 32.0f), float rotate = 0.0f,
		glm::vec4 color = glm::vec4(1.0f), bool bFrame = true);

	void DrawSprite(
		Shader& shader,
		Texture2D& texture,
		glm::vec2 position,
		glm::vec2 size = glm::vec2(32.0f),
		float rotate = 0.0f,
		glm::vec4 color = glm::vec4(1.0f)
	);

	void DrawSprite(Shader& shader,
		Texture2D& texture,
		glm::vec2 position,
		glm::vec2 size,
		float rotate,
		glm::vec4 color,
		glm::vec2 uvOffset, glm::vec2 uvScale);


private:

	
	unsigned quadVAO;  // хранит вертексы квадрата
	unsigned int  VBO, EBO;

	void initRendererData();



};


#endif // !SPRITE_RENDERER_H
