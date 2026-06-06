#ifndef WallSpell_h
#define WallSpell_h

#include "../TimedSpell.h"
#include <iostream>


class WallSpell : public TimedSpell {
public:


    WallSpell() { id = SpellId::Wall; state = SpellState::Appear;
    appearDuration = 0.5f;
    activeDuration = 5.0f;
    disappearDuration = 0.5f;
    }
    struct WallPart
    {
        uint32_t uid;
        float scale = 0.0f;
    };

    std::vector<WallPart> parts;
    

    bool Cast(Character& caster, Match* server) override;

    void ForceFinish(int uid, Match* server) override;
    bool OwnsProjectile(int uid) override;
private:

    void UpdateAppear(float dt, Match* server)override;
    void UpdateActive(float dt, Match* server)override;
    void UpdateDisappear(float dt, Match* server)override;

    void RemoveParts(int uid);


};
#endif // !Game