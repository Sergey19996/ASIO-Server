#include "Game.h"
#include "ResourceManager.h"
#include "Text/TextRender.h"
#include "io/Mouse.h"
#include "../NetShared/PlayerClass.h"
#include "../NetShared/entities/Mage.h"
#include "../NetShared/entities/Warrior.h"
#include "../NetShared/entities/Hunter.h"
#include "../NetShared/AI/MeleeMob.h"
#include "../NetShared/AI/RangerMob.h"
#include "../NetShared/AI/Companions/PriestCompanion.h"
#include "../NetShared/AI/Companions/RangerCompanion.h"
#include "../NetShared/AI/Companions/WarriorCompanion.h"

#include "../NetShared/managers/EffectManager.h"
#include "../NetShared/managers/CooldownManager.h"
#include "../NetShared/managers/SquadManager.h"
#include "../NetShared/managers/ProgressionManager.h"
SpriteRenderer* Renderer;
InstancedSpriteRenderer* instRenderer;
TextRenderer* text;

const unsigned int TEXTSIZE = 18;
const size_t MAX_CHAT_MESSAGES = 25;
#define VisibleTilesW 25
#define VisibleTilesH 18
std::vector<void(*)(GLFWwindow* windiw, int key, int scancode, int action, int mods)>Game::keyCallbacks;
bool Game::Keys[GLFW_KEY_LAST] = { 0 };  //GLFW_KEY_LAST это константа, которая задаёт максимальное количество клавиш, распознаваемых GLFW.
bool Game::KeysProcessed[GLFW_KEY_LAST] = { 0 };
GameState Game::state = GameState::LOGIN;

// Определяем цвета классов
static const glm::vec4 ClassColors[] = {
	glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), // None (Белый)
	glm::vec4(1.0f, 0.2f, 0.2f, 1.0f), // Warrior (Красный)
	glm::vec4(0.2f, 0.4f, 1.0f, 1.0f), // Mage (Синий)
	glm::vec4(0.2f, 1.0f, 0.3f, 1.0f),  // Hunter (Зеленый)
	// Новые цвета для компаньонов и мобов
   glm::vec4(0.4f, 0.9f, 1.0f, 1.0f), // 4: Companion (Голубой/Бирюзовый)
   glm::vec4(0.8f, 0.3f, 0.9f, 1.0f), // 5: Mob/Skeleton (Фиолетовый или костяной)
   glm::vec4(1.0f, 0.9f, 0.2f, 1.0f)  // 6: Healer (Желтый/Золотой)
};


// Initial velocity of the player paddle

const float PI = 3.1415926535f;

static glm::vec2 lastInputDir = { 0, 0 };
static glm::vec2 lastLookDir = { 0, 0 };
static float networkTickTimer = 0.0f;


Game::Game(unsigned int levelWidth, unsigned int levelHeight) : Width(levelWidth),Height(levelHeight),currentScreen(nullptr)
{


}

Game::~Game()
{
	delete text;
	text = nullptr;
	//delete GameLevel;
	GameLevel = nullptr;
	delete Renderer;
	Renderer = nullptr;
	delete instRenderer;
	instRenderer = nullptr;

	for (auto& screen : uiScreens) {
		delete screen.second;
		screen.second = nullptr;
	}

}

bool Game::Init()
{
	Mouse::mouseButtonCallBacks.push_back(Game::handleMouseStatic);
	UiHeight = 1080;
	ResourceManager::LoadShader("Assets/Shaders/ui_glyph.vert","Assets/Shaders/ui_glyph.frag",nullptr, ShaderID::UIGlyph);
	ResourceManager::LoadShader("Assets/Shaders/sprite.vert","Assets/Shaders/sprite.frag",nullptr,ShaderID::Sprite);
	ResourceManager::LoadShader("Assets/Shaders/text_2d.vert", "Assets/Shaders/text_2d.frag", nullptr, ShaderID::TextShader);

	ResourceManager::LoadShader("Assets/Shaders/fog_mask.vert","Assets/Shaders/fog_mask.frag",nullptr,ShaderID::FogMask);
	ResourceManager::LoadShader("Assets/Shaders/fog_mask.vert","Assets/Shaders/fog_blur.frag",nullptr,ShaderID::FogBlur);
	ResourceManager::LoadShader("Assets/Shaders/fog_mask.vert", "Assets/Shaders/Light_Shadow.frag", nullptr, ShaderID::LightShadow);

	ResourceManager::LoadShader("Assets/Shaders/fog_mask.vert", "Assets/Shaders/sprite.frag", nullptr, ShaderID::FogCopy);

	ResourceManager::LoadShader("Assets/Shaders/World_glyph.vert", "Assets/Shaders/World_glyph.frag", nullptr, ShaderID::World);
	ResourceManager::LoadShader("Assets/Shaders/instanced_sprite.vert", "Assets/Shaders/instanced_sprite.frag", nullptr, ShaderID::instanceWorld);
	ResourceManager::LoadShader("Assets/Shaders/fog_mask.vert", "Assets/Shaders/minimap.frag", nullptr, ShaderID::Minimap);
	// Загрузка шрифта
	ResourceManager::LoadFont("UI", "Assets/Fonts/PressStart2P-Regular.ttf", TEXTSIZE);

	ResourceManager::LoadTexture("Assets/Textures/white.png", false, "white");
	// Загружаем текстуру атласа способностей
	ResourceManager::LoadPixelTexture("Assets/Textures/spell_atlas.png", true, "spell_icons");
	ResourceManager::LoadPixelTexture("Assets/Textures/tile_atlas.png", true, "Level_tiles");
	ResourceManager::LoadPixelTexture("Assets/Textures/Character_atlas.png", true, "Character_atlas");
//	GameLevel = new gameLevel(Width / BRICK_SIZE,Height / BRICK_SIZE);
	Renderer = new SpriteRenderer();
	instRenderer = new InstancedSpriteRenderer();
	// Загружаем базу заклинаний
	spellDb.Load("Assets/Data/spells.json");
	tileDB.Load("Assets/Data/tiles.json");
	CharacterDatabase::Instance().Load("Assets/Data/characters.json");
	float uiScale = 1.0f;
	UiHeight = 1080 / uiScale;
	UiWidth = UiHeight * ((float)Width / (float)Height);
	glm::mat4 uiProj = glm::ortho(
		0.0f, UiWidth,
		UiHeight, 0.0f,
		-1.0f, 1.0f
	);

	camera.position = {0,0};
	camera.UpdateViewport(Width, Height);
	glm::mat4 WorldProj = glm::ortho(
		0.0f, (float)Width,
		(float)Height, 0.0f,
		-1.0f, 1.0f
	);
	
	glm::mat4 view = camera.GetViewMatrix();

	

	// 🌍 WORLD
	ResourceManager::GetShader(ShaderID::World).use().setMat4("projection", WorldProj, true);
	ResourceManager::GetShader(ShaderID::World).use().setMat4("view", view);
	ResourceManager::GetShader(ShaderID::instanceWorld).use().setMat4("projection", WorldProj, true);
	ResourceManager::GetShader(ShaderID::instanceWorld).use().setMat4("view", view);



	ResourceManager::GetShader(ShaderID::Sprite).use().setMat4("projection", uiProj, true);
	//ResourceManager::GetShader(ShaderID::Sprite).use().setMat4("view", view);

	ResourceManager::GetShader(ShaderID::TextShader).use().setMat4("view", glm::mat4(1.0f));


	// 🧾 UI
	ResourceManager::GetShader(ShaderID::UIGlyph).use().setMat4("projection", uiProj, true);


	ResourceManager::GetShader(ShaderID::Sprite).use().setInt("image", 0);

	text = new TextRenderer(UiWidth, UiHeight);
	text->SetFont(ResourceManager::GetFont("UI"));

	uiScreens[GameState::FRIENDS] = new FriendsUI(this);
	uiScreens[GameState::LOGIN] = new LoginUI(this);
	uiScreens[GameState::REGISTRATION] = new RegisterUI(this);
	uiScreens[GameState::SHOP] = new ShopUI(this);
	uiScreens[GameState::LOBBY] = new LobbyUI(this);
	uiScreens[GameState::ROOM] = new RoomUI(this);
	uiScreens[GameState::MATCHMAKING] = new MatchmakingUI(this);
	uiScreens[GameState::VICTORY] = new VictoryUi(this);
	uiScreens[GameState::DEFEAT] = new DefeatUi(this);
	uiScreens[GameState::INGAME] = new InGameUI(this);
	uiScreens[GameState::CRAFTING] = new CraftArrowUI(this);
	uiScreens[GameState::OPTIONS] = new OptionsUI(this);
	SetState(state);

	for (auto& screen : uiScreens) {
		screen.second->RefreshLayout(UiWidth, UiHeight);
	}

//	currentScreen->RefreshLayout(UiWidth, UiHeight);

	fog.Init(50, 50);
	//fog.SetLevelBlocks(GameLevel->blocks, GameLevel->LevelSize.x, GameLevel->LevelSize.y);

	

	fog.fogMaskShader = &ResourceManager::GetShader(ShaderID::FogMask);
	fog.blurShader = &ResourceManager::GetShader(ShaderID::FogBlur);
	fog.fogCopyShader = &ResourceManager::GetShader(ShaderID::FogCopy);
	fog.lightShader = &ResourceManager::GetShader(ShaderID::LightShadow);
	if (Connect("26.168.96.157", 60000))
	//if (Connect("127.0.0.1", 60000))
	{
		return true;
	}

	return true;

}

void Game::OnResize(int w, int h)
{
	Width = w;
	Height = h;

	fog.Resize(w, h);

	

	glm::mat4 worlProj = glm::ortho(
		0.0f, (float)Width,
		(float)Height, 0.0f,
		-1.0f, 1.0f
	);
	float uiScale = 1.0f;
	UiHeight = 1080 / uiScale;
	UiWidth= UiHeight * ((float)w / (float) h);
	glm::mat4 uiProj = glm::ortho(
		0.0f, UiWidth,
		UiHeight, 0.0f,
		-1.0f, 1.0f
	);
	currentScreen->RefreshLayout(UiWidth, UiHeight);

	glm::mat4 view = camera.GetViewMatrix();
	float worldWidth = VisibleTilesW * BRICK_SIZE;
	float worldHeight = VisibleTilesH * BRICK_SIZE;

	float scaleX = (float)Width / worldWidth;
	float scaleY = (float)Height / worldHeight;

	//float rawZoom = std::min(scaleX, scaleY);


	// Выбираем минимальный скейл, чтобы уровень влез целиком (Letterboxing)
	camera.zoom = std::min(scaleX, scaleY);

	// Вариант А: Округление до ближайших 0.5 (например, 1.0, 1.5, 2.0, 2.5) — картинка будет максимально чёткой
	//camera.zoom = std::round(rawZoom * 2.0f) / 2.0f;

	// Вариант Б: Если нужно, чтобы уровень строго вписывался в экран без Letterbox, 
// оставьте rawZoom, но тогда ОБЯЗАТЕЛЬНО примените Решение 2 (оно ниже).
	//if (camera.zoom < 0.5f) camera.zoom = 0.5f; // Защита от слишком сильного отдаления

	camera.UpdateViewport(Width, Height);
	
//
//
	ResourceManager::GetShader(ShaderID::Sprite).use().setMat4("projection", uiProj, true);
	ResourceManager::GetShader(ShaderID::World).use().setMat4("projection", worlProj, true);
	ResourceManager::GetShader(ShaderID::instanceWorld).use().setMat4("projection", worlProj, true);
	//ResourceManager::GetShader(ShaderID::instanceWorld).use().setMat4("view", view);
	//ResourceManager::GetShader(ShaderID::Sprite).use().setMat4("view", view);
//
//
//	// 🧾 UI
	ResourceManager::GetShader(ShaderID::UIGlyph).use().setMat4("projection", uiProj,true);

	ResourceManager::GetShader(ShaderID::TextShader).use().setMat4("view", view); // для отрисовки имен под пероснажами 


	if (!GameLevel) return; // СТОП-КРАН: если уровня нет, не считаем камеру
	

	// --- РАСЧЕТ ЦЕНТРИРОВАНИЯ ---
	// 1. Вычисляем, сколько места занимает уровень на экране с текущим зумом в пикселях 
	//float visibleLevelWidth = worldWidth * camera.zoom;
	//float visibleLevelHeight = worldHeight * camera.zoom;

	// 2. Находим свободное пространство и делим пополам
	// Мы инвертируем значение и делим на зум, так как камера двигается ВНУТРИ мира
	//float ScreenOffsetX = ((float)Width - visibleLevelWidth) / 2.0f;
//	float ScreenOffsetY = ((float)Height - visibleLevelHeight) / 2.0f;

	// Устанавливаем позицию камеры так, чтобы сместить мир в центр экрана
	// Делим на зум, потому что в GetViewMatrix() Translation идет ПЕРЕД Scale (в логическом порядке) 
//	camera.position.x = -ScreenOffsetX / camera.zoom;
//	camera.position.y = -ScreenOffsetY / camera.zoom;


	 view = camera.GetViewMatrix();
	// 🌍 WORLD
	//ResourceManager::GetShader(ShaderID::World).use().setMat4("view", view);
	//ResourceManager::GetShader(ShaderID::TextShader).use().setMat4("view", view); // для отрисовки имен под пероснажами 

	text->Resize(w, h);
}

