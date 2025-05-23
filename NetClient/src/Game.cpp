#include "Game.h"
#include "Level/gameLevel.h"
#include "SpriteRenderer.h"
#include "ResourceManager.h"
#include "Text/TextRender.h"

gameLevel* GameLevel;
SpriteRenderer* Renderer;
TextRenderer* text;
const int BRICK_SIZE = 32;
const unsigned int TEXTSIZE = 12;


std::vector<void(*)(GLFWwindow* windiw, int key, int scancode, int action, int mods)>Game::keyCallbacks;
std::string Game::chatMessage;
bool Game::Keys[GLFW_KEY_LAST] = { 0 };  //GLFW_KEY_LAST это константа, которая задаёт максимальное количество клавиш, распознаваемых GLFW.
bool Game::KeysProcessed[GLFW_KEY_LAST] = { 0 };
GameState Game::state = GameState::ACTION;


// Initial velocity of the player paddle
const float PLAYER_VELOCITY(10.0f);



Game::Game(unsigned int width, unsigned int height) : Width(width),Height(height)
{

}

Game::~Game()
{
	delete text;
	text = nullptr;
	delete GameLevel;
	GameLevel = nullptr;
	delete Renderer;
	Renderer = nullptr;
}

bool Game::Init()
{
	ResourceManager::LoadShader("Assets/Shaders/VertexShader.glsl","Assets/Shaders/FragmentShader.glsl",nullptr, "sprite");

	GameLevel = new gameLevel(Width / BRICK_SIZE,Height / BRICK_SIZE);
	Renderer = new SpriteRenderer(ResourceManager::GetShader("sprite"));


	glm::mat4 ortoMatrix = glm::ortho(0.0f, static_cast<float>(Width), static_cast<float>(Height), 0.0f, -1.0f, 1.0f);
	ResourceManager::GetShader("sprite").setMat4("projection", ortoMatrix);
	//
//	mapObjects[0].nUniqueID = 0;
//	mapObjects[0].vPos = { 3.0f, 3.0f };


	text = new TextRenderer(this->Width, this->Height);
	text->Load("Assets/Fonts/PressStart2P-Regular.ttf", TEXTSIZE);

	if (Connect("127.0.0.1", 60000))
	{
		return true;
	}

	return false;

}

void Game::Update(float dt)
{
	// Check for incoming network messages
	if (IsConnected())
	{
		while (!Incoming().empty())
		{
			auto msg = Incoming().pop_front().msg;

			switch (msg.header.id)
			{
			case(GameMsg::Client_Accepted):
			{
				std::cout << "Server accepted client - you're in!\n";
				olc::net::message<GameMsg> msg;
				msg.header.id = GameMsg::Client_RegisterWithServer;
				descPlayer.vPos = { 3.0f, 3.0f };
				msg << descPlayer;
				Send(msg);
				break;
			}

			case(GameMsg::Client_AssignID):
			{
				// Server is assigning us OUR id
				msg >> nPlayerID;
				std::cout << "Assigned Client ID = " << nPlayerID << "\n";
				break;
			}

			case(GameMsg::Game_AddPlayer):
			{
				sPlayerDescription desc;
				msg >> desc;
				mapObjects.insert_or_assign(desc.nUniqueID, desc);

				if (desc.nUniqueID == nPlayerID)
				{
					// Now we exist in game world
					bWaitingForConnection = false;
				}
				break;
			}

			case(GameMsg::Game_RemovePlayer):
			{
				uint32_t nRemovalID = 0;
				msg >> nRemovalID;
				mapObjects.erase(nRemovalID);
				break;
			}

			case(GameMsg::Game_UpdatePlayer):
			{
				sPlayerDescription desc;
				msg >> desc;
				mapObjects.insert_or_assign(desc.nUniqueID, desc);
				
				break;
			}
			case(GameMsg::chat_message): {


				break;
			}

			}
		}
	}

	if (bWaitingForConnection)
	{
		
		std::string sMenu = "Waiting To Connect...";
		float proportion = static_cast<float>(TEXTSIZE) / BRICK_SIZE;
		float offset = (1.0f - proportion) / 2.0f;
		text->RenderText(sMenu, Width/2, Height/2, 1);

	}
	else
	{
		mapObjects[nPlayerID].vVel = { 0.0f, 0.0f };
		keyEvents(); // Проверяем состояние Keys[] каждый кадр

		updateObjects(dt);
	}




}

