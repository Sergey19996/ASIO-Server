#include "BaseUI.h"
#include "../Game.h"    // уже можешь подключить настоящее определение
#include "../io/Mouse.h"
#include "../ResourceManager.h"
#include "../NetShared/PlayerClass.h"
#include "ActionBar.h"
#include "../NetShared/entities/Mage.h"
#include "../NetShared/entities/Warrior.h"
#include "../NetShared/entities/Hunter.h"

#include "../NetShared/managers/ProgressionManager.h"
#include "../NetShared/managers/SquadManager.h"


LoginUI::LoginUI(Game* g) : game(g)
{



    // Кнопка "Войти"
    buttons.push_back({ "SUBMIT LOGIN", GameAction::Count, 200, 400, 200, 50, false, false, [this]() {
        game->TryLogin(login, password);
    } });

    // Кнопка переключения на регистрацию
    buttons.push_back({ "GO TO REGISTER",GameAction::Count, 450, 400, 200, 50, false, false, [this]() {
        game->OpenRegistration(); // Переключаем экран
    } });

    // Область для выбора поля Логин (примерные координаты центра экрана)
    buttons.push_back({ "",GameAction::Count, 300, 200, 400, 50, false, false, [this]() {
        typingPassword = false;
    } });

    // Область для выбора поля Пароль
    buttons.push_back({ "",GameAction::Count, 300, 260, 400, 50, false, false, [this]() {
        typingPassword = true;
    } });
}

void LoginUI::Render(TextRenderer* r, SpriteRenderer* sr, int levelWidth, int levelHeight)
{
    float centerX = levelWidth * 0.5f;
    float centerY = levelHeight * 0.5f;
    float scale = 0.7f;
    Shader& uiShader = ResourceManager::GetShader(ShaderID::UIGlyph);

    // Цвет активного поля (Синий), неактивного (Белый)
    glm::vec4 activeCol = { 0.2f, 0.4f, 0.8f, 1.0f }; // Приятный синий
    glm::vec4 inactiveCol = { 1.0f, 1.0f, 1.0f, 1.0f }; // Чистый белый
    glm::vec4 frameCol = { 0.3f, 0.3f, 0.3f, 0.5f };  // Цвет подложки

    // Рассчитываем ширину рамки для 16 символов
    float frameW = r->MeasureTextWidth("WWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW", scale) + 20.0f;
    float frameH = 50.0f;
    float startX = centerX - (frameW * 0.5f);

    // --- 1. ПОЛЕ ЛОГИНА (buttons[2]) ---
    buttons[2].x = startX; buttons[2].y = centerY - 100;
    buttons[2].w = frameW;  buttons[2].h = frameH;

    // Рисуем рамку логина
    sr->DrawSprite(uiShader, '#', { buttons[2].x, buttons[2].y }, { buttons[2].w, buttons[2].h }, 0.0f,
        !typingPassword ? activeCol * 0.4f : frameCol); // Подсвечиваем фон синим, если активно

    // Текст логина (Белый)
    std::string lText = "Login: " + login + (!typingPassword ? "|" : "");
    r->RenderText(lText, startX + 10, centerY - 90, scale, !typingPassword ? activeCol : white);


    // --- 2. ПОЛЕ ПАРОЛЯ (buttons[3]) ---
    buttons[3].x = startX; buttons[3].y = centerY - 30;
    buttons[3].w = frameW;  buttons[3].h = frameH;

    // Рисуем рамку пароля
    sr->DrawSprite(uiShader, '#', { buttons[3].x, buttons[3].y }, { buttons[3].w, buttons[3].h }, 0.0f,
        typingPassword ? activeCol * 0.4f : frameCol);

    // Текст пароля (Белый или Синий)
    std::string pStars(password.length(), '*');
    std::string pText = "Pass: " + pStars + (typingPassword ? "|" : "");
    r->RenderText(pText, startX + 10, centerY - 20, scale, typingPassword ? activeCol : white);


    // --- 3. КНОПКИ ДЕЙСТВИЙ (buttons[0] и buttons[1]) ---
    float bW = 250.0f;
    float spacing = 20.0f;
    float btnStartX = centerX - (bW * 2 + spacing) * 0.5f;

    for (size_t i = 0; i < 2; ++i) {
        auto& b = buttons[i];
        b.x = btnStartX + i * (bW + spacing);
        b.y = centerY + 60; b.w = bW; b.h = 50;

        // Рисуем кнопку (белый текст на темном фоне)
        sr->DrawSprite(uiShader, '#', { b.x, b.y }, { b.w, b.h }, 0.0f,
            b.hovered ? glm::vec4(0.4f, 0.4f, 0.4f, 1.0f) : glm::vec4(0.15f, 0.15f, 0.15f, 1.0f));

        float tw = r->MeasureTextWidth(b.text, 0.5f);
        r->RenderText(b.text, b.x + (b.w - tw) * 0.5f, b.y + (b.h * 0.3f), 0.5f, white);
    }
}

void LoginUI::OnChar(unsigned int c)
{

        std::string& target = typingPassword ? password : login;
    
        // Лимит: например, 16 символов
        if (GetUTF8Length(target) >= 16) return;
    
        // Логика UTF-8 (теперь всё строго в target)
        if (c < 0x80) {
            // Обычная латиница и цифры (1 байт)
            target.push_back((char)c);
        }
        else if (c < 0x800) {
            target.push_back((char)((c >> 6) | 0xC0)); // Старший байт (110xxxxx)
            target.push_back((char)((c & 0x3F) | 0x80)); // Младший байт(10xxxxxx)
        }
        else if (c < 0x10000) {
            target.push_back((char)((c >> 12) | 0xE0)); // 1110xxxx
            target.push_back((char)(((c >> 6) & 0x3F) | 0x80)); // 10xxxxxx
            target.push_back((char)((c & 0x3F) | 0x80)); // 10xxxxxx
        }
}

void LoginUI::OnKey(int key, int action)
{
        if (action != GLFW_PRESS) return;

    std::string& target = typingPassword ? password : login;

    if (key == GLFW_KEY_TAB) {
        typingPassword = !typingPassword; // Переключение по Tab
    }
    else if (key == GLFW_KEY_BACKSPACE) {
        if (!target.empty()) {
            while (!target.empty() && (target.back() & 0xC0) == 0x80) target.pop_back();
            if (!target.empty()) target.pop_back();
        }
    }
    else if (key == GLFW_KEY_ENTER) {
        game->TryLogin(login, password);
    }
    else if (key == GLFW_KEY_ESCAPE)
    {
        glfwSetWindowShouldClose(glfwGetCurrentContext(), true);
    }
    //// Быстрое переключение режима на F1/F2
    //if (key == GLFW_KEY_F1) mode = AuthMode::LOGIN;
    //if (key == GLFW_KEY_F2) mode = AuthMode::REGISTER;
}



LobbyUI::LobbyUI(Game* g) : game(g)
{
    float bw = 300;
    float bh = 60;
   

    // Координаты пока ставим 0, мы обновим их в Render
    buttons.push_back({ "Find Match",GameAction::Count, 0, 0, bw, bh, false,false, [this]() { game->FindMatch(); } });
    buttons.push_back({ "SHOP",GameAction::Count,       0, 0, bw, bh, false,false, [this]() { game->OpenShop(); } });
    buttons.push_back({ "FRIENDS",GameAction::Count,    0, 0, bw, bh, false,false, [this]() { game->OpenFriends(); } });
    buttons.push_back({ "OPTIONS",GameAction::Count,    0, 0, bw, bh, false,false, [this]() { game->OpenOptions(GameState::LOBBY); }});

}

void LobbyUI::Render(TextRenderer* r, SpriteRenderer* sr, int levelWidth, int levelHeight){

 
    float scale = 1.0f;
    float smallScale = 0.8f; // Для статов можно чуть поменьше масштаб
    Shader& uiShader = ResourceManager::GetShader(ShaderID::UIGlyph);



    // Рассчитываем начальную точку для блока кнопок (центр экрана)
    float menuWidth = 300; // bw
    float totalHeight = (60 * buttons.size()) + (20 * (buttons.size() - 1)); // кнопки + отступы

    float startX = (levelWidth - menuWidth) * 0.5f;
    float startY = (levelHeight - totalHeight) * 0.5f;

    // Отрисовка кнопок
    for (size_t i = 0; i < buttons.size(); ++i)
    {
        auto& b = buttons[i];
        // Обновляем координаты кнопки под текущее окно (важно для кликов!)
        b.x = startX;
        b.y = startY + i * (b.h + 20); // 20 - spacing

        glm::vec4 color = b.hovered ? glm::vec4(0.3f, 0.3f, 0.3f, 0.9f) : glm::vec4(0.1f, 0.1f, 0.1f, 0.7f);

        sr->DrawSprite(uiShader, '#', { b.x, b.y }, { b.w, b.h }, 0.0f, color);

        float tw = r->MeasureTextWidth(b.text, scale);
        r->RenderText(b.text, b.x + (b.w - tw) * 0.5f, b.y + (b.h * 0.25f), scale);
    }

 

    // ... (ваш код отрисовки кнопок без изменений) ...

    // --- БЛОК ПРОФИЛЯ (Верхний левый угол) ---
    float gx = 20.0f;
    float gy = 20.0f;
    float panelW = 280.0f;
    float panelH = 140.0f; // Увеличим высоту, чтобы влезли статы

    // Рисуем подложку для всего блока профиля
    sr->DrawSprite(uiShader, '#', { gx, gy }, { panelW, panelH }, 0.0f, glm::vec4(0.1f, 0.1f, 0.1f, 0.8f));

    // 1. Приветствие и Имя
    std::string greetings = "Player: " + game->playerProfile.name;
    r->RenderText(greetings, gx + 15, gy + 15, scale, glm::vec3(1.0f, 1.0f, 0.0f)); // Желтый цвет для имени

    // 2. Монеты (с иконкой или просто текстом)
    std::string coinsText = "Coins: " + std::to_string(game->playerProfile.coins);
    r->RenderText(coinsText, gx + 15, gy + 50, smallScale, glm::vec3(1.0f, 0.84f, 0.0f)); // Золотистый

    // 3. Победы и Поражения (в одну строку для компактности)
    std::string statsText = "W: " + std::to_string(game->playerProfile.wins) +
        " / L: " + std::to_string(game->playerProfile.losses);

    // Считаем винрейт (защита от деления на 0)
    float totalMatches = game->playerProfile.wins + game->playerProfile.losses;
    std::string winrate = (totalMatches > 0)
        ? " (" + std::to_string((int)((game->playerProfile.wins / totalMatches) * 100)) + "%)"
        : "";

    r->RenderText(statsText + winrate, gx + 15, gy + 85, smallScale, glm::vec3(0.8f, 0.8f, 0.8f));

    // 4. Статус (в сети/оффлайн - опционально)
    r->RenderText("Status: Online", gx + 15, gy + 115, 0.6f, glm::vec3(0.0f, 1.0f, 0.0f));

}

