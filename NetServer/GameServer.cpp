#include "GameServer.h"
#include "../NetServer/gameLevel.h"
#include <iostream>
#include "game/utils/WorldGrid.h"
#include <cassert>
#define MatchW 50
#define MatchH 50
GameServer::GameServer(uint16_t nPort) : olc::net::server_interface<GameMsg>(nPort)
{
	std::cout << "Версия SQLite: " << sqlite3_libversion() << std::endl;

	InitDatabase();

}

GameServer::~GameServer()
{
	if (db) {
		sqlite3_close(db);
		std::cout << "[Database] Closed successfully." << std::endl;
	}
	
}


PlayerID GameServer::GeneratePlayerID()
{
	return nextPlayerID++;
}

const std::string& GameServer::GetPlayerName(PlayerID id) const
{
	static std::string empty = "Unknown";

	auto itPlayer = entities.find(id);
	if (itPlayer != entities.end()) // если не дошли до енда
	return itPlayer->second.profile.name;


		return empty;

}

void GameServer::SendToMatch(const Match& match, GameMsg type)
{
	olc::net::message<GameMsg> msg;
	msg.header.id = type;

	for (uint32_t pl : match.GetPlayersIdx())
	{
		
		
		auto client = GetConnectionByPlayer(pl);
		if (client && client->IsConnected()) // Проверка, что игрок не в дисконнекте
		{
			MessageClient(client, msg);
		}
	}
}

void GameServer::UpdateServer()
{

	//for (auto& match : matches)
	//	match->Update(felapsedTime);
	
	std::erase_if(matches, [&](const std::shared_ptr<Match>& match) {
		if (match->IsReadyToDelete()) {
			// Очищаем глобальные ссылки на этот матч перед его удалением
			for (auto& p : match->GetEntities()) {
				playerToMatch.erase(p.netId);
				
			}
			return true; // Удалить этот матч из вектора matches
		}

		// Если не удаляем, то обновляем
		match->Update(felapsedTime);
		return false; // Оставить матч
		});

	/*if (matchState != MatchState::INGAME)
		return;*/


		// --- ОЧИСТКА ОЖИДАЮЩИХ РЕКОННЕКТА ---
	for (auto it = entities.begin(); it != entities.end(); )
	{
		PlayerAccount& acc = it->second;

		// Если игрок оффлайн, тикаем таймер реконнекта
		if (!acc.isOnline)
		{
			acc.session.disconnectTimer -= felapsedTime;

			if (acc.session.disconnectTimer <= 0.0f)
			{
				std::cout << "[TIMEOUT]: Player " << acc.profile.name << " (ID: " << acc.profile.id<< ") failed to reconnect.\n";

				// 1. Убираем из матча (если он там был)
				if (playerToMatch.count(acc.profile.id)) {
					playerToMatch[acc.profile.id]->RemovePlayer(acc.profile.id);
					playerToMatch.erase(acc.profile.id);
				}

				// 2. Убираем из очереди поиска
				auto qIt = std::find(matchmakingQueue.begin(), matchmakingQueue.end(), acc.profile.id);
				if (qIt != matchmakingQueue.end()) matchmakingQueue.erase(qIt);

				// 3. Удаляем токен из карты реконнекта, чтобы нельзя было войти под этим ID
				tokenToSession.erase(acc.session.sessionToken);

				// 4. Окончательно удаляем аккаунт из памяти сервера
				it = entities.erase(it);
				continue;
			}
		}
		++it;
	}
}


void GameServer::HandleFindMatch(uint32_t pid)
{
	if (playerToMatch.find(pid) != playerToMatch.end())
		return;

	if (std::find(matchmakingQueue.begin(), matchmakingQueue.end(), pid) != matchmakingQueue.end())
		return;

	matchmakingQueue.push_back(pid);

	// Сразу уведомляем всех в очереди об изменении состава
	BroadcastMatchmakingStatus();

	

	if (matchmakingQueue.size() >= 3)
	{
		CreateMatchFromQueue();
	}
}

