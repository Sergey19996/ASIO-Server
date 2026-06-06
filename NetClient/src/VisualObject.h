#pragma once
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include "AnimationComponent.h"
struct VisualObject {
    glm::vec2 vCurrentPos;        // То, что мы рисуем на экране
    glm::vec2 vTargetPos;         // То, что пришло от сервера
    bool bPendingRemoval = false; // Ждет удаления
    float fLightIntensity = 1.0f; // По умолчанию светит на полную
    uint32_t ownerID = 0;         // ID владельца
    float rotation = 0.0f;
    // --- НОВЫЕ ПОЛЯ ДЛЯ ПЕРСОНАЖЕЙ ---
    float fCurrentHeadAngle = 0.0f; // Плавный угол головы/оружия (в градусах)

    // Наш новый менеджер анимаций, который заменяет fLegsAnimTime и fBodyAnimTime
    AnimationComponent animComp;
    // --- КОНСТРУКТОР ДЛЯ АВТО-ИНИЦИАЛИЗАЦИИ ---
    VisualObject()
        : vCurrentPos(0.0f, 0.0f)
        , vTargetPos(0.0f, 0.0f)
        , bPendingRemoval(false)
        , fLightIntensity(1.0f)
        , ownerID(0)
        , fCurrentHeadAngle(0.0f)
    {
        // Как только объект создается — жестко задаем базовые анимации для слоев 1 и 2
        animComp.PlayAnimation(1, "default"_sid);
        animComp.PlayAnimation(2, "default"_sid);
    }

    void Update(float dt, float targetHeadAngleDegrees, bool isMoving) {
        // 1. Плавный догон позиции
        float lerpFactor = 1.0f - expf(-15.0f * dt);
        vCurrentPos += (vTargetPos - vCurrentPos) * lerpFactor;

        if (bPendingRemoval) {
            fLightIntensity -= dt * 5.0f;
            if (fLightIntensity < 0.0f) fLightIntensity = 0.0f;
        }

        // 2. ПЛАВНЫЙ ПОВОРОТ ГОЛОВЫ И РУК (с учетом перехода через 360 градусов)
        float angleDiff = targetHeadAngleDegrees - fCurrentHeadAngle;

        while (angleDiff < -180.0f) angleDiff += 360.0f;
        while (angleDiff > 180.0f)  angleDiff -= 360.0f;

        fCurrentHeadAngle += angleDiff * 8.0f * dt;

        // 3. ОБНОВЛЕНИЕ ТАЙМЕРОВ МЕНЕДЖЕРА АНИМАЦИЙ
        // Передаем дельта-тайм в менеджер, чтобы он обновил таймеры всех активных слоев
        animComp.Update(dt);
    }

    // --- МЕТОДЫ ДЛЯ МЕНЕДЖЕРА РЕНДЕРА ---

    // Возвращает уже посчитанный сглаженный угол
    float GetSmoothedHeadAngle() const {
        return fCurrentHeadAngle;
    }

    bool HasReachedTarget() const {
        return glm::distance(vCurrentPos, vTargetPos) < 0.1f;
    }
};