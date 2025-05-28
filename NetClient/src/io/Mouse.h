#ifndef MOUSE_H
#define MOUSE_H

#include <glad/glad.h> // Для загрузки функций OpenGL
#include <GLFW/glfw3.h> // Библиотека для работы с окнами и ввода/вывода


#include <vector>

class Mouse {
public:

	static std::vector<void(*)(GLFWwindow* window, double _x, double _y)> cursorPosCallBacks;
	static std::vector<void(*)(GLFWwindow* window, int button, int action, int mods)> mouseButtonCallBacks;
	static std::vector<void(*)(GLFWwindow* window, double dx, double dy)> mouseWheelCallBacks;

	//cursor position changed
	static void cursorPosCallback(GLFWwindow* window, double _x, double _y);
	//mouse button state change
	static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

	//scroll wheel moved
	static void mouseWheelCallback(GLFWwindow* windowm, double dx, double dy);


	static double getMouseX();
	static double getMouseY();

	static double getDX();
	static double getDY();




	static double getScrollDX();
	static double getScrollDY();


	static bool button(int button);
	static bool buttonChanged(int button);
	static bool buttonWentUp(int button);
	static bool buttonWentDown(int button);


private:
	static double x;
	static double y;


	static double lastX;
	static double lastY;


	static double dx;
	static double dy;

	static double scrollDx;
	static double scrollDy;


	static double scrollDX;
	static double scrollDY;


	static bool firstMouse;

	static bool buttons[];
	static bool buttonsChanged[];


};

#endif