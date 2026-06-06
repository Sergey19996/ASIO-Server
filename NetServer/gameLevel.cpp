#include "gameLevel.h"
#include "../NetShared/FastNoiseLite.h"
#include "Match.h"

glm::ivec2 dirs[] = { {0,1}, {0,-1}, {1,0}, {-1,0} };

gameLevel::gameLevel(int w, int h, uint32_t seed,Match* match) : levelWidth(w), levelHeight(h), seed(seed), match(match)
{
    tiles.resize(w * h);
    nodeCache.resize(w * h);

    FastNoiseLite noise;
    noise.SetSeed(seed);
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise.SetFrequency(0.15f); // Низкая частота для более крупных "комнат"

    // --- ЭТАП 1: ГЕНЕРАЦИЯ БАЗОВОЙ СЕТКИ ---
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int idx = y * w + x;

            // 1. ГРАНИЦЫ (Непробиваемые)
            if (x == 0 || x == w - 1 || y == 0 || y == h - 1) {
                tiles[idx].type = TileType::Block;
            
                tiles[idx].hp = 100;
                continue;
            }

            // 2. ЗЕРКАЛЬНЫЙ ШУМ (для симметрии лево-право)
            int sampleX = (x < w / 2) ? x : (w - 1 - x);
            float val = noise.GetNoise((float)sampleX, (float)y);

            // 3. ПРОВЕРКА ГАРАНТИРОВАННЫХ ПУСТОТ (Центр и Углы)
            bool forceEmpty = false;

            // Центр (Арена 10x10)
            float distToCenter = glm::distance(glm::vec2(x, y), glm::vec2(w / 2.0f, h / 2.0f));
            if (distToCenter < 6.0f) forceEmpty = true;

            // Углы (Скругленные зоны спавна радиусом 10)
            float dx = (x < w / 2) ? (float)x : (float)(w - 1 - x);
            float dy = (y < h / 2) ? (float)y : (float)(h - 1 - y);
            if (dx * dx + dy * dy < 110.25f) forceEmpty = true;

            if (forceEmpty) {
                tiles[idx].type = TileType::Empty;
                tiles[idx].hp = 0;
            }
            else {
                // Динамический порог: чем ближе к углу/центру, тем меньше стен
                float currentThreshold = 0.15f;
                float minDistToKeyZone = std::min(distToCenter, std::sqrt(dx * dx + dy * dy));
                float specialTypeVal = noise.GetNoise((float)x * 2.0f, (float)y * 2.0f); // Другая частота для разнообразия


                if (minDistToKeyZone < 15.0f) {
                    currentThreshold += (15.0f - minDistToKeyZone) * 0.04f;
                }

                if (val > currentThreshold) {
                    tiles[idx].hp = 100;

                    // Распределяем спец-блоки (например, 20% всех блоков — особенные)
                    if (specialTypeVal > 0.9f) {
                        tiles[idx].type = TileType::Graveyard;
                        tiles[idx].hp = 250;
                        match->activeSpawners[idx] = { {x, y}, 5.0f };
                    }
                    else if (specialTypeVal > 0.85f) tiles[idx].type = TileType::Fire;
                    else if (specialTypeVal > 0.75f) tiles[idx].type = TileType::Ice;
                    else if (specialTypeVal > 0.65f) { tiles[idx].type = TileType::Explosive; }
                    else if (specialTypeVal < -0.95f) { tiles[idx].type = TileType::Healer; }
                    else if (specialTypeVal < -0.85f ) {
                        tiles[idx].type = TileType::CompanionAltar;
                        tiles[idx].hp = 500; // Прочный, чтобы случайно не разбили сразу
                    }
                    else if (specialTypeVal < -0.6f) { tiles[idx].type = TileType::Sand;}
                    else { tiles[idx].type = TileType::Block;  }
                  
                }
                else {
                    tiles[idx].type = TileType::Empty;
                }
            }
        }
    }
    // --- ЭТАП 1.5: ГАРАНТИЯ СВЯЗНОСТИ (ПРОКОПКА КОРИДОРОВ) ---
    glm::ivec2 center(w / 2, h / 2);
    glm::ivec2 corners[] = { {5, 5}, {w - 6, 5}, {5, h - 6}, {w - 6, h - 6} };

    for (auto& corner : corners) {
        glm::ivec2 cur = corner;
        // Копаем по X, пока не сравняемся с центром
        while (cur.x != center.x) {
            tiles[cur.y * w + cur.x].type = TileType::Empty;
            cur.x += (center.x > cur.x) ? 1 : -1;
           
        }
        // Копаем по Y, пока не дойдем до центра
        while (cur.y != center.y) {
            tiles[cur.y * w + cur.x].type = TileType::Empty;
            cur.y += (center.y > cur.y) ? 1 : -1;
           
        }
    }
    // --- ЭТАП 2: BFS (ПРОВЕРКА СВЯЗНОСТИ) ---
    // Выполняется строго ПОСЛЕ заполнения всей сетки
    std::vector<bool> reachable(w * h, false);
    std::vector<glm::ivec2> queue;
    glm::ivec2 startPt(w / 2, h / 2); // Начинаем из гарантированно пустого центра

    queue.push_back(startPt);
    reachable[startPt.y * w + startPt.x] = true;

    int head = 0;
    while (head < (int)queue.size()) {
        glm::ivec2 curr = queue[head++];
        glm::ivec2 dirs[] = { {0,1}, {0,-1}, {1,0}, {-1,0} };

        for (auto& d : dirs) {
            glm::ivec2 next = curr + d;
            if (next.x >= 0 && next.x < w && next.y >= 0 && next.y < h) {
                int nIdx = next.y * w + next.x;
                // Если тайл пустой и мы там еще не были
                if (tiles[nIdx].type == TileType::Empty && !reachable[nIdx]) {
                    reachable[nIdx] = true;
                    queue.push_back(next);
                }
            }
        }
    }

    // --- ЭТАП 4: РАССТАНОВКА МАЯКОВ (BEACONS) ---
    // Расставляем их в пустых местах с шагом 10
    for (int y = 5; y < h - 5; y += 10) {
        for (int x = 5; x < w - 5; x += 10) {
            int idx = y * w + x;
            if (tiles[idx].type == TileType::Empty) {
                tiles[idx].type = TileType::Beacon;
                tiles[idx].hp = 5;
            }
        }
    }
    for (int i = 0; i < w * h; i++) {
        // Застраиваем изолированные пустоты
        if ((tiles[i].type == TileType::Empty) && !reachable[i]) {
            tiles[i].type = TileType::Block;
            tiles[i].hp = 100;
        }

        // ГЛАВНОЕ ПРАВИЛО SOLID:
        TileType t = tiles[i].type;
        tiles[i].isSolid = (t == TileType::Block || t == TileType::Ice ||
            t == TileType::Explosive  || t == TileType::Healer || t == TileType::Beacon || t == TileType::Graveyard || t == TileType::CompanionAltar);

    }


}
gameLevel::~gameLevel()
{
   // LevelData.clear();

}



