#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Rendering/Shader.h"
#include <vector>
#include <unordered_map>

struct Light {
    glm::vec2 pos;
    float radius;
};
struct BeaconLight {
    uint16_t index; // чтобы легко находить и удалять
    glm::vec2 pos;
    float radius;
};
struct MinimapData {
    GLuint memoryTex;
    GLuint currentFogTex;
    // ... другие данные
};
class Fog
{
  

public:
    ~Fog();

    void Init(int levelWidth, int levelHeight);

    void Update(const glm::vec2& screenPlayerPos, float radiusPx);
    void RenderMask( const int& w, const int& h,const float& dayCycleProgress);   // FBO
   

    void DrawFullscreenQuad();

    void Resize(int w, int h);
    void SetLevelBlocks(const std::vector<uint8_t>& blocks,int maskResW,int maskResH);
    GLuint& GetFogTexture() { return fogTexture; };
    GLuint& GetMemoryTexture() { return memoryTexture; };
    GLuint& GetLightTexture() { return lightTexture; };
    void UpdateMemory();
    void BlurFog();
    void UpdateLight(const float& lightIntensity);
    void reset();
    void AddBeaconLight(uint16_t idx, glm::vec2 pos, float rad);
    void RemoveBeaconLight(uint16_t idx);
   
    Shader* fogMaskShader = nullptr;
    Shader* fogCopyShader = nullptr;
    Shader* blurShader = nullptr;
    Shader* lightShader = nullptr;
    float viewRadius = 0.0f;

    void AddLight(const glm::vec2& pos, float radius);
    void ClearLight();
    MinimapData GetMinimapData() {
        return { memoryTexture, fogTexture };
    }
    std::vector<BeaconLight>& GetBeacons() { return myBeacons; };

private:
    GLuint fogFBO = 0;


    glm::vec2 playerPos;

    int levelWidth, levelHeight;
    int maskResW = 0;
    int maskResH = 0;


    GLuint quadVAO = 0;
    GLuint quadVBO = 0;
    GLuint levelBlockTex = 0;

    GLuint fogTexture = 0;
    GLuint memoryTexture;
    GLuint memoryFBO;

    GLuint LightFBO;
    GLuint lightTexture = 0;

    // В классе Fog:
    std::vector<Light> activeLights;

    GLuint pingPongFBO[2];      // Два FBO
    GLuint pingPongTextures[2]; // Две текстуры для промежуточного блюра
    std::vector<BeaconLight> myBeacons;
};