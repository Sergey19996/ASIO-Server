#ifndef Game_h
#define Game_h
#include <olc_net.h>
#include <vector>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <unordered_map>
#include "GameStates.hpp"
// Represents the four possible (collision) directions
enum Direction {
	UP,
	RIGHT,
	DOWN,
	LEFT
};
// Defines a Collision typedef that represents collision data
//typedef std::tuple<bool, Direction, glm::vec2> Collision; // <collision?, what direction?, difference vector center - closest point>


class Game : public olc::net::client_interface<GameMsg> {
public:
	static std::vector<void(*)(GLFWwindow* windiw, int key, int scancode, int action, int mods)>keyCallbacks;
	static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int modes);
	static void character_callback(GLFWwindow* window, unsigned int codepoint);


	unsigned int Width, Height;

	Game(unsigned int width, unsigned int height);
	~Game();

	bool Init();


	void Update(float dt);
	void Render();
	void keyEvents();
	void RenderChat();
	
	void renderObject();
	void updateObjects(float felapsedTIme);
	void ProcessInput(float dt);

	Direction VectorDirection(glm::vec2 target); // <collision?, what direction?, difference vector center - closest point>
	

	GameState& GetState() { return state; };
	void SetState(GameState State) { state = State; };
	void PrepareChatInput();
	void ReleaseChatInput();
	void delete_Char();

	void createProjectile();


private:
	static bool Keys[];
	static bool KeysProcessed[];
	sPlayerDescription descPlayer;

	bool bWaitingForConnection = true;

	std::unordered_map<uint32_t,sPlayerDescription> mapObjects;
	std::unordered_map<uint32_t, sProjectileDescription> mapProjectiles;

	std::deque<sChatMessage> chatMessages;

	uint32_t nPlayerID = 0;

	static std::string chatMessage;
	static GameState state;

};


#endif // !Game