void LobbyUI::OnChar(unsigned int c)
{
}


void LobbyUI::OnKey(int key, int action)
{
   // if (action != GLFW_PRESS) return;

   
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        game->OpenOptions(GameState::LOBBY); // что бы запомнил откуда пришел
        
    }
}

bool UIButton::Contains(float px, float py) const{

    return px >= x && px <= x + w &&
        py >= y && py <= y + h;
}

FriendsUI::FriendsUI(Game* g) : game(g)
{
    float bw = 300;
    float bh = 60;
    float x = 25;
    float y = 200;

    UIButton play{ "Add Friend",GameAction::Count,    x, y,         bw, bh, false,false, [this]() { game->OpenLobby(); } };
    UIButton shop{ "Remove Friend",GameAction::Count,    x, y + 80,    bw, bh, false,false, [this]() { game->OpenLobby(); } };
    UIButton friends{ "Return",GameAction::Count, x, y + 160, bw, bh, false,false, [this]() { game->OpenLobby(); } };

    buttons.push_back(play);
    buttons.push_back(shop);
    buttons.push_back(friends);

}

void FriendsUI::Render(TextRenderer* r, SpriteRenderer* sr, int levelWidth, int levelHeight)
{
    float scale = 1.0f;
    Shader& glyphShader = ResourceManager::GetShader(ShaderID::UIGlyph);
    for (auto& b : buttons)
    {
        glm::vec4 color = b.hovered ? glm::vec4(0.2f, 0.2f, 0.2f, 0.8f) : glm::vec4(0, 0, 0, 0.5f);

        sr->DrawSprite(
            glyphShader,
            '#',
            { b.x, b.y },
            { b.w, b.h },
            0.0f,
            color);

        float tw = r->MeasureTextWidth(b.text, scale);
        float tx = b.x + (b.w - tw) * 0.5f;
        float ty = b.y + (b.h * 0.25f);

        r->RenderText(b.text, tx, ty, scale);
    }
    sr->DrawSprite(
        glyphShader,
        '#',
        { levelWidth / 2.0f, 100 },
        { levelWidth / 2.0f, levelHeight - 100.0f },
        0.0f,
        glm::vec4(0.2f, 0.2f, 0.2f, 0.8f));
}

void FriendsUI::OnChar(unsigned int c)
{
}


void FriendsUI::OnKey(int key, int action)
{
}

ShopUI::ShopUI(Game* g) : game(g)
{
    float bw = 300;
    float bh = 60;
    float x = 0;
    float y = 0;

    UIButton Return{ "Return",GameAction::Count,    x, y,         bw, bh, false, false,[this]() { game->OpenLobby(); } };
    buttons.push_back(Return);
}

void ShopUI::Render(TextRenderer* r, SpriteRenderer* sr, int levelWidth, int levelHeight)
{
    float scale = 1.0f;
    Shader& glyphShader = ResourceManager::GetShader(ShaderID::UIGlyph);
    for (auto& b : buttons)
    {
        glm::vec4 color = b.hovered ? glm::vec4(0.2f, 0.2f, 0.2f, 0.8f) : glm::vec4(0, 0, 0, 0.5f);

        sr->DrawSprite(
            glyphShader,
            '#',
            { b.x, b.y },
            { b.w, b.h },
            0.0f,
            color);

        float tw = r->MeasureTextWidth(b.text, scale);
        float tx = b.x + (b.w - tw) * 0.5f;
        float ty = b.y + (b.h * 0.25f);

        r->RenderText(b.text, tx, ty, scale);
    }
}

void ShopUI::OnChar(unsigned int c)
{
}


void ShopUI::OnKey(int key, int action)
{
}

RoomUI::RoomUI(Game* g) : game(g)
{
    float bw = 200;
    float bh = 100;
    float x = 25;
    float y = 200;

    UIButton Warrior{ "Warrior",GameAction::Count,    x, y,         bw, bh, false,false, [this]() {
       olc::net::message<GameMsg> msg;
    msg.header.id = GameMsg::Client_SelectClass;
    msg << PlayerClass::Warrior;
    game->Send(msg);
        } };
    UIButton Hunter{ "Hunter",GameAction::Count,    x +120, y ,    bw, bh, false,false, [this]() {
          olc::net::message<GameMsg> msg;
    msg.header.id = GameMsg::Client_SelectClass;
    msg << PlayerClass::Hunter;
    game->Send(msg);
        } };
    UIButton Mage{ "Mage",GameAction::Count, x + 240, y , bw, bh, false,false, [this]() {
           olc::net::message<GameMsg> msg;
    msg.header.id = GameMsg::Client_SelectClass;
    msg << PlayerClass::Mage;
    game->Send(msg);
        } };

    buttons.push_back(Warrior);
    buttons.push_back(Hunter);
    buttons.push_back(Mage);
}

void RoomUI::Render(TextRenderer* r, SpriteRenderer* sr, int levelWidth, int levelHeight)
{
   

    float scale = 1.0f;

    const int count = buttons.size();
    const float bw = buttons[0].w;
    const float bh = buttons[0].h;
    const float gap = 40.0f; // расстояние между кнопками

    float totalWidth = count * bw + (count - 1) * gap; //  totalWidth = 3 * bw + 2 * gap   / общая ширина группы - 3 кнопки на их ширину + 2 растояния между ними на пробел
    float startX = (levelWidth - totalWidth) * 0.5f; // startX = (screenW - totalWidth) / 2 левая граница группы - 
    float y = buttons[0].y;

    Shader& glyphShader = ResourceManager::GetShader(ShaderID::UIGlyph);
    for (int i = 0; i < count; ++i)
    {
        UIButton& b = buttons[i];

        float x = startX + i * (bw + gap);
        b.x = x;
        b.y = y;
        glm::vec4 color;
        if (b.selected) {
            // Цвет для ВЫБРАННОЙ кнопки (например, золотистый или ярко-синий)
            color = glm::vec4(0.1f, 0.6f, 0.1f, 1.0f);
        }
        else if (b.hovered) {
            // Цвет при наведении
            color = glm::vec4(0.3f, 0.3f, 0.3f, 0.8f);
        }
        else {
            // Обычный цвет
            color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        }

        sr->DrawSprite(glyphShader, '#', { b.x, b.y }, { bw, bh }, 0.0f, color);





        float tw = r->MeasureTextWidth(b.text, scale);
        float tx = x + (bw - tw) * 0.5f;
        float ty = y + (bh * 0.25f);

        r->RenderText(b.text, tx, ty, scale);
    }



    std::string greetings = "Choose Class " + game->playerProfile.name;
    float gtw = r->MeasureTextWidth(greetings, scale);
    float gw = 250 + gtw * 0.5f;
    float gh = 60;

    float gx = levelWidth / 2.0f - gw / 2.0f;
    float gy = 0.0f;


    sr->DrawSprite(
        glyphShader,
        '#',
        { gx,  gy },
        { gw, gh },
        0.0f,
        glm::vec4(0.2f, 0.2f, 0.2f, 0.8f));

    r->RenderText(greetings, gx + (gw - gtw) * 0.5f, gy + gh * 0.25f, scale);




}

void RoomUI::OnChar(unsigned int c)
{
}



void RoomUI::OnKey(int key, int action)
{
}


MatchmakingUI::MatchmakingUI(Game* g) : game(g)
{
    float bw = 200;
    float bh = 100;
    float x = 25;
    float y = 200;

    UIButton Cancel{ "Cancel",GameAction::Count,x, y, bw, bh, false,false, [this]() {
        game->CancelMatchMaking();
        game->OpenLobby(); } };
    buttons.push_back(Cancel);
}

void MatchmakingUI::Render(TextRenderer* r, SpriteRenderer* sr, int levelWidth, int levelHeight)
{
    float scale = 1.0f;

   

    const int count = buttons.size();
    const float bw = buttons[0].w;
    const float bh = buttons[0].h;
    const float gap = 40.0f; // расстояние между кнопками

    float totalWidth = count * bw; //  totalWidth = 3 * bw + 2 * gap   / общая ширина группы - 3 кнопки на их ширину + 2 растояния между ними на пробел
    float startX = (levelWidth - totalWidth) * 0.5f; // startX = (screenW - totalWidth) / 2 левая граница группы - 
    float y = (levelHeight - bh + buttons[0].y) * 0.5f;



    Shader& glyphShader = ResourceManager::GetShader(ShaderID::UIGlyph);
    for (auto& b : buttons)
    {

        float x = startX;
       
        b.x = x;
        b.y = y;
        glm::vec4 color = b.hovered ? glm::vec4(0.2f, 0.2f, 0.2f, 0.8f) : glm::vec4(0, 0, 0, 0.5f);

        sr->DrawSprite(
            glyphShader,
            '#',
            { b.x, b.y },
            { b.w, b.h },
            0.0f,
            color);

        float tw = r->MeasureTextWidth(b.text, scale);
        float tx = b.x + (b.w - tw) * 0.5f;
        float ty = b.y + (b.h * 0.25f);

        r->RenderText(b.text, tx, ty, scale);
    }

    std::string status =
        std::to_string(game->matchmakingCount) + " / 3 players found";

    float tw = r->MeasureTextWidth(title, scale);
    r->RenderText(title, (levelWidth - tw) * 0.5f, levelHeight * 0.4f, scale);

    float sw = r->MeasureTextWidth(status, scale);
    r->RenderText(status, (levelWidth - sw) * 0.5f, levelHeight * 0.5f, scale);
  
}

void MatchmakingUI::OnChar(unsigned int c)
{
}


void MatchmakingUI::OnKey(int key, int action)
{
}

VictoryUi::VictoryUi(Game* g) : game(g)
{
    float bw = 300;
    float bh = 60;
    float x = 100.1f;
    float y = 100.1f;

    UIButton Return{ "Return", GameAction::Count,   x, y,         bw, bh, false,false, [this]() {
          olc::net::message<GameMsg> msg;
        msg.header.id = GameMsg::Client_ReturnToLobby;
        game->Send(msg);

        } };
    buttons.push_back(Return);
}