void GameServer::CreateMatchFromQueue()
{
	static uint32_t nextMatchId = 1;

	// Берем следующее случайное число из генератора сервера
	uint32_t randomSeed = serverRng();

	// Создаем через shared_ptr
	auto match = std::make_shared<Match>(nextMatchId++, this, MatchW, MatchH, randomSeed);

	for (int i = 0; i < 3; ++i)
	{
		if (matchmakingQueue.empty()) break; // Защита от пустой очереди

		PlayerID pid = matchmakingQueue.back();
		matchmakingQueue.pop_back();
		// Теперь здесь хранится общая ссылка на объект
		playerToMatch[pid] = match;


		match->AddPlayer(pid);
		
	}

	matches.push_back(match);
}

void GameServer::BroadcastMatchmakingStatus()
{
	// Подготавливаем данные
	sMatchmakingStatus status;
	status.playersInQueue = (uint32_t)matchmakingQueue.size();

	// Создаем сообщение
	olc::net::message<GameMsg> msg;
	msg.header.id = GameMsg::Server_MatchmakingStatus;
	msg << status;

	// Рассылаем ТОЛЬКО тем, кто в очереди
	for (PlayerID pid : matchmakingQueue)
	{
		auto client = GetConnectionByPlayer(pid);
		if (client && client->IsConnected())
		{
			MessageClient(client, msg);
		}
	}
}

PlayerAccount* GameServer::GetPlayerByConnID(uint32_t connID)
{
	// 1. Находим PlayerID по соединению
	auto itID = connToPlayer.find(connID);
	if (itID == connToPlayer.end()) return nullptr;

	PlayerID pid = itID->second;

	// 2. Находим аккаунт в мапе игроков
	auto itPlayer = entities.find(pid);
	if (itPlayer != entities.end()) {
		// Возвращаем адрес объекта внутри мапа
		return &(itPlayer->second);
	}

	return nullptr;
}
void GameServer::ActuallyDeletePlayerFromToken(uint64_t token, PlayerAccount& acc)

{
	olc::net::message<GameMsg> msg;
	msg.header.id = GameMsg::Game_RemovePlayer;

	// Рассылаем постоянный PlayerID, чтобы клиенты удалили персонажа с экрана
	msg << acc.profile.id;
	MessageAllClients(msg);

	
}


bool GameServer::OnClientConnect(std::shared_ptr<olc::net::connection<GameMsg>> client)
{
	// Разрешаем все подключения
	return true;
}

void GameServer::OnClientValidated(std::shared_ptr<olc::net::connection<GameMsg>> client)
{
	if (client) {  // conid - сама связь на обьектсоединений
		idToConnObj[client->GetID()] = client;
	}
	olc::net::message<GameMsg> msg;
	msg.header.id = GameMsg::Client_Accepted;
	client->Send(msg);
}

void GameServer::OnClientDisconnect(std::shared_ptr<olc::net::connection<GameMsg>> client) // это вызывается когда в очереди под капотом мы не нашли этого клиента
{
	if (!client) return;

	uint32_t connID = client->GetID();

	// 1. Убираем объект соединения
	idToConnObj.erase(connID);

	// 2. Если это соединение было привязано к игроку
	if (connToPlayer.count(connID))
	{
		PlayerID pid = connToPlayer[connID];

		// Помечаем, что игрок не в сети
		if (entities.count(pid)) {
			entities[pid].isOnline = false;
		}

		// Разрываем связи между сокетом и игроком
		playerToConn.erase(pid);
		connToPlayer.erase(connID);

		std::cout << "[Server] Player " << pid << " disconnected, kept in memory for reconnection.\n";
	}
	
}

void  GameServer::OnMessage(std::shared_ptr<olc::net::connection<GameMsg>> client, olc::net::message<GameMsg>& msg)
{
	uint32_t connID = client->GetID();

	// 1. ОБРАБОТКА ЛОГИНА (у игрока еще нет PID)
	if (msg.header.id == GameMsg::Client_Login) {
		HandleLogin(client, msg,connID);
		return;
	}
	if (msg.header.id == GameMsg::Client_RegisterWithServer) {
		HandleRegister(client, msg, connID); // Создадим этот метод
		return;

	}
	// 2. ПРОВЕРКА АВТОРИЗАЦИИ (для всех остальных сообщений)
	if (connToPlayer.find(connID) == connToPlayer.end()) return;
	PlayerID pid = connToPlayer[connID]; // берём постоянный id клиента 

	// 3. МАРШРУТИЗАЦИЯ В МАТЧ
	if (playerToMatch.count(pid)) {
		playerToMatch[pid]->OnPlayerMessage(pid, msg);
	}
	else {
		// Логика главного меню (поиск матча, чат и т.д.)
		HandleMenuMessage(client,pid, msg);
	}
	


}

