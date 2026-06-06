#include "Fog.h"


Fog::~Fog()
{
    // Удаляем текстуры
    if (fogTexture) glDeleteTextures(1, &fogTexture);
    if (levelBlockTex) glDeleteTextures(1, &levelBlockTex);

    // Удаляем фреймбуфер
    if (fogFBO) glDeleteFramebuffers(1, &fogFBO);

    // Удаляем буферы геометрии
    if (quadVBO) glDeleteBuffers(1, &quadVBO);
    if (quadVAO) glDeleteVertexArrays(1, &quadVAO);

    if (memoryTexture) glDeleteTextures(1, &memoryTexture);
    if (memoryFBO) glDeleteFramebuffers(1, &memoryFBO);

}

void Fog::Init(int LevelW, int LevelH)
{
    // 1. Сохраняем размеры окна (нужны для расчетов и вьюпорта)
    this->maskResW = LevelW * 4.0f;
    this->maskResH = LevelH * 4.0f;



    // 3. Координаты полноэкранного прямоугольника (Quad)
    // Используем диапазон [0, 1], который удобно растягивать в шейдере
    float quadVertices[] = {
    0.0f, 0.0f, // Нижний левый
    1.0f, 0.0f,  // Нижний правый
    1.0f, 1.0f,  // Верхний правый
    0.0f, 0.0f, // Нижний левый (повтор для треугольника)
    1.0f, 1.0f, // Верхний правый (повтор для треугольника)
    0.0f, 1.0f  // Верхний левый
    };

    // Генерируем объекты в памяти GPU для хранения геометрии
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);

    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);

    // Загружаем координаты в видеопамять
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    // Описываем формат данных: 0-й атрибут, 2 float (x, y), без отступов
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

    glBindVertexArray(0); // Отвязываем VAO, чтобы случайно не испортить





    // 4. Настройка Framebuffer Object (FBO) — "закадрового холста"
    // Мы будем рисовать туман не на экран, а в отдельную текстуру
    glGenFramebuffers(1, &fogFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, fogFBO);

    // Создаем саму текстуру, куда пойдет "рисунок" тумана
    glGenTextures(1, &fogTexture);
    glBindTexture(GL_TEXTURE_2D, fogTexture);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_R8,    // Экономия: используем только красный канал (8 бит)
        maskResW,   // Уменьшенная ширина
        maskResH,   // Уменьшенная высота
        0,
        GL_RED,   // Формат данных — один канал
        GL_UNSIGNED_BYTE,
        nullptr   // Данных пока нет, просто выделяем память
    );
    // Настройка фильтрации: LINEAR сделает границы тумана мягкими при растягивании
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // CLAMP_TO_EDGE, чтобы туман не "зацикливался" по краям экрана
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Привязываем текстуру к фреймбуферу как "точку вывода цвета №0"
    glFramebufferTexture2D(   
        GL_FRAMEBUFFER,
        GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D,
        fogTexture,
        0
    );
    // Проверка: если видеокарта не смогла создать такой конфиг, программа выдаст ошибку
    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0); // Возвращаемся к обычному выводу на экран


    // 5. Текстура данных уровня (Level Data)
     // Здесь будут храниться препятствия или статические данные для тумана
    glGenTextures(1, &levelBlockTex);
    glBindTexture(GL_TEXTURE_2D, levelBlockTex);

    // Инициализируем пустой текстурой 1x1 (заглушка)
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_R8,
        1,
        1,
        0,
        GL_RED,
        GL_UNSIGNED_BYTE,
        nullptr
    );
    // Здесь используем NEAREST, так как это технические данные (блоки), а не картинка
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);





    glGenFramebuffers(1, &memoryFBO);
    glGenTextures(1, &memoryTexture);
    std::vector<uint8_t> zeroData(maskResW* maskResH, 0);
    glBindTexture(GL_TEXTURE_2D, memoryTexture);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_R8,
        this->maskResW,
        this->maskResH,
        0,
        GL_RED,
        GL_UNSIGNED_BYTE,
        zeroData.data()
    );


    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindFramebuffer(GL_FRAMEBUFFER, memoryFBO);
    // Привязываем текстуру к фреймбуферу как "точку вывода цвета №0"
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, memoryTexture, 0);

    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);




    glGenFramebuffers(2, pingPongFBO);
    glGenTextures(2, pingPongTextures);

    for (int i = 0; i < 2; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, pingPongFBO[i]);
        glBindTexture(GL_TEXTURE_2D, pingPongTextures[i]);

        // ВАЖНО: GL_LINEAR для мягкости
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, maskResW, maskResH, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        // Привязываем текстуру к фреймбуферу
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingPongTextures[i], 0);
    }
    // Проверка: если видеокарта не смогла создать такой конфиг, программа выдаст ошибку
    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0); // Возвращаемся к обычному выводу на экран



    glGenFramebuffers(1, &LightFBO);
    glGenTextures(1, &lightTexture);
    glBindTexture(GL_TEXTURE_2D, lightTexture);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_R8,    // Экономия: используем только красный канал (8 бит)
        maskResW,   // Уменьшенная ширина
        maskResH,   // Уменьшенная высота
        0,
        GL_RED,   // Формат данных — один канал
        GL_UNSIGNED_BYTE,
        nullptr   // Данных пока нет, просто выделяем память
    );


    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindFramebuffer(GL_FRAMEBUFFER, LightFBO);
    // Привязываем текстуру к фреймбуферу как "точку вывода цвета №0"
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, lightTexture, 0);

    assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