void VictoryUi::Render(TextRenderer* r, SpriteRenderer* sr, int levelWidth, int levelHeight)
{

   //std::cout << screenW << "  " << screenH << std::endl;
    float scale = 1.0f;
    const float margin = 20.0f; // отступ от края экрана
    Shader& glyphShader = ResourceManager::GetShader(ShaderID::UIGlyph);
  
    for (auto& b : buttons)
    {
        // ───── позиция кнопки ─────
        float bx = levelWidth - b.w - margin;
        float by = levelHeight - b.h - margin;

        b.x = bx;
        b.y = by;

        glm::vec4 color = b.hovered
            ? glm::vec4(0.2f, 0.2f, 0.2f, 0.8f)
            : glm::vec4(0, 0, 0, 0.5f);

        sr->DrawSprite(
            glyphShader,
            '#',
            { bx, by },
            { b.w, b.h },
            0.0f,
            color);

        // ───── текст по центру кнопки ─────
        float tw = r->MeasureTextWidth(b.text, scale);
        float tx = bx + (b.w - tw) * 0.5f;
        float ty = by + (b.h * 0.25f);

        r->RenderText(b.text, tx, ty, scale);
    }

    // ───── заголовок Defeat по центру сверху ─────
    std::string greetings = "Winner";
    float gtw = r->MeasureTextWidth(greetings, scale);
    float gw = 250 + gtw * 0.5f;
    float gh = 60;

    float gx = (levelWidth - gw) * 0.5f;
    float gy = 20.0f;

    sr->DrawSprite(glyphShader,
        '#',
        { gx, gy },
        { gw, gh },
        0.0f,
        glm::vec4(0.2f, 0.2f, 0.2f, 0.8f));

    r->RenderText(
        greetings,
        gx + (gw - gtw) * 0.5f,
        gy + gh * 0.25f,
        scale
    );
}

void VictoryUi::OnChar(unsigned int c)
{
}


void VictoryUi::OnKey(int key, int action)
{
}

DefeatUi::DefeatUi(Game* g) : game(g)
{
    float bw = 300;
    float bh = 60;
    float x = 100.1f;
    float y = 100.1f;

    UIButton Return{ "Return", GameAction::Count,   x, y,         bw, bh, false,false, [this]() {
         olc::net::message<GameMsg> msg;
        msg.header.id = GameMsg::Client_ReturnToLobby;
        game->Send(msg);
        } };
    buttons.push_back(Return);
}

void DefeatUi::Render(TextRenderer* r, SpriteRenderer* sr, int levelWidth, int levelHeight)
{
    float scale = 1.0f;
    const float margin = 20.0f; // отступ от края экрана
    Shader& glyphShader = ResourceManager::GetShader(ShaderID::UIGlyph);
    for (auto& b : buttons)
    {
        // ───── позиция кнопки ─────
        float bx = levelWidth - b.w - margin;
        float by = levelHeight - b.h - margin;

        b.x = bx;
        b.y = by;

        glm::vec4 color = b.hovered
            ? glm::vec4(0.2f, 0.2f, 0.2f, 0.8f)
            : glm::vec4(0, 0, 0, 0.5f);

        sr->DrawSprite(
            glyphShader,
            '#',
            { bx, by },
            { b.w, b.h },
            0.0f,
            color);

        // ───── текст по центру кнопки ─────
        float tw = r->MeasureTextWidth(b.text, scale);
        float tx = bx + (b.w - tw) * 0.5f;
        float ty = by + (b.h * 0.25f);

        r->RenderText(b.text, tx, ty, scale);
    }

    // ───── заголовок Defeat по центру сверху ─────
    std::string greetings = "Defeat";
    float gtw = r->MeasureTextWidth(greetings, scale);
    float gw = 250 + gtw * 0.5f;
    float gh = 60;

    float gx = (levelWidth - gw) * 0.5f;
    float gy = 20.0f;

    sr->DrawSprite(
        glyphShader,
        '#',
        { gx, gy },
        { gw, gh },
        0.0f,
        glm::vec4(0.2f, 0.2f, 0.2f, 0.8f));

    r->RenderText(
        greetings,
        gx + (gw - gtw) * 0.5f,
        gy + gh * 0.25f,
        scale
    );
}

void DefeatUi::OnChar(unsigned int c)
{
}


void DefeatUi::OnKey(int key, int action)
{
}

InGameUI::InGameUI(Game* g) : game(g)
{
    // Настраиваем кнопку-индикатор
    networkBtn.w = 20.0f;
    networkBtn.h = 20.0f;
    networkBtn.text = ""; // Текст не нужен, рисуем квадратик
    minimapShader = &ResourceManager::GetShader(ShaderID::Minimap);
    actionBar = std::make_unique<ActionBar>(g);
       
}

void InGameUI::Render(TextRenderer* r, SpriteRenderer* sr, int levelWidth, int levelHeight)
{
    if (!game) return;
    Shader& glyphShader = ResourceManager::GetShader(ShaderID::UIGlyph);

    // --- 1. Отрисовка статистики (FPS/Ping) ---
    sr->DrawSprite(glyphShader, '#', glm::vec2(statX, statY + 10.0f), glm::vec2(50.0f, 25.0f), 0.0f, glm::vec4(0, 0, 0, 0.8f));

    r->RenderText("FPS: " + std::to_string((int)game->currentFPS), statTextX, statY + 15.0f, 0.45f, glm::vec3(1.0f));

    glm::vec3 pColor = (game->currentPing > 200.0f) ? glm::vec3(1, 0.5f, 0) : (game->currentPing > 100.0f ? glm::vec3(1, 1, 0) : glm::vec3(0, 1, 0));
    r->RenderText("Ping: " + std::to_string((int)game->currentPing) + "ms", statTextX, statY + 37.0f, 0.45f, pColor);

    r->RenderText("In: " + std::to_string(game->currentInboundKBps) + " KB/s", statTextX, statY + 59.0f, 0.45f, glm::vec3(1.0f));

    // --- 2. Экшн-бар ---
    actionBar->Render(r, sr, levelWidth, levelHeight);

    // --- 3. Классовый UI ---
    auto it = game->playerEntities.find(game->nPlayerID);
    if (it != game->playerEntities.end()) {
        // player — это std::unique_ptr<Character>
        const auto& player = it->second;

        switch (player->entityType)
        {
        case ArchetypeId::Player_Warrior: {
            // Кастуем к Warrior, чтобы достать уровень адаптации и интенсивность света
            auto* w = static_cast<Warrior*>(player.get());
            float light = w->GetAdaptationProgress();
            int adaptLvl = (int)w->GetAdaptationLevel();

            DrawWarriorAdaptation(sr, classCenterX, warriorY, 40.0f, light, adaptLvl);
            r->RenderText("ADAPTATION Lvl: " + std::to_string(adaptLvl),
                classCenterX - 40, warriorY + 45, 0.35f, glm::vec3(1.0f));
            break;
        }

        case ArchetypeId::Player_Mage: {
            auto* m = static_cast<Mage*>(player.get());
            // balance в Mage колеблется от -3.0 (Ice) до +3.0 (Fire)
            // DrawMageIndicator ожидает нормализованные данные или конкретные значения
            DrawMageIndicator(sr, classCenterX, mageHunterY, 120.0f, 12.0f, (m->GetRawBalance() + 3) /6, player->chargeClient);

            r->RenderText("FIRE", classCenterX - 130, mageHunterY, 0.3f, glm::vec3(1.0f, 0.4f, 0.0f));
            r->RenderText("ICE", classCenterX + 105, mageHunterY, 0.3f, glm::vec3(0.5f, 0.8f, 1.0f));
            break;
        }

        case ArchetypeId::Player_Hunter: {
            auto* h = static_cast<Hunter*>(player.get());
            // У Hunter данные в колчане (quiver) и infoPoints
            // Передаем infoPoints и коэффициент заряда
            DrawHunterIndicator(sr, classCenterX, mageHunterY, 120.0f, h, player->chargeClient);

            // Дополнительно можно отрисовать иконки стрел из h->quiver
            break;
        }

        default:
            break;
        }
       
       /* if (player->playerClass == PlayerClass::Warrior) {
            DrawWarriorAdaptation(sr, classCenterX, warriorY, 40.0f, player.fSpecialBar, (int)player.nClassParam);
            r->RenderText("ADAPTATION Lvl: " + std::to_string(player.nClassParam), classCenterX - 40, warriorY + 45, 0.35f, glm::vec3(1.0f));
        }
        else if (player.nAvatarID == (int)PlayerClass::Mage) {
          
        }
        else if (player.nAvatarID == (int)PlayerClass::Hunter) {
         
        }*/

        // --- 4. Опыт (XP Bar) ---
        auto* prog = player->GetProgression();
        if (prog) {
            float barWidth = levelWidth * 0.25f; // Полоска на 25% ширины экрана
            float barHeight = 12.0f;
            float barX = levelWidth / 2.0f;
            float barY = 20.0f; // 20 пикселей от верхнего края

            DrawExperienceBar(sr, r, barX, barY, barWidth, barHeight,
                prog->GetXP(), prog->GetRequiredXP(), prog->GetLevel());
        }
        // --- 5. Слоты отряда (под XP Bar) ---
        auto* squad = player->GetSquad();
        if (squad && prog) {
            float slotsY = 20.0f + 12.0f + 10.0f; // Под полоской опыта (Y полоски + высота + отступ)
            int maxAllowed = 1 + (prog->GetLevel() / 2); // Ваша формула лимита
            if (maxAllowed > 8) maxAllowed = 8;

            DrawSquadSlots(sr, levelWidth / 2.0f, slotsY, (int)squad->GetSquadSize(), maxAllowed);
        }

        RenderMinimap(sr, &game->GetFog(), player->position, levelWidth, levelHeight);


      
    }

    DrawWindIndicator(sr, r, levelWidth, levelHeight);
    DrawDayCycleClock(sr, 100, 100, 200);

  
  
}

void InGameUI::Update(float dt, int mouseX, int mouseY)
{
  

    // Твоя стандартная проверка
    networkBtn.hovered = networkBtn.Contains((float)mouseX, (float)mouseY);

    // Обновляем позиции слотов (если экран изменился) и проверяем Hover
  //  actionBar->RefreshLayout(screenW, screenH);
    actionBar->Update(dt, mouseX, mouseY);
}