void GameServer::HandleMenuMessage(std::shared_ptr<olc::net::connection<GameMsg>>& client,PlayerID pid, olc::net::message<GameMsg>& msg)
{

	//HandleMenuMassage
	switch (msg.header.id)
	{

	//case GameMsg::Client_RegisterWithServer:
	//{
	//	ConnID connID = client->GetID();
	//	if (!connToPlayer.count(connID)) break;


	//	//Player& pl = players[idToSlot[pid]];

	//	//// Отправляем клиенту ЕГО постоянный ID
	//	//olc::net::message<GameMsg> msgID;
	//	//msgID.header.id = GameMsg::Client_AssignID;
	//	//msgID << pid;
	//	//MessageClient(client, msgID);

	//	// Рассылаем всем
	//	sPlayerDescription desc;
	//	desc.nUniqueID = pid;
	//	//desc.vPos = pl.character.position;
	//	//std::cout << pid << std::endl;
	//	//olc::net::message<GameMsg> add;
	//	//add.header.id = GameMsg::Game_AddPlayer;
	//	//add << desc;
	//	//MessageAllClients(add);
	//	break;
	//}

	case GameMsg::Client_UnregisterWithServer:
	{
		break;
	}

	case GameMsg::Server_Ping: {
		// Просто отправляем сообщение назад тому же клиенту (эхо)
		client->Send(msg);
		break;
	}


	case GameMsg::Client_FindMatch:
	{
		HandleFindMatch(pid);
		break;
	}
	case GameMsg::Client_ReturnToLobby:
	{

		// 1. Если игрок в матче, просим матч корректно его удалить
		if (playerToMatch.count(pid)) {
			playerToMatch[pid]->RemovePlayer(pid); // Логика внутри матча (см. ниже)
			playerToMatch.erase(pid);              // Убираем связь "игрок-матч" на сервере
		}

		// 2. Сбрасываем статус игрока в глобальном списке
		entities[pid].state = PlayerState::LOBBY;
		entities[pid].session.ready = false;
		entities[pid].session.selectedclass = PlayerClass::None;

		// 3. Отвечаем клиенту
		olc::net::message<GameMsg> reply;
		reply.header.id = GameMsg::Server_ReturnToLobby;
		MessageClient(client, reply);
		break;
	}
	case GameMsg::Client_CancelMatchmaking: {



		// 1. Удаляем игрока из очереди поиска матча
		auto it = std::find(matchmakingQueue.begin(), matchmakingQueue.end(), pid);
		if (it != matchmakingQueue.end()) {
			matchmakingQueue.erase(it);
		}

		// 2. Обновляем статус в глобальном объекте игрока
		if (entities.count(pid)) {
			entities[pid].state = PlayerState::LOBBY;
		}

		// 3. (Опционально) Рассылаем остальным обновленный статус очереди
		// Если ваш BroadcastMatchmakingStatus использует matchmakingQueue.size()
		BroadcastMatchmakingStatus();
		break;
	}


	}
}

