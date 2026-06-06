#pragma once
#include "net_common.h"
#include "../NetShared/LoginTypes.h"
#include "../NetShared/PlayerInfo.h"
#include "../NetShared/PlayerProfile.h"
#include "../NetShared/PlayerClass.h"

namespace olc {

	namespace net {

		//Message Header is sent at start of all messages. The template allows us
		//to use "enum class" to ensure that the messages are valid at compile time

		template <typename T>
		struct message_header
		{
			T id{};
			uint32_t size = 0;

		};

		template <typename T>
		struct message
		{
			message_header<T> header{};
			std::vector<uint8_t>body;

			//Return size message in bytes
			size_t size() const {
				return body.size();
			}

			//override for std::cout compatibility - produces friendly description of message 
			friend std::ostream& operator << (std::ostream& os, const message<T>& msg) {

				os << "ID: " << int(msg.header.id) << " Size:" << msg.header.size;
				return os;
			}

			// Добавьте эту перегрузку в ваш заголовочный файл с message
			template<typename EnumType>
			friend typename std::enable_if<std::is_enum<EnumType>::value, message<T>&>::type
				operator << (message<T>& msg, const EnumType& data)
			{
				using UnderType = typename std::underlying_type<EnumType>::type;
				return msg << static_cast<UnderType>(data); // Вызовет базовый оператор для чисел
			}

			template<typename EnumType>
			friend typename std::enable_if<std::is_enum<EnumType>::value, message<T>&>::type
				operator >> (message<T>& msg, EnumType& data)
			{
				using UnderType = typename std::underlying_type<EnumType>::type;
				UnderType temp;
				msg >> temp; // Вызовет базовый оператор для чисел
				data = static_cast<EnumType>(temp);
				return msg;
			}


			// PUSH базовый
			template<typename DataType>
			friend typename std::enable_if<!std::is_enum<DataType>::value, message<T>&>::type // Добавили проверку НЕ enum
				operator << (message<T>& msg, const DataType& data)
			{
				// Оставляем проверку на сложность только для того, что идет через memcpy
				static_assert(std::is_standard_layout<DataType>::value, "Data is too complex for direct memory copy");

				size_t i = msg.body.size();
				msg.body.resize(msg.body.size() + sizeof(DataType));
				std::memcpy(msg.body.data() + i, &data, sizeof(DataType));
				msg.header.size = msg.size();
				return msg;
			}

			//String supprot
			friend message<T>& operator << (message<T>& msg, const std::string& data)
			{
				// Сначала записываем строку (символы)
				msg.body.insert(msg.body.end(), data.begin(), data.end());

				// Затем размер строки — в конец, чтобы потом первым считался при чтении
				uint32_t size = data.size();
				msg << size;

				msg.header.size = msg.size();
				return msg;
			}
			//String supprot
			friend message<T>& operator >> (message<T>& msg, std::string& data)
			{
				uint32_t size = 0;
				msg >> size; // читаем размер строки

				// Проверка: достаточно ли данных
				if (msg.body.size() < size) throw std::runtime_error("Message too short for string");

				// Читаем строку с конца
				data.resize(size);
				std::memcpy(&data[0], msg.body.data() + msg.body.size() - size, size);
				msg.body.resize(msg.body.size() - size);

				msg.header.size = msg.size();
				return msg;
			}
			friend olc::net::message<GameMsg>& operator << (olc::net::message<GameMsg>& msg, const PlayerProfile& profile) {
				// Пишем в любом порядке, но помним: что положили ПОСЛЕДНИМ, то достанем ПЕРВЫМ
				msg << profile.name;
				msg << profile.coins << profile.wins << profile.losses;

				// Упаковываем векторы (сначала содержимое, потом размер)
				for (auto id : profile.unlocked) msg << id;
				msg << (uint32_t)profile.unlocked.size();

				for (auto id : profile.friends) msg << id;
				msg << (uint32_t)profile.friends.size();

				//msg << profile.selectedclass; // Enum подхватится вашим шаблоном
				return msg;
			}

