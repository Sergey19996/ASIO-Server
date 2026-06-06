#ifndef Game_h
#define Game_h
#include <olc_net.h>
#include <../NetShared/PlayerProfile.h>
#include <vector>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <unordered_map>
#include "GameStates.hpp"
#include "UI/BaseUI.h"
#include <string>
#include "Level/gameLevel.h"
#include "UI/HealthBarRenderer.h"
#include "../NetShared/WorldTypes.h"
#include "../NetShared/LoginTypes.h"
#include "../NetShared/NetMessage.h"
#include "../NetShared/SpellId.h"
#include "../NetShared/ActionSlot.h"
#include "Fog.h"
#include "Rendering/Camera2d.h"
#include "../NetShared/PlayerInfo.h"
#include "../NetShared/entities/Character.h"
#include "db/SpellDatabase.h"
#include "db/CharacterDatabase.h"
#include "VisualObject.h"


//#include "../NetShared/GameplayTypes.h"
// Represents the four possible (collision) directions

// Defines a Collision typedef that represents collision data
//typedef std::tuple<bool, Direction, glm::vec2> Collision; // <collision?, what direction?, difference vector center - closest point>


const float DAY_CYCLE_DURATION = 100.0f; // 10 минут на полный цикл
// Структура для товара в магазине
struct ShopItem {
	uint32_t id;
	std::string name;
	std::string description;
	uint32_t price;
	std::string type; // "spell", "skin", "boost", etc.
	bool isPurchased = false;
};


struct ExplosionEffect {
	glm::vec2 pos;
	float timer;       // сколько осталось до исчезновения
	float diametr;   // визуальный радиус
	float currentRadius;
};

struct VisualChain {
	glm::vec2 start;       // Позиция игрока
	glm::vec2 end;         // Точка притяжения (стрела)
	glm::vec4 color = { 0.0f, 1.0f, 0.7f, 1.0f };       // Цвет (бирюзовый для Bind)
	float lifetime;        // Синхронизируем с временем статус-эффекта
	uint32_t targetNetId;  // Кого привязали
	uint32_t linkedNetId;
};
struct CooldownInfo {
	float endTime; 
	float duration; };

struct FloatingText {
	glm::vec2 pos;
	std::string value;
	float lifetime = 1.0f;
	glm::vec3 color;
	bool isResisted;

	float scale = 1.0f;
	float alpha = 1.0f;
};



class Game : public olc::net::client_interface<GameMsg> {
public:
	static std::vector<void(*)(GLFWwindow* windiw, int key, int scancode, int action, int mods)>keyCallbacks;
	static void handleMouseStatic(GLFWwindow* window, int button, int action, int mods);
	static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int modes);
	static void character_callback(GLFWwindow* window, unsigned int codepoint);
	void OnKey(int key, int action);  // для клавиатуры
	void keyEvents(); // клавиатура + мышь
	void OnMouseButton(int button, int action);
	unsigned int Width, Height; // размер экрана в пикселях 
	float UiHeight, UiWidth;

	float currentPing;
	float currentFPS;
	int currentInboundKBps;

	Game(unsigned int levelWidth, unsigned int levelHeight);
	~Game();

	bool Init();

	void OnResize(int w, int h);
	glm::vec2 GetMouseWorldPos() const;
	glm::ivec2& GetLevelAmountCells();
	void Update(float dt);
	void Render();
	void RenderChat();
	glm::vec2 GetCurrentInputDir() const;
	void renderObject();
	void renderVFX();
	void updateObjects(float felapsedTIme);
	void ProcessInput(float dt);



	GameState& GetState() { return state; };
	BaseUI* GetCurrentScreen() { return currentScreen; };
	void PrepareChatInput();
	void ReleaseChatInput();

	
	void TryLogin(const std::string& name, const std::string& pass);
	void TryRegister(const std::string& name, const std::string& pass);
	void delete_Char();
	void DeleteLoginChar();
	Fog& GetFog() { return fog; };


	void CraftArrow(ArrowType type);


	void castSlot(ActionSlot slot);



	void CancelMatchMaking();
	void SetState(GameState s) {
		state = s;
		currentScreen = uiScreens[s];
		std::cout << "Switching to state: " << uiScreens.size() << std::endl;
		if (currentScreen) {
			currentScreen->OnActivate(); // Экран сам знает, как себя "почистить"
		}
	}

	void AppendLoginChar(unsigned int ch);
	void OnCharTyped(unsigned int ch);

	void FindMatch();
	
	//void StartGame();
	void OpenShop();
	void OpenFriends();
	void OpenOptions(GameState state);
	void OpenLobby();
	void OpenLogin();
	void OpenRegistration();
	void OpenInGame();
	void DrawChains(SpriteRenderer* sr);

	float GetServerTime() const { return serverTime; }

	const CooldownInfo* GetCooldown(ActionSlot slot) const;

	bool IsOnCooldown(ActionSlot slot) const;

	const CooldownInfo* GetCooldownBySpellId(SpellId id) const;

	PlayerProfile playerProfile;
	PlayerClass selectedClass;
