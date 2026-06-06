#pragma once
#include <random>
#include "../NetShared/entities/Character.h"
#include "../NetShared/PlayerClass.h"

using PlayerID = uint32_t;
using PlayerSlot = uint32_t;
using ConnID = uint32_t;

struct PlayerSessionData
{
	bool wasRestored = false;
	bool isOnline = true;
	float disconnectTimer = 10.0f;
	int retryCount = 0;
	uint32_t lastConnectionID = 0; // Чтобы знать, какой ID был у игрока

	bool classSelected = false;
	PlayerClass selectedclass = PlayerClass::None;
	bool alive = true;
	bool ready = false;
	uint64_t sessionToken = 0;
};


// Метод генерации токена (современный C++11/20)
uint64_t GenerateRandomToken();
 
