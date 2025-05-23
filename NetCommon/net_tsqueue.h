	#pragma once
	#include "net_common.h"

	namespace olc {

		namespace net {

			template<typename T>
			class tsqueue {
			public:
				tsqueue() = default;
				tsqueue(const tsqueue<T>&) = delete;  //because it has mutex in it
				virtual ~tsqueue() { Clear(); }
				

				//Get Front
				const T& front() {
					std::scoped_lock guard(muxQueue); // ���� ������ ����� ��� ������ muxQueue, ���� ����� �����
					return deqQueue.front();  // ������ �������

				}  //��� ������ �� ������� ��������� (}): scoped_lock ������������� ����������� �������.


				//Get Back
				const T& back() {

					std::scoped_lock guard(muxQueue);
					return deqQueue.back();

				}

				void push_back(const T& item) {

					std::scoped_lock guard(muxQueue);
					deqQueue.emplace_back(item);

					std::unique_lock<std::mutex> u1(muxBlocking);
					cvBlocking.notify_one();


				}

				void push_front(const T& item) {

					std::scoped_lock guard(muxQueue);
					deqQueue.emplace_front(std::move(item));

					std::unique_lock<std::mutex> u1(muxBlocking);
					cvBlocking.notify_one();
				}

				bool empty() {

					std::scoped_lock guard(muxQueue);
					return deqQueue.empty();
				}
				size_t count() {

					std::scoped_lock guard(muxQueue);
					return deqQueue.size();
				}
				void Clear() {
					std::scoped_lock guard(muxQueue);
					deqQueue.clear();

				}

				void wait() {
					while (empty()) //push back / push front �������� �����
					{
						std::unique_lock<std::mutex> u1(muxBlocking);
						cvBlocking.wait(u1);
					};

				}

				//Removes and return item from front
				T pop_front() {
					std::scoped_lock guard(muxQueue);
					auto t = std::move(deqQueue.front());
					deqQueue.pop_front();
					return t;

				}
				T pop_back() {
					std::scoped_lock guard(muxQueue);
					auto t = std::move(deqQueue.back());
					deqQueue.pop_back();
					return t;

				}



			protected:
				std::mutex muxQueue;  //for stoping thread
				std::deque<T> deqQueue; // core of class 

				std::condition_variable cvBlocking;
				std::mutex muxBlocking;
			};

		}

	}