			// ЧТЕНИЕ (POP) — В ОБРАТНОМ ПОРЯДКЕ!
			friend olc::net::message<GameMsg>& operator >> (olc::net::message<GameMsg>& msg, PlayerProfile& profile) {
				// 1. Последним вошел selectedclass -> первым вышел
			//	msg >> profile.selectedclass;

				// 2. Читаем вектор friends
				uint32_t fSize = 0;
				msg >> fSize;
				profile.friends.resize(fSize);
				for (int i = fSize - 1; i >= 0; i--) msg >> profile.friends[i]; // Читаем элементы с конца!

				// 3. Читаем вектор unlocked
				uint32_t uSize = 0;
				msg >> uSize;
				profile.unlocked.resize(uSize);
				for (int i = uSize - 1; i >= 0; i--) msg >> profile.unlocked[i];

				// 4. Читаем числа
				msg >> profile.losses >> profile.wins >> profile.coins;

				// 5. Первым вошло имя -> последним вышло
				msg >> profile.name;

				return msg;
			}
			// 
			// PULL базовый
			template<typename DataType>
			friend typename std::enable_if<!std::is_enum<DataType>::value, message<T>&>::type // Добавили проверку НЕ enum
				operator >> (message<T>& msg, DataType& data)
			{
				static_assert(std::is_standard_layout<DataType>::value, "Data is too complex for direct memory pull");

				size_t i = msg.body.size() - sizeof(DataType);
				std::memcpy(&data, msg.body.data() + i, sizeof(DataType));
				msg.body.resize(i);
				msg.header.size = msg.size();
				return msg;
			}
		
			template<typename ItemType>
			friend message<T>& operator << (message<T>& msg, const std::vector<ItemType>& data)
			{
				// 1. Сначала записываем сами данные элементов
				// Если это простые типы (POD), можно оптимизировать через memcpy, 
				// но безопаснее для расширяемости пройтись циклом:
				for (const auto& item : data) {
					msg << item;
				}

				// 2. В самом конце записываем количество элементов
				uint32_t size = static_cast<uint32_t>(data.size());
				msg << size;

				msg.header.size = msg.size();
				return msg;
			}
			template<typename ItemType>
			friend message<T>& operator >> (message<T>& msg, std::vector<ItemType>& data)
			{
				// 1. Сначала вытаскиваем размер вектора (он был записан последним)
				uint32_t size = 0;
				msg >> size;

				// 2. Подготавливаем вектор
				data.resize(size);

				// 3. Читаем элементы. 
				// ВАЖНО: так как это стек, читаем в обратном порядке от записи
				for (int i = size - 1; i >= 0; i--) {
					msg >> data[i];
				}

				msg.header.size = msg.size();
				return msg;
			}

		};
		// Для LoginRequest
		inline olc::net::message<GameMsg>& operator<<(
			olc::net::message<GameMsg>& msg, const sLoginRequest& req)
		{
			msg << req.password;
			msg << req.firstName;
			msg << req.sessionToken;
			return msg;
		}

		inline olc::net::message<GameMsg>& operator>>(
			olc::net::message<GameMsg>& msg, sLoginRequest& req)
		{
			msg >> req.sessionToken;
			msg >> req.firstName;
			msg >> req.password;
			return msg;
		}

		// Для LoginResult
		inline olc::net::message<GameMsg>& operator<<(
			olc::net::message<GameMsg>& msg, const sLoginResult& res)
		{
			msg << res.success;
			msg << res.reason;
			msg << res.assignedToken;
			msg << res.assignedID;
			msg << res.errorMessage;
			return msg;
		}

		inline olc::net::message<GameMsg>& operator>>(
			olc::net::message<GameMsg>& msg, sLoginResult& res)
		{
			msg >> res.errorMessage;
			msg >> res.assignedID;
			msg >> res.assignedToken;
			msg >> res.reason;
			msg >> res.success;
			return msg;
		}
		inline message<GameMsg>& operator<<(message<GameMsg>& msg, const sPlayerInfo& info)
		{
			msg << info.playerId;
			msg << info.playerClass;
			msg << info.name;
			return msg;
		}

		inline message<GameMsg>& operator>>(message<GameMsg>& msg, sPlayerInfo& info)
		{
			msg >> info.name;
			msg >> info.playerClass;
			msg >> info.playerId;
			return msg;
		}


		template<typename T>
		class connection;


		template <typename T>
		struct owned_message{

			std::shared_ptr<connection<T>> remote = nullptr;
			message<T> msg;

			//string maker
			friend std::ostream& operator << (std::ostream& os, const owned_message<T>& msg) {  // для удобной записи в поток (для записи в file,string,консоль
				os << msg.msg;
				return os;


			}


	};


	}

}
//Последнее записанное значение читается первым

//Первое записанное значение читается последним