Tile& gameLevel::GetTile(const glm::ivec2& cell)
{
    static Tile dummy{ TileType::Empty };

    if (cell.x < 0 || cell.y < 0 ||
        cell.x >= levelWidth || cell.y >= levelHeight)
        return dummy;
    return tiles[cell.y * levelWidth + cell.x];
}

glm::vec2 gameLevel::GetTileCenter(const int idx)
{
    if (idx < 0 || idx >= tiles.size())  
    return glm::vec2(0.0f, 0.0f); // Возвращаем дефолт, чтобы не упасть

    glm::vec2 tile = { (idx % levelWidth) + 0.5f, (idx / levelWidth) + 0.5f };

    return tile;
}

int gameLevel::FindRandomSolidTile()
{
    std::vector<int> candidates;
    for (int i = 0; i < tiles.size(); ++i) {
        // Ищем только обычные блоки, которые сейчас целы
        if (tiles[i].type == TileType::Block && !tiles[i].destroyed && tiles[i].isSolid) {
            // Проверка на границы (чтобы не заспавнить в вечной стене hp=9999)
           // if (tiles[i].hp < 1000) {
                candidates.push_back(i);
          //  }
        }
    }

    if (candidates.empty()) return -1; // На случай если все блоки уничтожены
    return candidates[rand() % candidates.size()];
}

