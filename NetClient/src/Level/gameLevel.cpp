#include "gameLevel.h"
#include "../ResourceManager.h"
#include "../NetShared/FastNoiseLite.h"
#include "../Rendering/Camera2d.h"
#include "GLFW/glfw3.h"

gameLevel::gameLevel(int Width, int Height, uint32_t seed) : LevelSize(Width, Height) {
    LevelData.resize(Width * Height, '.');
    tileVisuals.resize(Width * Height);
    blocks.resize(Width * Height, 0);

    FastNoiseLite noise;
    noise.SetSeed(seed);
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise.SetFrequency(0.15f); // СИНХРОННО С СЕРВЕРОМ (0.15f вместо 0.45f)

    // --- ЭТАП 1: ГЕНЕРАЦИЯ БАЗОВОЙ СЕТКИ ---
    for (int y = 0; y < Height; y++) {
        for (int x = 0; x < Width; x++) {
            int idx = y * Width + x;

            // 1. ГРАНИЦЫ
            if (x == 0 || x == Width - 1 || y == 0 || y == Height - 1) {
                LevelData[idx] = TileTypeToChar(TileType::Block);
                blocks[idx] = 255;
                continue;
            }

            // 2. ЗЕРКАЛЬНЫЙ ШУМ
            int sampleX = (x < Width / 2) ? x : (Width - 1 - x); // тут магия в том что x будет болье половины 
            float val = noise.GetNoise((float)sampleX, (float)y);

            // 3. ГАРАНТИРОВАННЫЕ ПУСТОТЫ
            bool forceEmpty = false;
            float distToCenter = glm::distance(glm::vec2(x, y), glm::vec2(Width / 2.0f, Height / 2.0f));
            if (distToCenter < 6.0f) forceEmpty = true;

            // Угловые зоны (Скругленные, радиус 10.5)
            float dx = (x < Width / 2) ? (float)x : (float)(Width - 1 - x);
            float dy = (y < Height / 2) ? (float)y : (float)(Height - 1 - y);
            if (std::sqrt(dx * dx + dy * dy) < 10.5f) forceEmpty = true;

            if (forceEmpty) {
                LevelData[idx] = TileTypeToChar(TileType::Empty);
                blocks[idx] = 0;
            }
            else {
                // Динамический порог (должен быть 1 в 1 как на сервере)
                float currentThreshold = 0.15f;
                float minDistToKeyZone = std::min(distToCenter, std::sqrt(dx * dx + dy * dy));
                float specialTypeVal = noise.GetNoise((float)x * 2.0f, (float)y * 2.0f);


                if (minDistToKeyZone < 15.0f) {
                    currentThreshold += (15.0f - minDistToKeyZone) * 0.04f;
                }

                if (val > currentThreshold) {


                    blocks[idx] = 255;

                   // if (specialTypeVal > 0.8f) tiles[idx].type = TileType::Fire;
                    // Распределяем спец-блоки (например, 20% всех блоков — особенные)
                     // Распределяем спец-блоки (например, 20% всех блоков — особенные)
               


                    if (specialTypeVal > 0.9f) {
                        LevelData[idx] = TileTypeToChar(TileType::Graveyard);
                        //    tiles[idx].hp = 250; 
                       //     match->activeSpawners.push_back({ {x, y}, 5.0f }); }

                    }else if (specialTypeVal > 0.85f) {
                        LevelData[idx] = TileTypeToChar(TileType::Fire);
                        blocks[idx] = 0;
                    }
                    else if (specialTypeVal > 0.75f) {
                        LevelData[idx] = TileTypeToChar(TileType::Ice);
                       
                    }
                    else if (specialTypeVal > 0.65f) {
                        LevelData[idx] = TileTypeToChar(TileType::Explosive);
                       
                    }
                    else if (specialTypeVal < -0.95f) {
                        LevelData[idx] = TileTypeToChar(TileType::Healer);
                      
                    }
                    else if (specialTypeVal < -0.85f) {
                        LevelData[idx] = TileTypeToChar(TileType::CompanionAltar);
                    }
                    else if (specialTypeVal < -0.6f) {
                        LevelData[idx] = TileTypeToChar(TileType::Sand);
                        blocks[idx] = 0;
                    }
                    else {
                        LevelData[idx] = TileTypeToChar(TileType::Block);
                       
                    }

                }
                else {
                    LevelData[idx] = TileTypeToChar(TileType::Empty);
                    blocks[idx] = 0;
                }
            }
        }
    }
    // --- ЭТАП 1.5: ГАРАНТИЯ СВЯЗНОСТИ (ПРОКОПКА КОРИДОРОВ) ---
    glm::ivec2 center(Width / 2, Height / 2);
    glm::ivec2 corners[] = { {5, 5}, {Width - 6, 5}, {5, Height - 6}, {Width - 6, Height - 6} };

    for (auto& corner : corners) {
        glm::ivec2 cur = corner;
        // Копаем по X, пока не сравняемся с центром
        while (cur.x != center.x) {
           // tiles[cur.y * w + cur.x].type = TileType::Empty;
            LevelData[cur.y * Width + cur.x] = TileTypeToChar(TileType::Empty);
            blocks[cur.y * Width + cur.x] =0;
            cur.x += (center.x > cur.x) ? 1 : -1;
        }
        // Копаем по Y, пока не дойдем до центра
        while (cur.y != center.y) {
            LevelData[cur.y * Width + cur.x] = TileTypeToChar(TileType::Empty);
            blocks[cur.y * Width + cur.x] = 0;


            cur.y += (center.y > cur.y) ? 1 : -1;
        }
    }
    // --- ЭТАП 2: BFS (ПРОВЕРКА СВЯЗНОСТИ) ---
    std::vector<bool> reachable(Width * Height, false);
    std::vector<glm::ivec2> queue;
    glm::ivec2 centerPt(Width / 2, Height / 2);

    // Гарантируем, что центр пустой для старта BFS
    if (blocks[centerPt.y * Width + centerPt.x] == 0) {
        queue.push_back(centerPt);
        reachable[centerPt.y * Width + centerPt.x] = true;
    }

    int head = 0;
    while (head < queue.size()) {
        glm::ivec2 curr = queue[head++];
        glm::ivec2 dirs[] = { {0,1}, {0,-1}, {1,0}, {-1,0} };
        for (auto& d : dirs) {
            glm::ivec2 next = curr + d;
            if (next.x >= 0 && next.x < Width && next.y >= 0 && next.y < Height) {
                int nIdx = next.y * Width + next.x;
                if (blocks[nIdx] == 0 && !reachable[nIdx]) {
                    reachable[nIdx] = true;
                    queue.push_back(next);
                }
            }
        }
    }
    // 3. Расстановка маяков (Beacons)
    for (int y = 5; y < Height - 5; y += 10) {
        for (int x = 5; x < Width - 5; x += 10) {
            int idx = y * Width + x;
            if (LevelData[idx] == TileTypeToChar(TileType::Empty) && reachable[idx]) {
                LevelData[idx] = TileTypeToChar(TileType::Beacon);
                blocks[idx] = 1; // Спец-тип для маяка

                tileVisuals[idx].growInterp = 0.0f;
                activeTiles.insert(idx);
            }
        }
    }


    // --- ЭТАП 3: ФИНАЛЬНАЯ ИНИЦИАЛИЗАЦИЯ И МАЯКИ ---
    for (int i = 0; i < Width * Height; i++) {
        // 1. Очистка карманов
        if (LevelData[i] == TileTypeToChar(TileType::Empty) && !reachable[i]) {
            blocks[i] = 255;
            LevelData[i] = TileTypeToChar(TileType::Block);
        }

        // 2. Инициализация визуала для ВСЕХ твердых объектов и спец-эффектов
        // Проверяем по символу или по значению в blocks
        if (LevelData[i] != TileTypeToChar(TileType::Empty)) {
            tileVisuals[i].growInterp = 0.0f;
            activeTiles.insert(i);
        }
        else {
            tileVisuals[i].growInterp = 1.0f;
        }
    }

 
}
gameLevel::~gameLevel()
{
	LevelData.clear();

}
void gameLevel::Update(const float& dt)
{
   
    for (auto it = activeTiles.begin(); it != activeTiles.end(); )
    {
        auto& tv = tileVisuals[*it];
        bool stillActive = false;


        // 1. Уменьшаем таймер респавна
        if (tv.isRespawning && tv.respawnTimer > 0.0f) {
           // std::cout << tv.respawnTimer << std::endl;
            tv.respawnTimer -= dt;
            stillActive = true;
        }
        // Обновляем рост
        if (tv.growInterp < 1.0f) {
            tv.growInterp = std::min(1.0f, tv.growInterp + dt * 3.0f);
            stillActive = true;
        }

        // Обновляем движение
        if (tv.moving) {
            tv.t += dt;
            if (tv.t >= tv.duration) {
                tv.t = tv.duration;
                tv.moving = false;
            }
            else {
                stillActive = true;
            }
        }

        // Если тайл больше не растет и не движется — убираем его из активных
        if (!stillActive)
            it = activeTiles.erase(it);
        else
            ++it;
    }
}