glm::vec2 Game::GetMouseWorldPos() const
{
	float mx = (float)Mouse::getMouseX();
	float my = (float)Mouse::getMouseY();

	// 1. Вычитаем экранное смещение (те самые "черные полосы" из OnResize)
	// Если вы не сохраняли offsetX/Y как переменные, их надо рассчитать
//	float worldWidthInPixels = (GameLevel->LevelSize.x * BRICK_SIZE) * camera.zoom;
//	float offsetX = (Width - worldWidthInPixels) / 2.0f;
//	float offsetY = (Height - worldWidthInPixels) / 2.0f;

	// 2. Переводим пиксель в мировую координату
	//  при делении на зум - мы возврощаем 800 на 600 из 2560 на 1950
	float worldX = mx  / camera.zoom + camera.position.x; // + camera position - даёт нам 0;0 позиции камеры в первой клетке уровня
	float worldY = my  / camera.zoom + camera.position.y;

	return { worldX, worldY };
}

glm::ivec2& Game::GetLevelAmountCells()
{
	
		return GameLevel->LevelSize;
}

void Game::Update(float dt)
{
	
	
	// 1. ВСЕГДА обрабатываем сеть
	UpdateNetwork(dt);

	// 2. Gameplay — только в игре
	if (state == GameState::INGAME || state == GameState::TYPINGCHAT || state == GameState::CRAFTING)
	{
		UpdateGameplay(dt);
		UpdateCamera(dt);
		
	}

	// 3. UI — если есть активный экран
	if (currentScreen)
	{
		double mx = Mouse::getMouseX();
		double my = Mouse::getMouseY();
		float normX = mx / Width;  // от 0 -> 1
		float normY = my / Height; 

		currentScreen->Update(dt, UiWidth * normX, UiHeight * normY);
	}
	if (statusTimer > 0) {
		statusTimer -= dt;
	}

}

void Game::Render()   
{
	if (state == GameState::INGAME || state == GameState::TYPINGCHAT || state == GameState::CRAFTING){
		ResourceManager::GetShader(ShaderID::World).use().setVec2("u_screenSize", glm::vec2((float)Width, (float)Height));
		ResourceManager::GetShader(ShaderID::instanceWorld).use().setVec2("u_screenSize", glm::vec2((float)Width, (float)Height));
		glm::mat4 view = camera.GetViewMatrix();
		ResourceManager::GetShader(ShaderID::TextShader).use().setMat4("view", view);
		ResourceManager::GetShader(ShaderID::World).use().setMat4("view", view);
		ResourceManager::GetShader(ShaderID::instanceWorld).use().setMat4("view", view);
		ResourceManager::GetShader(ShaderID::Sprite).use().setMat4("view", view);
		ResourceManager::GetShader(ShaderID::TextShader).use().setMat4("view", view);
		text->Resize(Width, Height);
		RenderGame();

	
	}
	if (currentScreen) {

	


		// Перед вызовом text->RenderText() в функции отрисовки меню:
		text->GetShader().use();
		text->GetShader().setBool("WorldSpace", false); // Отключаем логику тумана
		// Можно также сбросить fogTex на что-то безопасное, например:
		text->GetShader().setInt("fogTex", 99);
		ResourceManager::GetShader(ShaderID::TextShader).use().setMat4("view", glm::mat4(1.0f)); //обнуляем матрицу вида - что бы ui не учитывал камеру 
		text->Resize(UiWidth, UiHeight);
		currentScreen->Render(text, Renderer, UiWidth, UiHeight);
	}


	if (bWaitingForConnection)
	{

		std::string sMenu = "Waiting To Connect...";
		text->RenderText(sMenu, Width / 3, 20, 1);

	}
	if (statusTimer > 0) {
		// Делаем текст прозрачнее, когда время истекает (плавное исчезновение)
		float alpha = std::min(1.0f, statusTimer);
		glm::vec4 finalColor = statusColor;
		finalColor.a = alpha;

		text->Resize(UiWidth, UiHeight); // Используем UI разрешение
		float tw = text->MeasureTextWidth(statusMessage, 0.6f);

		// Рисуем по центру в верхней части экрана
		text->RenderText(statusMessage, (UiWidth - tw) * 0.5f, 50, 0.6f, finalColor);
	}
}

void Game::keyEvents(){

	if (state != GameState::INGAME) return;

	auto it = playerEntities.find(nPlayerID);
	if (it == playerEntities.end()) return;

	auto& visual = mapVisuals[nPlayerID];

	// 1. Собираем ввод клавиатуры
	glm::vec2 inputDir = { 0.0f, 0.0f };
	if (Keys[config.bindings[GameAction::MoveLeft]])  inputDir.x -= 1.0f;
	if (Keys[config.bindings[GameAction::MoveRight]]) inputDir.x += 1.0f;
	if (Keys[config.bindings[GameAction::MoveUp]])    inputDir.y -= 1.0f;
	if (Keys[config.bindings[GameAction::MoveDown]])  inputDir.y += 1.0f;

	// 2. Нормализуем вектор движения
	if (glm::length(inputDir) > 0.0f) {
		inputDir = glm::normalize(inputDir);
	}

	// 3. ИСПРАВЛЕНО: Направление взгляда (lookDir) для ЛОКАЛЬНОГО игрока ВСЕГДА берется от мышки!
	// Нельзя заставлять его смотреть туда, куда он бежит, иначе ломается стрейф (бег спиной/боком)
	glm::vec2 worldMouse = GetMouseWorldPos();
	glm::vec2 currentPos = visual.vCurrentPos;
	glm::vec2 gridMouse = worldMouse / (float)BRICK_SIZE;

	glm::vec2 lookDir = gridMouse - currentPos;
	if (glm::length(lookDir) > 0.001f) {
		lookDir = glm::normalize(lookDir);
	}
	else {
		lookDir = lastLookDir;
	}

	

	// 5. Проверяем изменения для сетевого тика
	bool inputChanged = (inputDir != lastInputDir);
	bool lookChanged = glm::distance(lookDir, lastLookDir) > 0.01f;

	if (networkTickTimer >= 0.033f) { // ~30 раз в секунду
		if (inputChanged || lookChanged) {
			sendUpdatePlayer(inputDir, lookDir);

			lastInputDir = inputDir;
			lastLookDir = lookDir;
		}
		networkTickTimer = 0.0f;
	}

}

void Game::RenderChat(){
	text->Resize(UiWidth, UiHeight);
	//chat
	Shader& glyphShader = ResourceManager::GetShader(ShaderID::UIGlyph);
	glyphShader.use();
	//ResourceManager::GetShader(ShaderID::UIGlyph).use();
	glm::vec4 color = { 1.0f,0.0f,0.0f,1.0f };
	glm::vec2 size = { UiWidth / 3.0f,UiHeight / 4.0f };
	glm::vec2 pos = { (1.0f - size.x / UiWidth) * UiWidth,   (1.0f - size.y / UiHeight) * UiHeight };
	char  chr = '#';
	glyphShader.setvec4("color", color);

	Renderer->DrawSprite(glyphShader,chr, pos, size, 0.0f, { 0.0f,0.0f,0.0f,0.9f });
	Renderer->DrawSprite(glyphShader,chr, { pos.x,pos.y + size.y - size.y * 0.1f }, { size.x,size.y * 0.1f }, 0.0f, { 0.1f,0.1f,0.1f,0.9f });
	text->RenderText(chatMessage, pos.x + size.x * 0.05f, pos.y + size.y - size.y * 0.1f + size.y * 0.05f, 0.5f);


	//Draw messages in chat 
	for (int i = 0; i < chatMessages.size(); i++)
	{
		const auto& msg = chatMessages[i];

		std::string senderName = "???";

		auto it = sessionPlayers.find(msg.nSenderID);
		if (it != sessionPlayers.end())
			senderName = it->second.name;

		text->RenderText(senderName + " : " + chatMessages[i].sText, pos.x + size.x * 0.05f, pos.y + (pos.y * 0.01f) + ((TEXTSIZE * 0.5f) * i), 0.5f);
	}
}

glm::vec2 Game::GetCurrentInputDir() const
{
	glm::vec2 inputDir = { 0.0f, 0.0f };

	// Опрашиваем состояние клавиш движения из вашего конфига
	if (Keys[config.bindings.at(GameAction::MoveLeft)])  inputDir.x -= 1.0f;
	if (Keys[config.bindings.at(GameAction::MoveRight)]) inputDir.x += 1.0f;
	if (Keys[config.bindings.at(GameAction::MoveUp)])    inputDir.y -= 1.0f;
	if (Keys[config.bindings.at(GameAction::MoveDown)])  inputDir.y += 1.0f;

	// Нормализуем вектор, чтобы скорость по диагонали не была выше обычной
	if (glm::length(inputDir) > 0.0f) {
		return glm::normalize(inputDir);
	}

	return inputDir; // Возвращает {0, 0}, если ни одна кнопка не зажата
}



void Game::handleMouseStatic(GLFWwindow* window, int button, int action, int mods)
{
	Game* game = static_cast<Game*>(glfwGetWindowUserPointer(window));
	if (game) {
		game->OnMouseButton(button, action); // Вызываем ваш метод логики
	}
}
void Game::OnMouseButton(int button, int action)
{
	// 1. Если открыт какой-либо экран (меню, бинды, инвентарь)
	if (currentScreen) {
		// Получаем координаты мыши в экранных координатах UI
		double mx = Mouse::getMouseX();
		double my = Mouse::getMouseY();
		float normX = (float)mx / Width;
		float normY = (float)my / Height;

		currentScreen->OnMouseButton(button, action, UiWidth * normX, UiHeight * normY);

		// Если это не игровой режим, блокируем прохождение клика в мир
		if (state != GameState::INGAME) return;
	}
	// 2. Игровая логика кастов
	if (state == GameState::INGAME )
	{
		if (!isDead){ 
			// Ищем, привязана ли эта кнопка мыши к какому-либо слоту
			GameAction foundSlot = GameAction::Count;
			for (auto const& [slot, boundKey] : config.bindings) {
				if (boundKey == button) {
					foundSlot = slot;
					break;
				}
			}
			ActionSlot slot = KeyConfig::ToActionSlot(foundSlot);
			if (foundSlot != GameAction::Count) {
				if (action == GLFW_PRESS) {
					//auto it = mapVisuals.find(nPlayerID);
					//if (it != mapVisuals.end()) {
					//	glm::vec2 worldMouse = GetMouseWorldPos();
					//	glm::vec2 currentPos = it->second.vCurrentPos;
					//	glm::vec2 gridMouse = worldMouse / (float)BRICK_SIZE;

					//	glm::vec2 lookDir = gridMouse - currentPos;
					//	if (glm::length(lookDir) > 0.001f) {
					//		lookDir = glm::normalize(lookDir);
					//	}
					//	else {
					//		lookDir = { 1.0f, 0.0f };
					//	}

					//	// Храним ЧИСТЫЙ угол взгляда
					////	float angleDegrees = glm::degrees(std::atan2(lookDir.y, lookDir.x));
					////	it->second.rotation = angleDegrees - 90.0f;

					//	glm::vec2 currentInputDir = GetCurrentInputDir();
					//	sendUpdatePlayer(currentInputDir, lookDir);

					//	lastLookDir = lookDir;
					//	lastInputDir = currentInputDir;
					//	networkTickTimer = 0.0f;
					//}

					SendCastMessage(GameMsg::Game_CastSpell, slot);
				}
				if (action == GLFW_RELEASE) {
				SendCastMessage(GameMsg::Game_ReleaseSpell, slot);
				}
			}
		}
		else
		{
			
			std::vector<uint32_t> alivePlayers;
			for (auto const& [id, obj] : playerEntities) {
				// Ищем только живых игроков (здоровье больше 0)
				if (id != nPlayerID && obj->health > 0)
					alivePlayers.push_back(id);
			}

			if (!alivePlayers.empty()) {
				if (action == GLFW_PRESS) {

				if (button == GLFW_MOUSE_BUTTON_LEFT) {
					currentListIndex = (currentListIndex + 1) % (int)alivePlayers.size();
				}
				else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
					currentListIndex--;
					if (currentListIndex < 0) currentListIndex = (int)alivePlayers.size() - 1;
					}
				}

				spectatorTargetIndex = alivePlayers[currentListIndex];
			}
		}


		
	}

	// 3. Логика отпускания (Release)
	/*if (state == GameState::INGAME && action == GLFW_RELEASE)
	{
		if (button == GLFW_MOUSE_BUTTON_RIGHT)
		{
			CancelShield();
			
		}
	}*/
}
void Game::keyCallback(GLFWwindow* window, int key, int scancode, int action, int modes)  //callbackи клавиатуры 
{
	Game* game = static_cast<Game*>(glfwGetWindowUserPointer(window));
	if (!game) return;

	// обновляем состояние клавиш
	if (action == GLFW_PRESS)
		Keys[key] = true;
	else if (action == GLFW_RELEASE)
		Keys[key] = false;

	game->OnKey(key, action);


}

