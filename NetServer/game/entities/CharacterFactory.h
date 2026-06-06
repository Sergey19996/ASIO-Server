#pragma once
#include <memory>
#include <string>
#include "../NetShared/PlayerClass.h"
#include "../NetShared/entities/Character.h"
class Character;

class Match;

class CharacterFactory {
public:
    static std::unique_ptr<Character> Create(
        PlayerClass cls,
        uint32_t netId,
        const std::string& name,
        Match* match
    );
    static std::unique_ptr<Character> CreateAI(
        ArchetypeId type,
        uint32_t netId,
        Match* server,
        glm::vec2 spawnPos,
        int home);
    static std::unique_ptr<Character> CreateCompanion(
        ArchetypeId type,
     
        uint32_t netId,
        Character* oner,
        Match* server
       
       );
};