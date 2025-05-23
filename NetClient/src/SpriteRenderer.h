#ifndef SPRITE_RENDERER_H
#define SPRITE_RENDERER_H
#include "Rendering/Shader.h"



#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


class SpriteRenderer {
public:

	SpriteRenderer(Shader& shader);
	~SpriteRenderer();


	void DrawSprite(char& texture, glm::vec2 position,
		glm::vec2 size = glm::vec2(32.0f, 32.0f), float rotate = 0.0f,
		glm::vec4 color = glm::vec4(1.0f));

	Shader& getShader() { return shader; };


private:

	Shader shader;
	unsigned quadVAO;  // хранит вертексы квадрата
	unsigned int  VBO, EBO;

	void initRendererData();



};


#endif // !SPRITE_RENDERER_H