bool gameLevel::IsSolid(const glm::ivec2& c) const
{
    if (c.x < 0 || c.y < 0 ||
        c.x >= levelWidth || c.y >= levelHeight)
        return true;

    const Tile& t = tiles[c.y * levelWidth + c.x];
    return t.isSolid && !t.destroyed;
}

void gameLevel::DamageTile(const glm::ivec2& c, int dmg,uint32_t attackerId)
{
    int idx = c.y * levelWidth + c.x; // Получаем индекс
    Tile& t = GetTile(c);
    char symbol = TileTypeToChar(t.type);

    switch (t.type)
    {
    case TileType::CompanionAltar: {
        t.hp -= dmg;
        if (t.hp <= 0) {
            // Передаем ответственность матчу
            match->TrySpawnCompanionForPlayer(attackerId, glm::vec2(c.x, c.y));

            // 2. Логика "телепортации" алтаря (как у Explosive)
            int newIdx = FindRandomSolidTile();
            tiles[newIdx].type = TileType::CompanionAltar;
            tiles[newIdx].hp = 100; // HP для следующего призыва

            // 3. Старую клетку превращаем в обломок (Block) с таймером респавна
            t.type = TileType::Block;
            t.isSolid = true;
            t.destroyed = true;
            t.respawnTimer = 30.0f; // Алтари респавнятся дольше обычных блоков
            t.hp = 5;

            MarkTileActive(idx);

            // 4. Уведомляем клиентов (нужно добавить 'A' или другой символ для Altar в TileTypeToChar)
            if (OnTileChanged) {
                OnTileChanged(c, '.', t.respawnTimer);
                OnTileChanged(IndexToCoords(newIdx), 'A', 0.0f);
            }
        }
        return;
        break;
    }
    case TileType::Beacon:{
        // 1. Смена владельца при каждом ударе
        if (t.ownerId != attackerId) {
            t.ownerId = attackerId;
            if (OnBeaconOwnerChanged) OnBeaconOwnerChanged(c, attackerId);
        }

        t.hp -= 1;
        if (t.hp <= 0) {
            // --- ЛОГИКА ТЕЛЕПОРТАЦИИ МАЯКА ---
            // 2. Ищем новую солид-клетку (не пустую!), которая не разрушена
            int newIdx = FindRandomSolidTile(); // Реализуйте поиск среди TileType::Block

            // 3. Переносим параметры маяка на новое место
            tiles[newIdx].type = TileType::Beacon;
            tiles[newIdx].hp = 50;
            tiles[newIdx].ownerId = -1; // Сброс владельца на новом месте

            // 4. Старый маяк превращаем в ОБЫЧНЫЙ разрушенный блок
            t.type = TileType::Block;
            t.isSolid = true;
            t.destroyed = true;
            t.respawnTimer = 5.0f;
            t.hp = 5;

            MarkTileActive(idx); // Запускаем таймер респавна блока на старом месте

            // Уведомляем клиентов об обоих изменениях
            if (OnTileChanged) {
                OnTileChanged(c, '.', t.respawnTimer); // Старый маяк исчез (визуально пусто до респавна блока)
                OnTileChanged(IndexToCoords(newIdx), symbol, 0.0f); // Новый маяк появился
            }
        }
        return;
        break;
    }
    case TileType::Explosive: {
        t.hp -= dmg;
        if (t.hp <= 0) {
            // 4. Старый маяк превращаем в ОБЫЧНЫЙ разрушенный блок
            t.type = TileType::Block;
            t.isSolid = true;
            t.destroyed = true;
            t.respawnTimer = 25.0f;
            t.hp = 5;
            match->ProcessAreaDamage(c, 3.0f, 20, -1, false, GetProjectileRules(ProjectileType::Explosion));
            int newIdx = FindRandomSolidTile(); // Реализуйте поиск среди TileType::Block

            // 3. Переносим параметры маяка на новое место
            tiles[newIdx].type = TileType::Explosive;
            tiles[newIdx].hp = 50;
            tiles[newIdx].ownerId = -1; // Сброс владельца на новом месте

          

            MarkTileActive(idx); // Запускаем таймер респавна блока на старом месте


            // Уведомляем клиентов об обоих изменениях
            if (OnTileChanged) {
                OnTileChanged(c, '.', t.respawnTimer); // Старый маяк исчез (визуально пусто до респавна блока)
                OnTileChanged(IndexToCoords(newIdx), symbol, t.respawnTimer); // Новый маяк появился
            }
        }
            return;
            break;
       
    }
    case TileType::Ice: {
        t.hp -= dmg;
        if (t.hp <= 0) {
            // 4. Старый маяк превращаем в ОБЫЧНЫЙ разрушенный блок
            t.type = TileType::Block;
            t.isSolid = true;
            t.destroyed = true;
            t.respawnTimer = 20.0f;
            t.hp = 5;
            match->ProcessAreaDamage(c, 3.0f, 10, -1, false, GetProjectileRules(ProjectileType::IceRoots));
            int newIdx = FindRandomSolidTile(); // Реализуйте поиск среди TileType::Block

            // 3. Переносим параметры маяка на новое место
            tiles[newIdx].type = TileType::Ice;
            tiles[newIdx].hp = 50;
            tiles[newIdx].ownerId = -1; // Сброс владельца на новом месте

           

            MarkTileActive(idx); // Запускаем таймер респавна блока на старом месте

            // Уведомляем клиентов об обоих изменениях
            if (OnTileChanged) {
                OnTileChanged(c, '.', t.respawnTimer); // Старый маяк исчез (визуально пусто до респавна блока)
                OnTileChanged(IndexToCoords(newIdx), symbol, t.respawnTimer); // Новый маяк появился
            }
        }
            return;
            break;
        
    }
    case TileType::Healer : {
        t.hp -= dmg;
        if (t.hp <= 0) {

            // 4. Старый маяк превращаем в ОБЫЧНЫЙ разрушенный блок
            t.type = TileType::Block;
            t.isSolid = true;
            t.destroyed = true;
            t.respawnTimer = 20.0f;
            t.hp = 5;

            if (attackerId != -1) {
                int plIdx = match->GetPlayerIndex(attackerId);
                auto& c = match->GetEntities()[plIdx].character;
                c->Heal(20);
                // Создаем отчет о лечении
                sDamageReport healReport;
                healReport.nVictimId = c->GetId();
                healReport.nAttackerId = c->GetId();
                healReport.Amount = 20;
                healReport.nType = 2; // Тип 2 = Лечение
                healReport.bResisted = false;


               match->damageQueue.push_back(healReport); // Добавляем в общую очередь

            };
                int newIdx = FindRandomSolidTile(); // Реализуйте поиск среди TileType::Block

            // 3. Переносим параметры маяка на новое место
            tiles[newIdx].type = TileType::Healer;
            tiles[newIdx].hp = 50;
            tiles[newIdx].ownerId = -1; // Сброс владельца на новом месте

           

            MarkTileActive(idx); // Запускаем таймер респавна блока на старом месте

            // Уведомляем клиентов об обоих изменениях
            if (OnTileChanged) {
                OnTileChanged(c, '.', t.respawnTimer); // Старый маяк исчез (визуально пусто до респавна блока)
                OnTileChanged(IndexToCoords(newIdx), symbol,t.respawnTimer); // Новый маяк появился
            }
        }
            return;
            break;   
    }
    case TileType::Graveyard: {
        t.hp -= dmg;
        if (t.hp <= 0) {
            // 4. Старый маяк превращаем в ОБЫЧНЫЙ разрушенный блок
            t.type = TileType::Block;
            t.isSolid = true;
            t.destroyed = true;
            t.respawnTimer = 20.0f;
            t.hp = 5;
           // match->ProcessAreaDamage(c, 3.0f, 10, -1, false, GetProjectileRules(ProjectileType::IceRoots));
            int newIdx = FindRandomSolidTile(); // Реализуйте поиск среди TileType::Block

            // 3. Переносим параметры маяка на новое место
            tiles[newIdx].type = TileType::Graveyard;
            tiles[newIdx].hp = 250;
            tiles[newIdx].ownerId = -1; // Сброс владельца на новом месте

            match->activeSpawners.erase(idx);

            float x = newIdx % levelWidth;
            float y = newIdx / levelWidth;
            match->activeSpawners[newIdx] = { {x, y}, 5.0f };


            MarkTileActive(idx); // Запускаем таймер респавна блока на старом месте

            // Уведомляем клиентов об обоих изменениях
            if (OnTileChanged) {
                OnTileChanged(c, '.', t.respawnTimer); // Старый маяк исчез (визуально пусто до респавна блока)
                OnTileChanged(IndexToCoords(newIdx), symbol, t.respawnTimer); // Новый маяк появился
            }
        }
        return;
        break;
    }
    default:
        break;
    }
  
    if (t.destroyed || !t.isSolid)
        return;

    t.hp -= dmg;
    if (t.hp <= 0) // тут всё кроме Beacon и не солид
    {
        t.destroyed = true;
        t.respawnTimer = 15.0f;
       

        // 🔥 Активируем тайл для апдейта (респавна)
        MarkTileActive(idx);
        if (OnTileChanged)
            OnTileChanged(c, '.', t.respawnTimer);
    }
}

