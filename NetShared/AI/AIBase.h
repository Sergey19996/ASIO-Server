#pragma once
#include "../entities/Character.h"

enum class AIState {
    Idle,      // Стоит или бродит
    Chase,     // Преследует цель
    Attack,    // Выполняет атаку
    Return,     // Возвращается на точку спавна (если ушел слишком далеко)
    Follow,

    MoveToPoint, // Состояние для команды 1
    Defending    // Состояние для команды 2
};
class AIBase : public Character {
public:

    // Серверный конструктор с Match*
    AIBase(uint32_t id, const std::string& name, Match* s = nullptr);
        

    virtual ~AIBase() = default;

#ifdef GAME_SERVER
    void OnUpdate(const float dt, const float lightIntensity) override;
    int homeTile = -1;
    glm::vec2 spawnPoint = { 0,0 };
    void OnBeforeDestroy() override;
#endif
protected:
#ifdef GAME_SERVER
    virtual void UpdateAI(float dt) = 0; // Каждому мобу своя логика
    float aiThinkTimer = 0.0f;
    float chaseRange = 2.0f; // Когда начинаем бежать за игроком


    float attackRange = 1.0f;    // Дистанция начала атаки
    float stopRange = 1.0f;      // Дистанция остановки перед целью
    float attackCooldown = 0.5f; // Кулдаун между ударами/выстрелами
    SpellId defaultSpell = SpellId::MeleeHit;

    uint32_t targetIdx = 0xFFFF;
    float deathCleanupTimer = 5.0f; // 5 секунд на исчезновение


    float attackTimer = 0.0f;
#endif
    AIState currentState = AIState::Idle;
  
};