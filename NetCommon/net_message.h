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
				return sizeof(message_header<T>) + body.size();
			}

			//override for std::cout compatibility - produces friendly description of message 
			friend std::ofstream& operator << (std::ostream& os, const message<T>& msg) {

				os << "ID: " << int(msg.header.id) << " Size:" << msg.header.size;
				return os;
			}
			// PUSH data into the message
			template<typename DataType>
			friend message<T>& operator << (message<T>& msg, const DataType& data)
			{
				static_assert(std::is_standard_layout<DataType>::value, "Data is too complex to be pushed into vector");
				size_t i = msg.body.size();
				msg.body.resize(msg.body.size() + sizeof(DataType));
				std::memcpy(msg.body.data() + i, &data, sizeof(DataType));
				msg.header.size = msg.size();
				return msg;
			}

			// PULL data from the message
			template<typename DataType>
			friend message<T>& operator >> (message<T>& msg, DataType& data) 
			{
				static_assert(std::is_standard_layout<DataType>::value, "Data is too complex to be pulled from vector");
				size_t i = msg.body.size() - sizeof(DataType);
				std::memcpy(&data, msg.body.data() + i, sizeof(DataType));
				msg.body.resize(i);
				msg.header.size = msg.size();
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