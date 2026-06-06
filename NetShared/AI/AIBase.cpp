#include "AIBase.h"

#ifdef GAME_SERVER
#include "../../NetServer/Match.h"
#endif // !GAME_SERVER

AIBase::AIBase(uint32_t id, const std::string& name, Match* s): Character(id, name, s) // Вызов родителя
{

}
#ifdef GAME_SERVER
void AIBase::OnUpdate(const float dt, const float lightIntensity)
{
    {
        if (IsDead()) {
           
            deathCleanupTimer -= dt;
            if (deathCleanupTimer <= 0) {
                // Только когда труп "сгнил", просим Match его окончательно удалить
                server->CleanupCharacter(this->id);
            }
            return; // Мертвые не думают и не двигаются
        }

        // 1. "Думаем" реже, чем обновляем физику (например, 10 раз в секунду)
        aiThinkTimer -= dt;
        if (aiThinkTimer <= 0) {
            UpdateAI(dt);
            aiThinkTimer = 0.1f;
        }

        // 2. Вызываем базовое обновление (если там есть важная логика)
        Character::OnUpdate(dt, lightIntensity);
    }
}
void AIBase::OnBeforeDestroy()
{
        if (server) {
            // Моб сам просит сервер уменьшить счетчик своего дома
            server->NotifyMobDied(this->homeTile);
          
        }
}
#endif // !GAME_SERVER