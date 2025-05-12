#pragma once

#include "net_common.h"
#include "net_tsqueue.h"
#include "net_message.h"


namespace olc {

	namespace net {

		//Forwar declare 
		template<typename T>
		class server_interface;


		template<typename T>
		class connection : public std::enable_shared_from_this<connection<T>>
		{
		public:
			enum class owner {
				server,
				client

			};


			connection(owner parent,asio::io_context& asioContext,asio::ip::tcp::socket socket,tsqueue<owned_message<T>>& gIn)  //qmassageIn 
				: m_asioContext(asioContext),m_socket(std::move(socket)), m_qMessagesIn(gIn){
			
				m_nOwnerType = parent;

				//Construct validation check data
				if (m_nOwnerType == owner::server) {

					//Connection is Server ->Client, COnstruct Random data for the client
					//to transform and send back for validation
					m_nHandShakeOut = uint64_t(std::chrono::system_clock::now().time_since_epoch().count());  // берём текущее время от 1970 и count()вернёт число

					//Pre-calculate the results for checking when the client responds
					m_nHandShakeCheck = scramble(m_nHandShakeOut);


				}
				else
				{
					//Connection is Client -> Server, so we have nothing to define 
					m_nHandShakeIn = 0;
					m_nHandShakeOut = 0;

				}

			}

			virtual ~connection() {}
			

			uint32_t GetID() const {

				return id;

			}

			void ConnectToClient(olc::net::server_interface<T>* server,uint32_t uid = 0) {

				if (m_nOwnerType == owner::server) {

					if (m_socket.is_open()) {

						id = uid;
						//ReadHeader();
						
						// A client has attempted to connect to the server, but we wish
						//the client to first validate itself, so first write out the 
						//handshake data to be validated 
						WriteValidation();  // отправляем клиенту  

						//Next, issue a task to sit and wait asynchonously for precisely
						//the validation data sent back from the client
						ReadValidation(server); // проверяем 


					}

				}

			}

			void ConnectToServer(const asio::ip::tcp::resolver::results_type& endpoints)
			{
				// Only clients can connect to servers
				if (m_nOwnerType == owner::client)
				{
					// Request asio attempts to connect to an endpoint
					asio::async_connect(m_socket, endpoints,
						[this](std::error_code ec, asio::ip::tcp::endpoint endpoint)
						{
							if (!ec)  // success connection
							{
								//ReadHeader();

								//First thing sesrver will do is send packet to be validated
								//o wait for thet and respond
								ReadValidation();


							}
						});
				}
			}

			void Disconnect() {
				if (IsConnected()) {
					asio::post(m_asioContext, [this]() { m_socket.close(); });
				}
			}
			bool IsConnected() const {

				return m_socket.is_open();
			}


			void Send(const message<T>& msg) {

				asio::post(m_asioContext, //Используется asio::post, чтобы убедиться, что код выполнится в потоке m_asioContext
					[this, msg]()
					{
						bool bWritingMessage = !m_qMessagesOut.empty();
						m_qMessagesOut.push_back(msg);  //cообщение добовляем в очередь 
						if (!bWritingMessage) { // если очередь была пуста 
							WriteHeader();  // начало цепочки async_write, которая сначала отправляет заголовок, потом тело сообщения.

						}
					});
					
			}

		

		private:
	
			void ReadHeader()
			{
				
				asio::async_read(m_socket, asio::buffer(&m_msgTemporaryIn.header, sizeof(message_header<T>)),  // во временный указатель записываем hedaer
					[this](std::error_code ec, std::size_t length)
					{
						if (!ec)
						{
						
							if (m_msgTemporaryIn.header.size > 0)  // если чтение прошло успешно  и место для тела есть 
							{
								m_msgTemporaryIn.body.resize(m_msgTemporaryIn.header.size); //корректируем размер тела 
								ReadBody();
							}
							else
							{
								AddToIncomingMessageQueue();
							}
						}
						else
						{
						
							std::cout << "[" << id << "] Read Header Fail.\n";
							m_socket.close();
						}
					});
			}
			void ReadBody() {
				asio::async_read(m_socket, asio::buffer(m_msgTemporaryIn.body.data(), m_msgTemporaryIn.body.size()), // во временный указатель пишем тело
					[this](std::error_code ec, std::size_t length) {
						if (!ec) {
							AddToIncomingMessageQueue();
						}
						else
						{
							std::cout << "[" << id << "] Read Body Fail.\n";
							m_socket.close();
						}
					});

			}

			void WriteHeader() {  // мы отправляем раздельными кусками - сперва header  затем body 
				asio::async_write(m_socket, asio::buffer(&m_qMessagesOut.front().header, sizeof(message_header<T>)), //Оборачиваем в буфер ASIO заголовок первого сообщения в очереди 
					[this](std::error_code ec, std::size_t length) {
						if (!ec)
						{
							if (m_qMessagesOut.front().body.size() > 0)  // если тело вообще есть
							{
								WriteBody();  // если у сообщения есть тело — отправим его
							}
							else
							{
								m_qMessagesOut.pop_front(); // иначе удалим сообщение из очереди;
								if (!m_qMessagesOut.empty())
								{
									WriteHeader();  // и начнём отправку следующего
								}
							}
						}

					});

			}

