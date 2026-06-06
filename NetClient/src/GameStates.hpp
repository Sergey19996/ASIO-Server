#ifndef GAME_STATE_HPP
#define GAME_STATE_HPP

enum class GameState
{
	INGAME,
	TYPINGCHAT,

    // Новые UI состояния по сценарию
    REGISTRATION,     // Ввод имени игрока
    LOGIN,          // Процесс входа (аутентификация)
    MAIN_MENU,      // Главное меню (после входа)
    LOBBY,          // Основное лобби
    SHOP,           // Магазин
    FRIENDS,        // Управление друзьями
    MATCHMAKING,    // Поиск матча
    ROOM,           // Игровая комната (перед началом игры)
    PROFILE,        // Профиль игрока
    SETTINGS,        // Настройки


    VICTORY,
    DEFEAT,
    CRAFTING,  // для охотника колсо кравта 
    OPTIONS,
};


#endif // !GAME_STATE_HPP
