#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#ifdef _WIN32
#define _WIN32_WINNT 0x0A00
#endif
#define ASIO_STANDALONE
#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>


std::vector<char> vBuffer(1 * 1024);

void GrabSomeData(asio::ip::tcp::socket& socket) {

	socket.async_read_some(asio::buffer(vBuffer.data(), vBuffer.size()),
		[&](std::error_code ec, std::size_t length) {
			if (!ec) {
				std::cout << "\n\nRead " << length << " bytes\n\n";
				for (int i = 0; i < length; i++) {
					std::cout << vBuffer[i];
				}

				GrabSomeData(socket);


			}
			else if (ec == asio::error::eof) {
				std::cout << "\n\nConnection closed by server\n";
			}
			else {
				std::cout << "Error on receive: " << ec.message() << std::endl;
			}


		});



}


int main() {

	asio::error_code ec;

	//context
	asio::io_context context;

	// create a work guard to keep context.run() from returning
	auto workGuard = asio::make_work_guard(context);


	//Start the context
	std::thread thrContext = std::thread([&]() {context.run(); });  // â îòäåëíüîì ïîòîêå 

	//get adress of website 
	asio::ip::tcp::endpoint endpoint(asio::ip::make_address("51.38.81.49", ec), 80);


	//socket context will deliver the omplementation
	asio::ip::tcp::socket socket(context);

	socket.connect(endpoint, ec);
	if (!ec) {
		std::cout << "Connected!" << std::endl;
	}
	else
	{
		std::cout << "Failed to connect to address:\n" << ec.message() << std::endl;
	}

	if (socket.is_open()) {

		GrabSomeData(socket);


		std::string sRequest =
			"GET /index.html HTTP/1.1\r\n"
			"Host: google.com\r\n"
			"Connection: close\r\n\r\n";

		socket.write_some(asio::buffer(sRequest.data(), sRequest.size()), ec);

		using namespace std::chrono_literals;
		std::this_thread::sleep_for(6000ms);

		context.stop();
		if (thrContext.joinable()) thrContext.join();

		//
		//workGuard.reset();
		///

	}

	system("pause");
	return 0;

}