void gameLevel::Update(float dt)
{

    for (auto it = activeTiles.begin(); it != activeTiles.end(); ) {
        {
           
            int idx = *it;
           
            Tile& tile = tiles[idx];
            tile.isUpdateActive = false;

            glm::ivec2 cell = { idx % levelWidth, idx / levelWidth };

            
            //if (tile.moving)
            //{
            //    tile.moveT += dt * tile.moveSpeed;
            //    tile.isUpdateActive = true;
            //    if (tile.moveT >= 1.0f)
            //    {
            //        tile.isUpdateActive = false;
            //        tile.moveT = 1.0f;
            //        tile.moving = false;
            //        
            //        // финализируем позицию
            //        GetTile(tile.to) = tile;
            //        GetTile(tile.from) = Tile{ TileType::Empty };
            //        
            //        if (OnTileChanged)
            //        {
            //            OnTileChanged(tile.from, '.',);
            //            OnTileChanged(tile.to, TileTypeToChar(tile.type));
            //        }
            //        
            //    }
            //   
            //}

            if (tile.destroyed)
            {
                tile.isUpdateActive = true;
                tile.respawnTimer -= dt;
                if (tile.respawnTimer <= 0)
                {
                  
                    tile.destroyed = false;
                    tile.hp = 100;
                    tile.growInterp = 0.0f;


                    if (OnTileChanged)
                        OnTileChanged(cell, '#',0.0f);
                }
            }
            else if (tile.growInterp < 1.0f)
            {
                
           
                tile.growInterp = std::min(1.0f, tile.growInterp + dt * 2.0f);
                tile.isUpdateActive = true;
            }

            // Если с тайлом больше ничего не происходит — выкидываем его из активных
            if (!tile.isUpdateActive) {
               
                it = activeTiles.erase(it);
               
            }
            else {
                ++it;
            }
        }
  
    }
}

