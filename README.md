📡 ASIO Networking Practice
This repository contains my learning projects focused on mastering asynchronous networking using ASIO (standalone, without Boost). The goal is to build a solid understanding of how to implement reliable client-server systems in modern C++.

🔧 Features Implemented:
Basic client-server architecture

Sending and receiving messages via tcp::socket

Thread-safe incoming message queue (tsqueue)

Connecting to a server by hostname and port

Asynchronous event loop using std::thread and asio::io_context

Custom message types with enum class

📁 Project Structure:
net_common.h – shared includes and type aliases (ASIO, std libs)

net_tsqueue.h – thread-safe queue for messages

net_message.h – message structure and serialization utilities

net_connection.h – low-level connection logic (send/receive)

net_client.h – client interface abstraction

simpleClient.cpp – usage example: connecting and sending messages

🎯 Learning Goals:
Understand asio::io_context and connection lifecycles

Implement async send/receive with minimal blocking

Create a modular and reusable networking foundation for future projects (e.g., multiplayer games)