void Fog::Update(const glm::vec2& screenPlayerPos, float radiusPx)
{
    playerPos = screenPlayerPos ;
    viewRadius = radiusPx;
}

void Fog::RenderMask( const int& w,const int& h,const float& dayCycleProgress)
{ 
    glBindFramebuffer(GL_FRAMEBUFFER, fogFBO); // пишем в FogTexture
    glViewport(0, 0, maskResW, maskResH);

    glDisable(GL_BLEND);

    fogMaskShader->use();
    fogMaskShader->setVec2("playerPos", playerPos);
    fogMaskShader->setFloat("playerAuraRadius", viewRadius);


    // 1. Фильтруем маяки игрока
    int bCount = 0;
    for (auto& b : myBeacons) {
        
        // Светит только если активирован мной (localPlayerId)
      //  if (b.active && b.ownerId == localPlayerId) {
            std::string posName = "beacons[" + std::to_string(bCount) + "].pos";
            std::string radName = "beacons[" + std::to_string(bCount) + "].radius";

            fogMaskShader->setVec2(posName.c_str(), b.pos);
            fogMaskShader->setFloat(radName.c_str(), b.radius); // Радиус маяка
            bCount++;
            if (bCount >= 8) break;
     //   }
    }
    fogMaskShader->setInt("beaconCount", bCount);

    // Передаем снаряды
    fogMaskShader->setInt("projectileCount", (int)activeLights.size());
    for (int i = 0; i < activeLights.size() && i < 16; i++) { // Ограничим, например, до 16



        std::string posName = "projectiles[" + std::to_string(i) + "].pos";
        std::string radName = "projectiles[" + std::to_string(i) + "].radius";
        fogMaskShader->setVec2(posName.c_str(), activeLights[i].pos);
     
        fogMaskShader->setFloat(radName.c_str(), activeLights[i].radius);
    }
    

    fogMaskShader->setFloat("softness", viewRadius * 0.5f);
    fogMaskShader->setVec2("tileSize", glm::vec2(32.0f));
    fogMaskShader->setVec2("levelSize", glm::vec2(maskResW/4, maskResH/4));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, levelBlockTex);
    fogMaskShader->setInt("blockTex", 0);

    DrawFullscreenQuad();
    UpdateLight(dayCycleProgress);
    BlurFog();

    UpdateMemory(); // --- ШАГ 2: Запоминаем увиденное ---

    glEnable(GL_BLEND);             // вернуть
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, w, h);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void Fog::DrawFullscreenQuad()
{
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void Fog::Resize(int w, int h)
{
    levelWidth = w;
    levelHeight = h;

 
  
}

void Fog::SetLevelBlocks(const std::vector<uint8_t>& blocks, int w, int h)
{
   /* maskResW = w;
    maskResH = h;*/
 
    glBindTexture(GL_TEXTURE_2D, levelBlockTex);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_R8,
        w,
        h,
        0,
        GL_RED,
        GL_UNSIGNED_BYTE,
        blocks.data()
    );



}

