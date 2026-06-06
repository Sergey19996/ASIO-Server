#include "SpriteRenderer.h"

SpriteRenderer::SpriteRenderer()
{
	
	this->initRendererData();
}

SpriteRenderer::~SpriteRenderer()
{
	glDeleteVertexArrays(1, &this->quadVAO);
	glDeleteBuffers(1, &this->EBO);
	glDeleteBuffers(1, &this->VBO);
}

void SpriteRenderer::DrawSprite(Shader& shader,char texture, glm::vec2 position, glm::vec2 size, float rotate, glm::vec4 color, bool bFrame)
{
	
	shader.use();

	glm::mat4 trans = glm::mat4(1.0f);
	glm::vec3 scale = { 32.0f,32.0f,1.0 };

	trans = glm::translate(trans, glm::vec3(position, 1.0f));

	if (rotate != 0.0f) { // Проверка на 0.5f странная, лучше 0.0f
		trans = glm::translate(trans, glm::vec3(size.x * 0.5f, size.y * 0.5f, 0.0f));
		trans = glm::rotate(trans, rotate, glm::vec3(0.0f, 0.0f, 1.0f)); // Просто радианы!
		trans = glm::translate(trans, glm::vec3(size.x * -0.5f, size.y * -0.5f, 0.0f));
	}


	trans = glm::scale(trans, glm::vec3(size, 1.0f));

	//if( texture == '#')
	shader.setInt("symbol", static_cast<int>(texture));   // Send sign

	
	shader.setVec2("quadSize", size); // для рамки

	shader.setMat4("model", trans);
	shader.setvec4("color", color);
	shader.setBool("bFrame", bFrame);

	//glActiveTexture(GL_TEXTURE0);  // что бы отправить данные о текстуре 
	//texture.Bind();

	glBindVertexArray(this->quadVAO);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);  // 6 indices
	glBindVertexArray(0);
}

void SpriteRenderer::DrawSprite(Shader& shader,Texture2D& texture, glm::vec2 position, glm::vec2 size, float rotate, glm::vec4 color)
{
	//prepare transformation
	shader.use();
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(position, 0.0f)); // first translate (transformations are: scale happens first, then rotation, and then final translation happens; reversed order)

	model = glm::translate(model, glm::vec3(0.5f * size.x, 0.5f * size.y, 0.0f)); // move origin of rotation to center of quad
	model = glm::rotate(model, glm::radians(rotate), glm::vec3(0.0f, 0.0f, 1.0f)); // then rotate
	model = glm::translate(model, glm::vec3(-0.5f * size.x, -0.5f * size.y, 0.0f)); // move origin back

	model = glm::scale(model, glm::vec3(size, 1.0f));// last scale

	shader.setMat4("model", model);
	shader.setvec4("spriteColor", color);

	glm::vec2 uvScale = { 1.0f,1.0f };
	glm::vec2 uvOffset = { 0.0f,0.0f };
	// Передаем координаты атласа
	shader.setVec2("uvOffset", uvOffset);
	shader.setVec2("uvScale", uvScale);


	glActiveTexture(GL_TEXTURE0);
	texture.Bind();

	glBindVertexArray(this->quadVAO);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}

void SpriteRenderer::DrawSprite(Shader& shader,
	Texture2D& texture,
	glm::vec2 position,
	glm::vec2 size,
	float rotate,
	glm::vec4 color,
	glm::vec2 uvOffset,
	glm::vec2 uvScale)
{
	shader.use();

	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(position, 0.0f)); // first translate (transformations are: scale happens first, then rotation, and then final translation happens; reversed order)

	model = glm::translate(model, glm::vec3(0.5f * size.x, 0.5f * size.y, 0.0f)); // move origin of rotation to center of quad
	model = glm::rotate(model, glm::radians(rotate), glm::vec3(0.0f, 0.0f, 1.0f)); // then rotate
	model = glm::translate(model, glm::vec3(-0.5f * size.x, -0.5f * size.y, 0.0f)); // move origin back

	model = glm::scale(model, glm::vec3(size, 1.0f));// last scale

	shader.setMat4("model", model);
	shader.setvec4("color", color);

	// Передаем координаты атласа
	shader.setVec2("u_uvOffset", uvOffset);
	shader.setVec2("u_uvScale", uvScale);

	//if( texture == '#')
	shader.setInt("symbol", static_cast<int>(0));   // Send sign

	glActiveTexture(GL_TEXTURE0);
	texture.Bind();

	glBindVertexArray(this->quadVAO);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}

void SpriteRenderer::initRendererData()
{



	////  VAO/VBO
	float vertices[] = {
		// vrtx         UV
	   1.0f,  1.0f,		1,1,			 // top right    0
	   1.0f,  0.0f,		1,0,			// bottom right 1
	   0.0f,  1.0f,		0,1,			// top left     2
	   // vrtx           UV
	   0.0f, 0.0f, 		0,0,			// bottom left  3
	};
	unsigned int indices[] = {
		0,3,2,
		1,3,0
	};



	
	glGenVertexArrays(1, &this->quadVAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);
	// 1. bind Vertex Array Object
	glBindVertexArray(this->quadVAO);


	// // 2. copy our vertices array in a buffer for OpenGL to use
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	// 3. copy our index array in a element buffer for OpenGL to use
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// 3. then set our vertex attributes pointers
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0); // layout = 0  // говорим как читать данные с VBO
	glEnableVertexAttribArray(0);
	// Разбиндивание VAO (необязательно, но хорошая практика)
	glBindVertexArray(0);
	//


}
