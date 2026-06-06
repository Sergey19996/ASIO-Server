#pragma once
#include "../AIBase.h"
#ifdef GAME_SERVER
class Match;
#endif
class CompanionBase : public AIBase {
public:
    CompanionBase(uint32_t id, const std::string& name, Character* owner = nullptr, Match* server = nullptr);
     

#ifdef GAME_SERVER
public:
    void OnUnderAttack(uint32_t attackerId)override;
    void SetFormationIndex(uint8_t idx);
    void SetTemporaryGoal(glm::vec2 pos, bool returnAfter = true);
    void SetDefendMode(glm::vec2 pos);
    AIState GetCurrentState() const { return currentState; }
    void SetState(AIState state) { currentState = state; };
    void OnBeforeDestroy() override;
protected:
    Character* owner = nullptr;
    float followRange;
    float teleportRange;

    void UpdateAI(float dt) override;
    virtual void UpdateTarget(float dt);
   // virtual Character* FindTarget();
    void SetTarget(uint32_t id);
    virtual void UpdateChase(float dt);
    virtual void ExecuteAttack(Character* target);
  
    void MoveAlongPath(glm::vec2 targetPos, float dt);

    float targetTimer = 0.0f;
    uint32_t targetNetId = 0; // 0 или 0xFFFFFFFF как "нет цели"
    Character* potentialTarget = nullptr;

    uint8_t formationIndex = 0;

    glm::vec2 commandTargetPos; // Точка, куда послали миньона
    bool returnToOwnerAfterMove = false;

    std::vector<glm::vec2> currentPath;
    float pathUpdateTimer = 0.0f;
   

#endif
};