void gameLevel::TryMoveTilesNearPlayers(const std::vector<glm::vec2>& playerPositions, int attempts)
{
    if (playerPositions.empty()) return;

    static const glm::ivec2 dirs[4] = { {1,0}, {-1,0}, {0,1}, {0,-1} };
    const int searchRadius = 3; // Радиус поиска блоков вокруг игрока (в тайлах)

    for (int a = 0; a < attempts; a++)
    {
        // 1. Берем случайного игрока
        glm::vec2 pPos = playerPositions[rand() % playerPositions.size()];
        glm::ivec2 gridPos = { (int)pPos.x, (int)pPos.y }; // Предполагаем 1 unit = 1 tile

        // 2. Ищем случайную точку в радиусе вокруг игрока
        glm::ivec2 from = gridPos + glm::ivec2((rand() % (searchRadius * 2 + 1)) - searchRadius,
            (rand() % (searchRadius * 2 + 1)) - searchRadius);

        // Проверка границ уровня
        if (from.x < 0 || from.y < 0 || from.x >= levelWidth || from.y >= levelHeight) continue;

        int i = from.y * levelWidth + from.x;
        Tile& t = tiles[i];

        // 3. Проверяем, можно ли двигать этот тайл
        if (!t.isSolid  || t.destroyed || t.moving) continue;

        // 4. Пытаемся подвинуть в случайном направлении
        glm::ivec2 to = from + dirs[rand() % 4];

        if (to.x < 0 || to.y < 0 || to.x >= levelWidth || to.y >= levelHeight) continue;

        Tile& dst = GetTile(to);
        if (dst.type == TileType::Empty)
        {
            // Перемещаем
            dst = t;
            t = Tile{ TileType::Empty };

            int targetIdx = to.y * levelWidth + to.x;
            MarkTileActive(targetIdx);

            if (OnTileMove) OnTileMove(from, to, 0.6f);
            if (OnTileChanged) {
                OnTileChanged(from, '.',0.0f);
                OnTileChanged(to, TileTypeToChar(dst.type),0.0f);
            }
            continue;
        }
    }
}

