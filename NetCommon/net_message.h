#pragma once
#include "net_common.h"

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
			// PUSH data into the message
			template<typename DataType>
			friend message<T>& operator << (message<T>& msg, const DataType& data)    // msg << data 
			{
				static_assert(std::is_standard_layout<DataType>::value, "Data is too complex to be pushed into vector");
				size_t i = msg.body.size(); //фиксирует ширину предидущей записи что бы не потерять предидущие данные 
				msg.body.resize(msg.body.size() + sizeof(DataType));  // увеличиваем размер body 
				std::memcpy(msg.body.data() + i, &data, sizeof(DataType)); // копируем с заранее подгатовленным оффсетом 
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
			
			// 
			// PULL data from the message
			template<typename DataType>
			friend message<T>& operator >> (message<T>& msg, DataType& data)  // msg >> data
			{
				static_assert(std::is_standard_layout<DataType>::value, "Data is too complex to be pulled from vector");
				size_t i = msg.body.size() - sizeof(DataType); // так мы понимаем сколько данных в переменной в которую пишем мы отнимаум её вес от общего
				std::memcpy(&data, msg.body.data() + i, sizeof(DataType)); // полученную разнсоть используем как оффсет для записи в эту Data из body
				msg.body.resize(i); // уменьшаем место  в body
				msg.header.size = msg.size(); // и в header 
				return msg;
			}




		};

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