#include "GameServer.h"


int main()
{
   
   
	GameServer server(60000);
	server.Start();
	



	// Настройки частоты обновления (Tickrate)
	const int ticksPerSecond = 20; // 60 обновлений в секунду
	const std::chrono::milliseconds skipTicks(1000 / ticksPerSecond);

	auto nextTick = std::chrono::steady_clock::now();

    while (true)
    {
        // 1. Сбор входящих пакетов (максимально быстро)
        // Мы не ждем, а выгребаем все, что пришло
        server.Update(-1, false);

        // 2. Проверяем, пора ли обновить логику (Fixed Update)
        auto currentTime = std::chrono::steady_clock::now();

        while (currentTime > nextTick)
        {
            // Считаем дельту для расчетов (будет примерно 0.0166с для 60 тиков)
            float deltaTime = std::chrono::duration<float>(skipTicks).count();

            server.felapsedTime = deltaTime;
            server.serverTime += deltaTime;

            auto startWork = std::chrono::steady_clock::now();

            // ВЫПОЛНЯЕМ ЛОГИКУ И РАССЫЛКУ
            server.UpdateServer();

            nextTick += skipTicks; // Сдвигаем время следующего тика

            auto endWork = std::chrono::steady_clock::now();
            auto workTime = std::chrono::duration_cast<std::chrono::microseconds>(endWork - startWork).count();

            if (workTime > 10000)  // Если логика заняла больше 10мс
                std::cout << "SERVER LAG: " << workTime << "us" << std::endl;
        }

        // 3. Отдыхаем чуть-чуть, чтобы не грузить CPU на 100%
        // Спим 1мс, чтобы моментально среагировать на новые пакеты в следующем цикле
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
   
    return 0;
}