void InGameUI::DrawDayCycleClock(SpriteRenderer* sr, const float& x, const float& y, const float& size)
{
    Shader& glyphShader = ResourceManager::GetShader(ShaderID::UIGlyph);
   
  

    float progress = game->dayCycleProgress; // 0.0 - 1.0

    // 1. Фон круга (теперь с разделением в шейдере)
    glyphShader.use();
    glyphShader.setInt("symbol", 0);
    glyphShader.setVec2("quadSize", glm::vec2(size, size));
    glyphShader.setBool("u_useHorizon", true); // Здесь горизонт НУЖЕН
    // Передаем "дневной" цвет, шейдер сам смешает его с ночью по горизонту
    glm::vec4 daySky = glm::vec4(0.3f, 0.3f, 0.6f, 0.8f);
    sr->DrawSprite(glyphShader, 0, glm::vec2(x, y), glm::vec2(size),1.57f, daySky);

    // 2. Светила (Инвертированные позиции)
    
    glm::vec2 center = glm::vec2(x + size * 0.5f, y + size * 0.5f);
    float r = size * 0.38f;
    float angle = progress * 2.0f * 3.14159f ;

    // Солнце (Инвертировано для соответствия теням)
    glm::vec2 sunPos = center + glm::vec2(cosf(angle), sinf(angle)) * r;
    sr->DrawSprite(glyphShader,'#', sunPos - glm::vec2(size * 0.1f), glm::vec2(size * 0.2f), 0.0f, glm::vec4(1, 0.9, 0.2, 1));

    // Луна (Напротив солнца)
    glm::vec2 moonPos = center - glm::vec2(cosf(angle), sinf(angle)) * r;
    sr->DrawSprite(glyphShader, '#', moonPos - glm::vec2(size * 0.07f), glm::vec2(size * 0.14f), 0.0f, glm::vec4(0.8, 0.8, 1, 0.8));

    // 3. Центральный индикатор (используем спрайтовый шейдер, чтобы избежать логики горизонта)
    

    float intensity = game->lightIntensity; // 0.0 (ночь) ... 1.0 (день)

    // Цвета: Золото (День) и Холодный Синий (Ночь)
    glm::vec3 dayColor = glm::vec3(1.0f, 0.9f, 0.1f);
    glm::vec3 nightColor = glm::vec3(0.4f, 0.6f, 1.0f);

    // Плавно смешиваем цвета в зависимости от времени суток
    glm::vec4 activeColor = glm::vec4(glm::mix(nightColor, dayColor, intensity), 1.0f);

    float indicatorSize = size * 0.18f; // Чуть увеличим для наглядности
    // Центрируем относительно общего круга
    glm::vec2 indicatorPos = center - glm::vec2(indicatorSize * 0.5f);

    // Рисуем индикатор
    sr->DrawSprite(glyphShader, 0, indicatorPos, glm::vec2(indicatorSize), 1.57f, activeColor);

    // Добавляем логику зарубок адаптации
    auto it = game->playerEntities.find(game->nPlayerID);
    if (it != game->playerEntities.end() && it->second->entityType == ArchetypeId::Player_Warrior)
    {
        glyphShader.use();
        glyphShader.setBool("u_useHorizon", false); // Для зарубок горизонт не нужен

        int maxLevels = 5;
        float clockRadius = size * 0.5f; // Почти по краю круга
        

        for (int i = 1; i <= maxLevels; ++i) {
            // Рассчитываем порог (0.2, 0.4, 0.6...)
            float threshold = (float)i / (float)maxLevels;

            // Находим угол на циферблате, соответствующий этой интенсивности
            // В твоей системе солнце ходит по кругу, так что привяжемся к высоте
            // (Это визуальные "отсечки" эффективности)
            float notchAngle = glm::radians(180.0f * (1.0f - threshold));

            // Рисуем симметричные зарубки слева и справа (восход и закат)
            for (float side : {-1.0f, 1.0f}) {
                float finalAngle =  (notchAngle * side)+3.14f; // 1.57 - это верхняя точка
                glm::vec2 notchPos = center + glm::vec2(cosf(finalAngle), sinf(finalAngle)) * clockRadius;

                // Цвет: если текущий свет выше этой отметки — зарубка "горит"
                glm::vec4 notchColor = (game->lightIntensity >= threshold)
                    ? glm::vec4(1.0f, 0.9f, 0.0f, 0.9f)  // Золотая
                    : glm::vec4(0.3f, 0.3f, 0.3f, 0.5f); // Потухшая

                glyphShader.setVec2("quadSize", glm::vec2(size * 0.1f)); // Маленький штрих
                glyphShader.setInt("symbol", 0);
                sr->DrawSprite(glyphShader, 0, notchPos - glm::vec2(size * 0.05f), glm::vec2(size*0.1f), finalAngle, notchColor);
            }
        }
    }
}

void InGameUI::DrawWindIndicator(SpriteRenderer* sr, TextRenderer* r, const float& levelWidth, const float& levelHeight)
{
    Shader& glyphShader = ResourceManager::GetShader(ShaderID::UIGlyph);
   

    // --- Конфигурация ---
    float uiScale = 1.5f;
    float scaledSize = 64.0f * uiScale; // Используем базовый размер (SKILL_SIZE)

    // Данные из игры
    float windAngle = game->currentWindAngle;
    float windDegrees = glm::degrees(windAngle);
    int windPower = game->currentWindForce;

    // Позиционирование (смещаем влево от края или привязываем к интерфейсу)
    // baseX можно передать или вычислить здесь. Допустим, привязка к правому нижнему углу:
    float windX = levelWidth - scaledSize - 150.0f;
    float windY = levelHeight - scaledSize - 60.0f;

    glm::vec2 compassCenter = glm::vec2(windX + scaledSize * 0.5f, windY + scaledSize * 0.5f);

    // 1. Фоновая подложка (Круглая, так как используем glyphShader с symbol 0)
    glyphShader.use();
    glyphShader.setInt("symbol", 0);
    glyphShader.setVec2("quadSize", glm::vec2(scaledSize));
    glyphShader.setBool("u_useHorizon", false); // Здесь 
    sr->DrawSprite(glyphShader, 0,
        glm::vec2(windX, windY),
        glm::vec2(scaledSize, scaledSize),
        0.0f, glm::vec4(0.05f, 0.05f, 0.05f, 0.7f));


    // 3. Наконечник (Яркий шарик на острие)
    float tipOffset = scaledSize * 0.35f;
    glm::vec2 tipPos = glm::vec2(
        compassCenter.x + cosf(windAngle) * tipOffset,
        compassCenter.y + sinf(windAngle) * tipOffset
    );

    float tipSize = 14.0f;
    // Снова переключаемся на круг (если текстура white - квадрат, используем glyph 0)
    glyphShader.use();
    glyphShader.setInt("symbol", 0);
    sr->DrawSprite(glyphShader, 0,
        tipPos - glm::vec2(tipSize * 0.5f),
        glm::vec2(tipSize, tipSize),
        0.0f, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

    // 4. Индикаторы силы (Шарики под компасом)
    float dotSize = 18.0f;
    float dotPadding = 8.0f;
    float totalDotsWidth = 3 * dotSize + 2 * dotPadding;
    float startDotsX = windX + (scaledSize - totalDotsWidth) * 0.5f;
    float dotsY = windY + scaledSize + 12.0f;

    glyphShader.setVec2("quadSize", glm::vec2(dotSize, dotSize));
    glyphShader.setBool("u_useHorizon", false);
    for (int i = 1; i <= 3; ++i)
    {
       
       
        float dx = startDotsX + (i - 1) * (dotSize + dotPadding);
        glm::vec4 dotColor = (windPower >= i)
            ? glm::vec4(0.0f, 1.0f, 0.6f, 1.0f)
            : glm::vec4(0.2f, 0.2f, 0.2f, 0.5f);

        sr->DrawSprite(glyphShader, 0,
            glm::vec2(dx, dotsY),
            glm::vec2(dotSize, dotSize),
            0.0f, dotColor);
    }
   
    // 5. Текст
    r->RenderText("WIND", windX + (scaledSize * 0.1f), windY - 30.0f, 0.5f, glm::vec3(1.0f));

    
}

void InGameUI::RenderMinimap(SpriteRenderer* sr,Fog* fog, const glm::vec2& playerPos, int levelWidth, int levelHeight)
{
   
    // 2. Настраиваем вьюпорт (правый верхний угол, квадрат 200x200)
    float mapSize = game->Height * 0.17f;  // размер всегда 25 процентов от ширины экрана 
    float padding = game->Width * 0.01f;
  
    
    
    // 1. Получаем данные из Fog
    auto fogData = fog->GetMinimapData();

  
    glViewport(padding, padding, mapSize, mapSize);

    // 3. Подготовка шейдера
    minimapShader->use();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fogData.memoryTex);
    minimapShader->setInt("memoryTex", 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, fogData.currentFogTex);
    minimapShader->setInt("currentFogTex", 1);

    // 4. Передача позиций (нормализуем относительно размеров уровня)
    // Предположим, размер уровня в тайлах 128x128 или берем из fog
    glm::vec2 lvlSize = game->GetLevelAmountCells();

    minimapShader->setVec2("playerPos", playerPos / lvlSize);
    // 2. Вычисляем полный размер мира в пикселях (например, 1600x1600)
    glm::vec2 worldSizeInPixels = lvlSize * 32.0f; // 32.0f это ваш BRICK_SIZE
    // Передаем маяки
    auto& beacons = fog->GetBeacons();
    int bCount = 0;
    for (auto& b : beacons) {
        if (bCount >= 8) break;
        std::string arrIdx = "beacons[" + std::to_string(bCount) + "]";
        minimapShader->setVec2(arrIdx.c_str(), b.pos / worldSizeInPixels);
        bCount++;
    }
    minimapShader->setInt("beaconCount", bCount);

    // Передаем врагов
    int eCount = 0;
    // nPlayerID - это твой локальный ID
    for (auto const& [id, visual] : game->mapVisuals) {
        if (id == game->nPlayerID) continue; // Пропускаем себя
        if (eCount >= 16) break;

        std::string name = "enemies[" + std::to_string(eCount) + "]";
        // Делим на lvlSize, так как шейдер ждет 0..1
        minimapShader->setVec2(name.c_str(), visual.vCurrentPos / lvlSize);
        eCount++;
    }
    minimapShader->setInt("enemyCount", eCount);
    // 5. Отрисовка
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Рисуем квад (вертексник ожидает 0..1)
    fog->DrawFullscreenQuad();

    // 6. Возвращаем вьюпорт обратно для остального UI
    glViewport(0, 0, game->Width, game->Height);
}

