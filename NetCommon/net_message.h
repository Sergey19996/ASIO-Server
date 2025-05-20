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
				size_t i = msg.body.size(); //��������� ������ ���������� ������ ��� �� �� �������� ���������� ������ 
				msg.body.resize(msg.body.size() + sizeof(DataType));  // ����������� ������ body 
				std::memcpy(msg.body.data() + i, &data, sizeof(DataType)); // �������� � ������� �������������� �������� 
				msg.header.size = msg.size();
				return msg;
			}

			//String supprot
			friend message<T>& operator << (message<T>& msg, const std::string& data)
			{
				// ������� ���������� ������ (�������)
				msg.body.insert(msg.body.end(), data.begin(), data.end());

				// ����� ������ ������ � � �����, ����� ����� ������ �������� ��� ������
				uint32_t size = data.size();
				msg << size;

				msg.header.size = msg.size();
				return msg;
			}
			//String supprot
			friend message<T>& operator >> (message<T>& msg, std::string& data)
			{
				uint32_t size = 0;
				msg >> size; // ������ ������ ������

				// ��������: ���������� �� ������
				if (msg.body.size() < size) throw std::runtime_error("Message too short for string");

				// ������ ������ � �����
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
				size_t i = msg.body.size() - sizeof(DataType); // ��� �� �������� ������� ������ � ���������� � ������� ����� �� �������� � ��� �� ������
				std::memcpy(&data, msg.body.data() + i, sizeof(DataType)); // ���������� �������� ���������� ��� ������ ��� ������ � ��� Data �� body
				msg.body.resize(i); // ��������� �����  � body
				msg.header.size = msg.size(); // � � header 
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
			friend std::ostream& operator << (std::ostream& os, const owned_message<T>& msg) {  // ��� ������� ������ � ����� (��� ������ � file,string,�������
				os << msg.msg;
				return os;


			}


	};


	}

}