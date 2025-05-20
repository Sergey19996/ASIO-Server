# 📡 ASIO Networking & OpenGL Practice

This repository contains my learning projects focused on mastering **asynchronous networking using standalone ASIO (without Boost)** and **basic graphics rendering with OpenGL/GLSL**. The goal is to build a reliable, modular client-server foundation in modern C++ and learn how to integrate low-level graphics and user input.

---

## 🔧 Features Implemented

### 📡 Networking (ASIO)
- Basic client-server architecture
- Sending and receiving messages via `tcp::socket`
- Thread-safe incoming message queue (`tsqueue`)
- Connecting to a server by hostname and port
- Asynchronous event loop using `asio::io_context` and `std::thread`
- Custom message types using `enum class`

### 🎮 Graphics (OpenGL/GLSL)
- Object rendering using **OpenGL shaders**
- Sending uniform variables like `radius` and `direction` to GLSL shaders
- **WASD keyboard controls** for movement and interaction

---

## 📁 Project Structure
Implement async send/receive with minimal blocking

Create a modular and reusable networking foundation for future projects (e.g., multiplayer games)

NetServer/ – Server-side logic
NetClient/ – Client-side OpenGL rendering and input handling
NetCommon/ – Shared code between client and server:
├── connection.h – Client-server connection logic
├── net_common.h – Common includes and type aliases (ASIO, std)
├── net_tsqueue.h – Thread-safe message queue
├── net_message.h – Message struct and serialization

---

## 🛠️ Build Information

- **Language Standard**: ISO C++14  
- **Architecture**: x64  
- **Build System**: Visual Studio  
- **Graphics API**: OpenGL + GLSL shaders  
- **Networking**: Standalone [ASIO](https://think-async.com/) (no Boost dependency)  
- **Inspiration**: Based on tutorials by [OneLoneCoder (YouTube)](https://www.youtube.com/c/OneLoneCoder)

---

## 🎯 Learning Goals

- Understand `asio::io_context` and connection lifecycles
- Implement async send/receive with minimal blocking
- Learn the principles of modular client-server architecture
- Create a reusable networking foundation for future projects (e.g., multiplayer games)
- Use OpenGL to visualize client-side logic and game-like behavior

---

## 🚀 Example Usage

- Run the server from `NetServer/`
- Launch the client from `NetClient/` to connect and render objects
- Move objects using **WASD** keys, with real-time updates sent via the network

---

Feel free to explore, fork, or contribute!
