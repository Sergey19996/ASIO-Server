#ifndef BASEUI_H
#define BASEUI_H
#include "../Text/TextRender.h"
#include "../SpriteRenderer.h"
#include <string>
#include <vector>
#include <functional>
#include "../NetCommon/net_common.h" // 👈 ГДЕ sPlayerDescription
#include "../NetShared/GameplayTypes.h"
#include "../NetShared/KeyConfig.h"

class Game;
const float M_PI = 3.14f;
struct UIButton {
    std::string text;
    GameAction action = GameAction::Count;
    float x, y;       // позиция
    float w, h;       // размеры
    bool hovered = false;
    bool selected = false;
    std::function<void()> onClick;

    bool Contains(float px, float py) const;

};

class Fog;
class ActionBar;
class BaseUI {
public:
    virtual ~BaseUI() {} // Всегда добавляй деструктор
    virtual void Render(TextRenderer* r, SpriteRenderer* sr, int levelWidth, int levelHeight);
    virtual void Update(float dt, int mouseX, int mouseY);
    virtual void OnMouseButton(int button, int action, int mouseX, int mouseY);
    virtual void OnKey(int key, int action) {}
    virtual void OnActivate();
    virtual void OnChar(unsigned int c) {}
    // Новый виртуальный метод для пересчета позиций
    virtual void RefreshLayout(int levelWidth, int levelHeight) {}

    
    void DrawBackground(SpriteRenderer* sr, int w, int h, glm::vec4 color);
protected:
    std::vector<UIButton> buttons;
    size_t GetUTF8Length(const std::string& str);
};

class LoginUI : public BaseUI {
public:
    Game* game;
    bool typingPassword = false;
    std::string login, password;

    LoginUI(Game* g);
    void Render(TextRenderer* r, SpriteRenderer* sr, int levelWidth, int levelHeight) override;
    void OnChar(unsigned int c) override;
    void OnKey(int key, int action) override;

private:

    glm::vec4 white = { 1.0f,1.0f,1.0f,1.0f };
    glm::vec4 green = { 0.0f,1.0f,0.0f,1.0f };
};

class RegisterUI : public BaseUI {
public:
    Game* game;
    bool typingPassword = false;
    std::string login, password;

    RegisterUI(Game* g);
    void Render(TextRenderer* r, SpriteRenderer* sr, int levelWidth, int levelHeight) override;
    void OnChar(unsigned int c) override;
    void OnKey(int key, int action) override;
private:
    glm::vec4 white = { 1.0f,1.0f,1.0f,1.0f };
    glm::vec4 green = { 0.0f,1.0f,0.0f,1.0f };
};



class LobbyUI : public BaseUI {
public:
    Game* game;

    LobbyUI(Game* g);

    void Render(TextRenderer* r, SpriteRenderer* sr, int levelWidth, int levelHeight) override;
    void OnChar(unsigned int c) override;

 


    void OnKey(int key, int action) override;


private:

   
};

class FriendsUI : public BaseUI {
public:
    Game* game;

    FriendsUI(Game* g);

    void Render(TextRenderer* r, SpriteRenderer* sr, int levelWidth, int levelHeight) override;
    void OnChar(unsigned int c) override;

  


    void OnKey(int key, int action) override;


private:

 
};

class ShopUI : public BaseUI {
public:
    Game* game;

    ShopUI(Game* g);

    void Render(TextRenderer* r, SpriteRenderer* sr, int levelWidth, int levelHeight) override;
    void OnChar(unsigned int c) override;

  


    void OnKey(int key, int action) override;


private:

   
};


class RoomUI : public BaseUI {
public:
    Game* game;

    RoomUI(Game* g);

    void Render(TextRenderer* r, SpriteRenderer* sr, int levelWidth, int levelHeight) override;
    void OnChar(unsigned int c) override;
  


    void OnKey(int key, int action) override;



private:

   
};

class MatchmakingUI : public BaseUI {
public:
    Game* game;

    MatchmakingUI(Game* g);

    void Render(TextRenderer* r, SpriteRenderer* sr, int levelWidth, int levelHeight) override;
    void OnChar(unsigned int c) override;

 


    void OnKey(int key, int action) override;


private:

  //  std::vector<UIButton> buttons;
    std::string title = "Searching for match...";
};


// AFTER MATCH ---
class VictoryUi : public BaseUI {
public:
    Game* game;

    VictoryUi(Game* g);

    void Render(TextRenderer* r, SpriteRenderer* sr, int levelWidth, int levelHeight) override;
    void OnChar(unsigned int c) override;

   


