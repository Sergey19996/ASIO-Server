﻿#pragma once

#include "net_common.h"
#include "net_tsqueue.h"
#include "net_message.h"
#include "net_connection.h"


namespace olc {


	namespace net {


		template <typename T>
		class server_interface {
		public:
			server_interface(uint16_t port)
				:m_asioAccepter(m_asioContext,asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port))  // создаёт сокет указывает что v4(стандарт пока ещё )
			{

			}

			virtual ~server_interface() {
				Stop();
			}

			bool Start() {
				try
				{
					WaitForClientConnection();  //начинает асинхронный прием сообщений

					m_threadContext = std::thread([this]() {m_asioContext.run(); });  // это главный "движок" следит за событиями на сокетах, обрабатывает таймеры, вызывает колбэки (handlers

				}
				catch (const std::exception& e)
				{
					//Something prohibited the server from listening
					std::cerr << "[SERVER] Exception: " << e.what() << "\n"; 
				}

				std::cout << "[Server] Started...\n";
				return true;
			}

			void Stop() {

				// Request the context to close
				m_asioContext.stop();

				// Tidy up the context thread
				if (m_threadContext.joinable()) m_threadContext.join();

				// Inform someone, anybody, if they care...
				std::cout << "[SERVER] Stopped!\n";
			}

			//ASYNC -Instruct asio to wait for connection
			void WaitForClientConnection() {
				// Prime context with an instruction to wait until a socket connects. This
				// is the purpose of an "acceptor" object. It will provide a unique socket
				// for each incoming connection attempt
				m_asioAccepter.async_accept(
					[this](std::error_code ec, asio::ip::tcp::socket Socket) {
						if (!ec)
						{
							std::cout << "[SERVER] new Connection: " << Socket.remote_endpoint() << "\n"; 

							std::shared_ptr<connection<T>> newconn =      // cоздайтся коннект с сообщением от клиента там же и генерируется valid переменная для shakehands
							 std::make_shared<connection<T>>(connection<T>::owner::server,
								m_asioContext, std::move(Socket), m_qMessagesIn);

							//Give the user server chance to deny connection
							if (OnClientConnect(newconn)) {  // cервер отправляет сообщение ServerAccept клиенту 

								//Connection allowed, so add to container of new connections
								m_deqConnections.push_back(std::move(newconn));  

								m_deqConnections.back()->ConnectToClient(this, nIDCounter++);  // тут мы присваиваем id клиенту и начинаем  запускать проверку (handshake)

								std::cout << "[" << m_deqConnections.back()->GetID() << "] Connection Approved\n";

							}
							else
							{
								std::cout << "[- - - -] Connection Denied\n";
							}

						}
						else
						{
							std::cerr << "[SERVER] new Connection Error: " << ec.message() << "\n";
						}
						//Prime the asio context with more work - again simply wait for
						//another connection
						WaitForClientConnection();

					}
				);
			}

			//Send a message to a specific client
			void MessageClient(std::shared_ptr<connection<T>> client, const message<T>& msg) {

				if (client && client->IsConnected()) {
					client->Send(msg);
				}
				else
				{
					OnClientDisconnect(client);
					client.reset();
					m_deqConnections.erase(
						std::remove(m_deqConnections.begin(), m_deqConnections.end(), client), m_deqConnections.end());
				}

			}
			//Send message to all clients
			void MessageAllClients(const message<T>& msg,std::shared_ptr<connection<T>>pIgnoreClient = nullptr){
				bool bInvalidClientExists = false;

				// Iterate through all clients in container
				for (auto& client : m_deqConnections)
				{
					// Check client is connected...
					if (client && client->IsConnected())
					{
						// ..it is!
						if (client != pIgnoreClient)
							client->Send(msg);
					}
					else
					{
						// The client couldnt be contacted, so assume it has
						// disconnected.
						OnClientDisconnect(client);
						client.reset();

						// Set this flag to then remove dead clients from container
						bInvalidClientExists = true;
					}
				}

				// Remove dead clients, all in one go - this way, we dont invalidate the
				// container as we iterated through it.
				if (bInvalidClientExists)
					m_deqConnections.erase(
						std::remove(m_deqConnections.begin(), m_deqConnections.end(), nullptr), m_deqConnections.end());

			}

			void Update(size_t nMaxMessages = -1,bool bWait = false) {

				//For not occupy 100% of a CPU cpre
				if (bWait) m_qMessagesIn.wait();  //дальше не пойдем если в  m_qMessagesIn ничего нет

				size_t nMessageCount = 0;


				while (nMessageCount < nMaxMessages && !m_qMessagesIn.empty()) {



					auto msg = m_qMessagesIn.pop_front();
					OnMessage(msg.remote, msg.msg);
					nMessageCount++;
				}

			}

		protected:

			// This server class should override thse functions to implement
			// customised functionality

			// Called when a client connects, you can veto the connection by returning false
			virtual bool OnClientConnect(std::shared_ptr<connection<T>> client)
			{
				return false;
			}

			// Called when a client appears to have disconnected
			virtual void OnClientDisconnect(std::shared_ptr<connection<T>> client)
			{

			}

			// Called when a message arrives
			virtual void OnMessage(std::shared_ptr<connection<T>> client, message<T>& msg)
			{
				
			}

			public:
				//Called when a client is validated
				virtual void OnClientValidated(std::shared_ptr<connection<T>> client) {

				}


		protected:
			//Thread Safe Queue for incoming message packets
			tsqueue<owned_message<T>> m_qMessagesIn;

			//Container of active validated connections
			std::deque<std::shared_ptr<connection<T>>> m_deqConnections;

			//Oreder of declaration is important like a initialization
			asio::io_context m_asioContext;
			std::thread m_threadContext;

			asio::ip::tcp::acceptor m_asioAccepter;  

			//Client will be identified in the "wider system" via an ID
			uint32_t nIDCounter = 10000;
			uint32_t nProjectileIDCounter = 30000;

		};


	}


}