void InGameUI::DrawMageIndicator(SpriteRenderer* sr, float x, float y, float width, float height, float resourceValue, float chargeRatio)
{
    Shader& shader = ResourceManager::GetShader(ShaderID::UIGlyph);
    shader.use();
    shader.setBool("u_useHorizon", false);

    // Центр и размеры
    float centerX = x;
    float halfWidth = width / 2.0f;
    float step = width / 6.0f; // Каждая единица баланса (от -3 до +3)
    // Баланс: 0.0 (Лед) <--- 0.5 (Центр) ---> 1.0 (Огонь)
    float fillProgress = (0.5f - resourceValue ) * width; // -0.5; 0; + 0.5;
    float balance = (resourceValue - 0.5f) * 6.0f;

    // 1. Фон шкалы
    sr->DrawSprite(shader, '#', glm::vec2(centerX - halfWidth, y), glm::vec2(width, height), 0.0f, glm::vec4(0.1f, 0.1f, 0.1f, 0.9f));

    // 2. Отрисовка Баланса (Огонь или Лед)
    glm::vec4 fireColor = glm::vec4(1.0f, 0.3f, 0.0f, 1.0f);
    glm::vec4 iceColor = glm::vec4(0.0f, 0.6f, 1.0f, 1.0f);
    glm::vec4 currentColor = (resourceValue > 0.5f) ? fireColor : iceColor;

    float rectWidth = std::abs(fillProgress);
    float rectX = (fillProgress > 0) ? centerX : centerX + fillProgress;

    sr->DrawSprite(shader, '#', glm::vec2(rectX, y + 1), glm::vec2(rectWidth, height - 2), 0.0f, currentColor);

    // 3. МАРКЕРЫ УРОВНЕЙ (ЗАСЕЧКИ 1, 2, 3)
    for (int i = 1; i <= 3; ++i) {
        // Отрисовка симметрично для огня (+) и льда (-)
        float offsets[] = { (float)i * step, -(float)i * step };

        for (float off : offsets) {
            float markerX = centerX + off;
            bool isReached = (off > 0) ? (balance >= i) : (balance <= -i);

            // Если порог достигнут, маркер "горит" ярче
            glm::vec4 markerColor = isReached ? glm::vec4(1, 1, 1, 1) : glm::vec4(1, 1, 1, 0.3f);
            float markerThickness = (i == 3) ? 3.0f : 1.5f; // Ульта шире
            float markerHeight = (i == 3) ? height + 6 : height + 2; // Ульта выше

            sr->DrawSprite(shader, '#', glm::vec2(markerX - markerThickness / 2, y - (markerHeight - height) / 2),
                glm::vec2(markerThickness, markerHeight), 0.0f, markerColor);
        }
    }
    // 3. НАЛОЖЕНИЕ ЗАРЯДА (PROGRESS OVERLAY)
    if (chargeRatio > 0.0f) {
        // Мы рисуем полоску заряда ПОВЕРХ текущего заполнения баланса
        // Цвет заряда: ярко-белый/желтый для огня, ярко-белый/циановый для льда
        glm::vec4 chargeColor = (resourceValue > 0.5f) ? glm::vec4(1.0f, 1.0f, 0.6f, 0.8f) : glm::vec4(0.6f, 1.0f, 1.0f, 0.8f);

        // Полоска заряда растет от центра (или от края баланса)
        // Сделаем, чтобы она "съедала" текущий баланс, показывая прогресс
        float chargeFillWidth = rectWidth * chargeRatio;
        float chargeX = (fillProgress > 0) ? centerX : centerX - chargeFillWidth;

        // Пульсация для эффекта магии
        chargeColor.a *= (0.6f + 0.4f * std::sin(glfwGetTime() * 15.0f));

        sr->DrawSprite(shader, '#', glm::vec2(chargeX, y - 2), glm::vec2(chargeFillWidth, height + 4), 0.0f, chargeColor);

        // Если заряд близок к максимуму, добавляем "искру" на кончике
        if (chargeRatio > 0.95f) {
            float flash = 0.5f + 0.5f * std::sin(glfwGetTime() * 40.0f);
            float tipX = (fillProgress > 0) ? centerX + chargeFillWidth : centerX - chargeFillWidth;
            sr->DrawSprite(shader, '#', glm::vec2(tipX - 2, y - 4), glm::vec2(4, height + 8), 0.0f, glm::vec4(1, 1, 1, flash));
        }
    }

    // 4. Разделитель центра (белый штрих)
    sr->DrawSprite(shader, '#', glm::vec2(centerX - 1, y - 2), glm::vec2(2, height + 4), 0.0f, glm::vec4(1.0f));
}

void InGameUI::DrawHunterIndicator(SpriteRenderer* sr, float x, float y, float width, Hunter* h, float chargeRatio)
{
    Shader& shader = ResourceManager::GetShader(ShaderID::UIGlyph);
    shader.use();

    float arrowWidth = 12.0f;
    float arrowHeight = 35.0f;
    float padding = 6.0f;

   
    int pendingPower = (int)std::ceil(chargeRatio * 5.0f);

    for (int i = 0; i < 5; i++) {
        // БЕРЕМ ГОТОВЫЕ ДАННЫЕ ИЗ ОБЪЕКТА
        const auto& arrow = h->quiver[i];

        bool inQuiver = arrow.active;
        bool isExplosive = arrow.hasExplosion;
        bool isBinding = arrow.hasBinding;
        bool isGhost = arrow.hasGhost;
        int  bonusPower = arrow.bonusPower;

        // Те же флаги из объекта
        bool isCharging = h->isCharging;

        // Логика: если не в колчане и нет эффектов - это ПУСТОЙ слот
        bool hasAnyEffect = isExplosive || isBinding || isGhost;
        bool isFlying = !inQuiver && hasAnyEffect;

        if (!inQuiver && !isFlying) {
            // Рисуем пустой слот и идем к следующей стреле
            glm::vec2 pos = glm::vec2(x + i * (arrowWidth + padding), y);
            sr->DrawSprite(shader, '#', pos, { arrowWidth, arrowHeight }, 0.0f, glm::vec4(0.1f, 0.1f, 0.1f, 0.4f));
            continue;
        }

      
       


        // --- ОПРЕДЕЛЕНИЕ ЦВЕТА (Смешивание) ---
        glm::vec4 color = glm::vec4(0.43f, 0.32f, 0.26f, 1.0f); // Базовая стрела (Коричневый)

        // 1. Все три эффекта (Ультимативная стрела)
        if (isExplosive && isBinding && isGhost) {
            color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f); // Чисто белый (Сияющий) или золотистый
        }
        // 2. Двойные комбинации
        else if (isExplosive && isBinding) {
            color = glm::vec4(1.0f, 0.0f, 1.0f, 1.0f); // Ярко-розовый / Маджента
        }
        else if (isBinding && isGhost) {
            color = glm::vec4(0.0f, 1.0f, 0.8f, 1.0f); // Бирюзовый / Аквамарин
        }
        else if (isGhost && isExplosive) {
            color = glm::vec4(1.0f, 0.9f, 0.0f, 1.0f); // Ярко-желтый / Лимонный
        }
        // 3. Одиночные эффекты
        else if (isExplosive) {
            color = glm::vec4(1.0f, 0.3f, 0.0f, 1.0f); // Оранжевый (Взрыв)
        }
        else if (isBinding) {
            color = glm::vec4(0.0f, 0.5f, 1.0f, 1.0f); // Глубокий синий (Путы)
        }
        else if (isGhost) {
            color = glm::vec4(0.1f, 1.0f, 0.2f, 1.0f); // Кислотно-зеленый (Призрак)
        }



        // Эффект "Накала" для заряжаемых стрел
        if (isCharging) {
            float pulse = 0.5f + 0.5f * std::sin(glfwGetTime() * 20.0f);
            color = glm::mix(color, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), pulse * 0.7f);
        }

        glm::vec2 pos = glm::vec2(x + i * (arrowWidth + padding), y);

        // --- РЕНДЕРИНГ СПРАЙТОВ ---
        if (inQuiver) {
            glm::vec2 renderPos = pos;

            // Проверяем, является ли текущая стрела i-й по счету в очереди на выстрел
            // Если pendingPower (рассчитанный из chargeRatio) охватывает этот индекс
            if (isCharging && i < pendingPower) {
                // Тряска усиливается для более "заряженных" стрел
                float shakeIntensity = 0.5f + (float)i * 0.3f;
                renderPos.x += std::sin(glfwGetTime() * 70.0f) * shakeIntensity;
                renderPos.y += std::cos(glfwGetTime() * 70.0f) * shakeIntensity;

                // Добавим накал индивидуально
                float pulse = 0.5f + 0.5f * std::sin(glfwGetTime() * 25.0f);
                color = glm::mix(color, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), pulse * 0.5f);
            }

            // Отрисовка стрелы с модифицированной позицией
            sr->DrawSprite(shader, '#', renderPos, { arrowWidth, arrowHeight }, 0.0f, color);
        
           
            // Наконечник (чуть выше тела)
            sr->DrawSprite(shader, '#', renderPos + glm::vec2(-2, -8), glm::vec2(arrowWidth + 4, 10), 0.0f, color);

            // Если стрела "Супер-заряжена" (power >= 3), добавим искру сверху
            if (isCharging && pendingPower >= 3) {
                float flash = 0.5f + 0.5f * std::sin(glfwGetTime() * 30.0f);
                sr->DrawSprite(shader, '#', renderPos + glm::vec2(2, -12), glm::vec2(8, 8), 45.0f, glm::vec4(1, 1, 1, flash));
            }
            // --- ОТРИСОВКА МОЩНОСТИ (ВНУТРИ СТРЕЛЫ) ---
            if (bonusPower > 0) {
                float segmentHeight = (arrowHeight - 4) / 5.0f;
                for (int p = 0; p < bonusPower; p++) {
                    glm::vec4 pwrColor = glm::vec4(1.0f, 1.0f, 1.0f, 0.8f);
                    if (bonusPower >= 4) pwrColor = glm::vec4(0.5f, 0.0f, 1.0f, 1.0f); // Тот самый фиолетовый

                    // ВАЖНО: Используй renderPos, чтобы сегменты тряслись вместе со стрелой
                    sr->DrawSprite(shader, '#',
                        renderPos + glm::vec2(1, (arrowHeight - 2) - (p + 1) * segmentHeight),
                        glm::vec2(arrowWidth - 2, segmentHeight - 1), 0.0f, pwrColor);
                }
            }
        }
        else if (isFlying) {
            // Стрела уже в мире, ждет активации (пульсирующий полупрозрачный силуэт)
            float pulse = 0.6f + 0.4f * std::sin(glfwGetTime() * 12.0f);
            color.a = 0.5f * pulse;
            sr->DrawSprite(shader, '#', pos, glm::vec2(arrowWidth, arrowHeight), 0.0f, color);
        }
        else {
            // Пустой слот (просто контур или тусклый блок)
            sr->DrawSprite(shader, '#', pos, glm::vec2(arrowWidth, arrowHeight), 0.0f, color);
        }
    }

    // --- 3. ШКАЛА ИНФОРМАЦИИ (ПОД СТРЕЛАМИ) ---
    float totalWidth = 5 * (arrowWidth + padding);
    float infoY = y + arrowHeight + 14.0f;
    // infoProgress — это 0.0 - 1.0 (на 6 делений)


    for (int i = 0; i < 6; i++) {
        glm::vec4 infoColor = (i < h->infoPoints)
            ? glm::vec4(0.0f, 1.0f, 0.7f, 1.0f)   // Активное очко
            : glm::vec4(0.1f, 0.1f, 0.1f, 0.6f);  // Пустое очко

        sr->DrawSprite(shader, '#', glm::vec2(x + i * (totalWidth / 6.0f), infoY),
            glm::vec2((totalWidth / 6.0f) - 2, 5.0f), 0.0f, infoColor);
    }
}


