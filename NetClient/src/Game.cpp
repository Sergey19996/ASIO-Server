#include "Game.h"
#include "Level/gameLevel.h"
#include "SpriteRenderer.h"
#include "ResourceManager.h"
#include "Text/TextRender.h"
#include "io/Mouse.h"
gameLevel* GameLevel;
SpriteRenderer* Renderer;
TextRenderer* text;
const int BRICK_SIZE = 32;
const unsigned int TEXTSIZE = 12;


std::vector<void(*)(GLFWwindow* windiw, int key, int scancode, int action, int mods)>Game::keyCallbacks;
std::string Game::chatMessage;
bool Game::Keys[GLFW_KEY_LAST] = { 0 };  //GLFW_KEY_LAST ��� ���������, ������� ����� ������������ ���������� ������, �������������� GLFW.
bool Game::KeysProcessed[GLFW_KEY_LAST] = { 0 };
GameState Game::state = GameState::ACTION;


// Initial velocity of the player paddle




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
			case(GameMsg::Game_AddProjectile):
			{
				sProjectileDescription desc; //  ������ ������ �����������
				msg >> desc;  // ���������� � ���� ������ 
			//	vecProjectiles.push_back(desc);

				mapProjectiles.insert_or_assign(desc.nUniqueID, desc); // ��� �� �������� � ���� �����������

				//if (desc.nUniqueID == nPlayerID)
				//{
					// Now we exist in game world
				//	bWaitingForConnection = false;
				//}
				break;
			}
			case(GameMsg::Game_UpdateProjectile):
			{
				sProjectileDescription desc; //  ������ ������ �����������
				msg >> desc;  // ���������� � ���� ������ 
				//	vecProjectiles.push_back(desc);

				mapProjectiles.insert_or_assign(desc.nUniqueID, desc); // ��� �� �������� � ���� �����������

				//if (desc.nUniqueID == nPlayerID)
				//{
					// Now we exist in game world
				//	bWaitingForConnection = false;
				//}
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
			//	sPlayerDescription desc;
			//	msg >> desc;
			//	mapObjects.insert_or_assign(desc.nUniqueID, desc);
				

			//	olc::net::message<GameMsg> msgUpdate;
				//msgUpdate.header.id = GameMsg::Game_UpdatePlayer;  // ��������� ID ��� ��������� ����������

				uint16_t numPlayers = 0;    // ������ �������� ��� ���-�� �������
				msg >> numPlayers;  // ���������� � ���� 

				// �����: ������ � �������� �������
				for (uint16_t i = 0; i < numPlayers; i++)   // ���� �� ���������� ���-�� �������
				{
					sPlayerDescription desc;   // ������ ��������
					msg >> desc;

					mapObjects.insert_or_assign(desc.nUniqueID, desc);

					// ��������� ��������� ������
				//	mapObjects[id] = desc;

					// ������ �������� �������:
				/*	std::cout << "Player " << id << " pos: "
						<< desc.vPos.x << ", " << desc.vPos.y << std::endl;*/
				}



				break;
			}
			case(GameMsg::chat_message): {

				sChatMessage chatgmsg;
				msg >> chatgmsg.nSenderID;  // reverse reading
				msg >> chatgmsg.sText;

				std::cout << "Reseave msg" << std::endl;
				chatMessages.push_back(chatgmsg);
				std::cout << "Push back in deque" << std::endl;


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
		keyEvents(); // ��������� ��������� Keys[] ������ ����

		updateObjects(dt);
	}




}

void Game::Render()
{
	if (!bWaitingForConnection)
	{
		GameLevel->Render(*Renderer);
		renderObject();


		RenderChat();

			
	}
}

void Game::keyEvents(){


	if (state == GameState::ACTION) {


	if (this->Keys[GLFW_KEY_A])
	{
		mapObjects[nPlayerID].vVel += glm::vec2(-1.0f, 0.0f); // A � �����
		
	}
	if (this->Keys[GLFW_KEY_D])
	{
		mapObjects[nPlayerID].vVel += glm::vec2(1.0f, 0.0f);  // D � ������
	}

	if (this->Keys[GLFW_KEY_W])
	{
		mapObjects[nPlayerID].vVel += glm::vec2(0.0f, -1.0f); // W � �����mapObjects[nPlayerID].vVel += (0.0f, -1.0f);

	}
	if (this->Keys[GLFW_KEY_S])
	{
		mapObjects[nPlayerID].vVel += glm::vec2(0.0f, 1.0f);  // S � ����


	}

	}

}

void Game::RenderChat(){

	//chat
	ResourceManager::GetShader("sprite").use();
	glm::vec4 color = { 1.0f,0.0f,0.0f,1.0f };
	glm::vec2 size = { Width / 3.0f,Height / 4.0f };
	glm::vec2 pos = { (1.0f - size.x / Width) * Width,   (1.0f - size.y / Height) * Height };
	char  chr = '#';
	ResourceManager::GetShader("sprite").setvec4("color", color);
	Renderer->DrawSprite(chr, pos, size, 0.0f, { 0.0f,0.0f,0.0f,0.9f });
	Renderer->DrawSprite(chr, { pos.x,pos.y + size.y - size.y * 0.1f }, { size.x,size.y * 0.1f }, 0.0f, { 0.1f,0.1f,0.1f,0.9f });
	text->RenderText(chatMessage, pos.x + size.x * 0.05f, pos.y + size.y - size.y * 0.1f + size.y * 0.05f, 0.5f);


	//Draw messages in chat 
	for (int i = 0; i < chatMessages.size(); i++)
	{
		text->RenderText(std::to_string(chatMessages[i].nSenderID) + " : " + chatMessages[i].sText, pos.x + size.x * 0.05f, pos.y + (pos.y * 0.01f) + ((TEXTSIZE * 0.5f) * i), 0.5f);
	}
}