bool gameLevel::MoveTileInstant(const glm::ivec2& from, const glm::ivec2& to)
{
    if (from == to) return false;
    if (to.x < 0 || to.y < 0 || to.x >= levelWidth || to.y >= levelHeight) return false;

    Tile& tileFrom = GetTile(from);
    Tile& tileTo = GetTile(to);
    // Вычисляем реальное время движения для клиента
   
    // Если на целевой клетке уже что-то стоит (кроме игрока, которого мы не видим здесь)
    if (tileTo.type != TileType::Empty) return false;

    if (tileFrom.type == TileType::Block && !tileFrom.destroyed)
    {
        tileTo = tileFrom;
        tileFrom = Tile{ TileType::Empty };

        int targetIdx = to.y * levelWidth + to.x;
        MarkTileActive(targetIdx);

        if (OnTileMove) OnTileMove(from, to, 0.6f);
        if (OnTileChanged) {
            OnTileChanged(from, '.',0.0f);
            OnTileChanged(to, TileTypeToChar(tileTo.type),0.0f);
        }
        return true; // Сообщаем, что успешно передвинули
    }
    return false;
}

void gameLevel::StartTileMovement(const glm::ivec2& from, const glm::ivec2& to, float speed)
{
  
    Tile& t = GetTile(from);
    if (!t.isSolid || t.destroyed || t.moving) return;

    Tile& dst = GetTile(to);
    if (dst.type != TileType::Empty) return;

    // Вычисляем реальное время движения для клиента
    float duration = 1.0f / speed;

    t.moving = true;
    t.from = from;
    t.to = to;
    t.moveT = 0.0f;
    t.moveSpeed = speed;

  //  dst.type = TileType::Block;
  //  dst.destroyed = true;


    // ПРОВЕРКА:

    int sourceIdx = from.y * levelWidth + from.x;

    MarkTileActive(sourceIdx);

    // Передаем динамическую длительность вместо 0.4f
    if (OnTileMove) OnTileMove(from, to, duration);

  /*  if (OnTileChanged) {
        OnTileChanged(from, '.');
        OnTileChanged(to, '#');
    }*/
}

std::vector<uint8_t> gameLevel::GetBlocksData() const
{
    std::vector<uint8_t> data;
    data.reserve(tiles.size());
    for (const auto& t : tiles) {
        // Если блок существует и не уничтожен — это 1 (стена), иначе 0
        data.push_back((t.isSolid && !t.destroyed) ? 1 : 0);
    }
    return data;
}