void Game::character_callback(GLFWwindow* window, unsigned int codepoint) {

	Game* game = static_cast<Game*>(glfwGetWindowUserPointer(window));
	if (!game) return;

	game->OnCharTyped(codepoint);
}

void Game::OnKey(int key, int action)  // только клавиатура 
{
	// 1. UI имеет приоритет
	if (currentScreen)
	{
		// Запоминаем текущее состояние перед вызовом
		GameState stateBefore = state;

		currentScreen->OnKey(key, action);

		// Если состояние изменилось (например, на INGAME), выходим
		if (state != stateBefore) return;
	}
	// 2. Игровая логика
	if (state == GameState::INGAME )
	{
		
		GameAction foundSlot = GameAction::Count;
		for (auto const& [slot, boundKey] : config.bindings) {
			if (boundKey == key) {
				foundSlot = slot;
				break;
			}
		}
		// ПРОВЕРКА: обрабатываем только если это СЛОТ, а не движение
		if (foundSlot >= GameAction::Slot1 && foundSlot <= GameAction::Slot11) {
			ActionSlot slot = KeyConfig::ToActionSlot(foundSlot);
			if (action == GLFW_PRESS && !isDead)
				SendCastMessage(GameMsg::Game_CastSpell, slot);
			else if (action == GLFW_RELEASE)
				SendCastMessage(GameMsg::Game_ReleaseSpell, slot);
			return;
		}

		if (key == GLFW_KEY_ENTER && action == GLFW_PRESS)
			PrepareChatInput();

		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
			OpenOptions(GameState::INGAME);
		//	SetState(GameState::OPTIONS);
		}
		//	glfwSetWindowShouldClose(glfwGetCurrentContext(), true);

		

		//if (key == GLFW_KEY_TAB && action == GLFW_PRESS) {

		//		// Отрисовка классового UI
		//		auto it = playerEntities.find(nPlayerID);
		//		if (it != playerEntities.end()) {
		//			const auto& player = it->second;
		//			if (player.nTypeAndSub == (int)PlayerClass::Hunter)
		//				SetState(GameState::CRAFTING);

		//			
		//		}
		//	}
				
	}
	else if (state == GameState::TYPINGCHAT && action == GLFW_PRESS)
	{
		if (key == GLFW_KEY_ENTER)
			ReleaseChatInput();

		if (key == GLFW_KEY_ESCAPE)
			glfwSetWindowShouldClose(glfwGetCurrentContext(), true);

		if (key == GLFW_KEY_BACKSPACE)
			delete_Char();
	}
	else if (state == GameState::CRAFTING && action == GLFW_PRESS)
	{
		if (key == GLFW_KEY_TAB) {
			//state = GameState::CRAFTING;
			SetState(GameState::INGAME);

		
		}
	}

}

void Game::renderObject()
{
	// настройка видимости для текста
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, fog.GetFogTexture()); // Та самая fogTexture
	text->GetShader().use();
	text->GetShader().setInt("fogTex", 1);
	text->GetShader().setBool("WorldSpace", 1); // переключаем мод
	text->GetShader().setVec2("levelSize", GameLevel->LevelSize);



	glm::vec2 worldMouse = GetMouseWorldPos();
	glm::vec2& playerPos = mapVisuals[nPlayerID].vCurrentPos;
	uint8_t observerTeam = playerEntities[spectatorTargetIndex]->teamId;

	// Вектор, куда мы сгребаем все слои всех персонажей перед отправкой на GPU
	std::vector<SpriteInstance> instanceBatch;

	// -------------------------------------------------------------------------
	// СБОРДАННЫХ ДЛЯ ИНСТАНСИНГА
	// -------------------------------------------------------------------------
	for (auto& visual : mapVisuals)
	{
		uint32_t id = visual.first;
		if (playerEntities.find(id) == playerEntities.end()) continue;

		auto& data = playerEntities[id];
		glm::vec2 currentPos = visual.second.vCurrentPos; // ПЛАВНАЯ ПОЗИЦИЯ В ГРИДЕ

		// СКИПАЕМ, если объект за экраном
		if (!IsVisible(currentPos, data->radius)) continue;

		// Переводим мировую позицию центра персонажа из грида в пиксели уровня
		glm::vec2 worldPosPixels = currentPos * (float)BRICK_SIZE;

		// Определяем цвет команды персонажа
		glm::vec4 entityColor;
		if (data->teamId == observerTeam) {
			if (data->id == spectatorTargetIndex)
				entityColor = glm::vec4(0.0f, 1.0f, 0.4f, 1.0f); // Я
			else
				entityColor = glm::vec4(0.2f, 0.6f, 1.0f, 1.0f); // Союзники
		}
		else {
			entityColor = glm::vec4(1.0f, 0.3f, 0.3f, 1.0f);     // Враги
		}

		// Сейчас (быстро, чисто, динамически):
		StringId classId = data->GetClassId(); // Получаем числовой ID (например, сохраненный "Mage"_sid)
		const CharacterInfo* charInfo = CharacterDatabase::Instance().GetCharacter(classId); // Мгновенный поиск

		if (charInfo)
		{
			auto myChar = playerEntities[id].get();
			if (!myChar) return;

			// Вектор движения
			glm::vec2 moveDir = myChar->inputVel;
			bool isMoving = (glm::length(moveDir) > 0.01f);

			// 1. Честный базовый угол персонажа БЕЗ корректировок рендеринга (+270)
			// Он нужен для правильной тригонометрии глаз!
			float baseAngleDegrees = 0.0f;
		
			//	if (id == nPlayerID) {
				//	baseAngleDegrees = visual.second.rotation;
			/*	}
				else {
					glm::vec2 lookDir = myChar->direction;
					if (glm::length(lookDir) > 0.001f) {
						baseAngleDegrees = glm::degrees(std::atan2(lookDir.y, lookDir.x));
					}
				}*/
			

			// 2. Угол для отрисовки спрайтов (с коррекцией под вашу графику)
			float sharedAngleDegrees = baseAngleDegrees + 270.0f;
			if (sharedAngleDegrees >= 360.0f) sharedAngleDegrees -= 360.0f;
			if (sharedAngleDegrees < 0.0f) sharedAngleDegrees += 360.0f;

			// 3. Тригонометрия на базе ЧИСТОГО угла (без +270), чтобы деротация глаз работала корректно
			float rad = glm::radians(baseAngleDegrees);
			float cosA = std::cos(rad);
			float sinA = std::sin(rad);

			// Достаем ссылку на менеджер анимаций
			auto& animController = visual.second.animComp;

		

			// Рендерим слои в строгом Z-порядке
			for (const auto& [zIndex, layer] : charInfo->layers)
			{
				glm::vec2 currentLayerSize = layer.size * (float)BRICK_SIZE;

				if (zIndex == 0) // --- СЛОЙ 0: НОГИ ---
				{
					// Определяем нужное состояние ног
					StringId legAnim = isMoving ? "walk"_sid : "idle"_sid;

					// Менеджер сам сбросит таймер слоя 0, если анимация изменилась
					animController.PlayAnimation(0, legAnim);

					auto it = layer.animations.find(legAnim);
					if (it != layer.animations.end()) {
						const auto& anim = it->second;

						// Считаем кадр по таймеру менеджера анимаций
						float animTimer = animController.GetTimer(0);
						int currentFrame = (int)(animTimer * anim.animSpeed) % anim.framesCount;

						// Ноги слегка выталкиваем по направлению движения
						glm::vec2 legsVisualOffset(0.0f);
						if (isMoving) {
							legsVisualOffset = glm::normalize(moveDir) * 8.0f;
						}

						SpriteInstance legsInstance;
						legsInstance.position = (worldPosPixels + legsVisualOffset) - (currentLayerSize * 0.5f);
						legsInstance.size = currentLayerSize;
						legsInstance.rotation = visual.second.rotation;
						legsInstance.color = glm::vec4(1.0f);
						legsInstance.uvOffset = anim.uvOffset;
						legsInstance.uvOffset.x += currentFrame * anim.uvScale.x;
						legsInstance.uvScale = anim.uvScale;

						instanceBatch.push_back(legsInstance);
					}
				}
				else if (zIndex == 1) // --- СЛОЙ 1: ГОЛОВА И РУКИ (ТУЛОВИЩЕ) ---
				{
					auto myChar = playerEntities[id].get();
					if (!myChar) return;

					StringId currentAnimState = animController.GetCurrentAnim(1);
					 sharedAngleDegrees = 0.0f;

					
					if (currentAnimState == "prepare_action"_sid || currentAnimState == "attack_action"_sid)
					{
						
						// ИСПРАВЛЕНО: Для чужих игроков всегда берем direction (взгляд) вместо inputVel (движение)
						glm::vec2 lookDir = myChar->direction;

						// Фолбэк на самый крайний случай (если с сервера прилетел нулевой вектор)
						if (glm::length(lookDir) < 0.001f) {
							lookDir = glm::vec2(1.0f, 0.0f);
						}
						sharedAngleDegrees = glm::degrees(std::atan2(lookDir.y, lookDir.x)) + 270.0f;
					}
					else // Обычное состояние (Покой или Бег)
					{
						
							sharedAngleDegrees = visual.second.rotation;
						
					}

					// Ищем анимацию в JSON конфигурации слоя
					auto it = layer.animations.find(currentAnimState);
					if (it == layer.animations.end()) {
						it = layer.animations.find("default"_sid); // Фолбэк
					}

					if (it != layer.animations.end()) {
						const auto& anim = it->second;

						// 2. Считаем кадр, используя динамическую скорость из компонента анимации
						float animTimer = animController.GetTimer(1);
						float currentSpeed = animController.GetAnimSpeed(1); // Используем наш геттер скорости

						int currentFrame = (int)(animTimer * 4.0f) % anim.framesCount;

						// Сборка спрайта
						SpriteInstance headInstance;
						headInstance.position = worldPosPixels - (currentLayerSize * 0.5f);
						headInstance.size = currentLayerSize;
						headInstance.rotation = sharedAngleDegrees;
						headInstance.color = glm::vec4(1.0f);
						headInstance.uvOffset = anim.uvOffset;
						headInstance.uvOffset.x += currentFrame * anim.uvScale.x;
						headInstance.uvScale = anim.uvScale;

						instanceBatch.push_back(headInstance);
					}
				}
				else if (zIndex == 2) // --- СЛОЙ 2: ГЛАЗА ---
				{
					StringId currentAnimState = animController.GetCurrentAnim(1);

					if (currentAnimState != "prepare_action"_sid && currentAnimState != "attack_action"_sid)
					{
						animController.PlayAnimation(2, "default"_sid);

						auto it = layer.animations.find("default"_sid);
						if (it != layer.animations.end()) {
							const auto& anim = it->second;

							float animTimer = animController.GetTimer(1);
							int currentFrame = (int)(animTimer * 4.0f) % anim.framesCount;

							// 1. Вычисляем МИРОВОЙ вектор взгляда
							glm::vec2 worldLookDir(0.0f);
							if (nPlayerID == id) {
								
								glm::vec2 dirToMouse = worldMouse - worldPosPixels;
								float distToMouse = glm::length(dirToMouse);
								if (distToMouse > 0.01f) {
									// Ограничиваем радиус движения зрачка (например, в пределах 32 пикселей)
									float factor = glm::clamp(distToMouse / 32.0f, 0.0f, 1.0f);
									worldLookDir = (dirToMouse / distToMouse) * layer.lookDistance * factor;
								}
							}
							/*else if (isMoving) {
								worldLookDir = glm::normalize(moveDir) * layer.lookDistance;
							}*/
							else {
								// Чужие игроки стоят: зрачки смотрят по направлению их взгляда
								glm::vec2 foreignLook = myChar->direction;
								if (glm::length(foreignLook) > 0.001f) {
									worldLookDir = glm::normalize(foreignLook) * layer.lookDistance;
								}
							}

							// 2. ИСПРАВЛЕНО: Переводим мировой вектор взгляда в локальные координаты головы.
							// Так как cosA и sinA теперь чистые, инвертируем ось Y (в 2D экранах Y идет вниз)
							glm::vec2 localLookOffset(
								worldLookDir.x * cosA + worldLookDir.y * sinA,
								-worldLookDir.x * sinA + worldLookDir.y * cosA
							);

							// 3. Расчет позиций для каждой глазницы
							for (const auto& localOffset : layer.localPositions) {
								glm::vec2 totalLocalOffset = localOffset + localLookOffset;

								// Поворачиваем локальную позицию обратно в мировые координаты для отрисовки
								glm::vec2 rotatedPos(
									totalLocalOffset.x * cosA - totalLocalOffset.y * sinA,
									totalLocalOffset.x * sinA + totalLocalOffset.y * cosA
								);

								SpriteInstance eyeInstance;
								eyeInstance.position = (worldPosPixels + rotatedPos) - (currentLayerSize * 0.5f);
								eyeInstance.size = currentLayerSize;
								eyeInstance.rotation = sharedAngleDegrees; // Спрайт глаза вращается вместе с головой
								eyeInstance.color = glm::vec4(1.0f);
								eyeInstance.uvOffset = anim.uvOffset;
								eyeInstance.uvOffset.x += currentFrame * anim.uvScale.x;
								eyeInstance.uvScale = anim.uvScale;

								instanceBatch.push_back(eyeInstance);
							}
						}
					}
					} // Конец zIndex == 2
			}
		}

		// -------------------------------------------------------------------------
		// ОТДЕЛЬНЫЙ РЕНДЕР ТЕКСТА И HP-БАРОВ (Они выводятся поверх персонажей)
		// -------------------------------------------------------------------------
		auto it = sessionPlayers.find(data->id);
		if (it != sessionPlayers.end())
		{
			float scale = 0.5f;
			text->GetShader().use();
			text->GetShader().setVec2("charTilePos", currentPos);
			std::string info = "lvl:" + std::to_string(playerEntities[data->id]->GetProgression()->GetLevel());

			float textWidth = text->MeasureTextWidth(it->second.name, 1.0f) * scale;
			float infoWidth = text->MeasureTextWidth(info, 1.0f) * 0.25f;

			float centeredX = (currentPos.x * BRICK_SIZE) - (textWidth / 2.0f);
			float lvlCenteredX = (currentPos.x * BRICK_SIZE) - (infoWidth / 2.0f);

			text->RenderText(it->second.name, centeredX, (currentPos.y + data->radius + 0.2f) * BRICK_SIZE, 0.5f);
			text->RenderText(info, lvlCenteredX, (currentPos.y - data->radius - 0.5f) * BRICK_SIZE, 0.25f);
		}

		if (data->health > 0)
		{
			HealthBar bar;
			bar.current = data->health;
			bar.max = data->maxHealth;

			Shader& glyphShader = ResourceManager::GetShader(ShaderID::World);
			glyphShader.use();
			glyphShader.setVec2("charTilePos", currentPos);

			uint32_t currEffectMask = data->GetEffects()->getCurrEffectMask();
			hpBars.DrawBar(Renderer, currentPos, data->radius, bar, (float)BRICK_SIZE, currEffectMask, data->shieldHp);
			if (currEffectMask > 0) {
				hpBars.DrawStatusEffects(Renderer, currentPos, data->radius, currEffectMask, (float)BRICK_SIZE);
			}
		}
	}
	instRenderer->DrawInstances(ResourceManager::GetShader(ShaderID::instanceWorld), ResourceManager::GetTexture("Character_atlas"),instanceBatch);

	