void InGameUI::DrawWarriorAdaptation(SpriteRenderer* sr, float x, float y, float radius, float fillProgress, int level)
{
    Shader& shader = ResourceManager::GetShader(ShaderID::UIGlyph);
    shader.use();
    shader.setInt("symbol", 0);
    shader.setBool("u_useHorizon", false);
    shader.setVec2("quadSize", glm::vec2(radius * 2 * fillProgress, 4));
    // 1. Отрисовка основного прогресс-кольца (fSpecialBar)
    // Для простоты используем 4 сегмента или шейдер. Если шейдера круга нет, 
    // рисуем центральную точку или простое кольцо:
    sr->DrawSprite(shader,'!', glm::vec2(x - radius, y - radius), glm::vec2(radius * 2 * fillProgress, 4), 0.0f, glm::vec4(0.3f, 0.7f, 1.0f, 0.8f));

    // 2. Отрисовка 5 кружочков уровней по окружности
    int maxLevels = 5;
    float dotRadius = 8.0f;
    for (int i = 0; i < maxLevels; ++i) {
        // Вычисляем угол для каждой точки (смещение на -90 градусов, чтобы начать сверху)
        float angle = glm::radians((360.0f / maxLevels) * i - 90.0f);
        glm::vec2 dotPos = glm::vec2(
            x + cos(angle) * radius,
            y + sin(angle) * radius
        );

        // Цвет: если уровень достигнут — яркий, если нет — пустой/темный
        glm::vec4 color = (i < level)
            ? glm::vec4(1.0f, 0.9f, 0.0f, 1.0f)  // Золотой (активен)
            : glm::vec4(0.2f, 0.2f, 0.2f, 0.5f); // Серый (пустой)
        shader.setVec2("quadSize", glm::vec2(dotRadius));
        sr->DrawSprite(shader,0, dotPos - glm::vec2(dotRadius / 2), glm::vec2(dotRadius), 0.0f, color);
    }
}

void InGameUI::RefreshLayout(int levelWidth, int levelHeight)
{
    // 1. Скилл-бар
    if (actionBar) {
        actionBar->RefreshLayout(levelWidth, levelHeight);
    }

    // 2. Статистика (FPS/Ping) - верхний правый угол
    float mx = levelWidth - 100.0f;
    statY = 25.0f;
    statX = mx - 50.0f - 10.0f; // mx - boxW - offset
    statTextX = mx - 55.0f;

    // 3. Классовые индикаторы (привязка к низу)
    // Используем ту же логику baseY, что и в скилл-баре
    float skillBarY = levelHeight - 55.0f - 35.0f;
    classCenterX = levelWidth * 0.5f;
    warriorY = skillBarY - 140.0f;
    mageHunterY = skillBarY - 60.0f;

    // 4. Сетевая кнопка
    networkBtn.x = levelWidth - 30.0f;
    networkBtn.y = 10.0f;



}

void InGameUI::DrawExperienceBar(SpriteRenderer* sr, TextRenderer* r, float x, float y, float width, float height, float currentXP, float requiredXP, int level)
{
    Shader& uiShader = ResourceManager::GetShader(ShaderID::UIGlyph);
    float progress = (requiredXP > 0) ? (currentXP / requiredXP) : 0.0f;

    // Считаем левый край всей полоски, чтобы 'x' был центром
    float startX = x - (width / 2.0f);

    // 1. Фон полоски (теперь центрирован)
    sr->DrawSprite(uiShader, '#', glm::vec2(startX, y), glm::vec2(width, height), 0.0f, glm::vec4(0, 0, 0, 0.6f));

    // 2. Заполнение (растет слева направо от startX)
    glm::vec4 xpColor = glm::vec4(0.6f, 0.2f, 0.8f, 0.9f);
    sr->DrawSprite(uiShader, '#', glm::vec2(startX, y),
        glm::vec2(width * progress, height), 0.0f, xpColor);

    // 3. Текст (ровно по центру x)
    std::string xpText = "LVL " + std::to_string(level) + "  " + std::to_string((int)currentXP) + "/" + std::to_string((int)requiredXP);
    float tWidth = r->MeasureTextWidth(xpText, 0.35f);

    r->RenderText(xpText, x - (tWidth / 2.0f), y - 5.0f, 0.35f, glm::vec3(1.0f));
}

void InGameUI::DrawSquadSlots(SpriteRenderer* sr, float x, float y, int currentCount, int maxAllowed)
{
    Shader& uiShader = ResourceManager::GetShader(ShaderID::UIGlyph);
    float slotSize = 16.0f;
    float padding = 4.0f;

    // Вычисляем общую ширину для центрирования
    float totalWidth = (maxAllowed * slotSize) + ((maxAllowed - 1) * padding);
    float startX = x - (totalWidth / 2.0f);

    for (int i = 0; i < maxAllowed; ++i) {
        glm::vec2 pos = glm::vec2(startX + i * (slotSize + padding), y);

        // 1. Фон слота (рамка)
        sr->DrawSprite(uiShader, '#', pos, glm::vec2(slotSize), 0.0f, glm::vec4(0.1f, 0.1f, 0.1f, 0.7f));

        if (i < currentCount) {
            // 2. Занятый слот (пока просто закрашиваем бирюзовым)
            // В будущем здесь можно рисовать иконку класса: 'W', 'M', 'R'
            sr->DrawSprite(uiShader, '#', pos + 2.0f, glm::vec2(slotSize - 4.0f), 0.0f, glm::vec4(0.2f, 0.9f, 0.8f, 1.0f));
        }
        else {
            // 3. Свободный слот (тонкая рамка)
            sr->DrawSprite(uiShader, '#', pos + (slotSize * 0.4f), glm::vec2(slotSize * 0.2f), 0.0f, glm::vec4(1, 1, 1, 0.2f));
        }
    }
}

void BaseUI::Render(TextRenderer* r, SpriteRenderer* sr, int levelWidth, int levelHeight)
{
    for (auto& btn : buttons) {
        glm::vec3 color = glm::vec3(0.6f); // Обычный серый

        if (btn.selected)
            color = glm::vec3(1.0f, 1.0f, 0.0f); // Желтый (активная вкладка)
        else if (btn.hovered)
            color = glm::vec3(1.0f, 1.0f, 1.0f); // Белый (наведение)

        // Отрисовка текста кнопки
        r->RenderText(btn.text, btn.x, btn.y, 0.5f, color);
    }
}


    void BaseUI::Update(float dt, int mouseX, int mouseY) {
        for (auto& b : buttons)
            b.hovered = b.Contains((float)mouseX, (float)mouseY);
    }

    void BaseUI::OnMouseButton(int button, int action, int mouseX, int mouseY) {
        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
            for (auto& b : buttons) {
                if (b.hovered && b.onClick) {
                    // Снимаем выделение со всех и ставим текущей
                    for (auto& bts : buttons) bts.selected = false;
                    b.selected = true;

                    b.onClick(); // Выполняем действие
                    break;
                }
            }
        }
    }


void BaseUI::OnActivate()
{
    for (auto& b : buttons)
    {
        b.selected = false;
        b.hovered = false;
    }
}

void BaseUI::DrawBackground(SpriteRenderer* sr, int w, int h, glm::vec4 color)
{
    // Рисуем полноэкранный прямоугольник (используем символ '#' как текстуру-заливку)
    sr->DrawSprite(ResourceManager::GetShader(ShaderID::UIGlyph), '#',
        glm::vec2(0, 0), glm::vec2(w, h), 0.0f, color);
}


size_t BaseUI::GetUTF8Length(const std::string& str)
{
    size_t length = 0;
    for (size_t i = 0; i < str.length(); i++) {
        // В UTF-8 все байты продолжения начинаются с 10xxxxxx (0x80)
        // Мы считаем только стартовые байты символов
        if ((str[i] & 0xC0) != 0x80) length++;
    }
    return length;
}

CraftArrowUI::CraftArrowUI(Game* g) : game(g)
{
    // Заполняем сектора (3 части круга)
    sectors.push_back({ ArrowType::Binding,  glm::vec4(0, 0.8, 1, 1), "BIND", 0.0f, 2.09f });
    sectors.push_back({ ArrowType::Ghost,  glm::vec4(0.7, 0, 1, 1), "GHOST", 2.09f, 4.18f });
    sectors.push_back({ ArrowType::Explosive, glm::vec4(1, 0.3, 0, 1), "BOOM", 4.18f, 6.28f });
    UiWidth =0;
    UiHeight = 0;

}

void CraftArrowUI::Update(float dt, int mouseX, int mouseY)
{
    // Логика наведения на сектор круга
    glm::vec2 center = { UiWidth, UiHeight  };
    glm::vec2 m = { mouseX, mouseY };
    float dist = glm::distance(m, center);
    hoveredIndex = -1; // Сбрасываем каждый кадр
    if (dist > 30.0f && dist < 150.0f) { // Радиус кольца
        float angle = atan2f(mouseY - center.y, mouseX - center.x);
        if (angle < 0) angle += 6.28f;// Перевод в диапазон 0 -> 2*PI
        // 1. ЛОГИКА НАВЕДЕНИЯ
        for (int i = 0; i < (int)sectors.size(); i++) {
            if (angle >= sectors[i].startAngle && angle < sectors[i].endAngle) {
                hoveredIndex = i; // Сохраняем индекс для Рендера

                // 2. ЛОГИКА КЛИКА
                if (Mouse::buttonWentDown(GLFW_MOUSE_BUTTON_LEFT)) {
                    game->CraftArrow(sectors[hoveredIndex].type);
                    // Проверяем ресурсы локального игрока перед отправкой
                 /*   if (game->GetLocalPlayer().fSpecialBar >= 0.5f) {
                        SendCraftRequest(sectors[i].type);
                    }*/
                }
                break;
            }
        }
    }

    // Закрытие на Tab или повторный клик вне круга
    //if (Keyboard::keyWentDown(GLFW_KEY_TAB)) {
    //    // Вернуть GameState::INGAME через callback или ссылку на game
    //}
}

