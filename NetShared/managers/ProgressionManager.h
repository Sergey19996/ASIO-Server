#pragma once
#include <vector>
#include <cstdint>

class Character;

class ProgressionManager {
    Character* owner;
    uint8_t level = 1;
    float currentXP = 0.0f;

    // Таблица из нашего обсуждения
    const std::vector<float> xpTable = { 0, 100, 250, 450, 700, 1050, 1550, 2200, 3100, 4500 , 7000};

public:
    ProgressionManager(Character* owner);

    void AddExperience(float amount);
    void AddLevel(uint32_t lvl);
    void SetExp(float exp);
    uint8_t GetLevel() const { return level; }
    float GetXP() const { return currentXP; }
    float GetRequiredXP() { return xpTable[level];}
    // Метод для синхронизации клиента (сеттер без логики)
    void SetDataRaw(uint32_t lvl, float xp) { level = lvl; currentXP = xp; }

private:
    void OnLevelUp();
};