//	glm::vec2& playerPos = mapVisuals[nPlayerID].vCurrentPos;


	Shader& glyphShader = ResourceManager::GetShader(ShaderID::World);
	glyphShader.use();
	//ResourceManager::GetShader(ShaderID::World).use();
	//char PlayerChar = '$'; // 36 

	//Shader& worldShader = ResourceManager::GetShader(ShaderID::World);
	//worldShader.use();
	//worldShader.setVec2("u_screenRes", glm::vec2(Width, Height));

	

	/*glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, fog.fogTexture);
	worldShader.setSampler2D("fogTex", 5);*/


	char ProjectileChar = '@'; //64
	//glm::vec2 worldMouse = GetMouseWorldPos();
	glm::vec2 gridMouse = worldMouse / (float)BRICK_SIZE;
	float fogCellRadius = fog.viewRadius / BRICK_SIZE;
	//uint8_t observerTeam = playerEntities[spectatorTargetIndex]->teamId;


	for (auto& visual : mapProjVisuals)
	{  


		// Нам нужны данные о радиусе и ID из логической карты
		// Используем ID визуального объекта, чтобы найти его описание
		uint32_t id = visual.first;
		if (projectiles.find(id) == projectiles.end()) continue; // На всякий случай

		auto& data = projectiles[id];
		glm::vec2 currentPos = visual.second.vCurrentPos; // ПЛАВНАЯ ПОЗИЦИЯ


		float diameter = data.fRadius * 2.0f;
		// СКИПАЕМ, если за экраном
		if (!IsVisible(currentPos, data.fRadius)) continue;
		glyphShader.use();
		glyphShader.setFloat("radius", data.fRadius);
		glyphShader.setVec2("direction", gridMouse - currentPos);
		
		if (data.nOwnerID == nPlayerID)
		{
			glyphShader.setBool("ProjectileOwner", true);

		}

		Renderer->DrawSprite(glyphShader,ProjectileChar, { (currentPos.x - data.fRadius) * BRICK_SIZE , (currentPos.y - data.fRadius) * BRICK_SIZE },
			{BRICK_SIZE * diameter,BRICK_SIZE * diameter });

		// Сразу сбрасываем direction
		glyphShader.setVec2("direction", { 0.0f, 0.0f });

		// Затем текст 
		float dist = glm::distance(currentPos, playerPos); // в метрах/клетках
		if (dist <= fogCellRadius) {
			// Затем текст (если он не использует этот шейдер, можно не беспокоиться)
			std::string sMenu = std::to_string(data.nUniqueID);
			glm::vec2 textPos = { currentPos.x * BRICK_SIZE,(currentPos.y + data.fRadius + 0.2f) * BRICK_SIZE };
			text->GetShader().use();
			text->GetShader().setVec2("charTilePos", currentPos);
			text->RenderText(sMenu, textPos.x, textPos.y, 0.5);
		}
	}

	// ❤️ HP бары
//	hpBars.Render(Renderer, mapObjects,mapVisuals, BRICK_SIZE);

	
	
}

void Game::renderVFX()
{
	Shader& glyphShader = ResourceManager::GetShader(ShaderID::World);
	for (auto& e : activeExplosions)
	{
		float t = 1.0f - (e.timer / 0.5f);       // 0 → 1
		e.currentRadius = e.diametr * t;   // растущий радиус

		float centerDifference = (BRICK_SIZE * e.currentRadius) / 2.0f;

		glyphShader.use();
		glyphShader.setFloat("radius", e.currentRadius);

		char ExplosionChar = '@';
		Renderer->DrawSprite(glyphShader,ExplosionChar,
			{ e.pos.x * BRICK_SIZE - centerDifference, e.pos.y * BRICK_SIZE - centerDifference },
			{ BRICK_SIZE * e.currentRadius, BRICK_SIZE * e.currentRadius });

		glyphShader.setVec2("direction", { 0.0f, 0.0f });
	}
	auto it = activeDamages.begin();
	while (it != activeDamages.end()) {
	
			// 2. Отрисовка
			// Используй свой TextShader. 
			// Если текст должен быть в мире (над головой), WorldSpace = 1
			text->GetShader().use().setBool("WorldSpace", 0);

			std::string displayStr = it->value;
			if (it->isResisted) displayStr += "#"; // Помечаем резист воина



			// Прозрачность через lifetime
			float alpha = std::min(it->alpha, it->lifetime * 2.0f);
			text->RenderText(displayStr, it->pos.x * BRICK_SIZE, it->pos.y * BRICK_SIZE, it->scale, it->color * alpha);

			++it;
		
	}
	DrawChains(Renderer);


}

void Game::updateObjects(float fElapsedTime)
{
	
}

void Game::ProcessInput(float dt)
{

}


void Game::PrepareChatInput()
{
	state = GameState::TYPINGCHAT;
	chatMessage.clear();
	
}

void Game::ReleaseChatInput()  
{
	state = GameState::INGAME;

	if (chatMessage.empty())
		return;

	olc::net::message<GameMsg> msg;
	msg.header.id = GameMsg::chat_message;

	msg << chatMessage;   // ТОЛЬКО текст
	msg << nPlayerID;
	Send(msg);

	chatMessage.clear();
}


void Game::TryLogin(const std::string& name, const std::string& pass)
{

	sLoginRequest req;
	req.sessionToken = nSessionToken;

	// 1. Очищаем буферы нулями
	std::memset(req.firstName, 0, sizeof(req.firstName));
	std::memset(req.password, 0, sizeof(req.password));

	// 2. Копируем имя (безопасно: берем минимум от размера строки и размера буфера минус 1 для \0)
	size_t nameLen = std::min(name.size(), sizeof(req.firstName) - 1);
	std::memcpy(req.firstName, name.c_str(), nameLen);

	// 3. Копируем пароль
	size_t passLen = std::min(pass.size(), sizeof(req.password) - 1);
	std::memcpy(req.password, pass.c_str(), passLen);

	std::cout << req.password << std::endl;

	olc::net::message<GameMsg> msg;
	msg.header.id = GameMsg::Client_Login;
	msg << req;
	Send(msg);
}

void Game::TryRegister(const std::string& name, const std::string& pass)
{
	sLoginRequest req;
	req.sessionToken = 0; // При регистрации токена еще нет

	std::memset(req.firstName, 0, sizeof(req.firstName));
	std::memset(req.password, 0, sizeof(req.password));

	// Копируем имя
	size_t nameLen = std::min(name.size(), sizeof(req.firstName) - 1);
	std::memcpy(req.firstName, name.c_str(), nameLen);

	// Копируем пароль
	size_t passLen = std::min(pass.size(), sizeof(req.password) - 1);
	std::memcpy(req.password, pass.c_str(), passLen);

	olc::net::message<GameMsg> msg;
	msg.header.id = GameMsg::Client_RegisterWithServer;
	msg << req;
	Send(msg);
}

void Game::delete_Char(){
	if (!chatMessage.empty()) chatMessage.pop_back(); // удаляем один char

}

void Game::DeleteLoginChar()
{
	if (!playerProfile.name.empty()) playerProfile.name.pop_back();
}

//
//void Game::sendDetonateSticky()
//{
//	olc::net::message<GameMsg> msg;
//	msg.header.id = GameMsg::Game_DetonateSticky;
//	Send(msg); // на сервер
//}

void Game::CraftArrow(ArrowType type)
{
	// 1. Предварительная проверка на клиенте (опционально, для экономии трафика)
   // if (GetLocalPlayer().infoPoints < 3) return; 

   // 2. Формируем сетевое сообщение
	olc::net::message<GameMsg> msg;
	msg.header.id = GameMsg::Game_CraftArrow;
	msg << (uint8_t)type; // Приводим к базовому типу для передачи

	// 3. Отправляем на сервер через ваш сетевой клиент
	Send(msg);
}

void Game::castSlot(ActionSlot slot)
{


	olc::net::message<GameMsg> msg;
	msg.header.id = GameMsg::Game_CastSpell;
	msg << slot;
	

	Send(msg);
}

void Game::CancelMatchMaking()
{
	olc::net::message<GameMsg> msg;
	msg.header.id = GameMsg::Client_CancelMatchmaking;
	Send(msg);
	

}

void Game::AppendLoginChar(unsigned int ch)
{
	if (ch >= 32 && ch <= 126) // только ASCII
		playerProfile.name.push_back((char)ch);
}