std::vector<glm::vec2> gameLevel::FindPath(glm::vec2 startWorld, glm::vec2 endWorld)
{
    currentQueryId++; // Магия мгновенной очистки кэша

    glm::ivec2 startPos((int)startWorld.x, (int)startWorld.y);
    glm::ivec2 targetPos((int)endWorld.x, (int)endWorld.y);

    int startIdx = startPos.y * levelWidth + startPos.x;
    int targetIdx = targetPos.y * levelWidth + targetPos.x;

    // 1. Валидация целей
    if (targetPos.x < 0 || targetPos.x >= levelWidth || targetPos.y < 0 || targetPos.y >= levelHeight) return {};
    if (tiles[targetIdx].isSolid) return {};

    std::priority_queue<Node, std::vector<Node>, std::greater<Node>> openList;

    // 2. Инициализация старта
    nodeCache[startIdx].gCost = 0;
    nodeCache[startIdx].lastQueryId = currentQueryId;
    nodeCache[startIdx].parentIdx = -1;

    // Используем Чебышёва для начального H
    float startH = (float)std::max(std::abs(startPos.x - targetPos.x), std::abs(startPos.y - targetPos.y));
    openList.push({ startPos, 0, startH });

    bool found = false;
    while (!openList.empty()) {
        Node current = openList.top();
        openList.pop();

        int currIdx = current.pos.y * levelWidth + current.pos.x;
        if (current.g > nodeCache[currIdx].gCost) continue;
        if (currIdx == targetIdx) { found = true; break; }

        for (auto& d : dirs) {
            glm::ivec2 next = current.pos + d;
            if (next.x < 0 || next.x >= levelWidth || next.y < 0 || next.y >= levelHeight) continue;

            int nIdx = next.y * levelWidth + next.x;
            if (tiles[nIdx].isSolid) continue;

            float moveCost =  1.0f;
            float newG = current.g + moveCost;

            if (nodeCache[nIdx].lastQueryId != currentQueryId || newG < nodeCache[nIdx].gCost) {
                nodeCache[nIdx].gCost = newG;
                nodeCache[nIdx].lastQueryId = currentQueryId;
                nodeCache[nIdx].parentIdx = currIdx;

                // Эвристика Чебышёва (расстояние для 8 направлений)
                float h = (float)std::max(std::abs(next.x - targetPos.x), std::abs(next.y - targetPos.y));
                openList.push({ next, newG, h });
            }
        }
    }


    std::vector<glm::vec2> path;
    if (found) {
        int currIdx = targetIdx;
        int startIdx = (int)startWorld.y * levelWidth + (int)startWorld.x;

        while (currIdx != -1) {
            // Превращаем индекс обратно в координаты
            float x = (float)(currIdx % levelWidth) + 0.5f;
            float y = (float)(currIdx / levelWidth) + 0.5f;
            path.push_back({ x, y });

            if (currIdx == startIdx) break;
            currIdx = nodeCache[currIdx].parentIdx;
        }
        std::reverse(path.begin(), path.end());
    }
    return path;
}

bool gameLevel::IsPathClear(glm::vec2 start, glm::vec2 end, float radius)
{
    float dist = glm::distance(start, end);
    if (dist < 0.1f) return true;

    glm::vec2 dir = (end - start) / dist;
    // Перпендикуляр для проверки ширины прохода
    glm::vec2 side = glm::vec2(-dir.y, dir.x) * radius;

    // Шагаем вдоль линии
    for (float d = 0.0f; d < dist; d += 0.4f) {
        glm::vec2 checkPoints[3] = {
            start + dir * d,          // Центр
            start + dir * d + side,   // Левый край
            start + dir * d - side    // Правый край
        };

        for (const auto& p : checkPoints) {
            int x = (int)p.x;
            int y = (int)p.y;
            if (x < 0 || x >= levelWidth || y < 0 || y >= levelHeight || tiles[y * levelWidth + x].isSolid)
                return false;
        }
    }
    return true;
}
