#ifndef SPELL_MANAGER
#define SPELL_MANAGER


#include <vector>
#include "Spell.h"
#include <memory>
#include <algorithm>


class SpellManager {
public:
	std::vector<std::unique_ptr<Spell>> activeSpells;

	void Add(std::unique_ptr<Spell> s) {
		activeSpells.push_back(std::move(s));

	}

	void Update(float dt, Match* server) {
     
		for (auto& s : activeSpells)
			s->Update(dt, server);

      
   
		activeSpells.erase(
			std::remove_if(activeSpells.begin(), activeSpells.end(),
				[](auto& s) {return s->IsFinished(); }),
			activeSpells.end()
		);
	}
	void ForceFinishProjectile(int projectileID, Match* server) {

		for (auto& s : activeSpells) {  // бежим по всем активным спелам
			 
			if (s->OwnsProjectile(projectileID)) {
         
				s->ForceFinish(projectileID,server);   // в Dissapear - markprojectileforRemoval
				return;
			}

		}

	}
   void OnProjectileHitWorld(uint16_t projId, glm::ivec2 cell, Match* server) {
        for (auto& spell : activeSpells) {
            if (spell->OwnsProjectile(projId)) {
                // Вызываем специфичный для спелла метод (нужно добавить в базовый класс Spell)
                spell->NotifyWorldHit(cell, server);
            }
        }
    }
   void OnProjectileHitCharacter(uint32_t projId, uint32_t targetId, Match* server) {
       for (auto& spell : activeSpells) {
           if (spell->OwnsProjectile(projId)) {
               // Вызываем специфичный для спелла метод (нужно добавить в базовый класс Spell)
               spell->NotifyCharacterHit(projId, targetId, server);
           }
       }
   }
    Spell* FindActive(SpellId id, uint32_t owner)
    {
        for (auto& s : activeSpells)
            if (s->GetSpellId() == id &&
                s->GetOwner() == owner &&
                !s->IsFinished())
                return s.get();
        return nullptr;
    }
    void RemoveFinished() {
        activeSpells.erase(
            std::remove_if(activeSpells.begin(), activeSpells.end(),
                [](auto& s) { return s->IsFinished(); }),
            activeSpells.end()
        );
    }
    void Cancel(uint32_t casterId, SpellId id, Match* server) {

        if (auto* s = FindActive(id, casterId))
            s->ForceFinish(-1, server);

    }

};


#endif // !SPELL_MANAGER