void GameServer::HandleLogin(std::shared_ptr<olc::net::connection<GameMsg>> client, olc::net::message<GameMsg>& msg, uint32_t& connId)
{
	sLoginRequest req;
	msg >> req;

	std::string name(req.firstName); // Автоматически обрежется по первому \0
	std::string pass(req.password);


	std::cout << req.firstName << "  " << req.password << "    " << req.sessionToken << std::endl;
	sLoginResult res;
	// Обязательно обнуляем всё перед использованием!
	std::memset(&res, 0, sizeof(sLoginResult));

	// 1. Ищем игрока
	PlayerID pid = GetPlayerIdByName(name);
	std::cout << pid << std::endl;
	auto safe_copy = [](char* dest, const char* src, size_t dest_size) {
		std::memset(dest, 0, dest_size);
		if (src) {
			std::memcpy(dest, src, std::min(std::strlen(src), dest_size - 1));
		}
		};
	// 2. Проверка существования
	if (pid == 0) {
		res.success = false;
		safe_copy(res.errorMessage, "User not found", sizeof(res.errorMessage));

		olc::net::message<GameMsg> reply;
		reply.header.id = GameMsg::Server_LoginResult;
		reply << res;
		MessageClient(client, reply);
		return;
	}

	// 3. Загружаем и проверяем пароль
	PlayerAccount acc = LoadPlayerFromDB(pid);
	std::cout << acc.profile.password << "  " << acc.profile.id << "    " << acc.profile.name << std::endl;;
	if (acc.profile.password != pass) {
		res.success = false;
		safe_copy(res.errorMessage, "Invalid password", sizeof(res.errorMessage));

		olc::net::message<GameMsg> reply;
		reply.header.id = GameMsg::Server_LoginResult;
		reply << res;
		MessageClient(client, reply);
		return;
	}

	// 3.5 Проверка на "уже в сети"
	if (entities.count(pid) > 0 && entities[pid].isOnline) {
		// Вариант А: Запретить вход
		/*
		res.success = false;
		safe_copy(res.errorMessage, "Account already online", sizeof(res.errorMessage));
		// ... отправить reply и return
		*/

		// Вариант Б: Выкинуть старую сессию (обычно лучше для игрока)
		uint32_t oldConnId = playerToConn[pid];
		if (idToConnObj.count(oldConnId)) {
			// Отправляем старому клиенту уведомление (опционально)
			// KickClient(idToConnObj[oldConnId]); 
			idToConnObj[oldConnId]->Disconnect();
		}

		// Очищаем старые привязки
		connToPlayer.erase(oldConnId);
		idToConnObj.erase(oldConnId);
		// Токен тоже стоит инвалидировать, если он меняется
	}


	// 4. Если всё успешно — создаем сессию
	acc.session.sessionToken = GenerateRandomToken();
	acc.profile.name = name; // Сохраняем имя из запроса

	// Обновляем состояние сервера
	entities[pid] = acc;
	entities[pid].isOnline = true;
	tokenToSession[acc.session.sessionToken] = pid;
	connToPlayer[connId] = pid;
	playerToConn[pid] = connId;
	idToConnObj[connId] = client; // Не забудьте сохранить объект соединения!

	// 5. Отправляем финальный успех
	res.success = true;
	res.assignedID = pid;
	res.assignedToken = acc.session.sessionToken;
	safe_copy(res.reason, "Welcome back!", sizeof(res.reason));



	olc::net::message<GameMsg> reply;
	reply.header.id = GameMsg::Server_LoginResult;
	reply << res;
	MessageClient(client, reply);





	// Собираем данные профиля вручную в сообщение
	olc::net::message<GameMsg> syncMsg;
	syncMsg.header.id = GameMsg::Server_SyncPlayerProfile;

	//auto& sProf = allServerPlayers[p.netId].profile;

	// 1. Записываем имя (будет в самом низу)
	syncMsg << acc.profile.name;

	// 2. Записываем базовые числа
	syncMsg << acc.profile.coins << acc.profile.wins << acc.profile.losses;
	
	// 3. Записываем список Unlocked
	for (auto& id : acc.profile.unlocked) syncMsg << id;
	syncMsg << (uint32_t)acc.profile.unlocked.size(); // Размер идет ПОСЛЕ элементов (чтобы считаться ПЕРВЫМ)

	// 4. Записываем список Friends
	for (auto& id : acc.profile.friends) syncMsg << id;
	syncMsg << (uint32_t)acc.profile.friends.size(); // Последнее, что записали — размер друзей

	MessageClient(client, syncMsg); //по постоянному id находим id временного соединения с сокетом








}