void CraftArrowUI::Render(TextRenderer* r, SpriteRenderer* sr, int levelWidth, int levelHeight)
{
    // 1. Получаем шейдер и настраиваем центр
    Shader& uiShader = ResourceManager::GetShader(ShaderID::UIGlyph);
    uiShader.use();
    UiWidth = levelWidth / 2.0f;
    UiHeight = levelHeight / 2.0f;
    glm::vec2 center = glm::vec2(UiWidth, UiHeight);
    float outerRadius = 150.0f;
    float innerRadius = 40.0f;
   
    // 2. Затемняем задний фон всей игры
    sr->DrawSprite(uiShader, '#', glm::vec2(0), glm::vec2(levelWidth, levelHeight), 0.0f, glm::vec4(0, 0, 0, 0.6f));

    // Находим текущий наведенный сектор (из Update)
    // Предположим, у нас есть переменная int hoveredIndex в классе

    for (int i = 0; i < (int)sectors.size(); i++) {
        auto& s = sectors[i];
        bool isHovered = (hoveredIndex == i);

        // Вычисляем позицию иконки/текста в центре сектора
        float midAngle = (s.startAngle + s.endAngle) / 2.0f;
        glm::vec2 offset = glm::vec2(cosf(midAngle), sinf(midAngle)) * (outerRadius * 0.7f);
        glm::vec2 iconPos = center + offset;

        // 3. Рисуем подложку сектора (визуально как дугу или блок)
        glm::vec4 bgColor = isHovered ? s.color * 1.2f : s.color * 0.5f;
        bgColor.a = 0.8f;

        // Для простоты рисуем прямоугольник-плашку в месте сектора
        sr->DrawSprite(uiShader, '#', iconPos - glm::vec2(40, 20), glm::vec2(80, 40), 0.0f, bgColor);

        // 4. Отрисовка Текста и Иконки
        glm::vec4 textColor = isHovered ? glm::vec4(1.0f) : glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);

        // Название стрелы
        r->RenderText(s.label, iconPos.x - 25, iconPos.y - 5, 0.45f, textColor);

        // Если выбрано — рисуем рамку
        if (isHovered) {
            sr->DrawSprite(uiShader, '#', iconPos - glm::vec2(50, 25), glm::vec2(10, 50), 0.0f, textColor);
            sr->DrawSprite(uiShader, '#', iconPos + glm::vec2(40, -25), glm::vec2(10, 50), 0.0f, textColor);
        }
    }

    // 5. Центральный элемент (Информация о ресурсах)
    // Рисуем круг в центре
    sr->DrawSprite(uiShader, 0, center - glm::vec2(innerRadius), glm::vec2(innerRadius * 2), 0.0f, glm::vec4(0.1, 0.1, 0.1, 1.0));

    // Показываем цену
    r->RenderText("COST: 3", center.x - 25, center.y - 5, 0.4f, glm::vec4(0.0f, 1.0f, 0.7f, 1.0f));
}

void CraftArrowUI::OnKey(int key, int action)
{
    //if (key == GLFW_KEY_TAB && action == GLFW_PRESS) {
    //    // Вызываем метод Game для возврата в игру
    //    // Предполагаем, что у тебя есть метод SetState или доступ к переключению
    //    game->SetState(GameState::INGAME);

    //    //// Обязательно скрываем курсор обратно, если он включался для меню
    //    //glfwSetInputMode(glfwGetCurrentContext(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    //}
}

RegisterUI::RegisterUI(Game* g) : game(g) {
    // Кнопка "Создать аккаунт"
    buttons.push_back({ "CREATE ACCOUNT",GameAction::Count, 200, 400, 200, 50, false, false, [this]() {
        game->TryRegister(login, password);
    } });

    // Кнопка "Назад"
    buttons.push_back({ "BACK TO LOGIN",GameAction::Count, 450, 400, 200, 50, false, false, [this]() {
        game->OpenLogin();
    } });


    //// Кнопка "Войти"
    //buttons.push_back({ "SUBMIT LOGIN", 200, 400, 200, 50, false, false, [this]() {
    //    game->TryLogin(login, password);
    //} });

    //// Кнопка переключения на регистрацию
    //buttons.push_back({ "GO TO REGISTER", 450, 400, 200, 50, false, false, [this]() {
    //    game->OpenRegistration(); // Переключаем экран
    //} });

    // Область для выбора поля Логин (примерные координаты центра экрана)
    buttons.push_back({ "",GameAction::Count, 300, 200, 400, 50, false, false, [this]() {
        typingPassword = false;
    } });

    // Область для выбора поля Пароль
    buttons.push_back({ "",GameAction::Count, 300, 260, 400, 50, false, false, [this]() {
        typingPassword = true;
    } });

}

void RegisterUI::Render(TextRenderer* r, SpriteRenderer* sr, int levelWidth, int levelHeight)
{

    float centerX = levelWidth * 0.5f;
    float centerY = levelHeight * 0.5f;
    float scale = 0.7f;
    Shader& uiShader = ResourceManager::GetShader(ShaderID::UIGlyph);

    // Цвет активного поля (Синий), неактивного (Белый)
    glm::vec4 activeCol = { 0.2f, 0.4f, 0.8f, 1.0f }; // Приятный синий
    glm::vec4 inactiveCol = { 1.0f, 1.0f, 1.0f, 1.0f }; // Чистый белый
    glm::vec4 frameCol = { 0.3f, 0.3f, 0.3f, 0.5f };  // Цвет подложки

    // Рассчитываем ширину рамки для 16 символов
    float frameW = r->MeasureTextWidth("WWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW", scale) + 20.0f;
    float frameH = 50.0f;
    float startX = centerX - (frameW * 0.5f);

    // --- 1. ПОЛЕ ЛОГИНА (buttons[2]) ---
    buttons[2].x = startX; buttons[2].y = centerY - 100;
    buttons[2].w = frameW;  buttons[2].h = frameH;

    // Рисуем рамку логина
    sr->DrawSprite(uiShader, '#', { buttons[2].x, buttons[2].y }, { buttons[2].w, buttons[2].h }, 0.0f,
        !typingPassword ? activeCol * 0.4f : frameCol); // Подсвечиваем фон синим, если активно

    // Текст логина (Белый)
    std::string lText = "Login: " + login + (!typingPassword ? "|" : "");
    r->RenderText(lText, startX + 10, centerY - 90, scale, !typingPassword ? activeCol : white);


    // --- 2. ПОЛЕ ПАРОЛЯ (buttons[3]) ---
    buttons[3].x = startX; buttons[3].y = centerY - 30;
    buttons[3].w = frameW;  buttons[3].h = frameH;

    // Рисуем рамку пароля
    sr->DrawSprite(uiShader, '#', { buttons[3].x, buttons[3].y }, { buttons[3].w, buttons[3].h }, 0.0f,
        typingPassword ? activeCol * 0.4f : frameCol);

    // Текст пароля (Белый или Синий)
    std::string pStars(password.length(), '*');
    std::string pText = "Pass: " + pStars + (typingPassword ? "|" : "");
    r->RenderText(pText, startX + 10, centerY - 20, scale, typingPassword ? activeCol : white);


    // --- 3. КНОПКИ ДЕЙСТВИЙ (buttons[0] и buttons[1]) ---
    float bW = 250.0f;
    float spacing = 20.0f;
    float btnStartX = centerX - (bW * 2 + spacing) * 0.5f;

    for (size_t i = 0; i < 2; ++i) {
        auto& b = buttons[i];
        b.x = btnStartX + i * (bW + spacing);
        b.y = centerY + 60; b.w = bW; b.h = 50;

        // Рисуем кнопку (белый текст на темном фоне)
        sr->DrawSprite(uiShader, '#', { b.x, b.y }, { b.w, b.h }, 0.0f,
            b.hovered ? glm::vec4(0.4f, 0.4f, 0.4f, 1.0f) : glm::vec4(0.15f, 0.15f, 0.15f, 1.0f));

        float tw = r->MeasureTextWidth(b.text, 0.5f);
        r->RenderText(b.text, b.x + (b.w - tw) * 0.5f, b.y + (b.h * 0.3f), 0.5f, white);
    }
}

void RegisterUI::OnChar(unsigned int c)
{

    std::string& target = typingPassword ? password : login;

    // Лимит: например, 16 символов
    if (GetUTF8Length(target) >= 16) return;

    // Логика UTF-8 (теперь всё строго в target)
    if (c < 0x80) {
        // Обычная латиница и цифры (1 байт)
        target.push_back((char)c);
    }
    else if (c < 0x800) {
        target.push_back((char)((c >> 6) | 0xC0)); // Старший байт (110xxxxx)
        target.push_back((char)((c & 0x3F) | 0x80)); // Младший байт(10xxxxxx)
    }
    else if (c < 0x10000) {
        target.push_back((char)((c >> 12) | 0xE0)); // 1110xxxx
        target.push_back((char)(((c >> 6) & 0x3F) | 0x80)); // 10xxxxxx
        target.push_back((char)((c & 0x3F) | 0x80)); // 10xxxxxx
    }
}

void RegisterUI::OnKey(int key, int action)
{
    if (action != GLFW_PRESS) return;

    std::string& target = typingPassword ? password : login;

    if (key == GLFW_KEY_TAB) {
        typingPassword = !typingPassword; // Переключение по Tab
    }
    else if (key == GLFW_KEY_BACKSPACE) {
        if (!target.empty()) {
            while (!target.empty() && (target.back() & 0xC0) == 0x80) target.pop_back();
            if (!target.empty()) target.pop_back();
        }
    }
    else if (key == GLFW_KEY_ENTER) {
        game->TryRegister(login, password);
    }
}

KeyBindUI::KeyBindUI(Game* g) : game(g)
{
    InitButtons();
}

void KeyBindUI::InitButtons()
{
     buttons.clear();
    float startY = 50.0f;
    float offsetY = 45.0f;

    // Список соответствия меток и действий
    std::vector<std::pair<std::string, GameAction>> schema = {
        {"Move Up",    GameAction::MoveUp},    {"Move Down",  GameAction::MoveDown},
        {"Move Left",  GameAction::MoveLeft},  {"Move Right", GameAction::MoveRight},
        {"Mouse Left",    GameAction::Slot1},     {"Mouse Right",    GameAction::Slot2},
        {"Mouse Middle",    GameAction::Slot3},     {"Skill 4",    GameAction::Slot4},
        {"Skill 1",    GameAction::Slot5},     {"Skill 2",    GameAction::Slot6},
        {"Skill 3",    GameAction::Slot7},     {"Skill 4",    GameAction::Slot8},
        {"Skill 5",    GameAction::Slot9}, {"Skill 6",    GameAction::Slot10}, 
        {"Skill 7",    GameAction::Slot11},
    };

    for (size_t i = 0; i < schema.size(); ++i) {
        AddBindingButton(schema[i].first, schema[i].second, 100, startY + i * offsetY);
    }
}

void KeyBindUI::AddBindingButton(std::string label, GameAction action, float x, float y)
{
    UIButton btn;
    btn.text = label;
    btn.action = action;
    // Сдвигаем X на 250 вправо, где будет рисоваться название клавиши
    btn.x = x + 250;
    btn.y = y;
    btn.w = 100; // Ширина области клика под название клавиши
    btn.h = 35;
    btn.onClick = [this, action]() { this->waitingForKey = action; };
    buttons.push_back(btn);
}

void KeyBindUI::OnMouseButton(int button, int action, int mouseX, int mouseY)
{
   
    // Если ждем кнопку для бинда — любой клик (LMB, RMB и т.д.) записывается
    if (waitingForKey != GameAction::Count && action == GLFW_PRESS) {
        game->config.bindings[waitingForKey] = button;
        waitingForKey = GameAction::Count;
        return; // Важно: не пускаем событие дальше в базовый класс
    }

    // Если не ждем бинда — вызываем логику нажатия кнопок интерфейса
    BaseUI::OnMouseButton(button, action, mouseX, mouseY);
}

