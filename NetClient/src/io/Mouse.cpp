#include "Mouse.h"

double Mouse::x = 0;              // Текущая координата X курсора
double Mouse::y = 0;              // Текущая координата Y курсора

double Mouse::lastX = 0;          // Предыдущее положение X
double Mouse::lastY = 0;          // Предыдущее положение Y

double Mouse::dx = 0;             // Разница между текущим и предыдущим положением X
double Mouse::dy = 0;             // Разница между текущим и предыдущим положением Y

double Mouse::scrollDX = 0;       // Движение колеса мыши по горизонтали
double Mouse::scrollDY = 0;       // Движение колеса мыши по вертикали

bool Mouse::firstMouse = true;    // Флаг, определяющий первый вызов для корректной инициализации

bool Mouse::buttons[GLFW_MOUSE_BUTTON_LAST] = { 0 };        // Состояние кнопок мыши (нажата/не нажата)
bool Mouse::buttonsChanged[GLFW_MOUSE_BUTTON_LAST] = { 0 }; // Состояние изменения кнопок (была ли нажата/отпущена)

std::vector<void(*)(GLFWwindow* window, double _x, double _y)> Mouse::cursorPosCallBacks;
std::vector<void(*)(GLFWwindow* window, int button, int action, int mods)>  Mouse::mouseButtonCallBacks;
std::vector<void(*)(GLFWwindow* window, double dx, double dy)>  Mouse::mouseWheelCallBacks;


void Mouse::cursorPosCallback(GLFWwindow* window, double _x, double _y)
{
	x = _x;    // Устанавливаем текущее положение X
	y = _y;    // Устанавливаем текущее положение Y

	if (firstMouse) {
		lastX = x;
		lastY = y;
		firstMouse = false; // После первого вызова больше не сбрасываем позицию
	}

	dx = x - lastX;    // Вычисляем движение по X
	dy = lastY - y;    // Вычисляем движение по Y (обратная ось Y)
	lastX = x;         // Обновляем последнее положение X
	lastY = y;         // Обновляем последнее положение Y

	for (void(*func)(GLFWwindow*, double _x, double _y) : Mouse::cursorPosCallBacks) {
		func(window, _x, _y);
	}


}

void Mouse::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
//Эта функция вызывается GLFW при нажатии/отпускании кнопок мыши.
{
	if (action != GLFW_RELEASE) {         // Если кнопка нажата или удерживается
		if (!buttons[button]) {           // Если кнопка была не нажата
			buttons[button] = true;       // Устанавливаем её состояние как нажато
		}
	}
	else {
		buttons[button] = false;          // Если кнопка отпущена
	}

	buttonsChanged[button] = action != GLFW_REPEAT;  // Обновляем состояние изменения кнопки (только не для повторов)


	for (void(*func)(GLFWwindow*, int button, int action, int mods) : Mouse::mouseButtonCallBacks) {
		func(window, button, action, mods);
	}
}

void Mouse::mouseWheelCallback(GLFWwindow* windowm, double dx, double dy)
{
	scrollDX = dx;   // Устанавливаем движение колеса по горизонтали
	scrollDY = dy;   // Устанавливаем движение колеса по вертикали


	for (void(*func)(GLFWwindow* windowm, double dx, double dy) : Mouse::mouseWheelCallBacks) {
		func(windowm, dx, dy);
	}

}

double Mouse::getMouseX()
{

	return x;
}

double Mouse::getMouseY()
{
	return y;
}

double Mouse::getDX()
{
	double _dx = dx;
	dx = 0;


	return _dx;
}

double Mouse::getDY()
{
	double _dy = dy;
	dy = 0;


	return _dy;
}

double Mouse::getScrollDX()
{
	double dx = scrollDX;
	scrollDX = 0;

	return dx;
}

double Mouse::getScrollDY()
{
	double dy = scrollDY;
	scrollDY = 0;

	return dy;
}

bool Mouse::button(int button)
{


	return buttons[button];
}

bool Mouse::buttonChanged(int button)
//buttonChanged — проверяет, изменилось ли состояние кнопки с момента последнего вызова.
{
	bool ret = buttonsChanged[button];
	buttonsChanged[button] = false;
	return ret;
}

bool Mouse::buttonWentUp(int button)
//buttonWentUp — проверяет, была ли кнопка отпущена.
{
	return !buttons[button] && buttonChanged(button); // Кнопка была отпущена
}

bool Mouse::buttonWentDown(int button)
//buttonWentDown — проверяет, была ли кнопка нажата.
{
	return buttons[button] && buttonChanged(button); // Кнопка была нажата
}
