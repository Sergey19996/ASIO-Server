#pragma once

#include "net_common.h"
#include "net_tsqueue.h"
#include "net_message.h"


namespace olc {

	namespace net {

		template<typename T>
		class connection : public std::enable_shared_from_this<connection<T>>
		{
		public:
			connection() {}

			virtual ~connection() {}
			

			void ConnectToServer(const asio::ip::tcp::resolver::results_type& endpoints)
			{
				// Only clients can connect to servers
				//if (m_nOwnerType == owner::client)
				//{
				//	// Request asio attempts to connect to an endpoint
				//	asio::async_connect(m_socket, endpoints,
				//		[this](std::error_code ec, asio::ip::tcp::endpoint endpoint)
				//		{
				//			if (!ec)
				//			{
				//				ReadHeader();
				//			}
				//		});
				//}
			}

			bool Disconnect();
			bool IsConnected() const;


			bool Send(const message<T>& msg);

		protected:
			// each connection has a unique socket to a remote 
			asio::ip::tcp::socket m_socket;

			//this context is shared with whole asio insstance
			asio::io_context& m_asioContext;

			//This queue holds all messages to be sent to the remote side
			// of this connections
			tsqueue<message<T>> m_qMessagesOut;


			//This queue holds all messages that have been recieved from
			//the remote side of this connection. Note it is a reference
			//as the @_owner_@ of this connection is expected to provide a queue 
			tsqueue<owned_message<T>>& m_qMessagesIn;



		private:
			// ASYNC - Prime context ready to read a message header
			void ReadHeader()
			{
				// If this function is called, we are expecting asio to wait until it receives
				// enough bytes to form a header of a message. We know the headers are a fixed
				// size, so allocate a transmission buffer large enough to store it. In fact, 
				// we will construct the message in a "temporary" message object as it's 
				// convenient to work with.
				//asio::async_read(m_socket, asio::buffer(&m_msgTemporaryIn.header, sizeof(message_header<T>)),
				//	[this](std::error_code ec, std::size_t length)
				//	{
				//		if (!ec)
				//		{
				//			// A complete message header has been read, check if this message
				//			// has a body to follow...
				//			if (m_msgTemporaryIn.header.size > 0)
				//			{
				//				// ...it does, so allocate enough space in the messages' body
				//				// vector, and issue asio with the task to read the body.
				//				m_msgTemporaryIn.body.resize(m_msgTemporaryIn.header.size);
				//				ReadBody();
				//			}
				//			else
				//			{
				//				// it doesn't, so add this bodyless message to the connections
				//				// incoming message queue
				//				AddToIncomingMessageQueue();
				//			}
				//		}
				//		else
				//		{
				//			// Reading form the client went wrong, most likely a disconnect
				//			// has occurred. Close the socket and let the system tidy it up later.
				//			std::cout << "[" << id << "] Read Header Fail.\n";
				//			m_socket.close();
				//		}
				//	});
			}


		};

	}


}