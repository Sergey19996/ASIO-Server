#include <olc_net.h>

enum class CustomMsgTypes : uint32_t {
	ServerAccept,
	ServerDeny,
	ServerPing,
	MessageAll,
	ServerMessage

};

class CustomServer : public olc::net::server_interface<CustomMsgTypes>{
public:
	CustomServer(uint16_t nPort) : olc::net::server_interface<CustomMsgTypes>(nPort) {

	}

protected:

	virtual bool OnClientConnected(std::shared_ptr<olc::net::connection<CustomMsgTypes>> client) {
		return true;
	}
	//when client appeares
	virtual void OnClientDisconnected(std::shared_ptr<olc::net::connection<CustomMsgTypes>> client) {

	}
	//called when message arrives
	virtual void OnMessage(std::shared_ptr<olc::net::connection<CustomMsgTypes>>client, std::shared_ptr<olc::net::connection<CustomMsgTypes>>server) {

	}
};

int main() {
	CustomServer server(60000);
	server.Start();
	while (1)
	{
		server.Update();
	}
	return 0;

}