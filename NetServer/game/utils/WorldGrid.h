#pragma once
#include <glm/glm.hpp>
#include <cmath>
#include "../NetServer/gameLevel.h"
class gameLevel; // forward declaration

inline glm::ivec2 WorldToCell(const glm::vec2& pos)
{
    return {
        int(std::floor(pos.x)),
        int(std::floor(pos.y))
    };
}
inline glm::vec2 GridToWorld(const glm::ivec2& cell)
{
    return glm::vec2(
        cell.x * GRID_CELL_SIZE,
        cell.y * GRID_CELL_SIZE
    );
}

inline bool IsBlockedCell(const glm::vec2& pos, const gameLevel& level)
{
    glm::ivec2 cell = WorldToCell(pos);

    if (cell.x < 0 || cell.y < 0 ||
        cell.x >= level.levelWidth ||
        cell.y >= level.levelHeight)
        return true;
   
   // char tle = level.IsSolid(cell);

    //char tile = level.LevelData[cell.y * level.screenW + cell.x];
    return level.IsSolid(cell);
}

inline bool IsBlockedCircle(const glm::vec2& center,float radius,const gameLevel& level)
{
    static const glm::vec2 offsets[4] = {
        { 1,  0},
        {-1,  0},
        { 0,  1},
        { 0, -1}
    };

    for (auto& o : offsets)
    {
        glm::vec2 p = center + o * radius;
        if (IsBlockedCell(p, level))
            return true;
    }

    return false;
}
inline bool ResolveCircleWorldCollision(glm::vec2& position,float radius,gameLevel& level, bool& Kill, glm::vec2*  outNormal = nullptr,glm::ivec2* outHitCell = nullptr)
{

    bool isKill = Kill;
    Kill = false;
    glm::ivec2 base = WorldToCell(position);
    bool collided = false;

    float bestDist = FLT_MAX;
    glm::ivec2 bestCell;

    for (int y = base.y - 1; y <= base.y + 1; y++)
        for (int x = base.x - 1; x <= base.x + 1; x++)
        {
            glm::ivec2 cell(x, y);
            const Tile& t = level.GetTile(cell);

            if ((!t.isSolid) || t.destroyed || t.growInterp <= 0.0f)
                continue;

            float half = 0.5f * t.growInterp;

            glm::vec2 center(cell.x + 0.5f, cell.y + 0.5f);


            if (t.moving)
            {
                glm::vec2 a(t.from.x + 0.5f, t.from.y + 0.5f); //центр стартовой
                glm::vec2 b(t.to.x + 0.5f, t.to.y + 0.5f); // центр целевой 
                center = glm::mix(a, b, t.moveT);
            }
            else
            {
                center = glm::vec2(cell.x + 0.5f, cell.y + 0.5f);
            }


            glm::vec2 min = center - glm::vec2(half);
            glm::vec2 max = center + glm::vec2(half);

            glm::vec2 nearest;
            nearest.x = std::clamp(position.x, min.x, max.x); // если позиция меньше min.x вернет - min.x
            nearest.y = std::clamp(position.y, min.y, max.y); // если позиция больше max.y - вернет max.y иначе pos

            glm::vec2 delta = position - nearest;
            float dist = glm::length(delta); // гиппотенуза



            if (dist < radius)
            {
                if(isKill == true)
                if (dist < radius * 0.3f)
                {
                    // Игрок раздавлен!
                    // Тут вызываем функцию смерти игрока, например:
                    // level.Match->KillPlayer(attackerId); 
                    // return true; // Прерываем проверку, так как игрок труп
                   // isKill = true;
                    Kill = true;
                    return true;
                }
               


                float overlap = radius - dist;



                glm::vec2 normal;

                if (dist > 0.0001f) {
                    normal = delta / dist; // Направление от стены к игроку
                    position += normal * overlap;
                }
                else {
                    normal = glm::vec2(1, 0); // Крайний случай: выталкиваем вбок
                    position.x += overlap;
                }

                if (outNormal) * outNormal = normal; // Передаем нормаль наружу
                

                collided = true;

                if (dist < bestDist)
                {
                    bestDist = dist;  //c каждым циклом значение будет всё меньше (от самого большего допустимого)
                    bestCell = cell;
                }
            }
           
        }

    if (collided && outHitCell)
        *outHitCell = bestCell;

    return collided;
}