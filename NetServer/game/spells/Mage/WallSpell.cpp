#include "WallSpell.h"
#include "../../../Match.h"  // здесь уже можно подключать
#include "../../utils/WorldGrid.h"


bool WallSpell::Cast(Character& caster, Match* server)
{
    // 1. —Ќј„јЋј ѕ–ќ¬≈–я≈ћ: может ли маг вообще совершить действие?
    if (!caster.CanCastSpell()) {
        return false;
    }

    glm::vec2 wallCenter = caster.position + caster.direction * (1.0f + caster.radius);
    glm::vec2 rotatedDir = glm::vec2(-caster.direction.y, caster.direction.x);

    glm::vec2 vel = { 0.0f,0.0f };
    float radius = 0.0f;
  

    //// 1. —оздаем снар€д с базовым уроном мага
    ProjectileParams p;
    p.ownerId = caster.GetId();
    for (int i = -1; i <= 1; i++) {
        glm::vec2 spawnPos = wallCenter + rotatedDir * float(i);

        if (IsBlockedCell(spawnPos, server->level)) {
            continue;
        }

        // 1. »Ќ“≈√–ј÷»я: »спользуем фабрику сервера дл€ каждого сегмента
        // ћетод выделит индекс, назначит тип Wall, подт€нет правила преграды 
        // и добавит сегмент в список активных объектов мира.

        uint16_t idx = server->CreateProjectile(ProjectileType::Wall, p, spawnPos, vel, radius);

        // 2. ≈сли пул снар€дов кончилс€ Ч прекращаем возведение стены
        if (idx == 0xFFFF) {
            std::cout << "[WARN] Projectile Pool Empty for Wall segment!" << std::endl;
            return false;
        }

        // 3. “юнинг параметров сегмента
        auto& slot = server->projectiles[idx];
        slot.data.fRadius = 0.5f;
        slot.data.vVel = { 0.0f, 0.0f }; // —тена неподвижна

        // 4. —охран€ем индекс части дл€ управлени€ временем жизни заклинани€
        parts.push_back({ (uint32_t)idx, 0.0f });
    }

    caster.LockMovement();
    caster.LockActions();


    return true;
}


void WallSpell::ForceFinish(int uid, Match* server)
{
    RemoveParts(uid);
}

bool WallSpell::OwnsProjectile(int uid)
{
    for (auto& p : parts)
        if (p.uid == uid) return true;

    return false;
}

void WallSpell::UpdateAppear(float dt, Match* server)
{
    float t = std::min(lifeTimer / appearDuration, 1.0f);

    for (auto& p : parts) {
        auto& slot = server->projectiles[p.uid];
        if (!slot.active) continue;

        p.scale = t;
        slot.data.fRadius = t * 0.5f;

    if (t >= 1.0f) {
        state = SpellState::Active;
        lifeTimer = 0;
        slot.collisionEnabled = true;
        ReleaseCaster(server);
    }

    }


}

void WallSpell::UpdateActive(float dt, Match* server)
{
    if (lifeTimer >= activeDuration) {
        state = SpellState::Disappear;
        lifeTimer = 0;
    }


}

void WallSpell::UpdateDisappear(float dt, Match* server)
{
    float t = 1.0f - std::min(lifeTimer / disappearDuration, 1.0f);

    for (auto& p : parts) {
        auto& slot = server->projectiles[p.uid];
        if (slot.active) {
            slot.data.fRadius = t * 0.5f;
        }
    }

    if (t <= 0.0f) {
        for (auto& p : parts) {
            server->MarkProjectileForRemoval(p.uid);
        }
        state = SpellState::Finished;
    }

}

void WallSpell::RemoveParts(int uid)
{
    // убираем только один сегмент
    parts.erase(
        std::remove_if(parts.begin(), parts.end(),
            [uid](auto& p) { return p.uid == uid; }),
        parts.end()
    );

    // если больше нет частей Ч заклинание закончено
    if (parts.empty())
        state = SpellState::Finished;
}
