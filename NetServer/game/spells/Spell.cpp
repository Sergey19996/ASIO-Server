#include "Spell.h"
#include "../../Match.h"  // здесь уже можно подключать
void Spell::ReleaseCaster(Match* server)
{
    if (bHasUnlocked) return; 

    // Используем безопасный метод поиска индекса через Match
    uint32_t idx = server->GetPlayerIndex(ownerId);
    
    // Проверяем, что индекс валиден (обычно 0xFFFF или -1 при неудаче)
    if (idx < server->GetEntities().size()) {
        auto& pl = server->GetEntities()[idx];
        if (pl.active && pl.character) {
            pl.character->UnlockMovement();
            pl.character->UnlockActions(); // ОЧЕНЬ ВАЖНО
            bHasUnlocked = true; 
           
        }
    }
}