			void WriteBody() {
				asio::async_write(m_socket, asio::buffer(m_qMessagesOut.front().body.data(), m_qMessagesOut.front().body.size()),//Оборачиваем в буфер ASIO тело первого сообщения в очереди 
					[this](std::error_code ec, std::size_t length) {
						if (!ec) { // отправка тела была успешной

							m_qMessagesOut.pop_front();   //чистим 


							if (!m_qMessagesOut.empty()) {  // если всё ещё не пустая
								 
								WriteHeader();  // отправляем писать header

							}

						}
						else
						{
							std::cout << "[" << id << "] Write Body Fail.\n";
							m_socket.close();
						}

					}

				);


			}

			void AddToIncomingMessageQueue() {

				if (m_nOwnerType == owner::server)
					m_qMessagesIn.push_back({ this->shared_from_this(),  m_msgTemporaryIn });  //  Кладём и указатель на того, кто прислал (для сервера)
				else
					m_qMessagesIn.push_back({ nullptr,m_msgTemporaryIn });

				ReadHeader(); //сразу запускаем чтение следующего сообщения:
				
			}

			//"Encrypt" data
			uint64_t scramble(uint64_t nInput) {
				uint64_t out = nInput ^ 0xDEADBEEFC0DECAFE;  // там где не совпадут 0 и 1 м запишим 1 всё остальное 0 ^ XOR

			 //                  	                  "|" OR складывает обе части :
				out = (out & 0xF0F0F0F0F0F0F0F0) >> 4 | (out & 0x0F0F0F0F0F0F0F0F) << 4; //превращает 0xAB в 0xBA.
				return out ^ 0xC0DEFACE12345678;  // там где не совпадут 0 и 1 м запишим 1 всё остальное 0 ^ XOR
			}

			//ASYNC -Used by both client and server to write validation packet
			void WriteValidation() {
				asio::async_write(m_socket, asio::buffer(&m_nHandShakeOut, sizeof(uint64_t)),
					[this](std::error_code ec, std::size_t length) {

						if (!ec) {

							//Validation data sent, clinets should sit and wait
							//for a response ( or a closure)
							if (m_nOwnerType == owner::client)
								ReadHeader();

						}
						else
						{
							m_socket.close();
						}


					});
			}

			void ReadValidation(olc::net::server_interface<T>* server = nullptr) { // читает  &m_nHandShakeIn

				asio::async_read(m_socket, asio::buffer(&m_nHandShakeIn, sizeof(uint64_t)),
					[this, server](std::error_code ec, std::size_t length) {

						if (!ec) {

							if (m_nOwnerType == owner::server) {

								if (m_nHandShakeIn == m_nHandShakeCheck) {

									//Client has provided Valid solution, so allow it to connect 
									std::cout << "Client Validated" << std::endl;
									server->OnClientValidated(this->shared_from_this());


									//Sit waiting to receive data now
									ReadHeader();

								}
								else
								{
									//Client Gave Incorrect data, so Disconnect
									std::cout << "Client Disconnected (Fail Validation)" << std::endl;
									m_socket.close();
								}

							}
							else // if client
							{
								//Connection is a client, so solve puzzle
								m_nHandShakeOut = scramble(m_nHandShakeIn);

								//Write the result
								WriteValidation();  // отправялет m_nHandShakeOut

							}


						}
						else
						{
							//Some biggerfailure occured
							std::cout << "Client Disconnected (ReadValidation)" << std::endl;
							m_socket.close();
						}

					}
				);

			}

			protected:
			
				asio::ip::tcp::socket m_socket;

				asio::io_context& m_asioContext;

				tsqueue<message<T>> m_qMessagesOut; //Очередь исходящих сообщений (то есть сообщений, которые надо отправить по сети).

				tsqueue<owned_message<T>>& m_qMessagesIn; //Ссылка на очередь входящих сообщений (то есть, полученных по сети).
															/*Сообщение(message<T>)
																И, скорее всего, кто его отправил(например, указатель на connection).*/


				message<T> m_msgTemporaryIn; //Временное хранилище для сообщения, которое сейчас читается из сокета.
											//Это нужно потому, что чтение происходит по частям: сначала заголовок, потом тело.

				owner m_nOwnerType = owner::server;
				uint32_t id = 0;

				//Handshake Validation
				uint64_t m_nHandShakeOut = 0; //Send
				uint64_t m_nHandShakeIn = 0;  //received
				uint64_t m_nHandShakeCheck = 0; // use erver for compare 
				 

		};

	}


}