//void gameLevel::SyncFromServer(const std::vector<uint8_t>& serverBlocks)
//{
//  
//    if (serverBlocks.size() != tileVisuals.size()) return;
//
//    for (int i = 0; i < (int)serverBlocks.size(); i++)
//    {
//        // Если сервер прислал 1 (или 255), значит там блок
//        bool isWall = (serverBlocks[i] > 0);
//
//        // Обновляем LevelData для отрисовки '#'
//        LevelData[i] = isWall ? '#' : '.';
//
//        // Обновляем данные для тумана
//        blocks[i] = isWall ? 255 : 0;
//
//        // Если блок только что появился (был '.' стал '#'), можно запустить анимацию роста
//        if (isWall && LevelData[i] == '.') {
//            activeTiles.insert(i);
//            tileVisuals[i].growInterp = 0.0f;
//        }
//    }
//}
void gameLevel::Render(SpriteRenderer& renderer, InstancedSpriteRenderer* instRenderer, Camera2D& camera, TileDatabase& tileDb) {

    Shader& SpriteShader = ResourceManager::GetShader(ShaderID::World);
    Shader& instSpriteShader = ResourceManager::GetShader(ShaderID::instanceWorld);

    Texture2D& defaultTexture = ResourceManager::GetTexture("Level_tiles");

    // Границы экрана с запасом
    int startX = std::max(0, (int)std::floor(camera.position.x / 32.0f) - 1);
    int startY = std::max(0, (int)std::floor(camera.position.y / 32.0f) - 1);
    int endX = std::min((int)LevelSize.x, (int)std::ceil((camera.position.x + camera.visibleArea.x) / 32.0f) + 1);
    int endY = std::min((int)LevelSize.y, (int)std::ceil((camera.position.y + camera.visibleArea.y) / 32.0f) + 1);

    // Очищаем векторы перед началом кадра
    floorLayer.clear();
    decalLayer.clear();
    shadowLayer.clear();
    objectLayer.clear();

    const TileInfo& groundInfo = tileDb.tiles.at(TileType::Empty);

    // =========================================================================
    // ЕДИНСТВЕННЫЙ ПРОХОД ПО ЭКРАНУ (Сортировка по слоям)
    // =========================================================================
    for (int y = startY; y < endY; y++) {
        for (int x = startX; x < endX; x++) {
            int idx = y * (int)LevelSize.x + x;
            char tileChar = LevelData[idx];
            auto& tv = tileVisuals[idx];
            TileType type = CharToTileType(tileChar);

            // Оставляем дебаг-индикатор на обычном поклеточном рендерере
            if (type == TileType::Empty && tv.isRespawning && tv.respawnTimer <= 5.0f) {
                float pulse = (std::sin(glfwGetTime() * 15.0f) * 0.5f + 0.5f);
                glActiveTexture(GL_TEXTURE0);
                defaultTexture.Bind(); // Принудительно привязываем атлас, так как старый рендерер может его сбить
                renderer.DrawSprite(
                    SpriteShader, defaultTexture,
                    { x * 32.0f, y * 32.0f }, { 32.0f, 32.0f },
                    0.0f, glm::vec4(1.0f, 0.0f, 0.0f, 0.2f + 0.3f * pulse),
                    tileDb.decals.at(1).uvOffset, tileDb.decals.at(1).uvScale
                );
            }

            unsigned int hash = (x * 73856093) ^ (y * 19349663);

            // 1. БАЗОВЫЙ ПОЛ ДЛЯ ВСЕХ: Кладем траву в абсолютно каждую клетку!
            // Теперь ни под песком, ни под маяками, ни под стенами не будет черных дыр
            float groundRotation = (hash % 4) * 90.0f;
            floorLayer.push_back({
                { x * 32.0f, y * 32.0f }, { 32.0f, 32.0f },
                groundRotation, glm::vec4(1.0f),
                groundInfo.uvOffset, groundInfo.uvScale
                });

            // 2. ДЕКАЛИ: Случайные травинки/кости спавним только там, где на карте чистая земля
            if (type == TileType::Empty) {
                if ((hash % 100) < 35) {
                    int decalId = 1 + ((hash / 100) % tileDb.decals.size());
                    if (tileDb.decals.count(decalId)) {
                        const TileInfo& decalInfo = tileDb.decals.at(decalId);
                        float offsetX = static_cast<float>((hash % 17) - 8);
                        float offsetY = static_cast<float>(((hash / 17) % 17) - 8);

                        decalLayer.push_back({
                            { x * 32.0f + offsetX, y * 32.0f + offsetY }, { 32.0f, 32.0f },
                            0.0f, glm::vec4(1.0f),
                            decalInfo.uvOffset, decalInfo.uvScale
                            });
                    }
                }
                continue; // Если клетка пустая, идти дальше не нужно
            }

            // 3. ОБРАБОТКА ОБЪЕКТОВ (Песок, Огонь, Маяки, Камни, Стены и т.д.)
            if (tileDb.tiles.count(type) == 0) continue;
            auto& objectInfo = tileDb.tiles.at(type);

            // А) Проверяем большие объекты 2х2 (Песок и Огонь), чтобы отсечь дубликаты
            if (type == TileType::Sand || type == TileType::Fire) {
                bool isLeftEdge = (x == 0 || CharToTileType(LevelData[idx - 1]) != type);
                bool isTopEdge = (y == 0 || CharToTileType(LevelData[idx - (int)LevelSize.x]) != type);

                // Если мы не в левом верхнем углу этой зоны 2х2 — пропускаем, чтобы не спавнить копии
                if (!isLeftEdge || !isTopEdge) continue;
            }

            glm::vec2 objSize = glm::vec2(32.0f * objectInfo.sizeX, 32.0f * objectInfo.sizeY);

            // Б) Сбор мягкой тени под объектом (если прописана в JSON)
            if (!objectInfo.shadowTag.empty() && tileDb.shadows.count(objectInfo.shadowTag) > 0) {
                auto& shadowInfo = tileDb.shadows.at(objectInfo.shadowTag);
                glm::vec2 shadowSize = glm::vec2(32.0f * shadowInfo.sizeX, 32.0f * shadowInfo.sizeY);
                glm::vec2 shadowPos = glm::vec2(
                    (x * 32.0f) + (objSize.x * 0.5f) - (shadowSize.x * 0.5f),
                    (y * 32.0f) + (objSize.y * 0.5f) - (shadowSize.y * 0.5f)
                );
                shadowLayer.push_back({ shadowPos, shadowSize, 0.0f, glm::vec4(1.0f), shadowInfo.uvOffset, shadowInfo.uvScale });
            }

            // В) Сбор самого объекта
            float rotationAngle = 0.0f;
            if (type == TileType::Block) { // Поворот кирпичей
                rotationAngle = (hash % 4) * 90.0f;
            }

            // Разделяем по слоям рендеринга для правильной сортировки:
            if (type == TileType::Sand || type == TileType::Fire) {
                glm::vec2 surfacePos = glm::vec2(
                    (x * 32.0f) + (32.0f * 0.5f) - (objSize.x * 0.5f),
                    (y * 32.0f) + (32.0f * 0.5f) - (objSize.y * 0.5f)
                );

                decalLayer.push_back({ surfacePos, objSize, rotationAngle, glm::vec4(1.0f), objectInfo.uvOffset, objectInfo.uvScale });
            }
            else {
                // Маяки, Стены, Лед, Взрывчатка — это твердые высокие объекты, кладем в верхний слой
                objectLayer.push_back({ { x * 32.0f, y * 32.0f }, objSize, rotationAngle, glm::vec4(1.0f), objectInfo.uvOffset, objectInfo.uvScale });
            }
        }
    }
    // =========================================================================
    // ОДНОЭТАПНАЯ ОТРИСОВКА СЛОЕВ ЧЕРЕЗ ВАШ КЛАСС
    // =========================================================================
    instRenderer->DrawInstances(instSpriteShader, defaultTexture, floorLayer);   // Слой 1: Пол / Трава
    instRenderer->DrawInstances(instSpriteShader, defaultTexture, decalLayer);   // Слой 2: Трава, кровь, кости
    instRenderer->DrawInstances(instSpriteShader, defaultTexture, shadowLayer);  // Слой 3: Мягкие круглые тени
    instRenderer->DrawInstances(instSpriteShader, defaultTexture, objectLayer);  // Слой 4: Стены, свечи, алтари
}

 //   Shader& SpriteShader = ResourceManager::GetShader(ShaderID::World);
 //   Shader& instSpriteShader = ResourceManager::GetShader(ShaderID::instanceWorld);
 //   // 1. До циклов вытаскиваем основной атлас по строке всего один раз!
 //   Texture2D& defaultTexture = ResourceManager::GetTexture("Level_tiles");
 //   glActiveTexture(GL_TEXTURE0);
 //   defaultTexture.Bind();

 //   std::string lastBoundTag = "Level_tiles"; // Запоминаем, что сейчас заб
 // 

 //   // 2. Находим границы в тайлах (используем floor/ceil, чтобы не было "мерцания" на краях)
 //   // Добавляем небольшой запас (Padding) в 1-2 тайла, чтобы объекты не исчезали мгновенно
 //   int startX = std::max(0, (int)std::floor(camera.position.x / 32.0f) - 1);
 //   int startY = std::max(0, (int)std::floor(camera.position.y / 32.0f) - 1);

 //   int endX = std::min((int)LevelSize.x, (int)std::ceil((camera.position.x + camera.visibleArea.x) / 32.0f) + 1);
 //   int endY = std::min((int)LevelSize.y, (int)std::ceil((camera.position.y + camera.visibleArea.y) / 32.0f) + 1);


 //   //
 //   float backX = std::max(0.0f, camera.position.x); // если камера позишн уйдет в - мы возьмём 0
 //   float backY = std::max(0.0f, camera.position.y);

 ////   float backEndX = std::min(LevelSize.x * 32.0f, camera.position.x + camera.visibleArea.x); // если камера попытается взять позицию выше карты - возьмём карту
 // //  float backEndY = std::min(LevelSize.y * 32.0f, camera.position.y + camera.visibleArea.y);

 //   
 //   // =========================================================================
 // // ПРОХОД 1: Базовая подложка земли (Трава/Пол)
 // // =========================================================================
 //   if (tileDb.tiles.count(TileType::Empty)) {
 //       const TileInfo& groundInfo = tileDb.tiles.at(TileType::Empty);

 //       for (int y = startY; y < endY; y++) {
 //           for (int x = startX; x < endX; x++) {

 //               // --- МАГИЯ СТАТИЧНОГО РАНДОМА ПО КООРДИНАТАМ ---
 //               // Простая математическая формула (хэш), которая смешивает X и Y.
 //               // Она всегда возвращает одно и то же число для конкретной клетки,
 //               // но визуально для соседа это будет совершенно другой угол.
 //               unsigned int hash = (x * 73856093) ^ (y * 19349663);

 //               // Берем остаток от деления на 4 (получаем 0, 1, 2 или 3)
 //               int randomDir = hash % 4;

 //               // Превращаем в угол поворота на 4 стороны света
 //               float rotationAngle = randomDir * 90.0f;

 //               renderer.DrawSprite(
 //                   SpriteShader,
 //                   defaultTexture,
 //                   { x * 32.0f, y * 32.0f },
 //                   { 32.0f, 32.0f },
 //                   rotationAngle, // <-- ПЕРЕДАЕМ НАШ УГОЛ ПОВОРОТА СЮДА
 //                   glm::vec4(1.0f),
 //                   groundInfo.uvOffset,
 //                   groundInfo.uvScale
 //               );


 //               // 2. РЕНДЕРИНГ ДЕКАЛЕЙ (Только если клетка на карте пустая/земля)
 //               int idx = y * (int)LevelSize.x + x;
 //               TileType currentType = CharToTileType(LevelData[idx]);

 //               if (currentType == TileType::Empty) {
 //                   // Используем хэш, чтобы решить, появится ли тут декаль (допустим, шанс 35%)
 //                   // hash % 100 дает число от 0 до 99
 //                   if ((hash % 100) < 35) {

 //                       // Выбираем тип декали из базы данных (у нас есть ID 1 и 2)
 //                       // (hash / 100) — берем другую часть хэша, чтобы выбор не зависел от шанса спавна
 //                       int decalId = 1 + ((hash / 100) % tileDb.decals.size());

 //                       if (tileDb.decals.count(decalId)) {
 //                           const TileInfo& decalInfo = tileDb.decals.at(decalId);

 //                           // --- ВЫЧИСЛЕНИЕ СЛУЧАЙНОГО СДВИГА ОТ ЦЕНТРА ---
 //                           // Нам нужен сдвиг по X и Y в диапазоне от -8.0f до +8.0f пикселей.
 //                           // Таким образом, декаль сместится, но гарантированно останется внутри своей плитки 32х32.
 //                           float offsetX = static_cast<float>((hash % 17) - 8); // от -8 до +8
 //                           float offsetY = static_cast<float>(((hash / 17) % 17) - 8);

 //                           // Итоговая позиция: базовый левый верхний угол ячейки + наш хаотичный сдвиг
 //                           glm::vec2 decalPos = glm::vec2(x * 32.0f + offsetX, y * 32.0f + offsetY);

 //                          
 //                           // Передаем текстурные координаты конкретной травинки/камня
 //                        //   SpriteShader.setVec2("uvOffset", decalInfo.uvOffset);
 //                        //   SpriteShader.setVec2("uvScale", decalInfo.uvScale);

 //                           // Отрисовываем декаль поверх только что нарисованной земли
 //                           renderer.DrawSprite(
 //                               SpriteShader, defaultTexture,
 //                               decalPos, { 32.0f, 32.0f },
 //                               0.0f, glm::vec4(1.0f),decalInfo.uvOffset,decalInfo.uvScale
 //                           );
 //                       }
 //                   }
 //               }


 //           }
 //       }
 //   }

 //   // Лямбда-функция для отрисовки конкретного тайла (чтобы не дублировать код в проходах 2 и 3)
 //   auto DrawTileInfo = [&](TileType type, int x, int y, auto& tv, float rotate = 0.0f) {
 //       if (!tileDb.tiles.count(type)) return;
 //       const TileInfo& info = tileDb.tiles.at(type);

 //       // Динамическая смена атласа, если у объекта свой texture_tag
 //       if (info.textureTag != lastBoundTag) {
 //           Texture2D& newTex = ResourceManager::GetTexture(info.textureTag);
 //           newTex.Bind();
 //           lastBoundTag = info.textureTag;
 //       }

 //       // Анимация по серверному времени
 //       glm::vec2 currentUvOffset = info.uvOffset;
 //       if (info.framesCount > 1 && info.animSpeed > 0.0f) {
 //      //     int currentFrame = static_cast<int>(serverTime * info.animSpeed) % info.framesCount;
 //       //    currentUvOffset.x += currentFrame * info.uvScale.x;
 //       }

 //       // Расчет размеров и эффектов (включая большие 64x64 тайлы)
 //       glm::vec2 size = glm::vec2(32.0f * info.sizeX, 32.0f * info.sizeY);
 //       float scaleEffect = tv.moving ? 1.0f : tv.growInterp;
 //       size *= scaleEffect;

 //       // Центрирование от исходной ячейки
 //       glm::vec2 cellCenter = glm::vec2(x * 32.0f + 16.0f, y * 32.0f + 16.0f);
 //       glm::vec2 pos = tv.moving ? (tv.from + (tv.to - tv.from) * (tv.t / tv.duration) - size * 0.5f) : (cellCenter - size * 0.5f);

 //    //   SpriteShader.setVec2("u_uvOffset", currentUvOffset);
 //     //  SpriteShader.setVec2("u_uvScale", info.uvScale);

 //        // --- МАГИЯ СТАТИЧНОГО РАНДОМА ПО КООРДИНАТАМ ---
 //               // Простая математическая формула (хэш), которая смешивает X и Y.
 //               // Она всегда возвращает одно и то же число для конкретной клетки,
 //               // но визуально для соседа это будет совершенно другой угол.
 //    

 //       renderer.DrawSprite(SpriteShader, defaultTexture, pos, size, rotate, glm::vec4(1.0f),currentUvOffset,info.uvScale);
 //       };

 //   // =========================================================================
 //   // ПРОХОД 2: Только напольные эффекты / большие зоны (Песок и Огонь)
 //   // =========================================================================
 //   for (int y = startY; y < endY; y++) {
 //       for (int x = startX; x < endX; x++) {
 //           int idx = y * (int)LevelSize.x + x;
 //           TileType type = CharToTileType(LevelData[idx]);

 //           if (type == TileType::Sand || type == TileType::Fire) {

 //               unsigned int hash = (x * 73856093) ^ (y * 19349663);

 //               // Берем остаток от деления на 4 (получаем 0, 1, 2 или 3)
 //               int randomDir = hash % 4;

 //               // Превращаем в угол поворота на 4 стороны света
 //               float rotationAngle = randomDir * 90.0f;

 //               DrawTileInfo(type, x, y, tileVisuals[idx],rotationAngle);
 //           }
 //       }
 //   }
 //   // =========================================================================
 //   for (int y = startY; y < endY; y++) {
 //       for (int x = startX; x < endX; x++) {
 //           int idx = y * (int)LevelSize.x + x;
 //           char tileChar = LevelData[idx];
 //           TileType type = CharToTileType(tileChar);

 //           if (type == TileType::Empty || type == TileType::Sand || type == TileType::Fire) continue;

 //           if (tileDb.tiles.count(type) == 0) continue;
 //           auto& objectInfo = tileDb.tiles.at(type);

 //           if (!objectInfo.shadowTag.empty()) {
 //               if (tileDb.shadows.count(objectInfo.shadowTag) > 0) {
 //                   auto& shadowInfo = tileDb.shadows.at(objectInfo.shadowTag);

 //                   // Вычисляем размеры тени и объекта в пикселях на основе их sizeX/sizeY из БД
 //                   glm::vec2 shadowSize = glm::vec2(32.0f * shadowInfo.sizeX, 32.0f * shadowInfo.sizeY);
 //                   glm::vec2 objSize = glm::vec2(32.0f * objectInfo.sizeX, 32.0f * objectInfo.sizeY);

 //                   // Центрируем тень 2х2 относительно объекта 1х1 (или 2х2)
 //                   glm::vec2 shadowPos = glm::vec2(
 //                       (x * 32.0f) + (objSize.x * 0.5f) - (shadowSize.x * 0.5f),
 //                       (y * 32.0f) + (objSize.y * 0.5f) - (shadowSize.y * 0.5f)
 //                   );

 //                   renderer.DrawSprite(
 //                       SpriteShader,
 //                       defaultTexture,
 //                       shadowPos,
 //                       shadowSize, // <-- Рисуем в её честном размере 64х64
 //                       0.0f,
 //                       glm::vec4(1.0f),
 //                       shadowInfo.uvOffset,
 //                       shadowInfo.uvScale
 //                   );
 //               }
 //           }
 //       }
 //   }

 //   // =========================================================================
 //   // ПРОХОД 3: Твердые блоки, интерактивные объекты и дебаг-респавны
 //   // =========================================================================
 //   for (int y = startY; y < endY; y++) {
 //       for (int x = startX; x < endX; x++) {
 //           int idx = y * (int)LevelSize.x + x;
 //           char tileChar = LevelData[idx];
 //           auto& tv = tileVisuals[idx];
 //           TileType type = CharToTileType(tileChar);

 //           // Обработка индикатора респавна
 //           if (type == TileType::Empty && tv.isRespawning && tv.respawnTimer <= 5.0f) {
 //               float pulse = (std::sin(glfwGetTime() * 15.0f) * 0.5f + 0.5f);
 //             //  SpriteShader.setInt("symbol", 35);
 //               renderer.DrawSprite(SpriteShader, '35', {x * 32.0f, y * 32.0f}, {32.0f, 32.0f}, 0.0f, glm::vec4(1.0f, 0.0f, 0.0f, 0.2f + 0.3f * pulse));
 //            //   SpriteShader.setInt("symbol", 0);
 //           }

 //           // Песок, огонь и землю мы уже нарисовали, пропускаем их
 //           if (type == TileType::Empty || type == TileType::Sand || type == TileType::Fire) continue;

 //           if (type == TileType::Block) {

 //               unsigned int hash = (x * 73856093) ^ (y * 19349663);

 //               // Берем остаток от деления на 4 (получаем 0, 1, 2 или 3)
 //               int randomDir = hash % 4;

 //               // Превращаем в угол поворота на 4 стороны света
 //               float rotationAngle = randomDir * 90.0f;
 //               DrawTileInfo(type, x, y, tv, rotationAngle);
 //               continue;
 //           }

 //           // Рисуем стены, алтари, маяки и т.д. поверх песка/огня
 //           DrawTileInfo(type, x, y, tv);
 //       }
 //   }
//};