void Game::OnCharTyped(unsigned int ch)
{
	if (currentScreen)
	{
		currentScreen->OnChar(ch);
		//return;
	}

	if(state == GameState::TYPINGCHAT)
	if (ch >= 32 && ch <= 126) { // Printable ASCII Engl

		// Добавь символ к строке чата
		chatMessage += static_cast<char>(ch);

	}
}

void Game::FindMatch()
{
	olc::net::message<GameMsg> msg;
	msg.header.id = GameMsg::Client_FindMatch;
	Send(msg);

	SetState(GameState::MATCHMAKING);
}
//
//void Game::StartGame()
//{
//	state = GameState::INGAME;
//	
//	currentScreen = nullptr;
//
//	playerProfile.selectedclass = selectedClass;
//
//	//ConnectToServer();
//
//}

void Game::OpenShop()
{
	SetState(GameState::SHOP);
}

void Game::OpenFriends()
{
	
	SetState(GameState::FRIENDS);
}

void Game::OpenOptions(GameState state)
{
	// Передаем этот стейт в UI
	if (auto* opt = dynamic_cast<OptionsUI*>(uiScreens[GameState::OPTIONS])) {
		opt->SetReturnState(state);
	}
	SetState(GameState::OPTIONS);
}

void Game::OpenLobby()
{
	SetState(GameState::LOBBY);
}
void Game::OpenRegistration()
{
	SetState(GameState::REGISTRATION);
}
void Game::OpenInGame()
{
	SetState(GameState::INGAME);
}
void Game::DrawChains(SpriteRenderer* sr)
{
	Shader& shader = ResourceManager::GetShader(ShaderID::World);
	shader.use();

	for (auto& chain : activeChains) {
		glm::vec2 diff = chain.end - chain.start;
		float dist = glm::length(diff);
		glm::vec2 dir = diff / dist;

		// Количество сфер зависит от расстояния (каждые 8 пикселей — сфера)
		int segments = (int)(dist * 32.0f / 8.0f);

		for (int i = 0; i <= segments; i++) {
			float t = (float)i / (float)segments;  // от 0 -> до 1 
			glm::vec2 segmentPos = chain.start + dir * (t * dist);

			// Эффект волны/вибрации для цепи
			float wave = std::sin(glfwGetTime() * 10.0f + t * 5.0f) * 0.05f;
			segmentPos.x += wave;

			// Рисуем маленькую сферу (символ '*' или '.')
			float size = 4.0f + std::sin(glfwGetTime() * 15.0f + i) * 2.0f; // Пульсация
			sr->DrawSprite(shader, '@', segmentPos * 32.0f, { size, size }, 0.0f, chain.color); //отрисуем как проджектайл
		}

		// Рисуем "Якорь" в точке втыкания
		sr->DrawSprite(shader, '#', chain.end * 32.0f - glm::vec2(4, 4), { 8, 8 }, 45.0f, chain.color);
	}
}
void Game::OpenLogin()
{
	SetState(GameState::LOGIN);
}
//
//void Game::CancelShield()
//{
//	olc::net::message<GameMsg> msg;
//	msg.header.id = GameMsg::Game_CancelSpell;
//	msg << SpellId::Shield;
//	Send(msg);
//}

const CooldownInfo* Game::GetCooldown(ActionSlot slot) const
{
	auto it = playerEntities.find(nPlayerID);
	if (it != playerEntities.end()) {
		const auto& player = it->second;

		// Теперь GetBoundSpell сработает полиморфно (вернет спелл мага, если это маг)
		SpellId id = player->GetBoundSpell(slot);

		// Возвращаем данные для UI (иконка, время)
		return GetCooldownBySpellId(id);
	}
	return nullptr;
}

bool Game::IsOnCooldown(ActionSlot slot) const
{
	return GetCooldown(slot) != nullptr;
}


void Game::ShowStatus(const std::string& msg, glm::vec4 color, float duration)
{
	statusMessage = msg;
	statusColor = color;
	statusTimer = duration;
}

bool Game::connectToServer()
{
	if (Connect("127.0.0.1", 60000))
	{
		return true;
	}
	return false;
}

void Game::UpdateLogin(const float& dt)
{

}

void Game::UpdateLobby(const float& dt)
{

}

void Game::UpdateMatchMaking(const float& dt)
{
}

void Game::UpdateGameplay(const float& dt)
{

	if (bWaitingForConnection)
		return;
//	if (state != GameState::INGAME) return;
	//std::cout << (int)playerEntities[nPlayerID]->health << std::endl;
	/*if (playerEntities[nPlayerID]->health <= 0)
		isDead = true;*/

	serverTime += dt;
	networkTickTimer += dt;
	pingTimer += dt;

	// Приводим время к диапазону 0.0 - 1.0 (где 0.5 - полдень, 1.0 - полночь)
	dayCycleProgress = fmodf(serverTime, DAY_CYCLE_DURATION) / DAY_CYCLE_DURATION;  // обновление день/ночь
	// Вычисляем интенсивность освещения (0.0 - ночь, 1.0 - яркий день)
	lightIntensity = (cosf(dayCycleProgress * 2.0f * 3.14159f + 3.14159f) + 1.0f) * 0.5f;

	fpsCounter++;
	if (pingTimer >= 1.0f) {
		olc::net::message<GameMsg> msg;
		msg.header.id = GameMsg::Server_Ping;

		// Записываем текущее время прямо в пакет
		uint64_t timeNow = std::chrono::system_clock::now().time_since_epoch().count();
		msg << timeNow;

		Send(msg);
		pingTimer = 0;

		currentFPS = fpsCounter;
		fpsCounter = 0;

		currentInboundKBps = tempInboundBytes / 1024; // Делим на 1024, чтобы получить КБ
		tempInboundBytes = 0;

	}

	fog.ClearLight(); // 1. Очищаем перед циклом

	// 1. Мировая позиция персонажа
	glm::vec2 playerWorldPos = mapVisuals[spectatorTargetIndex].vCurrentPos * (float)BRICK_SIZE;
	float playerRadius = MIN_RADIUS + (MAX_RADIUS - MIN_RADIUS) * lightIntensity;
	fog.viewRadius = playerRadius;
	fog.AddLight(playerWorldPos, playerRadius);
	fog.Update(playerWorldPos, 64.0f); // ещё один свет - от гг как от маяка
	// --- Б. Добавляем Компаньонов и других союзников ---
	for (auto const& [id, entity] : playerEntities) {
		if ( entity->teamId == playerEntities[spectatorTargetIndex]->teamId) {
			// Берем позицию из визуальной части (чтобы свет не дергался)
			glm::vec2 compPos = mapVisuals[id].vCurrentPos * 32.0f;
			fog.AddLight(compPos, MIN_RADIUS); // Константный или зависимый от дня радиус
		}
	}


	// 1. Сначала плавно двигаем ВСЕ визуальные объекты к их целям и считаем углы/анимации
	glm::vec2 worldMouse = GetMouseWorldPos();

	for (auto& [id, visual] : mapVisuals) {
		auto entityIt = playerEntities.find(id);
		if (entityIt != playerEntities.end()) {
			auto& entity = entityIt->second;

			// 1. Рассчитываем целевой угол движения СТРОГО по inputVel (для всех!)
			float targetAngleDegrees = visual.GetSmoothedHeadAngle(); // По умолчанию сохраняем текущий

			glm::vec2 moveDir = entity->inputVel;
			bool isMoving = (glm::length(moveDir) > 0.01f);

			if (isMoving) {
				// Считаем чистый математический угол движения от atan2 (БЕЗ доворотов на +270!)
				// Довороты под текстуру мы сделаем уже непосредственно в рендере.
				targetAngleDegrees = glm::degrees(std::atan2(moveDir.y, moveDir.x));
			}

			// 2. Обновляем визуал (внутри плавно сглаживается fCurrentHeadAngle)
			visual.Update(dt, targetAngleDegrees, isMoving);
		}
		else {
			visual.Update(dt, visual.GetSmoothedHeadAngle(), false);
		
		}

	}
	// =========================================================================
//  ОБНОВЛЕНИЕ ВИЗУАЛА СНАРЯДОВ (Используя единый VisualObject)
// =========================================================================
	for (auto& [id, visual] : mapProjVisuals) {
		auto projIt = projectiles.find(id);
		if (projIt != projectiles.end()) {
			auto& proj = projIt->second;

			// 1. Угол снаряда всегда направлен туда, куда он летит (по вектору скорости vVel)
			float targetAngleDegrees = visual.GetSmoothedHeadAngle();

			// Проверяем, летит ли снаряд (длина вектора скорости больше нуля)
			bool isFlying = (glm::length(proj.vVel) > 0.01f);

			if (isFlying) {
				// Рассчитываем чистый математический угол полета снаряда
				targetAngleDegrees = glm::degrees(std::atan2(proj.vVel.y, proj.vVel.x));
			}

			// 2. Вызываем ваш единый метод Update
			// Передаем isFlying вместо isMoving, чтобы анимация снаряда (если она есть) крутилась в полете
			visual.Update(dt, targetAngleDegrees, isFlying);
		}
		else {
			// Если снаряд потерял логическую сущность, плавно затухаем его (bPendingRemoval)
			// и передаем false, останавливая анимации
			visual.Update(dt, visual.GetSmoothedHeadAngle(), false);
		}
	}

	// Плавный поворот по кратчайшему пути
	float deltaAngle = targetWindAngle - currentWindAngle;

	// Нормализация разности (чтобы не крутить полный круг)
	if (deltaAngle > PI) deltaAngle -= 2.0f * PI;
	if (deltaAngle < -PI) deltaAngle += 2.0f * PI;

	currentWindAngle += deltaAngle * dt * 5.0f;

	// Держим текущий угол в пределах 0...2PI для стабильности
	if (currentWindAngle > 2.0f * PI) currentWindAngle -= 2.0f * PI;
	if (currentWindAngle < 0.0f) currentWindAngle += 2.0f * PI;

	GameLevel->Update(dt);
	keyEvents();        // WASD, mouse
	updateObjects(dt); // физика, позиции



	// Обновляем туман с новым радиусом
	//fog.Update(worldPos, currentRadius);
	UpdateVFX(dt);

	// Этот блок идет в самом конце обновления снарядов в UpdateGameplay(dt):
	for (auto it = mapProjVisuals.begin(); it != mapProjVisuals.end(); )
	{
		// Если флаг удаления взведен И свет/альфа-канал полностью угасли
		if (it->second.bPendingRemoval && it->second.fLightIntensity <= 0.0f)
		{
			// Метод erase() удаляет элемент и возвращает валидный итератор на следующий
			it = mapProjVisuals.erase(it);
		}
		else
		{
			// Если снаряд еще "живет" или угасает, просто идем дальше
			++it;
		}
	}

}

void Game::UpdateVFX(const float& dt)
{
	for (auto it = activeExplosions.begin(); it != activeExplosions.end(); )
	{
		it->timer -= dt;
		fog.AddLight(it->pos * 32.0f,it->currentRadius * 32.0f);
		if (it->timer <= 0.0f)
			it = activeExplosions.erase(it);
		else
			++it;
	}
	auto it = activeDamages.begin();

	while (it != activeDamages.end()) {
		// 1. Анимация: летит вверх и плавно исчезает
		it->pos.y -= dt * 1.2f;
		it->lifetime -= dt;

		if (it->lifetime <= 0) {
			it = activeDamages.erase(it);
		}
		else {
			
			++it;
		}
	}
	for (auto vc = activeChains.begin(); vc != activeChains.end(); )
	{
		vc->lifetime -= dt;

		// 1. Поиск игрока в карте объектов
		auto itTarget = playerEntities.find(vc->targetNetId);
		if (itTarget != playerEntities.end()) {
			vc->start = itTarget->second->position; // Начало всегда на жертве
		}

		if (vc->linkedNetId != -1) {
			// Динамическая связь (игрок-игрок)
			auto itLinked = playerEntities.find(vc->linkedNetId);
			if (itLinked != playerEntities.end()) {
				vc->end = itLinked->second->position; // Конец следует за вторым игроком
			}
		}

		// 2. Добавляем свет в точке якоря (стрелы)
		fog.AddLight(vc->end * 32.0f, 1.2f * 32.0f);

		// 3. Удаление по времени
		if (vc->lifetime <= 0.0f) {
			vc = activeChains.erase(vc);
		}
		else {
			++vc;
		}
	}
}

