#pragma once
#include "net_common.h"
#include "net_message.h"
#include "net_tsqueue.h"
#include "net_connection.h"

namespace olc {


	namespace net {


		template <typename T>
		class client_interface {
		public:

			client_interface() : m_socket(m_context) {}
			~client_interface() {
				Disconnect();
			}
			
			bool Connect(const std::string& host, const uint16_t port) {
				try {

					//Create Connection
					m_connection = std::make_unique<connection<T>>();  //TODO

					//Resolve hostname/ip-adress into tangiable physical address
					asio::ip::tcp::resolver resolver(m_context);
					m_endpoints = resolver.resolve(host, std::to_string(port));

					//conect to server
					m_connection->ConnectToServer(m_endpoints);

					//Start thread
					thrContext = std::thread([this]() {m_context.run(); });


				}
				catch (std::exception& e) {
					std::cerr << "Client Exception: " << e.what() << "\n";
					return false;

				}

				return false;
			}
			void Disconnect() {

				if (IsConnected()) {

					m_connection->Disconnect();
				}

				m_context.stop();
				if (thrContext.joinable())
					thrContext.join();

				//Destroy connection object
				m_connection.release();

			}
			bool IsConnected() {
				if (m_connection)
					return m_connection->IsConnected();
				else
					return false;

				

			}
			tsqueue<owned_message<T>>& Incoming() {
				return m_qMessageIn;
			}
			void Send(const message<T>& msg)
			{
				if (IsConnected())
					m_connection->Send(msg);
			}

		protected:
		
			asio::io_context m_context;
			std::thread thrContext;
			asio::ip::tcp::socket m_socket;

			std::unique_ptr<connection<T>> m_connection;
		

		private:
		
			tsqueue<owned_message<T>> M_qMessagesIn;



		};



	 }


}