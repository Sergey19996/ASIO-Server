#include <iostream>


#include <glad/glad.h>   // всё остальное
#include <GLFW/glfw3.h>  // окна/вводы.эвенты
#include <glm/gtc/matrix_transform.hpp>
#include "src/Game.h"
#include "src/ResourceManager.h"
#include "src/GameStates.hpp"
#include "src/io/Mouse.h"

#define SCREENWIDTH 800
#define SCREENHEIGHT 608


Game game(SCREENWIDTH, SCREENHEIGHT);



void keyChanged(GLFWwindow* window, int key, int scancode, int action, int mods) {

	switch (action)
	{
	case GLFW_RELEASE:
	//	std::cout << "Release code" << key << std::endl;
	//	game.sceneEvent();
		break;
	case GLFW_PRESS:

		if (key == GLFW_KEY_ENTER) {   // when we start typing 
			if (game.GetState() == GameState::ACTION) {
				game.PrepareChatInput();



			}
			else {  // when we typed and release  chat
				game.ReleaseChatInput();

		
			}
		}

			else if (key == GLFW_KEY_BACKSPACE && game.GetState() == GameState::TYPINGCHAT) {
				game.delete_Char();

			}
			
		

		break;
	case GLFW_REPEAT:
	//	std::cout << "Repeat code" << key << std::endl;
		
		break;

	};

	// chat
	
	// chat
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
	

		glfwSetWindowShouldClose(window, true);
	}



}
void MousePosChanged(GLFWwindow* window, double _x, double _y) {
	

	
}
void MouseWheelChanged(GLFWwindow* window, double dx, double dy) {

}
void MouseButtonChanged(GLFWwindow* window, int button, int action, int mods) {
	if (Mouse::buttonWentDown(GLFW_MOUSE_BUTTON_1)) {
	
	//	game.createProjectile();




	}
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height);



int main() {

	glfwInit();

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);



	// glfw window creation
	// --------------------
	GLFWwindow* window = glfwCreateWindow(SCREENWIDTH, SCREENHEIGHT, "Mine", NULL, NULL);
	if (window == NULL) {

		std::cout << "FAILED to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);   // glwfpoolevent() вызывает их 
	glfwSetKeyCallback(window, Game::keyCallback); // glwfpoolevent() вызывает их 
	glfwSetCharCallback(window, Game::character_callback); // glwfpoolevent() вызывает их 

	//cursor moved
	glfwSetCursorPosCallback(window, Mouse::cursorPosCallback);
	//mouse btn pressed
	glfwSetMouseButtonCallback(window, Mouse::mouseButtonCallback);
	//mouse scroll
	glfwSetScrollCallback(window, Mouse::mouseWheelCallback);


	glfwSwapInterval(1);

	// glad: load all OpenGL function pointers
	// ---------------------------------------
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {

		std::cout << "FAILED to initialize GLAD" << std::endl;
		return -1;

	}
	//   для текстур с альфа каналом
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//
	if (!game.Init())
	{
		glfwTerminate();
		ResourceManager::clear();
		return 0;
	}

	float timeValue = glfwGetTime(); // Получаем начальное время

	//const float AnimationTimer = 0.2f;
	float timer = 0.0f;

	Game::keyCallbacks.push_back(keyChanged);
	Mouse::mouseButtonCallBacks.push_back(MouseButtonChanged);  // определяем тело функции для вызова в ивенте
	Mouse::mouseWheelCallBacks.push_back(MouseWheelChanged);
	Mouse::cursorPosCallBacks.push_back(MousePosChanged);


	while (!glfwWindowShouldClose(window)) {
		float LastTime = timeValue;  // Сохраняем предыдущее время
		timeValue = glfwGetTime(); // Получаем текущее время
		float felapsedTime = timeValue - LastTime;
		timer += felapsedTime;



		game.Update(felapsedTime);


		glfwSwapBuffers(window);
		glfwPollEvents();

		glClearColor(0.2, 0.3, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);



		game.Render();




	}
	glfwTerminate();
	ResourceManager::clear();
	return 0;




	return 0;

}
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {

	glViewport(0, 0, width, height);


}