void Game::Render()
{
	if (!bWaitingForConnection)
	{
		GameLevel->Render(*Renderer);
		renderObject();


		//chat
		ResourceManager::GetShader("sprite").use();
		glm::vec4 color = { 1.0f,0.0f,0.0f,1.0f };
		glm::vec2 size = { Width / 3.0f,Height / 4.0f };
		glm::vec2 pos = {  (1.0f - size.x / Width) * Width,   (1.0f - size.y / Height) * Height  };
		char  chr = '#';
		ResourceManager::GetShader("sprite").setvec4("color", color);
		Renderer->DrawSprite(chr, pos, size, 0.0f, {0.0f,0.0f,0.0f,0.9f});
		Renderer->DrawSprite(chr, { pos.x,pos.y + size.y - size.y * 0.1f }, { size.x,size.y * 0.1f }, 0.0f, { 0.1f,0.1f,0.1f,0.9f });
		text->RenderText(chatMessage, pos.x + size.x * 0.05f , pos.y + size.y - size.y * 0.1f + size.y * 0.05f, 0.5f);
	}
}

void Game::keyEvents(){


	if (state == GameState::ACTION) {


	if (this->Keys[GLFW_KEY_A])
	{
		mapObjects[nPlayerID].vVel += glm::vec2(-1.0f, 0.0f); // A — влево
		
	}
	if (this->Keys[GLFW_KEY_D])
	{
		mapObjects[nPlayerID].vVel += glm::vec2(1.0f, 0.0f);  // D — вправо
	}

	if (this->Keys[GLFW_KEY_W])
	{
		mapObjects[nPlayerID].vVel += glm::vec2(0.0f, -1.0f); // W — вверхmapObjects[nPlayerID].vVel += (0.0f, -1.0f);

	}
	if (this->Keys[GLFW_KEY_S])
	{
		mapObjects[nPlayerID].vVel += glm::vec2(0.0f, 1.0f);  // S — вниз


	}

	}

}


void Game::keyCallback(GLFWwindow* window, int key, int scancode, int action, int modes)
{
	if (action != GLFW_RELEASE)   //если кнопка не релиз	
	{
		if (!Keys[key]) {    //если эта кнока false
			Keys[key] = true; //ставим true
		}

	}
	else
	{
		Keys[key] = false;
	}

	KeysProcessed[key] = action != GLFW_REPEAT; //Если событие не повторное (GLFW_PRESS или GLFW_RELEASE), то выражение вернёт true.

	for (void(*func)(GLFWwindow*, int, int, int, int) : Game::keyCallbacks) {
		func(window, key, scancode, action, modes);
	}

}

void Game::character_callback(GLFWwindow* window, unsigned int codepoint) {
	if (state == GameState::TYPINGCHAT) {
		if (codepoint >= 32 && codepoint <= 126) { // Printable ASCII Engl

			// Добавь символ к строке чата
			chatMessage += static_cast<char>(codepoint);
		
		}
	}
}

void Game::renderObject()
{
	ResourceManager::GetShader("sprite").use();
	char PlayerChar = '$';
	// Draw World Objects
	for (auto& object : mapObjects)
	{
		ResourceManager::GetShader("sprite").setFloat("radius", object.second.fRadius);
		ResourceManager::GetShader("sprite").setvec2("direction", object.second.vVel);

		Renderer->DrawSprite(PlayerChar, { object.second.vPos.x * BRICK_SIZE, object.second.vPos.y * BRICK_SIZE });

		// Сразу сбрасываем direction
		ResourceManager::GetShader("sprite").setvec2("direction", { 0.0f, 0.0f });

		// Затем текст (если он не использует этот шейдер, можно не беспокоиться)
		std::string sMenu = std::to_string(object.second.nUniqueID);
		float proportion = static_cast<float>(TEXTSIZE) / BRICK_SIZE;
		float offset = (1.0f - proportion) / 2.0f;
		text->RenderText(sMenu, (object.second.vPos.x + offset) * BRICK_SIZE, (object.second.vPos.y + 1 + offset) * BRICK_SIZE, 1);
		
	}




	// Send player description
	olc::net::message<GameMsg> msg;
	msg.header.id = GameMsg::Game_UpdatePlayer;
	msg << mapObjects[nPlayerID];

	//sPlayerDescription desc;
	//msg >> desc;


	Send(msg);
	
}

