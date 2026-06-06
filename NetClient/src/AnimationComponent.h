#pragma once
#include <string>
#include <unordered_map>
#include "../../../NetShared/StringId.h"



struct LayerState {
    StringId currentAnim = "default"_sid;
    float timer = 0.0f;
    bool isOneShot = false;
    int framesCount = 1;
    float animSpeed = 1.0f;
};

class AnimationComponent {
private:
    // Состояния для каждого zIndex слоя
    std::unordered_map<int, LayerState> layers;

public:
    // Запуск обычной (зацикленной) анимации
    void PlayAnimation(int zIndex, StringId animId) {
        auto& state = layers[zIndex];
        if (state.currentAnim != animId) {
            state.currentAnim = animId;
            state.timer = 0.0f;
            state.isOneShot = false;
        }
    }
    //void CheckOneShotEnd(int zIndex, int framesCount, float animSpeed) {
    //    auto it = layers.find(zIndex);
    //    if (it != layers.end() && it->second.isOneShot) {
    //        // Вычисляем текущий кадр на основе накопленного времени и скорости
    //        int currentFrame = (int)(it->second.timer * animSpeed);

    //        // Если вышли за пределы количества кадров из JSON — сбрасываем в "default"
    //        if (currentFrame >= framesCount) {
    //            it->second.currentAnim = "default"_sid;
    //            it->second.timer = 0.0f;
    //            it->second.isOneShot = false;
    //        }
    //    }
    //}
    // Запуск одноразовой анимации (вспышки) с известными параметрами из JSON
    void PlayOneShot(int zIndex, StringId animId, int framesCount, float animSpeed) {
        auto& state = layers[zIndex];
        state.currentAnim = animId;
        state.timer = 0.0f;
        state.isOneShot = true;
        state.framesCount = framesCount;
        state.animSpeed = animSpeed;
    }

    // Обновление таймеров и автосброс (вызывается в общем Update)
    void Update(float dt) {
        for (auto& [zIndex, state] : layers) {
            state.timer += dt;

            if (state.isOneShot) {
                // Вычисляем, какой кадр ДОЛЖЕН БЫТЬ сейчас
                int currentFrame = (int)(state.timer * state.animSpeed);

                // Если текущий кадр превысил или равен общему числу кадров — анимация завершена
                if (currentFrame >= state.framesCount) {
                    state.currentAnim = "default"_sid;
                    state.timer = 0.0f;
                    state.isOneShot = false;
                }
            }
        }
    }

    // Геттеры для рендера
    StringId GetCurrentAnim(int zIndex) const {
        auto it = layers.find(zIndex);
        return it != layers.end() ? it->second.currentAnim : "default"_sid;
    }

    float GetTimer(int zIndex) const {
        auto it = layers.find(zIndex);
        return it != layers.end() ? it->second.timer : 0.0f;
    }
    float GetAnimSpeed(int zIndex) const {
        auto it = layers.find(zIndex);
        return it != layers.end() ? it->second.animSpeed: 0.0f;
    }
    void ForceDefault(int zIndex) {
        auto& state = layers[zIndex];
        state.currentAnim = "default"_sid;
        state.timer = 0.0f;
        state.isOneShot = false;
    }
};