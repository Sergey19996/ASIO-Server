#ifndef GAME_SERVER_H
#define GAME_SERVER_H

#include "MatchState.hpp"
#include "../NetShared/NetPlayerProfile.h"
#include "ReconnectionSession.h"
#include "map"
//#include "../NetShared/PlayerInfo.h"
#include "Match.h"
#include "sqlite3.h"

struct PendingClient
{
	std::string name;
	bool loggedIn = false;
};

struct PlayerAccount {
	// ВСЕ ДАННЫЕ ИЗ БД (Прогресс игрока)
	NetPlayerProfile profile;

//	PlayerID id;
//	std::string name;
	PlayerSessionData session;
	PlayerState state = PlayerState::LOBBY;
	bool isOnline = true;

	// Вспомогательный метод для удобства
	PlayerID GetID() const { return profile.id; }
};

class GameServer : public olc::net::server_interface<GameMsg>
{
public:
	GameServer(uint16_t nPort);
	~GameServer();


	// 1. СЕТЕВОЙ УРОВЕНЬ (Временные связи)
	// Ключ: ConnID (из olc::net - индекс сокета) -> Значение: ID игрока
	// ЗАЧЕМ: Когда от сокета приходит пакет, мы должны мгновенно понять, КТО это.
	// СВЯЗЬ: Очищается сразу при OnClientDisconnect.
	std::unordered_map<ConnID, PlayerID> connToPlayer; 
	// для быстрого поиска - по вечному id находим временное соединение
	std::unordered_map<PlayerID, ConnID> playerToConn;


	// Ключ: ConnID -> Значение: Умный указатель на объект связи
	std::unordered_map<uint32_t, std::shared_ptr<olc::net::connection<GameMsg>>> idToConnObj;

	// 2. УРОВЕНЬ АККАУНТОВ И СЕССИЙ (Персистентные данные)
	// Ключ: PlayerID (постоянный) -> Значение: Данные аккаунта (имя, монеты, токен)
	// ЗАЧЕМ: Главное хранилище данных игрока, пока он в сети.
	// СВЯЗЬ: Здесь хранится sessionToken, который мы сверяем при реконнекте.
	std::unordered_map<PlayerID, PlayerAccount> entities;

	// Список всех запущенных матчей. unique_ptr сам удалит матч, когда он закончится.
	std::vector<std::shared_ptr<Match>> matches;

	// Ключ: PlayerID -> Значение: Указатель на объект матча
	// ЗАЧЕМ: Чтобы перенаправлять игровые пакеты (движение, стрельба) сразу в Match.
	// СВЯЗЬ: Если игрока нет в этом мапе — значит он в Главном Меню.
	std::unordered_map<PlayerID, std::shared_ptr<Match>> playerToMatch;

	// Очередь тех, кто нажал кнопку "В бой"
	// ЗАЧЕМ: Matchmaker берет ID отсюда и создает новый Match.
	std::vector<PlayerID> matchmakingQueue;



	// НОВЫЙ ВЕКТОР: Список индексов снарядов, которые сейчас в игре
	// Вспомогательный метод для удаления из середины вектора за O(1)
//	void RemoveActiveProjectile(uint16_t id);

	// Основная карта для реконнекта
	std::unordered_map<uint64_t, PlayerID> tokenToSession; // реконнект



	std::unordered_map<uint32_t, PendingClient> pendingClients;



	float felapsedTime = 0.0f;
	float serverTime = 0.0f;



	PlayerID GeneratePlayerID();
	const std::string& GetPlayerName(PlayerID id) const;

	template<typename T>
	void SendToMatch(const Match& match, GameMsg type, const T& payload);
	void SendToMatch(const Match& match, GameMsg type);

	void UpdateServer();


	void HandleFindMatch(uint32_t pid);
	void CreateMatchFromQueue();
	std::shared_ptr<olc::net::connection<GameMsg>> GetConnectionByPlayer(PlayerID pid);
	void SavePlayerToDB(const PlayerAccount& acc);
protected:
	void BroadcastMatchmakingStatus();
	PlayerAccount* GetPlayerByConnID(uint32_t connID);

	void ActuallyDeletePlayerFromToken(uint64_t token, PlayerAccount& session);
	std::mt19937 serverRng{ std::random_device{}() };

	// События сети
	bool OnClientConnect(std::shared_ptr<olc::net::connection<GameMsg>> client) override;
	void OnClientValidated(std::shared_ptr<olc::net::connection<GameMsg>> client) override;
	void OnClientDisconnect(std::shared_ptr<olc::net::connection<GameMsg>> client) override;
	void OnMessage(std::shared_ptr<olc::net::connection<GameMsg>> client, olc::net::message<GameMsg>& msg) override;
	void HandleMenuMessage(std::shared_ptr<olc::net::connection<GameMsg>>& client, PlayerID pid, olc::net::message<GameMsg>& msg);
	void HandleLogin(std::shared_ptr<olc::net::connection<GameMsg>> client, olc::net::message<GameMsg>& msg, uint32_t& connId);
	void HandleRegister(std::shared_ptr<olc::net::connection<GameMsg>> client, olc::net::message<GameMsg>& msg, uint32_t& connId);
	

	//MatchState matchState;
	MatchMode matchMode = MatchMode::LAST_MAN_STANDING;
	
	float endMatchTimer;


//	std::vector<SpatialCell> spatialGrid;
	
	PlayerID nextPlayerID = 1000000; // в будущем переписать - так как матч сам должен давать id для игроков и мобов 
	
private:
	// Указатель на структуру базы данных SQLite
	sqlite3* db = nullptr;
	bool InitDatabase();
	PlayerID GetPlayerIdByName(std::string name);
	PlayerAccount LoadPlayerFromDB(PlayerID pid);
	PlayerID RegisterPlayerInDB(std::string name, std::string password);
	
};

template<typename T>
inline void GameServer::SendToMatch(const Match& match, GameMsg type, const T& payload)
{
	olc::net::message<GameMsg> msg;
	msg.header.id = type;
	msg << payload;

	for (uint32_t pl : match.GetPlayersIdx())
	{
		auto client = GetConnectionByPlayer(pl);
		if (client && client->IsConnected())
		{
			MessageClient(client, msg);
		}
	}
}





#endif // !GAME_SERVER_
