#include <iostream>
#include <olc_net.h>

#define VK_RETURN 0x0D

enum class CustomMsgTypes : uint32_t {
	ServerAccept,
	ServerDeny,
	ServerPing,
	MessageAll,
	ServerMessage,
	Message

};

std::mutex cout_mutex;
std::atomic<bool> inputEnabled = true;

class CustomClient : public olc::net::client_interface<CustomMsgTypes>
{
public:
	void PingServer() {
		olc::net::message<CustomMsgTypes> msg;
		msg.header.id = CustomMsgTypes::ServerPing;  // id == T

		// Caution with this...
		std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();

		msg << timeNow;
		Send(msg);

	}
	void MessageAll() {

		olc::net::message<CustomMsgTypes> msg;
		msg.header.id = CustomMsgTypes::MessageAll;
		Send(msg);

	}
	void Message( bool running) {
	

		ClearInput();
	
		std::cout << name <<": ";
		char ch;
		while (running) {
			std::cin.get(ch);  // блокирует поток ожидая ввода с клавиатуры  до Enter 

			if (ch == '\n') break;         // Завершить ввод при нажатии Enter
			if(ch == 27)  std::cout << "\r" << std::string(input.length(), ' ') << "\r" << std::flush;
			input.push_back(ch);           // Собираем строку

		
		}
		input =  name +": " + input; // в конце добавляем Input 
		

		olc::net::message<CustomMsgTypes> msg;  // cоздаём пустышку
		msg.header.id = CustomMsgTypes::Message; // указываем id

		

		msg << input;

		uint32_t len = input.size(); //записываем размер сообщения

		msg << len; // пишем в сообщение

	//	for (auto it = input.rbegin(); it != input.rend(); ++it)   //внедряем in в body
	//		msg.body.insert(msg.body.begin(), *it);

	//	msg.header.size = msg.size(); // пишем Общий размер сообщения 
		Send(msg);

		
	}
	void ClearInput() {
	//	std::lock_guard<std::mutex> lock(cout_mutex);
		
		buffer = input;
		reWrite = true;
		std::cout << "\r" << std::string(input.length(), ' ') << "\r" << std::flush;
		input.clear();
		
	
	}

	void ReadBuffer() {

	
		input = buffer;
		std::cout << input << std::flush;
		buffer.clear();
		
	}

	void readMessage() {


	}
	std::string name;
private:
	std::string input;
	std::string buffer;

	bool reWrite = false;
};

int main() {
	CustomClient c;
	c.Connect("127.0.0.1", 60000);

	bool key[4] = { false,false,false,false };
	bool old_key[4] = { false,false,false,false };
	
	std::cout << "Client" << std::endl;
	std::cout << "Enter you name : " << std::endl;

	char ch;
	while (true) {
		std::cin.get(ch);  // блокирует поток ожидая ввода с клавиатуры  до Enter 

		if (ch == '\n') break;         // Завершить ввод при нажатии Enter
		if (ch == 27)  std::cout << "\r" << std::string(c.name.length(), ' ') << "\r" << std::flush;
		c.name.push_back(ch);           // Собираем строку


	}

	bool bQuit = false;
	std::thread inputThread([&] {
		while (!bQuit)
		{
			if (inputEnabled) {
			std::lock_guard<std::mutex> lock(cout_mutex);
			c.Message(inputEnabled);

			}
			std::this_thread::sleep_for(std::chrono::milliseconds(10)); // немного отдохнуть
		}
		});
	while (!bQuit)
	{
		if (GetForegroundWindow() == GetConsoleWindow())
		{
			key[0] = GetAsyncKeyState('1') & 0x8000;
			key[1] = GetAsyncKeyState('2') & 0x8000;
			key[2] = GetAsyncKeyState('3') & 0x8000;
			
		}

		if (key[0] && !old_key[0]) c.PingServer();
		if (key[1] && !old_key[1]) c.MessageAll();
		if (key[2] && !old_key[2]) bQuit = true;
	

		for (int i = 0; i < 3; i++) old_key[i] = key[i];

		if (c.IsConnected())
		{
			if (!c.Incoming().empty())   // incoming - очередь просто 
			{


				auto msg = c.Incoming().pop_front().msg;

		

				switch (msg.header.id) {


				case CustomMsgTypes::ServerAccept:
				{
					
					//Server has responded to a ping request
					std::cout << "Server Accepted Connection\n";
					

				
				}
				break;
				case CustomMsgTypes::ServerPing:
				{
					// Server has responded to a ping request
					std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();
					std::chrono::system_clock::time_point timeThen;
					msg >> timeThen;
				
					std::cout << "Ping: " <<std::chrono::duration<double>(timeNow-timeThen).count() << "\n";
				}
				break;
				case CustomMsgTypes::ServerMessage:
				{
					//Server has responded to a ping request
					uint32_t clientID;
					msg >> clientID;
				
					std::cout << " Hello from [" << clientID << "]\n";


				}
				break;

				case CustomMsgTypes::Message:
				{
					inputEnabled = false;

					c.ClearInput();
					
					std::string received;   //создаётся string сообщение

					uint32_t Id = 0;
					msg >> Id;

					uint32_t len = 0; //переменная для размера
					msg >> len; // c прибывшего сообщения пишем размер
						received.resize(len);  // подготавливаем размер
					//	for (int i = len - 1; i >= 0; --i)  // и читаем с конца
					//	msg >> received[len-i-1];
						msg >> received;

						std::cout << "ID: [" << Id << "] " << received << "\n";
						

						//std::string out = "ID: [" + std::to_string(Id) + "] " + received + "\n";
						
						c.ReadBuffer();
						inputEnabled = true;
					
				}
				break;
				}
				


			}
		}
		else
		{
			
			std::cout << "Server Down\n";
			bQuit = true;
		}


	}

	inputThread.join();

	return 0;

}