void Game::keyCallback(GLFWwindow* window, int key, int scancode, int action, int modes)
{
	if (action != GLFW_RELEASE)   //���� ������ �� �����	
	{
		if (!Keys[key]) {    //���� ��� ����� false
			Keys[key] = true; //������ true
		}

	}
	else
	{
		Keys[key] = false;
	}

	KeysProcessed[key] = action != GLFW_REPEAT; //���� ������� �� ��������� (GLFW_PRESS ��� GLFW_RELEASE), �� ��������� ����� true.

	for (void(*func)(GLFWwindow*, int, int, int, int) : Game::keyCallbacks) {
		func(window, key, scancode, action, modes);
	}

}

void Game::character_callback(GLFWwindow* window, unsigned int codepoint) {
	if (state == GameState::TYPINGCHAT) {
		if (codepoint >= 32 && codepoint <= 126) { // Printable ASCII Engl

			// ������ ������ � ������ ����
			chatMessage += static_cast<char>(codepoint);
		
		}
	}
}

void Game::renderObject()
{
	glm::vec2 MousePos = { Mouse::getMouseX() / BRICK_SIZE, Mouse::getMouseY() / BRICK_SIZE };

	ResourceManager::GetShader("sprite").use();
	char PlayerChar = '$';


	// Draw World Objects
	for (auto& object : mapObjects)
	{
		ResourceManager::GetShader("sprite").setFloat("radius", object.second.fRadius);
		ResourceManager::GetShader("sprite").setvec2("direction", MousePos - object.second.vPos);

		Renderer->DrawSprite(PlayerChar, { object.second.vPos.x * BRICK_SIZE, object.second.vPos.y * BRICK_SIZE });

		

		// ����� ���������� direction
		ResourceManager::GetShader("sprite").setvec2("direction", { 0.0f, 0.0f });

		// ����� ����� (���� �� �� ���������� ���� ������, ����� �� ������������)
		std::string sMenu = std::to_string(object.second.nUniqueID);
		float proportion = static_cast<float>(TEXTSIZE) / BRICK_SIZE;
		float offset = (1.0f - proportion) / 2.0f;
		text->RenderText(sMenu, (object.second.vPos.x + offset) * BRICK_SIZE, (object.second.vPos.y + 1 + offset) * BRICK_SIZE, 1);
		
	}
	for (auto& object : mapProjectiles)
	{
		ResourceManager::GetShader("sprite").setFloat("radius", object.second.fRadius);
		ResourceManager::GetShader("sprite").setvec2("direction", MousePos - object.second.vPos);

		Renderer->DrawSprite(PlayerChar, { object.second.vPos.x * BRICK_SIZE, object.second.vPos.y * BRICK_SIZE });

		// ����� ���������� direction
		ResourceManager::GetShader("sprite").setvec2("direction", { 0.0f, 0.0f });

		// ����� ����� (���� �� �� ���������� ���� ������, ����� �� ������������)
		std::string sMenu = std::to_string(object.second.nUniqueID);
		float proportion = static_cast<float>(TEXTSIZE) / BRICK_SIZE;
		float offset = (1.0f - proportion) / 2.0f;
		text->RenderText(sMenu, (object.second.vPos.x + offset) * BRICK_SIZE, (object.second.vPos.y + 1 + offset) * BRICK_SIZE, 1);

	}



	// Send player description
	olc::net::message<GameMsg> msg;
	msg.header.id = GameMsg::Game_UpdatePlayer;
	msg << mapObjects[nPlayerID].vVel;

	//sPlayerDescription desc;
	//msg >> desc;


	Send(msg);
	
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

void Game::ReleaseChatInput()  // TODO
{
	state = GameState::ACTION;

	if (chatMessage.size() != 0) {

	
	//Send()
	olc::net::message<GameMsg> msg;
	msg.header.id = GameMsg::chat_message;
	msg << chatMessage;  //string
	msg << nPlayerID; // id
	Send(msg);  // Record first string then id


	sChatMessage chatgmsg;
	msg >> chatgmsg.nSenderID;
	msg >> chatgmsg.sText;

	chatMessages.push_back(chatgmsg);
	chatMessage.clear();
	}
}

void Game::delete_Char(){
	if (!chatMessage.empty()) chatMessage.pop_back(); // ������� ���� char

}

void Game::createProjectile()
{
	glm::vec2 MousePos = { Mouse::getMouseX() / BRICK_SIZE, Mouse::getMouseY() / BRICK_SIZE }; // ����� ������ �����

	olc::net::message<GameMsg> msg;   // c������ ������ ���������
	msg.header.id = GameMsg::Game_AddProjectile; // ������� � ����� ����� ����� ������� �� �������


	sProjectileDescription desc;  // ������� ������ �����
	desc.nOwnerID = nPlayerID; // ������� ��� ������
	desc.vPos = mapObjects[nPlayerID].vPos;  // ������� �������
	desc.vVel = glm::normalize(MousePos - desc.vPos) ; //������� ������ ������� � ������ �� � � �� �


	msg << desc; // ���������� �� � ���������

	//mapObjects.insert_or_assign(desc.nOwnerID, desc);
	Send(msg); // ���������� �� ������
}