void Fog::UpdateMemory()
{
    // 1. Переключаемся на фреймбуфер памяти
    glBindFramebuffer(GL_FRAMEBUFFER, memoryFBO); // пишем в MemoryTexture 
    glViewport(0, 0, maskResW, maskResH);

    // 2. ВКЛЮЧАЕМ ХИТРОЕ СМЕШИВАНИЕ
    glEnable(GL_BLEND);
    glBlendEquation(GL_MAX); // Выбирает МАКСИМАЛЬНОЕ значение между старым и новым
    glBlendFunc(GL_ONE, GL_ONE);  // Просто копируем цвет

    // 3. Рисуем только что созданную fogTexture поверх memoryTexture
    fogCopyShader->use(); // читает ТОЛЬКО fogTex
    fogCopyShader->setInt("image", 0);
    fogCopyShader->setvec4("spriteColor", glm::vec4(1.0f));
    fogCopyShader->setVec2("pixelSize", glm::vec2(1.0f / (float)maskResW, 1.0f /  (float)maskResH));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fogTexture);

    DrawFullscreenQuad();

    glBlendEquation(GL_FUNC_ADD); // Возвращаем дефолт
    glDisable(GL_BLEND);// 4. Возвращаем всё назад
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Fog::BlurFog()
{
    bool horizontal = true, first_iteration = true;
    int amount = 4; // количество проходов (чем больше, тем мягче)

    blurShader->use();

    for (int i = 0; i < amount; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, pingPongFBO[horizontal]);
        blurShader->setInt("horizontal", horizontal);

        glActiveTexture(GL_TEXTURE0); // Используем тот же слот 0
        // В самый первый раз берем исходную маску света, потом — результат предыдущего шага
        glBindTexture(GL_TEXTURE_2D, first_iteration ? fogTexture : pingPongTextures[!horizontal]);
        blurShader->setInt("image", 0); // Сообщаем шейдеру, что картинка в слоте 0
        DrawFullscreenQuad();

        horizontal = !horizontal;
        if (first_iteration) first_iteration = false;
    }

    glBindFramebuffer(GL_READ_FRAMEBUFFER, pingPongFBO[!horizontal]);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fogFBO);
    glBlitFramebuffer(
        0, 0, maskResW, maskResH,
        0, 0, maskResW, maskResH,
        GL_COLOR_BUFFER_BIT, GL_NEAREST
    );

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Fog::UpdateLight(const float& dayCycleProgress)
{
    // 1. Переключаемся на фреймбуфер памяти
    glBindFramebuffer(GL_FRAMEBUFFER, LightFBO); // пишем в MemoryTexture 
    glViewport(0, 0, maskResW, maskResH);
    glClear(GL_COLOR_BUFFER_BIT); // Очистка обязательна
   

    // 3. Рисуем только что созданную fogTexture поверх memoryTexture
    lightShader->use(); // читает ТОЛЬКО fogTex
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, levelBlockTex);
    lightShader->setInt("levelBlockTex", 0);
    lightShader->setVec2("levelSizeTiles", glm::vec2(maskResW / 4, maskResH / 4)); 
    lightShader->setFloat("u_dayCycle", dayCycleProgress);

   
   

    DrawFullscreenQuad();

   // glBlendEquation(GL_FUNC_ADD); // Возвращаем дефолт
    glDisable(GL_BLEND);// 4. Возвращаем всё назад
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Fog::reset()
{
   

    glBindTexture(GL_TEXTURE_2D, fogTexture);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_R8,
        maskResW, // Используем ScaleW
        maskResH, // Используем ScaleH
        0,
        GL_RED,
        GL_UNSIGNED_BYTE,
        nullptr
    );

    glBindTexture(GL_TEXTURE_2D, memoryTexture);

    // !!! ИСПРАВЛЕНИЕ: Инициализируем память нулями так же, как в Init()
    std::vector<uint8_t> zeroData(maskResW * maskResH, 0);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_R8,
        maskResW, // Используем ScaleW
        maskResH, // Используем ScaleH
        0,
        GL_RED,
        GL_UNSIGNED_BYTE,
        zeroData.data() // Передаем данные с нулями
    );
    activeLights.clear();
    myBeacons.clear();
}

void Fog::AddBeaconLight(uint16_t idx, glm::vec2 pos, float rad)
{
    // Проверяем, нет ли его уже (на всякий случай)
    RemoveBeaconLight(idx);
    if (myBeacons.size() < 8) {
    myBeacons.push_back({ idx, pos, rad });
    }
    else
    {
        myBeacons.erase(myBeacons.begin());
        myBeacons.push_back({ idx, pos, rad });
    }
}

void Fog::RemoveBeaconLight(uint16_t idx)
{
    myBeacons.erase(std::remove_if(myBeacons.begin(), myBeacons.end(),
        [idx](const BeaconLight& b) { return b.index == idx; }),
        myBeacons.end());
}

void Fog::AddLight(const glm::vec2& pos, float radius)
{
    if (activeLights.size() < 16) { // Ограничение шейдера
      
        activeLights.push_back({ pos, radius });
    }
}

void Fog::ClearLight()
{
    activeLights.clear();
}
