#pragma once
#include <cstdint>
enum class TileType : uint8_t {
	Empty,
//	Wall,       // Неразрушимая стена (границы)
	Block,      // Обычный блок
	Beacon,
	Ice,        // Ледяной (замораживает при разрушении)
	Explosive,  // Взрывающийся
	Sand,      // Замедляющий
	Fire,
	Healer,     // Восстанавливающий HP
	Graveyard,
	CompanionAltar,
};

// Конвертация типа в символ для передачи по сети и отрисовки
inline char TileTypeToChar(TileType type) {
	switch (type) {
	case TileType::Empty:     return '.';
//	case TileType::Wall:      return 'W'; // Неубиваемая стена
	case TileType::Block:     return '#'; // Обычный кирпич
	case TileType::Beacon:    return 'B';
	case TileType::Ice:       return 'I';
	case TileType::Explosive: return 'E';
	case TileType::Sand:     return 'S';
	case TileType::Healer:    return 'H';
	case TileType::Fire:      return 'F';
	case TileType::Graveyard: return 'G';
	case TileType::CompanionAltar:	return 'A';
	default:                  return '?';
	}
}

// Конвертация обратно (для парсинга на клиенте)
inline TileType CharToTileType(char c) {
	switch (c) {
	case '.': return TileType::Empty;
//	case 'W': return TileType::Wall;
	case '#': return TileType::Block;
	case 'B': return TileType::Beacon;
	case 'I': return TileType::Ice;
	case 'E': return TileType::Explosive;
	case 'S': return TileType::Sand;
	case 'H': return TileType::Healer;
	case 'F': return TileType::Fire;
	case 'A': return TileType::CompanionAltar;
	default:  return TileType::Empty;
	}
}