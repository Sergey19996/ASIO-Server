#pragma once
#include "Spell.h"
class TimedSpell : public Spell {
public:
    float appearDuration = 0.0f;
    float activeDuration = 0.0f;
    float disappearDuration = 0.0f;

    void Update(float dt, Match* server) override
    {
        lifeTimer += dt;

        switch (state)
        {
        case SpellState::Appear:    UpdateAppear(dt, server); break;
        case SpellState::Active:    UpdateActive(dt, server); break;
        case SpellState::Disappear: UpdateDisappear(dt, server); break;
        default: break;
        }
    }

protected:
    virtual void UpdateAppear(float dt, Match* server) = 0;
    virtual void UpdateActive(float dt, Match* server) = 0;
    virtual void UpdateDisappear(float dt, Match* server) = 0;
};