void Game::updateObjects(float fElapsedTime)
{
	// Update objects locally
	for (auto& object : mapObjects)
	{
		// Потенциальная позиция после движения
		glm::vec2 vPotentialPosition = object.second.vPos + object.second.vVel * fElapsedTime * PLAYER_VELOCITY;
		
		// учитываем центр круга
			glm::vec2 vCircleCenter = vPotentialPosition + glm::vec2(object.second.fRadius);

		// Текущая и целевая клетка
		glm::ivec2 vCurrentCell = glm::floor(object.second.vPos);  // Преобразуем float -> int через floor
		glm::ivec2 vTargetCell = glm::floor(vPotentialPosition);

		// Верхний левый и нижний правый угол области проверки
		glm::ivec2 vAreaTL = glm::max(glm::min(vCurrentCell, vTargetCell) - glm::ivec2(1, 1), glm::ivec2(0, 0));
		glm::ivec2 vAreaBR = glm::min(glm::max(vCurrentCell, vTargetCell) + glm::ivec2(1, 1), GameLevel->ScreenSize);


		// Перебор всех клеток в области
		for (int y = vAreaTL.y; y <= vAreaBR.y; ++y)
		{
			for (int x = vAreaTL.x; x <= vAreaBR.x; ++x)
			{
				// Проверка коллизии с клеткой (например, символ стены)
				if (GameLevel->LevelData[y * GameLevel->ScreenSize.x + x] == '#')
				{
					glm::vec2 vNearestPoint;
					// Inspired by this (very clever btw) 
					// https://stackoverflow.com/questions/45370692/circle-rectangle-collision-response
					vNearestPoint.x = std::max(float(x), std::min(vCircleCenter.x, float(x + 1)));
					vNearestPoint.y = std::max(float(y), std::min(vCircleCenter.y, float(y + 1)));

					//glm::ivec2 vAreaTL = glm::max(glm::min(vCurrentCell, vTargetCell) - glm::ivec2(1, 1), glm::ivec2(0, 0));
				//	glm::ivec2 vAreaBR = glm::min(glm::max(vCurrentCell, vTargetCell) + glm::ivec2(1, 1), GameLevel->ScreenSize);


					// But modified to work :P
				
					glm::vec2 vRayToNearest = vNearestPoint - vCircleCenter;
					float fOverlap = object.second.fRadius - glm::length(vRayToNearest); 
					if (std::isnan(fOverlap)) fOverlap = 0;// Thanks Dandistine!

					// If overlap is positive, then a collision has occurred, so we displace backwards by the 
					// overlap amount. The potential position is then tested against other tiles in the area
					// therefore "statically" resolving the collision
					if (fOverlap > 0)
					{
						// Statically resolve the collision
						vPotentialPosition = vPotentialPosition - glm::normalize(vRayToNearest) * fOverlap;
					}
					

				}


			}
			
		}
	

		// Set the objects new position to the allowed potential position
		object.second.vPos = vPotentialPosition;
		
	}

	

}

void Game::ProcessInput(float dt)
{

}

bool Game::checkCollision(sPlayerDescription& player, glm::vec2& boxpos)
{



	//get center point circle first
	glm::vec2 center(player.vPos + player.fRadius);
	//Calculate AABB info
	glm::vec2 aabb_half_extens(0.5f);  // центр локальный квадрата
	glm::vec2 aabb_center(boxpos + aabb_half_extens); // центр квадрата  в мире 
	glm::vec2 difference = center - aabb_center; // вектор от центра квадрата до центра круга в мире 

	glm::vec2 clmaped = glm::clamp(difference, -aabb_half_extens, aabb_half_extens); //nearest point in local space   glm::clamp(glm::vec2(3.0, -5.0), glm::vec2(-2.0, -2.0), glm::vec2(2.0, 2.0)) // вернёт: glm::vec2(2.0, -2.0)
	glm::vec2 closest = aabb_center + clmaped; // nearest point in world

	difference = closest - center; // вектор от сентра к ближайшей точк
	//std::cout << glm::length(difference) << std::endl;
	
	if (glm::length(difference) <=player.fRadius) {
		std::cout << "Collided" << std::endl;
		return true;
		


	}
	else
	{
		std::cout << "NOT Collided" << std::endl;
		return false;
	}




	return false;
}

void Game::PrepareChatInput()
{
	state = GameState::TYPINGCHAT;
	chatMessage.clear();
}

void Game::ReleaseChatInput()  // TODO
{
	state = GameState::ACTION;

	chatMessage.clear();
}

void Game::delete_Char(){
	if (!chatMessage.empty()) chatMessage.pop_back(); // удаляем один char

}