    void OnKey(int key, int action) override;


private:

    
    std::string title = "Searching for match...";
};

class DefeatUi : public BaseUI {
public:
    Game* game;

    DefeatUi(Game* g);

    void Render(TextRenderer* r, SpriteRenderer* sr, int levelWidth, int levelHeight) override;
    void OnChar(unsigned int c) override;

   


    void OnKey(int key, int action) override;


private:

  
    std::string title = "Searching for match...";
};
// AFTER MATCH ---

struct CraftOption {
    ArrowType type;
    std::string name;
    glm::vec4 color;
    char icon;
};

class CraftArrowUI : public BaseUI {
public:
    Game* game;
    struct sector {
        ArrowType type;
        glm::vec4 color;
        std::string label;
        float startAngle, endAngle;
    };
    std::vector<sector> sectors;

    CraftArrowUI(Game* g);

    void Update(float dt, int mouseX, int mouseY)override;
    void Render(TextRenderer* r, SpriteRenderer* sr, int levelWidth, int levelHeight) override;
    void OnKey(int key, int action)override;
private:
   int hoveredIndex;
   float UiWidth;
    float UiHeight;
};

class KeyBindUI : public BaseUI {
public:
    Game* game;
    GameAction waitingForKey = GameAction::Count; // Какое действие ждет нажатия
    KeyBindUI(Game* g);

    void InitButtons();
    void AddBindingButton(std::string label, GameAction action, float x, float y);
    void OnMouseButton(int button, int action, int mouseX, int mouseY) override;
    void OnKey(int key, int Action) override;
    void Render(TextRenderer* r, SpriteRenderer* sr, int levelWidth, int levelHeight) override;
   static std::string GetKeyName(int key);



};


enum class OptionsTab { General, Video, Keybinds, Language,Exit};
enum class GameState; // Forward declaration
class OptionsUI : public BaseUI {
public:
    Game* game;
    OptionsTab currentTab = OptionsTab::General;

    GameState returnState; // По умолчанию
    void SetReturnState(GameState s) { returnState = s; }
    // Под-классы (или просто наборы кнопок) для разделов
    KeyBindUI* keybinds;

    OptionsUI(Game* g);
    ~OptionsUI() override;
    void InitMainButtons();
    void AddTabButton(std::string label, OptionsTab tab, float x, float y);
    void UpdateTabSelection();

    void OnMouseButton(int button, int action, int mx, int my) override;

    void Update(float dt, int mx, int my);
    void OnKey(int key, int Action) override;
    void Render(TextRenderer* r, SpriteRenderer* sr, int w, int h) override;
};

class Hunter;
class InGameUI : public BaseUI {
public:
    Game* game;
    InGameUI(Game* g);

    void Render(TextRenderer* r, SpriteRenderer* sr, int levelWidth, int levelHeight) override;
    void Update(float dt, int levelWidth, int levelHeight) override;

    void DrawDayCycleClock(SpriteRenderer* sr, const float& x,const  float& y,const float& size);
    void DrawWindIndicator(SpriteRenderer* sr, TextRenderer* r, const float& levelWidth, const float& levelHeight);
    void RenderMinimap(SpriteRenderer* sr,Fog* fog, const glm::vec2& playerPos, int levelWidth, int levelHeight);
   
 
    void DrawMageIndicator(SpriteRenderer* sr, float x, float y, float width, float height, float resourceValue, float chargeRatio);
    void DrawHunterIndicator(SpriteRenderer* sr, float x, float y, float width, Hunter* h, float chargeRatio);
    void DrawWarriorAdaptation(SpriteRenderer* sr, float x, float y, float radius, float fillProgress, int level);
   
    void RefreshLayout(int levelWidth, int levelHeight) override;
    void DrawExperienceBar(SpriteRenderer* sr, TextRenderer* r, float x, float y, float width, float height, float currentXP, float requiredXP, int level);
    void DrawSquadSlots(SpriteRenderer* sr, float x, float y, int currentCount, int maxAllowed);
private:
    UIButton networkBtn; // Наш индикатор
    Shader* minimapShader = nullptr;
    std::unique_ptr<ActionBar> actionBar; // Владеем панелью слотов

    float SKILL_SIZE = 48.0f;
    float SKILL_PADDING = 8.0f;
    bool isNight;


    // Позиции для FPS/Ping
    float statX, statY;
    float statTextX;

    // Позиции для классового UI
    float classCenterX, classCenterY;
    float warriorY, mageHunterY;

    // Позиция миникарты (если нужно)
    float minimapX, minimapY;
};




#endif // BASEU