void GameServer::HandleRegister(std::shared_ptr<olc::net::connection<GameMsg>> client, olc::net::message<GameMsg>& msg, uint32_t& connId)
{
	sLoginRequest req;
	msg >> req;

	std::string name = req.firstName;
	std::string pass = req.password;

	sLoginResult res;
	res.success = false;

	std::cout << req.firstName << "  " << req.password << "    " << req.sessionToken;
	// 1. Проверка на занятость имени
	if (GetPlayerIdByName(name) != 0) {
		// Можно добавить код ошибки: "Имя уже занято"
	}
	else {
		// 2. Только запись в БД
		RegisterPlayerInDB(name, pass);
		res.success = true;
		std::cout << "[DB] New user registered: " << name << "\n";
	}

	olc::net::message<GameMsg> reply;
	reply.header.id = GameMsg::Server_RegisterResult; // Используем тот же тип или новый (на ваш вкус)
	reply << res;
	MessageClient(client, reply);
}

bool GameServer::InitDatabase()
{
	// 1. Открываем соединение с файлом
	int rc = sqlite3_open("server.db", &db);
	if (rc != SQLITE_OK) {
		std::cerr << "DB Error: " << sqlite3_errmsg(db) << std::endl;
		return false;
	}

	// 2. SQL-запрос для создания таблиц под твой NetPlayerProfile
	// Мы создаем 3 таблицы, чтобы хранить и числа, и списки (векторы)
	const char* sql =
		// Основная таблица (простые поля)
		"CREATE TABLE IF NOT EXISTS players ("
		"id INTEGER PRIMARY KEY AUTOINCREMENT,"
		"name TEXT UNIQUE NOT NULL,"
		"password TEXT NOT NULL,"
		"coins INTEGER DEFAULT 0,"
		"wins INTEGER DEFAULT 0,"
		"losses INTEGER DEFAULT 0"
		");"

		// Таблица для вектора unlocked (предметы)
		"CREATE TABLE IF NOT EXISTS unlocked_items ("
		"player_id INTEGER,"
		"item_id INTEGER,"
		"FOREIGN KEY(player_id) REFERENCES players(id)"
		");"

		// Таблица для вектора friends (друзья)
		"CREATE TABLE IF NOT EXISTS friends ("
		"player_id INTEGER,"
		"friend_id INTEGER,"
		"FOREIGN KEY(player_id) REFERENCES players(id)"
		");";

	char* errMsg = nullptr;
	rc = sqlite3_exec(db, sql, nullptr, nullptr, &errMsg);

	if (rc != SQLITE_OK) {
		std::cerr << "SQL Error: " << errMsg << std::endl;
		sqlite3_free(errMsg);
		return false;
	}

	std::cout << "[Database] All tables initialized successfully!" << std::endl;
	return true;
}

PlayerID GameServer::GetPlayerIdByName(std::string name)
{
	sqlite3_stmt* stmt;
	const char* sql = "SELECT id FROM players WHERE name = ?;";
	PlayerID foundID = 0;

	if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
		sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);

		if (sqlite3_step(stmt) == SQLITE_ROW) {
			// Если нашли строку — забираем ID из первой колонки (индекс 0)
			foundID = (PlayerID)sqlite3_column_int(stmt, 0);
		}
	}
	sqlite3_finalize(stmt);
	return foundID;
}

PlayerAccount GameServer::LoadPlayerFromDB(PlayerID pid)
{
	PlayerAccount acc;
	sqlite3_stmt* stmt;

	// --- ШАГ 1: Загружаем основные статы ---
	const char* sqlMain = "SELECT name, wins, losses, coins,password FROM players WHERE id = ?;";
	if (sqlite3_prepare_v2(db, sqlMain, -1, &stmt, nullptr) == SQLITE_OK) {
		sqlite3_bind_int(stmt, 1, (int)pid);
		if (sqlite3_step(stmt) == SQLITE_ROW) {
			acc.profile.id = pid;
			acc.profile.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
			acc.profile.wins = sqlite3_column_int(stmt, 1);
			acc.profile.losses = sqlite3_column_int(stmt, 2);
			acc.profile.coins = sqlite3_column_int(stmt, 3);
			acc.profile.password = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
		}
		sqlite3_finalize(stmt);
	}

	// --- ШАГ 2: Загружаем список разблокированных предметов (вектор unlocked) ---
	const char* sqlItems = "SELECT item_id FROM unlocked_items WHERE player_id = ?;";
	if (sqlite3_prepare_v2(db, sqlItems, -1, &stmt, nullptr) == SQLITE_OK) {
		sqlite3_bind_int(stmt, 1, (int)pid);
		while (sqlite3_step(stmt) == SQLITE_ROW) {
			acc.profile.unlocked.push_back(sqlite3_column_int(stmt, 0));
		}
		sqlite3_finalize(stmt);
	}

	// --- ШАГ 3: Загружаем список друзей (вектор friends) ---
	const char* sqlFriends = "SELECT friend_id FROM friends WHERE player_id = ?;";
	if (sqlite3_prepare_v2(db, sqlFriends, -1, &stmt, nullptr) == SQLITE_OK) {
		sqlite3_bind_int(stmt, 1, (int)pid);
		while (sqlite3_step(stmt) == SQLITE_ROW) {
			acc.profile.friends.push_back(sqlite3_column_int(stmt, 0));
		}
		sqlite3_finalize(stmt);
	}

	return acc;
}