//	gameLevel* GameLevel;
	std::unique_ptr<gameLevel>GameLevel;
	uint8_t matchmakingCount;
	float currentWindAngle = 0.0f; // Угол ветра от сервера
	float targetWindAngle = 0.0f;
	uint8_t currentWindForce = 0;

	// day/night
	float dayCycleProgress = 0.0f;
	float lightIntensity = 0.0f;
	//

	//std::unordered_map<uint32_t, sEntityDescription> mapObjects;
	std::unordered_map<uint32_t, VisualObject> mapVisuals;
	std::unordered_map<uint32_t, std::unique_ptr<Character>> playerEntities;
	uint32_t nPlayerID = 0;
	KeyConfig config; //бинды кнопок
	SpellDatabase spellDb;
	TileDatabase tileDB;


private:

	std::string statusMessage = "";
	glm::vec4 statusColor = { 1, 1, 1, 1 };
	float statusTimer = 0.0f; // Сколько секунд еще показывать сообщение
	void ShowStatus(const std::string& msg, glm::vec4 color, float duration = 3.0f);

	float reconnectTimer = 0.0f;
	const float reconnectInterval = 2.0f; // Пытаемся каждые 2 секунды
	

	bool bIsReconnecting = false;
	int connectionAttempts = 0;

	
	bool isShieldActive = false;

	bool connectToServer();

	void UpdateLogin(const float& dt);
	void UpdateLobby(const float& dt);
	void UpdateMatchMaking(const float& dt);
	void UpdateGameplay(const float& dt);
	void UpdateVFX(const float& dt);
	void UpdateCamera(float dt);
	inline glm::vec2 GridToScreen(int idx, int levelWidth);


	void RenderGame();
	bool IsVisible(const glm::vec2& posInTiles, const float& radius);
	void UpdateNetwork(float& dt);
	void sendUpdatePlayer(const glm::vec2& inputDir, const glm::vec2& lookDir);
	
	void SendCastMessage(GameMsg type, ActionSlot slot);
//	void DrawEntityInfo(const EntityVisual& ev);
	static bool Keys[];
	static bool KeysProcessed[];


	bool bWaitingForConnection = true;

	
	std::unordered_map<uint16_t, sProjectileDescription> projectiles;

	// В основном классе игры

	std::unordered_map<uint16_t, VisualObject> mapProjVisuals;

	std::unordered_map<uint32_t, sPlayerInfo> sessionPlayers;
	std::vector<ExplosionEffect> activeExplosions;
	std::deque<sChatMessage> chatMessages;
	

	
	std::map<SpellId, CooldownInfo> spellCooldowns;
	std::vector<FloatingText> activeDamages;
	

	std::string chatMessage;
	static GameState state;


	std::map<GameState, BaseUI*> uiScreens;  // освободить память ! 
	BaseUI* currentScreen = nullptr;


	std::vector<ShopItem> shopItems;

	// Сетевая информация
	std::string serverAddress;
	uint16_t serverPort = 60000;
	
	// Таймеры
	float connectionTimer = 0.0f;
	float matchmakingTimer = 0.0f;
	int tempInboundBytes;

	bool connectionError = false;
	std::string errorMessage;


	HealthBarRenderer hpBars;


	float serverTime = 0.0f;
	//float serverTimeOffset = 0.0f;
	float pingTimer; // Для замера задержки
	int fpsCounter = 0;

	Fog fog;
	Camera2D camera;
	uint64_t nSessionToken = 0; // Изначально 0


	int spectatorTargetIndex = 0; // Индекс игрока, за которым смотрим
	bool isDead = false;
	int currentListIndex = 0;


	std::vector<VisualChain> activeChains;
	//uint8_t observerTeam;

};


#endif // !Game
