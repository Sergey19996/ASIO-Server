#include "ActionSlot.h"
#include "map"
#include "GLFW/glfw3.h"


enum class GameAction {
    MoveUp, MoveDown, MoveLeft, MoveRight,
    Slot1, Slot2, Slot3,
    Slot4, Slot5, Slot6, Slot7, Slot8, // Это клавиши 1, 2, 3, 4, 5
    Slot9, Slot10,Slot11,
    Count
};

struct KeyConfig {
    std::map<GameAction, int> bindings;

    KeyConfig() {
        bindings[GameAction::MoveUp] = GLFW_KEY_W; // 0
        bindings[GameAction::MoveDown] = GLFW_KEY_S; // 0
        bindings[GameAction::MoveLeft] = GLFW_KEY_A;
        bindings[GameAction::MoveRight] = GLFW_KEY_D;

        // Теперь используем только GameAction
        bindings[GameAction::Slot1] = GLFW_MOUSE_BUTTON_LEFT;
        bindings[GameAction::Slot2] = GLFW_MOUSE_BUTTON_RIGHT;
        bindings[GameAction::Slot3] = GLFW_MOUSE_BUTTON_MIDDLE;
        bindings[GameAction::Slot4] = GLFW_KEY_Q;
        bindings[GameAction::Slot5] = GLFW_KEY_E;
        bindings[GameAction::Slot6] = GLFW_KEY_R;
        bindings[GameAction::Slot7] = GLFW_KEY_T;
        bindings[GameAction::Slot8] = GLFW_KEY_F;
        bindings[GameAction::Slot9] = GLFW_KEY_1;
        bindings[GameAction::Slot10] = GLFW_KEY_2;
        bindings[GameAction::Slot11] = GLFW_KEY_3;
    }

    // Хелпер для конвертации действия в слот для сервера
    static ActionSlot ToActionSlot(GameAction action) {
        if (action >= GameAction::Slot1 && action <= GameAction::Slot11) {
            // Магия приведения типов: разница между индексами
            return static_cast<ActionSlot>((int)action - (int)GameAction::Slot1);
        }
        return ActionSlot::None;
    }
};