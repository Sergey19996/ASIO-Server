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
					std::scoped_lock lock(muxQueue); // если другой поток уже держит muxQueue, этот будет ждать
					return deqQueue.front();  // доступ защищЄн

				}  //ѕри выходе из области видимости (}): scoped_lock автоматически освобождает мьютекс.


				//Get Back
				const T& back() {

					std::scoped_lock lock(muxQueue);
					return deqQueue.back();

				}

				void push_back(const T& item) {

					std::scoped_lock lock(muxQueue);
					deqQueue.emplace_back(item);

				}

				void push_front(const T& item) {

					std::scoped_lock lock(muxQueue);
					deqQueue.emplace_front(std::move(item));

				}

				bool empty() {

					std::scoped_lock lock(muxQueue);
					return deqQueue.empty();
				}
				size_t count() {

					std::scoped_lock lock(muxQueue);
					return deqQueue.size();
				}
				void Clear() {
					std::scoped_lock lock(muxQueue);
					deqQueue.clear();

				}

				//Removes and return item from front
				T pop_front() {
					std::scoped_lock lock(muxQueue);
					auto t = std::move(deqQueue.front());
					deqQueue.pop_front();
					return t;

				}
				T pop_back() {
					std::scoped_lock lock(muxQueue);
					auto t = std::move(deqQueue.back());
					deqQueue.pop_back();
					return t;

				}



			protected:
				std::mutex muxQueue;  //for stoping thread
				std::deque<T> deqQueue; // core of class 


			};

		}

	}