PlayerID GameServer::RegisterPlayerInDB(std::string name, std::string password)
{
	sqlite3_stmt* stmt;
	const char* sql = "INSERT INTO players (name, password) VALUES (?, ?);";
	PlayerID newID = 0;

	if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
		sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
		sqlite3_bind_text(stmt, 2, password.c_str(), -1, SQLITE_TRANSIENT);

		if (sqlite3_step(stmt) == SQLITE_DONE) {
			newID = (PlayerID)sqlite3_last_insert_rowid(db);
		}
		sqlite3_finalize(stmt);
	}
	return newID;
}

void GameServer::SavePlayerToDB(const PlayerAccount& acc)
{
	// Открываем транзакцию — это ОЧЕНЬ важно для скорости
	sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);

	sqlite3_stmt* stmt;

	// 1. Обновляем основные данные (включая пароль)
	const char* sqlMain = "UPDATE players SET coins = ?, wins = ?, losses = ?, password = ? WHERE id = ?;";
	if (sqlite3_prepare_v2(db, sqlMain, -1, &stmt, nullptr) == SQLITE_OK) {
		sqlite3_bind_int(stmt, 1, acc.profile.coins);
		sqlite3_bind_int(stmt, 2, acc.profile.wins);
		sqlite3_bind_int(stmt, 3, acc.profile.losses);
		sqlite3_bind_text(stmt, 4, acc.profile.password.c_str(), -1, SQLITE_TRANSIENT);
		sqlite3_bind_int(stmt, 5, acc.profile.id);
		sqlite3_step(stmt);
		sqlite3_finalize(stmt);
	}

	// Вспомогательная лямбда для сохранения векторов (items и friends)
	auto saveVector = [&](const std::string& table, const std::string& column, const std::vector<uint32_t>& vec) {
		// Удаляем старое
		std::string delSql = "DELETE FROM " + table + " WHERE player_id = " + std::to_string(acc.profile.id) + ";";
		sqlite3_exec(db, delSql.c_str(), nullptr, nullptr, nullptr);

		// Готовим запрос ОДИН раз
		std::string insSql = "INSERT INTO " + table + " (player_id, " + column + ") VALUES (?, ?);";
		if (sqlite3_prepare_v2(db, insSql.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
			for (uint32_t val : vec) {
				sqlite3_bind_int(stmt, 1, acc.profile.id);
				sqlite3_bind_int(stmt, 2, val);
				sqlite3_step(stmt);
				sqlite3_reset(stmt); // Очищаем стейтмент для следующей итерации
			}
			sqlite3_finalize(stmt);
		}
		};

	// 2. Сохраняем предметы
	saveVector("unlocked_items", "item_id", acc.profile.unlocked);

	// 3. Сохраняем друзей
	saveVector("friends", "friend_id", acc.profile.friends);

	// Закрываем транзакцию (сохраняем всё на диск)
	sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr);
}

std::shared_ptr<olc::net::connection<GameMsg>> GameServer::GetConnectionByPlayer(PlayerID pid)
{
	// 1. Находим ConnID по PlayerID
	auto itID = playerToConn.find(pid);
	if (itID == playerToConn.end()) return nullptr;

	// 2. Находим объект сокета по ConnID
	auto itConn = idToConnObj.find(itID->second);
	if (itConn == idToConnObj.end()) return nullptr;

	return itConn->second;
}