void KeyBindUI::OnKey(int key, int Action)
{
    if (Action == GLFW_PRESS && waitingForKey != GameAction::Count) {
        // Сохраняем новый бинд
        game->config.bindings[waitingForKey] = key;
        waitingForKey = GameAction::Count; // Сбрасываем режим
        return;
    }

   
}

void KeyBindUI::Render(TextRenderer* r, SpriteRenderer* sr, int levelWidth, int levelHeight)
{ // Золотистый цвет (R:255, G:215, B:0)
    glm::vec4 gold = glm::vec4(1.0f, 0.84f, 0.0f, 0.8f);
    glm::vec4 frameBg = glm::vec4(0.05f, 0.05f, 0.05f, 0.95f); // Еще темнее для контраста

    for (auto& btn : buttons) {
        bool isWaiting = (waitingForKey == btn.action);
        std::string keyName = isWaiting ? "???" : GetKeyName(game->config.bindings[btn.action]);

        // 1. Настройка текста
        float textScale = 0.55f; // Крупный шрифт
        float textWidth = r->MeasureTextWidth(keyName, textScale);

        // 2. РАСЧЕТ ЦЕНТРА (Горизонталь и Вертикаль)
        // Центр X: середина кнопки минус половина ширины текста
        float textX = btn.x + (btn.w / 2.0f) - (textWidth / 2.0f);
        // Центр Y: середина кнопки минус примерно половина высоты шрифта (обычно 24-30 пикселей)
        float textY = btn.y + (btn.h / 2.0f) - (12.0f * textScale);

        // 3. Рисуем название действия СЛЕВА
        r->RenderText(btn.text, btn.x - 250.0f, btn.y + 5, 0.45f, glm::vec3(0.8f));

        // 4. Рисуем ЗОЛОТУЮ РАМКУ (вокруг области бинда)
        // Внешний контур (чуть шире, если текст не влезает)
        float currentW = std::max(btn.w, textWidth + 20.0f);

        sr->DrawSprite(ResourceManager::GetShader(ShaderID::UIGlyph), '#',
            glm::vec2(btn.x - 5, btn.y - 2), glm::vec2(currentW + 10, btn.h + 4), 0.0f, gold);

        // Внутренняя заливка
        sr->DrawSprite(ResourceManager::GetShader(ShaderID::UIGlyph), '#',
            glm::vec2(btn.x - 3, btn.y), glm::vec2(currentW + 6, btn.h), 0.0f, frameBg);

        // 5. Рисуем сам бинд (центр вычислен выше)
        glm::vec3 keyColor = isWaiting ? glm::vec3(1, 0, 0) : (btn.hovered ? glm::vec3(1, 1, 0) : glm::vec3(0, 1, 0));
        r->RenderText(keyName, textX, textY, textScale, keyColor);
    }

    r->RenderText("Press ESC to exit", 100, levelHeight - 50, 0.4f, glm::vec3(0.5f));
}

std::string KeyBindUI::GetKeyName(int key)
{
    if (key >= GLFW_MOUSE_BUTTON_1 && key <= GLFW_MOUSE_BUTTON_LAST) {
        if (key == 0) return "LMB";
        if (key == 1) return "RMB";
        if (key == 2) return "MMB";
        return "Mouse " + std::to_string(key);
    }
    const char* name = glfwGetKeyName(key, 0);
    if (name) return std::string(name);

    // Обработка спец. клавиш, которые glfwGetKeyName возвращает как null
    if (key == GLFW_KEY_SPACE) return "SPACE";
    if (key == GLFW_KEY_LEFT_SHIFT) return "LSHIFT";
    return "Key " + std::to_string(key);
}

OptionsUI::OptionsUI(Game* g) :game(g)
{
    keybinds = new KeyBindUI(g);
    InitMainButtons(); // Кнопки переключения вкладок
    returnState = GameState::LOBBY;
}

OptionsUI::~OptionsUI()
{
    delete keybinds;
    keybinds = nullptr;
}

void OptionsUI::AddTabButton(std::string label, OptionsTab tab, float x, float y)
{
    UIButton btn;
    btn.text = label;
    btn.x = x; btn.y = y; btn.w = 200; btn.h = 50;

    // Лямбда-функция: при клике меняем вкладку у менеджера
    btn.onClick = [this, tab]() {
        this->currentTab = tab;  
        this->UpdateTabSelection(); // Метод для переключения флагов selected
        };

    buttons.push_back(btn);
}

void OptionsUI::UpdateTabSelection()
{
    // В векторе buttons первые кнопки — это наши вкладки
   // Проходим по ним и ставим selected только той, что совпадает с currentTab
   // (Для этого можно либо сравнивать текст, либо добавить индекс)
    for (size_t i = 0; i < buttons.size(); ++i) {
        // Допустим, порядок кнопок совпадает с порядком в enum OptionsTab
        buttons[i].selected = (static_cast<OptionsTab>(i) == currentTab);
    }
}

void OptionsUI::OnMouseButton(int button, int action, int mx, int my)
{
  

    // 2. Если открыта вкладка биндов — отдаем управление ей
    if (currentTab == OptionsTab::Keybinds) {
        // Запоминаем, ждали ли мы кнопку до вызова
        bool wasWaiting = (keybinds->waitingForKey != GameAction::Count);
        keybinds->OnMouseButton(button, action, mx, my);
        // Если мы были в режиме ожидания, значит этот клик ушел в бинд.
        // Блокируем дальнейшую обработку, чтобы не переключить вкладку случайно.
        if (wasWaiting) return;
    }
    else if ((currentTab == OptionsTab::General)|| (currentTab == OptionsTab::Video))
    {
        // 1. Сначала проверяем нажатия на сами вкладки (BaseUI)
        BaseUI::OnMouseButton(button, action, mx, my);
    }
    else if ((currentTab == OptionsTab::Exit)) {
        glfwSetWindowShouldClose(glfwGetCurrentContext(), true);
    }

    // 3. Обработка других вкладок (ползунки громкости, чекбоксы графики)
}

void OptionsUI::Update(float dt, int mx, int my)
{
    // Обновляем ховер для кнопок-вкладок (General, Video, Keys)
    BaseUI::Update(dt, mx, my);

    // ОБЯЗАТЕЛЬНО: Обновляем ховер для кнопок ВНУТРИ активной вкладки
    if (currentTab == OptionsTab::Keybinds && keybinds) {
        keybinds->Update(dt, mx, my);
    }
}

void OptionsUI::OnKey(int key, int Action)
{


    // Обновляем ховер для кнопок-вкладок (General, Video, Keys)
  //  BaseUI::Update(dt, mx, my);
    BaseUI::OnKey(key, Action);
    // ОБЯЗАТЕЛЬНО: Обновляем ховер для кнопок ВНУТРИ активной вкладки
    if (currentTab == OptionsTab::Keybinds && keybinds) {

        if (key == GLFW_KEY_ESCAPE) {
            // Выход в меню
            currentTab = OptionsTab::General;
            return;
        }

        keybinds->OnKey(key, Action);
        return;
    }



    if (key == GLFW_KEY_ESCAPE && Action == GLFW_PRESS) {
        // Возвращаемся именно туда, откуда пришли
        game->SetState(this->returnState);
        return;
    }
}

void OptionsUI::Render(TextRenderer* r, SpriteRenderer* sr, int w, int h)
{
    glm::vec2 backSize = { w / 4.0f,h / 4.0f };
    // 2. Центрируем: (ШиринаЭкрана - ШиринаМеню) / 2
    glm::vec2 backPos = { (w - backSize.x) / 2.0f, (h - backSize.y) / 2.0f };
    float offset = 5.0f;
    float gap = 10.0f;
    glm::vec2 backSizeFrame = backSize - (offset * 2.0f);

    // Считаем высоту ОДНОЙ кнопки: (Вся высота - все зазоры) / количество
    int totalButtons = buttons.size();
    float ButtonH = (backSizeFrame.y - (gap * (totalButtons - 1))) / totalButtons;

   
    // Рисуем общий фон и заголовок
    // DrawBackground(sr, w, h, glm::vec4(0.05f, 0.05f, 0.1f, 0.95f));
    sr->DrawSprite(ResourceManager::GetShader(ShaderID::UIGlyph), '#',
        backPos, backSize, 0.0f, glm::vec4(0.05f, 0.05f, 0.1f, 0.95f));
    // Рисуем кнопки вкладок
  //  BaseUI::Render(r, sr, w, h);

    int count = 0;
    for (auto& btn : buttons) {
        // 1. Позиция Y для всей строки (кнопка + текст)
        float currentY = backPos.y + offset + (count * (ButtonH + gap));

        // 2. Цвета
        glm::vec3 color = btn.selected ? glm::vec3(1.0f, 1.0f, 0.0f) : (btn.hovered ? glm::vec3(1.0f) : glm::vec3(0.6f));

        // 3. Фон кнопки (рисуем на всю ширину рамки)
        sr->DrawSprite(ResourceManager::GetShader(ShaderID::UIGlyph), '#',
            glm::vec2(backPos.x + offset, currentY),
            glm::vec2(backSizeFrame.x, ButtonH),
            0.0f, glm::vec4(0.1f, 0.1f, 0.15f, 1.0f)); // Чуть светлее основного фона

        // 4. Текст (центрируем внутри ButtonH)
        float textScale = 1.0f;
        float textWidth = r->MeasureTextWidth(btn.text, textScale);

        // Центрируем текст внутри кнопки
        float textX = backPos.x + offset + (backSizeFrame.x - textWidth) / 2.0f;
        float textY = currentY + (ButtonH / 2.0f) - (12.0f * textScale); // 12.0f - примерная половина высоты шрифта

        r->RenderText(btn.text, textX, textY, textScale, color);

        // ВАЖНО: Обнови данные в btn, чтобы по ним работал клик мышки!
        btn.x = backPos.x + offset; // относительно окна
        btn.y = currentY;
        btn.w = backSizeFrame.x;
        btn.h = ButtonH;

        count++;
    }


        // Рисуем контент текущей вкладки
        if (currentTab == OptionsTab::Keybinds) {
            keybinds->Render(r, sr, w, h);
        }
        else if (currentTab == OptionsTab::Video) {
            // RenderVideoSettings(r, sr);
        }
    
}

void OptionsUI::InitMainButtons()
{

    // Кнопки-вкладки вверху экрана
    AddTabButton("General", OptionsTab::General, 0, 20);
    AddTabButton("Video", OptionsTab::Video, 0, 20);
    AddTabButton("Keys", OptionsTab::Keybinds, 0, 20);
    AddTabButton("Exit", OptionsTab::Exit, 0, 20);
    // ...
}
