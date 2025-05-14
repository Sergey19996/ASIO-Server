#include <olc_net.h>

enum class CustomMsgTypes : uint32_t {
	ServerAccept,
	ServerDeny,
	ServerPing,
	MessageAll,
	ServerMessage,
	Message

};

class CustomServer : public olc::net::server_interface<CustomMsgTypes>{
public:
	CustomServer(uint16_t nPort) : olc::net::server_interface<CustomMsgTypes>(nPort) {

	}

protected:

	virtual bool OnClientConnect(std::shared_ptr<olc::net::connection<CustomMsgTypes>> client)  {
		olc::net::message<CustomMsgTypes> msg;  //создаётся пустое сообщение
		msg.header.id = CustomMsgTypes::ServerAccept; //в id присваиваем Accept
		client->Send(msg);  // отправляем клиенту 
		return true;  //клиент принят
	}
	//when client appeares
	virtual void OnClientDisconnect(std::shared_ptr<olc::net::connection<CustomMsgTypes>> client)  {
		std::cout << "Removing client [" << client->GetID() << "]\n";
	}
	//called when message arrives
	virtual void OnMessage(std::shared_ptr<olc::net::connection<CustomMsgTypes>> client, olc::net::message<CustomMsgTypes>& msg) { //тут client это тот кто отправил и его msg
		switch (msg.header.id) {

		case CustomMsgTypes::ServerPing:
		{
			std::cout << "[" << client->GetID() << "]: Server Ping\n";

			client->Send(msg);
		}
		break;

		case CustomMsgTypes::MessageAll:
		{
			std::cout << "[" << client->GetID() << "]: Message All\n";
			olc::net::message<CustomMsgTypes>msg;
			msg.header.id = CustomMsgTypes::ServerMessage; 
			msg << client->GetID();
			MessageAllClients(msg, client);
		}
		break;
		case CustomMsgTypes::Message: //что ты положишь в конец body, то получатель вытащит первым.
		{

			//чтение текста для string
			std::string received;
			uint32_t len = 0;
			msg >> len;

			received.resize(len);
			//for (int i = len - 1; i >= 0; --i)
			//	msg >> received[i];

			msg >> received;

			//Подготовка нового сообщения
			olc::net::message<CustomMsgTypes> forwardMsg;
			forwardMsg.header.id = CustomMsgTypes::Message;

			// Сначала пишем строку (в обратном порядке)
		//	for (auto it = received.rbegin(); it != received.rend(); ++it)
		//		forwardMsg << *it;

			forwardMsg << received;
			// Затем размер строки
			forwardMsg << len;

			// Затем ID клиента
			forwardMsg << client->GetID();

			// Обновим размер
			//forwardMsg.header.size = forwardMsg.size();

			MessageAllClients(forwardMsg, client);
		}
		break;
		}
	
	}
};

int main() {
	CustomServer server(60000);
	server.Start();
	while (1)
	{
		server.Update(-1,true);
	}
	return 0;

}