void Game::UpdateCamera(float dt)
{
	if (!nPlayerID) return;
	glm::vec2 playerPixelPos;
	if (!isDead) {

	playerPixelPos = mapVisuals[nPlayerID].vCurrentPos * (float)BRICK_SIZE;

	}
	else
	{
		playerPixelPos = mapVisuals[spectatorTargetIndex].vCurrentPos * (float)BRICK_SIZE;
	}
	// 2. Вычитаем половину экрана (делим на zoom, так как ortho масштабирует мир)
	camera.position.x = playerPixelPos.x - (Width / 2.0f / camera.zoom);
	camera.position.y = playerPixelPos.y - (Height / 2.0f / camera.zoom);

	// 3. (Опционально) Если хотите, чтобы в центре был именно центр спрайта игрока:
	camera.position.x += (BRICK_SIZE / 2.0f);
	camera.position.y += (BRICK_SIZE / 2.0f);
}

inline glm::vec2 Game::GridToScreen(int idx, int levelWidth)
{
	int x = idx % levelWidth;
	int y = idx / levelWidth;
	return glm::vec2(
		x * BRICK_SIZE + BRICK_SIZE * 0.5f ,  //центр клетки 
		y * BRICK_SIZE + BRICK_SIZE * 0.5f
	);
}

void Game::RenderGame()
{

	if (!bWaitingForConnection)
	{
		 // 1. Рисуем маску (обновляет fogTexture и memoryTexture)
        fog.RenderMask( Width, Height, dayCycleProgress);
		


		// Лямбда для быстрой настройки шейдеров карт
		auto setupShader = [&](Shader& shader) {
			shader.use();
			shader.setVec2("u_screenRes", { GameLevel->LevelSize.x * 32.0f, GameLevel->LevelSize.y * 32.0f });
			shader.setInt("fogTex", 5);
			shader.setInt("memoryTex", 6);
			shader.setInt("lightMapTex", 7);
			shader.setFloat("lightIntensity", lightIntensity);
			shader.setFloat("u_dayCycle", dayCycleProgress);
			shader.setFloat("u_time", serverTime);

			// Передаем актуальную матрицу камеры (чтобы мир двигался!)
			shader.setMat4("view", camera.GetViewMatrix());
			};

		// Биндим текстуры в слоты (они общие для всех шейдеров на текущий кадр)
		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_2D, fog.GetFogTexture());

		glActiveTexture(GL_TEXTURE6);
		glBindTexture(GL_TEXTURE_2D, fog.GetMemoryTexture());

		glActiveTexture(GL_TEXTURE7);
		glBindTexture(GL_TEXTURE_2D, fog.GetLightTexture());

		// НАСТРАИВАЕМ ОБА ШЕЙДЕРА:
		setupShader(ResourceManager::GetShader(ShaderID::World));
		setupShader(ResourceManager::GetShader(ShaderID::instanceWorld)); // <-- ТЕПЕРЬ ИНСТАНСИНГ ТОЖЕ В ТУМАНЕ И СВЕТЕ!

		// 2. Рисуем уровень
		GameLevel->Render(*Renderer, instRenderer, camera, tileDB);

        // 3. Накладываем туман ПАМЯТИ только на уровень
     //   fog.ApplyToMap();


		




        renderObject();
        // 4. Рисуем игроков и спецэффекты (они рисуются на фоне уже серой карты)
        renderVFX();

        // 5. Накладываем ТЕКУЩИЙ свет (скрывает игроков вне радиуса в черное)
      //  fog.ApplyToObjects();
		ResourceManager::GetShader(ShaderID::TextShader).use().setMat4("view", glm::mat4(1.0f)); // для текста матрицу view делаем неактуальной
		text->GetShader().use();
		text->GetShader().setBool("WorldSpace", 0); // переключаем мод
		if(state == GameState::TYPINGCHAT)
		RenderChat();


	}
}

bool Game::IsVisible(const glm::vec2& posInTiles, const float& radius)
{
	// Границы камеры в тайлах
	float startX = camera.position.x / BRICK_SIZE;
	float startY = camera.position.y / BRICK_SIZE;
	float endX = startX + (Width / camera.zoom / BRICK_SIZE);
	float endY = startY + (Height / camera.zoom / BRICK_SIZE);

	// Проверка с учетом радиуса объекта (чтобы не исчезал наполовину)
	return (posInTiles.x + radius >= startX && posInTiles.x - radius <= endX &&
		posInTiles.y + radius >= startY && posInTiles.y - radius <= endY);
}

