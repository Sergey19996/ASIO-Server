#pragma once 
#include <string>
#include "../NetShared/PlayerClass.h"

struct sPlayerInfo
{
    uint32_t playerId;
    std::string name;
    PlayerClass playerClass;
};