void Game::UpdateNetwork(float& dt)
{
	// Check for incoming network messages
	if (IsConnected())
	{
		while (!Incoming().empty())
		{
			auto ownedMsg = Incoming().pop_front(); // Получаем весь объект
			auto& msg = ownedMsg.msg;
			// !!! СРАЗУ ЗАМЕРЯЕМ РАЗМЕР В БАЙТАХ !!!
			tempInboundBytes += msg.size(); // Если в message есть метод size()

			if (Incoming().count() > 10) {
				std::cout << "Client lagging! Messages in queue: " << Incoming().count() << std::endl;
			}

			switch (msg.header.id)
			{
			case(GameMsg::Client_Accepted):
			{
				std::cout << "Server accepted client - you're in!\n";
				bWaitingForConnection = false;

				break;
			}
			case GameMsg::Server_RegisterResult:
			{
				sLoginResult res;
				msg >> res;
				if (res.success) {
					ShowStatus("ACCOUNT CREATED! PLEASE LOGIN", { 0, 1, 0, 1 }); // Зеленый
					OpenLogin();
					// Здесь можно переключить UI на LOGIN автоматически
				}
				else {
					// Если в res.errorMessage пусто, выводим дефолт
					std::string err = (res.errorMessage[0] != '\0') ? res.errorMessage : "NAME ALREADY TAKEN";
					ShowStatus("ERROR: " + err, { 1, 0, 0, 1 }); // Красный
				}
				break;
			}

			case GameMsg::Server_LoginResult:
			{
				sLoginResult res;
				msg >> res;
				if (res.success) {
					nSessionToken = res.assignedToken;
					nPlayerID = res.assignedID;
					spectatorTargetIndex = nPlayerID;

					SetState(GameState::LOBBY);
					//SetState(GameState::VICTORY);
					std::string err = (res.errorMessage[0] != '\0') ? res.errorMessage : "NAME ALREADY TAKEN";
					ShowStatus(err, { 0, 1, 0, 1 });
				}
				else {
					std::string err = (res.errorMessage[0] != '\0') ? res.errorMessage : "NAME ALREADY TAKEN";
					ShowStatus("ERROR: " + err, { 1, 0, 0, 1 });
				}
				break;
			}
			case GameMsg::Server_SessionRestored:
			{
				// Здесь логика возврата в матч или просто в лобби без ввода пароля
				break;
			}



			//sLoginResult res;
			//msg >> res; // Теперь извлекаем структуру, содержащую токен

			//if (res.success)
			//{
			//	// СОХРАНЯЕМ ТОКЕН, который выдал сервер
			//	nSessionToken = res.assignedToken;
			//	nPlayerID = res.assignedID;

			//	std::cout << "Token saved: " << nSessionToken << "\n";
			//	std::cout << "nPlayerID " << nPlayerID << "\n";
			//	// 🔥 ВОТ ЭТОГО НЕ ХВАТАЛО
			//	sessionPlayers[nPlayerID].name = playerProfile.name;

			//	if (GetState() != GameState::INGAME) {
			//		SetState(GameState::LOBBY);
			//	}

			//	olc::net::message<GameMsg> msgReg;
			//	msgReg.header.id = GameMsg::Client_RegisterWithServer;

			//	Send(msgReg);
		//	}
		//	break;
		//}
			case GameMsg::Game_PlayerInfo:
			{
				sPlayerInfo info;
				msg >> info;

				sessionPlayers[info.playerId] = info;

				break;
			}
			case GameMsg::Server_SyncPlayerProfile:
			{
				// 1. Читаем вектор Friends (записан последним)
				uint32_t fSize = 0;
				msg >> fSize;
				playerProfile.friends.clear();
				playerProfile.friends.resize(fSize);
				// Читаем элементы с конца, так как они заходили в стек по порядку
				for (int i = (int)fSize - 1; i >= 0; i--) {
					msg >> playerProfile.friends[i];
				}

				// 2. Читаем вектор Unlocked
				uint32_t uSize = 0;
				msg >> uSize;
				playerProfile.unlocked.clear();
				playerProfile.unlocked.resize(uSize);
				for (int i = (int)uSize - 1; i >= 0; i--) {
					msg >> playerProfile.unlocked[i];
				}

				// 3. Читаем статистику (в обратном порядке от записи: losses -> wins -> coins)
				msg >> playerProfile.losses;
				msg >> playerProfile.wins;
				msg >> playerProfile.coins;

				// 4. Читаем имя (записано самым первым)
				msg >> playerProfile.name;

				break;

			}
			case(GameMsg::Game_AddPlayer):
			{

				sEntityDescription desc;
				msg >> desc;

				// Восстанавливаем позицию из int16_t
				glm::vec2 worldPos = { desc.posX / 100.0f, desc.posY / 100.0f };

				// 1. Обновляем визуальную цель (используем быструю вставку/обращение)
				auto& visual = mapVisuals[desc.nUniqueID];
				if (visual.ownerID == 0) {
					visual.vCurrentPos = worldPos;
					visual.ownerID = desc.nUniqueID;
				}
				visual.vTargetPos = worldPos;

				// Если это наш локальный игрок — снимаем флаг ожидания
				if (desc.nUniqueID == nPlayerID) {
					bWaitingForConnection = false;
				}

				ArchetypeId archId = static_cast<ArchetypeId>(desc.nArchetypeId);
				Character* entityPtr = nullptr;

				// 2. Если объекта еще нет в памяти — СОЗДАЕМ
				auto entityIt = playerEntities.find(desc.nUniqueID);
				if (entityIt == playerEntities.end()) {

					std::string pName = sessionPlayers[desc.nUniqueID].name;
					std::unique_ptr<Character> newEntity = nullptr;

					switch (archId)
					{
						// --- ИГРОКИ ---
					case ArchetypeId::Player_Mage:
						newEntity = std::make_unique<Mage>(desc.nUniqueID, pName);
						break;
					case ArchetypeId::Player_Warrior:
						newEntity = std::make_unique<Warrior>(desc.nUniqueID, pName);
						break;
					case ArchetypeId::Player_Hunter:
						newEntity = std::make_unique<Hunter>(desc.nUniqueID, pName);
						break;

						// --- МОБЫ ---
					case ArchetypeId::Mob_Melee:
						newEntity = std::make_unique<MeleeMob>(desc.nUniqueID, "Enemy");
						break;
					case ArchetypeId::Mob_Ranged:
						newEntity = std::make_unique<RangerMob>(desc.nUniqueID, "Enemy");
						break;

						// --- КОМПАНЬОНЫ ---
					case ArchetypeId::Companion_Warrior:
						newEntity = std::make_unique<WarriorCompanion>(desc.nUniqueID);
						break;
					case ArchetypeId::Companion_Priest:
						newEntity = std::make_unique<PriestCompanion>(desc.nUniqueID);
						break;
					case ArchetypeId::Companion_Hunter:
						newEntity = std::make_unique<RangerCompanion>(desc.nUniqueID);
						break;

					default:
						newEntity = std::make_unique<Character>(desc.nUniqueID, "Unknown");
						break;
					}

					// Настраиваем базовые параметры сущности
					newEntity->id = desc.nUniqueID;
					newEntity->entityType = archId;
					newEntity->teamId = desc.nTeamID;
					newEntity->position = worldPos;

					// Сохраняем указатель и переносим владение в мапу
					entityPtr = newEntity.get();
					playerEntities[desc.nUniqueID] = std::move(newEntity);
				}
				else {
					entityPtr = entityIt->second.get();
				}

				// 3. ОБНОВЛЯЕМ ДАННЫЕ (для всех: только что созданных и уже существующих)
				if (entityPtr) {
					float healthPercent = desc.nHealth / 255.0f;
					entityPtr->health = entityPtr->maxHealth * healthPercent;
					entityPtr->position = worldPos; // Синхронизируем позицию

					// --- ЛОГИКА ДОБАВЛЕНИЯ ЖИВОГО КОМПАНЬОНА В СКВАД ВЛАДЕЛЬЦА ---
					bool isCompanion = (archId >= ArchetypeId::Companion_Warrior && archId <= ArchetypeId::Companion_Hunter);
						// Ищем владельца в мапе (teamId компаньона равен ID игрока-владельца)
						auto ownerIt = playerEntities.find(entityPtr->teamId);

						if (ownerIt != playerEntities.end()) {
							Character* owner = ownerIt->second.get();

							// Проверяем, что владелец — это корректный класс игрока
							if (owner && owner->entityType >= ArchetypeId::Player_Warrior && owner->entityType <= ArchetypeId::Player_Hunter) {
								if (owner->GetSquad()) {
									owner->GetSquad()->AddCompanion(entityPtr);
								}
							}
						}
				}

				
				break;
			}

			case GameMsg::Server_MatchEnded:
			{
				sMatchEnd data;
				msg >> data;

				//mapObjects.clear();
				mapVisuals.clear();
				spectatorTargetIndex = nPlayerID;
				isDead = false;



				if (data.winnerID == nPlayerID)
					SetState(GameState::VICTORY);
				else
					SetState(GameState::DEFEAT);
				break;
			}
			case(GameMsg::Game_UpdateWorld):
			{
				// 1. Читаем количество игроков
				uint16_t numPlayers = 0;
				msg >> numPlayers;

				for (uint16_t i = 0; i < numPlayers; i++) {
					sEntityDescription player;
					msg >> player; // Извлекаем сжатую структуру 24 байта

					ArchetypeId archId = static_cast<ArchetypeId>(player.nArchetypeId);

					// Проверяем, существует ли уже сущность
					auto entityIt = playerEntities.find(player.nUniqueID);
					Character* entity = nullptr;

					// --- 1. ЕСЛИ ОБЪЕКТА ЕЩЕ НЕТ — СОЗДАЕМ ---
					if (entityIt == playerEntities.end()) {
						std::string pName = sessionPlayers[player.nUniqueID].name;
						std::unique_ptr<Character> newEntity = nullptr;

						switch (archId)
						{
							// --- ИГРОКИ ---
						case ArchetypeId::Player_Mage:
							newEntity = std::make_unique<Mage>(player.nUniqueID, pName);
							break;
						case ArchetypeId::Player_Warrior:
							newEntity = std::make_unique<Warrior>(player.nUniqueID, pName);
							break;
						case ArchetypeId::Player_Hunter:
							newEntity = std::make_unique<Hunter>(player.nUniqueID, pName);
							break;

							// --- МОБЫ ---
						case ArchetypeId::Mob_Melee:
							newEntity = std::make_unique<MeleeMob>(player.nUniqueID, "Enemy");
							break;
						case ArchetypeId::Mob_Ranged:
							newEntity = std::make_unique<RangerMob>(player.nUniqueID, "Enemy");
							break;

							// --- КОМПАНЬОНЫ ---
						case ArchetypeId::Companion_Warrior:
							newEntity = std::make_unique<WarriorCompanion>(player.nUniqueID);
							break;
						case ArchetypeId::Companion_Priest:
							newEntity = std::make_unique<PriestCompanion>(player.nUniqueID);
							break;
						case ArchetypeId::Companion_Hunter:
							newEntity = std::make_unique<RangerCompanion>(player.nUniqueID);
							break;

						default:
							newEntity = std::make_unique<Character>(player.nUniqueID, "Unknown");
							break;
						}

						// Настраиваем базовые параметры при создании
						newEntity->entityType = archId;
						newEntity->teamId = player.nTeamID;

						// Сохраняем указатель ДО std::move
						entity = newEntity.get();
						playerEntities[player.nUniqueID] = std::move(newEntity);
					}
					else {
						// Если объект уже был, просто берем сохраненный указатель
						entity = entityIt->second.get();
					}

					// --- 2. ОБНОВЛЯЕМ ДАННЫЕ СУЩНОСТИ (для всех: и новых, и старых) ---
					if (entity) {
						// Распаковка данных из сети
						glm::vec2 worldPos = { player.posX / 100.0f, player.posY / 100.0f };
						glm::vec2 worldVel = { player.velX / 127.0f, player.velY / 127.0f };
						float worldRadius = player.radius / 100.0f;
						float healthPercent = player.nHealth / 255.0f;

						// Присвоение параметров компонентам сущности
						entity->id = player.nUniqueID;
						entity->teamId = player.nTeamID;
						entity->position = worldPos;
						entity->inputVel = worldVel; // Для анимации
						entity->radius = worldRadius;
						entity->health = entity->maxHealth * healthPercent;
						entity->shieldHp = player.nShieldH / 255.0f;
						entity->chargeClient = player.fChargeRatio / 255.0f;

						float angle = (player.nDirection / 255.0f) * 2.0f * M_PI;
						entity->direction = glm::vec2(std::cos(angle), std::sin(angle));
						

						entity->GetEffects()->setCurrentEffectMask(player.nEffectsMask);

						// Логика удаления мертвого компаньона из UI / Сквада
						bool isCompanion = (archId >= ArchetypeId::Companion_Warrior && archId <= ArchetypeId::Companion_Hunter && entity->health <= 0);
						// 3. Проверяем, нашли ли мы владельца в мапе
						// 1. Сначала ТОЛЬКО ищем
						auto ownerIt = playerEntities.find(entity->teamId);

						// 2. ОБЯЗАТЕЛЬНО проверяем, что нашли (не равны концу карты)
						if (ownerIt != playerEntities.end()) {
							// 3. Только ТЕПЕРЬ безопасно достаем указатель!
							Character* owner = ownerIt->second.get();

							// 4. Проверяем, что владелец валиден и является игроком
							if (owner && owner->entityType >= ArchetypeId::Player_Warrior && owner->entityType <= ArchetypeId::Player_Hunter) {
								// 5. Безопасно удаляем компаньона из сквада
								if (owner->GetSquad()) {
									owner->GetSquad()->RemoveCompanion(player.nUniqueID);
								}
							}
						}

							// --- 3. ОБНОВЛЕНИЕ ВИЗУАЛА (Интерполяция) ---
							auto& visual = mapVisuals[player.nUniqueID];
							if (visual.ownerID == 0) { // Если новый визуал
								visual.vCurrentPos = worldPos;
								visual.ownerID = player.nUniqueID;
							}
							visual.vTargetPos = worldPos; // Цель для плавной интерполяции
						

						// --- 4. СПЕЦИФИЧЕСКАЯ ЛОГИКА КЛАССОВ ---
						if (archId == ArchetypeId::Player_Mage) {
							static_cast<Mage*>(entity)->SetBalance((player.fSpecialBar / 255.0f) * 6.0f - 3.0f);
						}
						else if (archId == ArchetypeId::Player_Warrior) {
							auto* warrior = static_cast<Warrior*>(entity);
							warrior->SetResourceValue(player.fSpecialBar / 255.0f);
							warrior->SetExtraData(player.nClassParam);
							warrior->UpdateLightSize(dt, lightIntensity); // Убедитесь, что dt и lightIntensity доступны в этой области видимости
						}
						else if (archId == ArchetypeId::Player_Hunter) {
							static_cast<Hunter*>(entity)->UpdateFromNetwork(player.nClassParam, player.fSpecialBar / 255.0f);
						}
					}
				} // Конец цикла по игрокам

				// --- 5. ЧИТАЕМ КОЛИЧЕСТВО СНАРЯДОВ ---
				uint16_t numProjectiles = 0;
				msg >> numProjectiles;

				for (uint16_t i = 0; i < numProjectiles; i++) {
					sProjectileDescription proj;
					msg >> proj;

					projectiles.insert_or_assign(proj.nUniqueID, proj);

					// Интерполяция для снаряда
					auto& projVisual = mapProjVisuals[proj.nUniqueID];
					if (projVisual.ownerID == 0) { // Если снаряд только появился в мапе визуалов
						projVisual.vCurrentPos = proj.vPos;
						projVisual.ownerID = proj.nOwnerID;
					}
					projVisual.vTargetPos = proj.vPos ;
				}

				// --- 6. ЧИТАЕМ СЕРВЕРНОЕ ВРЕМЯ ---
				msg >> serverTime;

				break;
			}
			case GameMsg::Server_CastStart:
			{
				sCastStart data;
				msg >> data;

				auto it = mapVisuals.find(data.casterId);
				if (it != mapVisuals.end()) {
					// Включаем зацикленную анимацию ЗАМАХА (Фаза 1). 
					// Она будет крутиться на персонаже всё время, пока игрок удерживает кнопку ЛКМ
					it->second.animComp.PlayAnimation(1, "prepare_action"_sid);
				}
				break;
			}
			// Логика лаунча занимается СТРОГО анимацией рук любого игрока на экране
		case GameMsg::Server_CastLaunch:
		{
			sCastLaunch data;
			msg >> data;

			auto it = mapVisuals.find(data.casterId);
			if (it != mapVisuals.end()) {
				// Игрок отпустил кнопку -> включаем резкую OneShot анимацию броска рук вперед (Фаза 2)
				// Она затрет собой анимацию замаха и вернет персонажа в "default" по окончании кадров
				it->second.animComp.PlayOneShot(1, "attack_action"_sid, 4, 16.0f);
			}
			break;
		}
			case(GameMsg::Game_CastSpell):
			{
				sProjectileDescription desc; //  создаём пустой проджектайл
				msg >> desc;  // записываем в него данные 
				//vecProjectiles.push_back(desc);
				
				
				projectiles.insert_or_assign(desc.nUniqueID, desc); // так же внедряем в мапу проджектайл

				// Сразу создаем визуальный объект и запоминаем владельца
				//if (mapProjVisuals.find(desc.nUniqueID) == mapProjVisuals.end()) {
				//	mapProjVisuals[desc.nUniqueID].vCurrentPos = desc.vPos;
				//	mapProjVisuals[desc.nUniqueID].vTargetPos = desc.vPos;
				//	mapProjVisuals[desc.nUniqueID].ownerID = desc.nOwnerID; // ВАЖНО
				//	mapProjVisuals[desc.nUniqueID].fLightIntensity = 1.0f;
				//}
				break;
			}
			case(GameMsg::Game_RemoveProjectile):
			{

				uint16_t id;
				msg >> id;

				
				projectiles.erase(id);

				if (mapProjVisuals.contains(id)) {
					mapProjVisuals[id].bPendingRemoval = true; // Визуал оставляем "доживать"
				}


				break;
			}

			case(GameMsg::Game_RemovePlayer):
			{
				uint32_t nRemovalID = 0;
				msg >> nRemovalID;
			//	mapObjects.erase(nRemovalID);
				mapVisuals.erase(nRemovalID);
				playerEntities.erase(nRemovalID);
				break;
			}


			case(GameMsg::chat_message): {

				sChatMessage chatgmsg;
				msg >> chatgmsg.nSenderID; // Сначала ID
				msg >> chatgmsg.sText;     // Потом текст

				chatMessages.push_back(chatgmsg);

				// Если превысили лимит, удаляем самое старое (первое)
				if (chatMessages.size() > MAX_CHAT_MESSAGES) {
					chatMessages.pop_front();
				}
				break;
			}
			case GameMsg::Server_MatchmakingStatus:
			{
				uint32_t count;
				msg >> count;

				matchmakingCount = count;
				break;
			}
			case GameMsg::Server_RoomStarted:
			{
				SetState(GameState::ROOM);
				fog.reset();
				projectiles.clear();
			//	mapObjects.clear();
				mapProjVisuals.clear();
				mapVisuals.clear();
				playerEntities.clear();
				spellCooldowns.clear();
				activeExplosions.clear();
				chatMessages.clear();
				activeChains.clear();
				break;
			}
			case GameMsg::Server_GameStarted:
			{
				int w, h;
				uint32_t seed;
				
				msg >> w;
				msg >> h;
				msg >> seed;
				
				// 1. Пересоздаем уровень локально по зерну
				
				GameLevel = std::make_unique<gameLevel>(w, h, seed); //cj
				ResourceManager::GetShader(ShaderID::World).use().setVec2("levelSizePx", glm::vec2(GameLevel->LevelSize.x * 32.0f, GameLevel->LevelSize.y * 32.0f));
				ResourceManager::GetShader(ShaderID::instanceWorld).use().setVec2("levelSizePx", glm::vec2(GameLevel->LevelSize.x * 32.0f, GameLevel->LevelSize.y * 32.0f));
				// 2. Обновляем текстуру блоков в системе тумана
				fog.SetLevelBlocks(GameLevel->blocks, GameLevel->LevelSize.x, GameLevel->LevelSize.y);

				// 3. Очищаем старые объекты
				projectiles.clear();
			;	
				SetState(GameState::INGAME);
				break;
			}
			case GameMsg::Server_ReturnToLobby:
			{
				
				OpenLobby();
				break;
			}
			case GameMsg::Game_Explosion:
			{
				glm::vec2 pos;
				float radius;

				msg >> radius;
				msg >> pos;

				ExplosionEffect e;
				e.pos = pos;
				e.diametr = radius * 2.0f;
				e.timer = 0.5f; // 0.5 секунд анимации
			
				activeExplosions.push_back(e);
				break;
			}
			case GameMsg::Game_BindLink:
			{

				VisualChain e;

				//ExplosionEffect e;
				msg >> e.lifetime;
				msg >> e.linkedNetId;
				msg >> e.end;
				msg >> e.targetNetId;
				

				activeChains.push_back(e);
				
				break;
			}
			case GameMsg::Server_SpellCooldown:
			{
				sSpellCooldown cd;
				msg >> cd;

				spellCooldowns[(SpellId)cd.spellId] = {
					cd.serverTime + cd.cooldown,
					cd.cooldown
				};
				break;
			}
			case GameMsg::Game_TileUpdate:
			{


				uint16_t count;
				msg >> count;


				for (int i = 0; i < count; i++)
				{
					uint16_t idx;
					char symbol ;
					float respawnTime; // Предположим, сервер теперь шлет и это

					msg >> respawnTime;
					msg >> idx;
					msg >> symbol;

					if (idx >= GameLevel->LevelData.size())
						break;




					fog.RemoveBeaconLight(idx);

					GameLevel->LevelData[idx] = symbol;

					if (symbol == '#') {
						GameLevel->activeTiles.insert(idx);
						GameLevel->tileVisuals[idx].growInterp = 0.0f;
						GameLevel->blocks[idx] = 255;

						GameLevel->tileVisuals[idx].isRespawning = false;
						GameLevel->tileVisuals[idx].respawnTimer = 0.0f;
					}
					else if(symbol == 'B')
					{
						GameLevel->blocks[idx] = 1;
						GameLevel->tileVisuals[idx].ownerId = 0xFFFF;
						GameLevel->tileVisuals[idx].isRespawning = false;
						GameLevel->tileVisuals[idx].respawnTimer = 0.0f;
					}
					else // если тайл уничтожен 
					{
						GameLevel->blocks[idx] = 0;
						GameLevel->tileVisuals[idx].ownerId = 0xFFFF;

						GameLevel->tileVisuals[idx].respawnTimer = respawnTime;
						GameLevel->tileVisuals[idx].isRespawning = (respawnTime > 0.0f);
						// ВАЖНО: Добавляем в активные, чтобы Update видел этот таймер
						if (GameLevel->tileVisuals[idx].isRespawning) {
							GameLevel->activeTiles.insert(idx);
						}
					}


				}
				fog.SetLevelBlocks(GameLevel->blocks, GameLevel->LevelSize.x, GameLevel->LevelSize.y);
				/*uint16_t index;
				char value;
				msg >> value;
				msg >> index;

				if (index >= GameLevel->LevelData.size())
					break;

				char& tile = GameLevel->LevelData[index];
				tile = value;
				 
				if (value == '#') {
					GameLevel->activeTiles.insert(index);
					GameLevel->tileVisuals[index].growInterp = 0.0f;
					GameLevel->blocks[index] = 255;
				}
				else
				{
					GameLevel->blocks[index] = 0;
				}
				fog.SetLevelBlocks(GameLevel->blocks, GameLevel->LevelSize.x, GameLevel->LevelSize.y);*/
			}
			break;
			case GameMsg::Game_TileMove:
			{
				uint16_t fromIdx, toIdx;
				float duration;

				msg >> duration;
				msg >> toIdx;
				msg >> fromIdx;

				char tile = GameLevel->LevelData[fromIdx];

				GameLevel->LevelData[fromIdx] = '.';
				GameLevel->LevelData[toIdx] = tile;
				GameLevel->blocks[fromIdx] = 0;
				GameLevel->blocks[toIdx] = 255;

				auto& tv = GameLevel->tileVisuals[toIdx];

				// 🔥 ПОЛНЫЙ СБРОС ВИЗУАЛА
				tv = TileVisual{};

				tv.growInterp = 1.0f; // ❗ не даём тайлу исчезнуть
				tv.moving = true;
				tv.t = 0.0f;
				tv.duration = duration;
				//float screenW = Width / BRICK_SIZE;

				tv.from = GridToScreen(fromIdx, GameLevel->LevelSize.x);
				tv.to = GridToScreen(toIdx, GameLevel->LevelSize.x);

				GameLevel->activeTiles.insert(toIdx);
				fog.SetLevelBlocks(GameLevel->blocks, GameLevel->LevelSize.x, GameLevel->LevelSize.y);
			break;
			}
			case GameMsg::Game_EnvironmentUpdate:
			{
				sEnvironmentData data;
				msg >> data;
				targetWindAngle = data.windAngle; // Сохраняем для плавной отрисовки
				currentWindForce = data.windForce;
				dayCycleProgress = data.dayProgress;
				break;
			}
			case GameMsg::Server_Ping: {
				uint64_t timeThen;
				msg >> timeThen;
				uint64_t timeNow = std::chrono::system_clock::now().time_since_epoch().count();

				// Пинг в миллисекундах
				currentPing = (timeNow - timeThen) / 1000000.0f; // если в наносекундах
				break;
			}
			case GameMsg::Game_DamageEvent: {
				uint16_t count;
				msg >> count; // Достаем количество отчетов первым!

				for (int i = 0; i < count; i++) {
					sDamageReport report;
					msg >> report;

					//// 1. Проверяем, существует ли жертва в наших сущностях
					//if (playerEntities.count(report.nVictimId)) {
					//	auto& victim = playerEntities[report.nVictimId];
					//	auto& entity = playerEntities[nPlayerID];
					//	// 2. Если это компаньон и он погиб (HP <= 0)
					//	// Примечание: сервер должен присылать актуальное HP в этом же пакете или отдельным Update
					//	if (victim->entityType == EntityType::Companion && victim->health <=0 && entity->teamId == victim->teamId) {
					//		entity->GetSquad()->RemoveCompanion(report.nVictimId);
					//		break;
					//	}
					//}
				

					if (mapVisuals.count(report.nVictimId)) {
						FloatingText ft;
						
						// Я КРАСАВЧИК (Мой урон)
						if (report.nAttackerId == nPlayerID) {
							ft.scale = 0.5f; // Крупнее
							ft.alpha = 1.0f;   // Ярче
						}
						// МЕНЯ БЬЮТ!
						else if (report.nVictimId == nPlayerID) {
							ft.scale = 0.5f;
							//report.nType = 99; // Условный тип "Критическая угроза" (красный цвет)
							ft.alpha = 1.0f;
							if (playerEntities[nPlayerID]->health <= 0)
							isDead = true;
						}
						// ВООБЩЕ ЧУЖОЙ БОЙ (можно пропустить или сделать крошечным)
						else {
							// Опционально: return; // если не хочешь видеть чужой урон вообще
							ft.scale = 0.25f;
							ft.alpha = 0.4f;
						}

						ft.pos = mapVisuals[report.nVictimId].vCurrentPos; // Начальная позиция - центр игрока
						ft.pos.y += 0.25f;

					//	ft.value = std::to_string((int)report.Amount);
						//ft.isResisted = report.bResisted;
						// Логика отображения значения
						if (report.nType == 2) {
							ft.value = "+" + std::to_string((int)report.Amount);
							ft.color = { 0.2f, 1.0f, 0.2f }; // Зеленый цвет для хила
						}
						else {
							ft.value = std::to_string((int)report.Amount);
							// Ваша логика цветов для урона
							if (report.nType == 0) ft.color = { 1.0f, 0.6f, 0.0f };
							else if (report.nType == 1) ft.color = { 0.0f, 0.7f, 1.0f };
							else ft.color = { 1.0f, 1.0f, 1.0f };
						}

						ft.isResisted = report.bResisted;
						activeDamages.push_back(ft);



						// ДОПОЛНИТЕЛЬНО: Если пришел опыт
						if (report.ExperienceGained > 0 && report.nAttackerId == nPlayerID) {
							FloatingText xpFt;
							xpFt.pos = mapVisuals[report.nVictimId].vCurrentPos;
							xpFt.pos.y += 0.5f; // Чуть выше обычного урона, чтобы не перекрывали друг друга
							xpFt.value = "+" + std::to_string((int)report.ExperienceGained) + " XP";
							xpFt.color = { 0.8f, 0.2f, 1.0f }; // Фиолетовый
							xpFt.scale = 0.6f;
							xpFt.alpha = 1.0f;
							activeDamages.push_back(xpFt);

							//// Обновляем данные в локальном менеджере своего персонажа
							//auto* myProg = playerEntities[nPlayerID].get()->GetProgression();
							//// Добавляем опыт локально, чтобы полоска сразу дернулась
							//myProg->AddExperience(report.ExperienceGained);
						}
					}
				}
				break;
			}
			case GameMsg::Game_EntityStats: {
				uint8_t lvl;
				uint32_t id;
				int health; // Убедитесь, что тип совпадает с character->maxHealth
				float exp;
				// Достаем в обратном порядке (LIFO):
				msg >> exp;
				msg >> lvl;    // 1. Достаем то, что положили последним (Level)
				msg >> id;     // 2. Достаем то, что положили вторым (Id)
				msg >> health; // 3. Достаем то, что положили первым (maxHealth)

				if (playerEntities.count(id)) {
					// Используйте SetLevel, иначе AddLevel будет прибавлять уровень 
					// к текущему при каждом обновлении (был 5, стал 10, потом 15...)
					playerEntities[id]->GetProgression()->AddLevel(lvl);
					playerEntities[id]->GetProgression()->SetExp(exp);
					playerEntities[id]->maxHealth = health;
				}
				break;
			}
			case GameMsg::Server_BeaconOwnerUpdate:
			{
				uint32_t ownerId;
				uint16_t index;
				msg >> ownerId >> index;

				if (index < GameLevel->tileVisuals.size()) {
					GameLevel->tileVisuals[index].ownerId = ownerId;

					// Превращаем индекс в мировую позицию (центр тайла)
					glm::vec2 worldPos = GridToScreen(index, GameLevel->LevelSize.x) + glm::vec2(16.0f);

					if (ownerId == nPlayerID) {
						// Если захватили МЫ — добавляем в туман как источник света
						fog.AddBeaconLight(index, worldPos, 250.0f);
					}
					else {
						// Если захватил ВРАГ или маяк нейтрален — удаляем источник света у нас
						fog.RemoveBeaconLight(index);
					}
					}
					break;
				}
				break;
			}

		}
}
//	if (Incoming().empty());
//{
//	if (!bIsReconnecting) {
//		std::cout << "Lost connection! Starting reconnect attempts...\n";
//		bIsReconnecting = true;
//		reconnectTimer = 0.0f;
//		connectionAttempts = 0;
//	}
//
//	reconnectTimer += dt;
//
//	if (reconnectTimer >= reconnectInterval)
//	{
//		reconnectTimer = 0.0f;
//		connectionAttempts++;
//		std::cout << "Attempting to reconnect (" << connectionAttempts << ")..." << std::endl;
//
//		// Пытаемся подключиться к серверу снова
//		if (Connect("26.168.96.157", 60000))
//		{
//			// Если сокет открылся, сразу шлем логин
//			// Сервер узнает нас по имени и восстановит сессию
//			olc::net::message<GameMsg> msg;
//
//			// Сначала упаковываем строку (она запишется в конец буфера)
//			msg << playerProfile.name;
//
//			// Затем упаковываем фиксированные данные
//			sLoginRequest req;
//			req.sessionToken = nSessionToken;
//			msg << req;
//
//			Send(msg);
//
//			bIsReconnecting = false;
//			std::cout << "Socket re-established. Login sent.\n";
//		}
//	}
//	}
}

void Game::sendUpdatePlayer(const glm::vec2& inputDir, const glm::vec2& lookDir)
{
	olc::net::message<GameMsg> msg;
	msg.header.id = GameMsg::Game_UpdatePlayer;

	// ВАЖНО: соблюдайте тот же порядок чтения на сервере!
	msg << lookDir;
	msg << inputDir;

	Send(msg);

}

const CooldownInfo* Game::GetCooldownBySpellId(SpellId id) const
{
	auto it = spellCooldowns.find(id);
	if (it == spellCooldowns.end())
		return nullptr;

	// Если время кулдауна уже вышло, можно либо возвращать nullptr, 
	// либо проверять это уже в UI (как мы делали выше).
	return &it->second;
}

void Game::SendCastMessage(GameMsg type, ActionSlot slot)
{
	olc::net::message<GameMsg> msg;
	msg.header.id = type;
	msg << slot;
	Send(msg);
}

	






