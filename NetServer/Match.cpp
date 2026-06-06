#include "Match.h"
#include "game/utils/WorldGrid.h"
#include <iostream>
#include "GameServer.h" 
#define GLM_ENABLE_EXPERIMENTAL  // Обязательно ПЕРЕД всеми инклудами GLM
#include <glm/gtx/norm.hpp>
#include "netConverters.h"
#include "../NetShared/PlayerProfile.h"
#include "game/spells/SpellFactory.h"
#include "../NetShared/managers/EffectManager.h"
#include "../NetShared/managers/CooldownManager.h"
#include "../NetShared/managers/SquadManager.h"
#include "../NetShared/managers/ProgressionManager.h"

#include "DamageContext.h"



const float DAY_CYCLE_DURATION = 100.0f; // 10 минут на полный цикл
static float spawnerTick = 0.0f;
Match::Match(uint32_t id, GameServer* server, int w, int h, uint32_t seed): matchID(id), server(server),level(w, h, seed,this) // Создаем level сразу через новый конструктор с шумом
{
	CalculateValidSpawns();

	// Инициализируем очередь свободных ID всеми доступными индексами
	for (uint16_t i = 0; i < MAX_PROJECTILES; ++i) {
		freeProjectileIds.push(i);
		projectiles[i].active = false; // На всякий случай сбрасываем флаг
	}

	level.OnTileChanged =
		[this](const glm::ivec2& cell, char value,float respTimer)
		{
			uint16_t index = cell.y * level.levelWidth + cell.x;
			TileUpdate tile = { index,value, respTimer };

			pendingTileUpdates.push_back(tile);  //записываем в буфер который в конце апдейта отправится клиенту и очиститься 
		};

	level.OnTileMove =
		[this](const glm::ivec2& from,
			const glm::ivec2& to,
			float duration)
		{
			SendTileMove(from, to, duration);
		};

	level.OnBeaconOwnerChanged = [this](const glm::ivec2& cell, uint32_t ownerId)
		{
			olc::net::message<GameMsg> msg;
			msg.header.id = GameMsg::Server_BeaconOwnerUpdate;

			uint16_t index = (uint16_t)(cell.y * level.levelWidth + cell.x);
			msg << index << ownerId; // Кто и какой маяк захватил

			MessageAllMatchClients(msg);
		};


}
void Match::Update(float dt)
{
	if (state != MatchState::INGAME && state != MatchState::FINISHED) return;
	matchTime += dt;


	// 1.день/ночь
	 dayProgress = fmodf(matchTime, DAY_CYCLE_DURATION) / DAY_CYCLE_DURATION;

	// 2. Считаем интенсивность для логики воина (на сервере)
	 lightIntensity = (cosf(dayProgress * 2.0f * 3.14159f + 3.14159f) + 1.0f) * 0.5f;

	// 1. Конец матча
	 if (state == MatchState::FINISHED) {
		 if (!bStatsProcessed) {
			 // Прямой доступ к public полю сервера
			 auto& allServerPlayers = server->entities; // берём всех онлайн игроков сервера 
			
			 for (auto& p : entities) { // итерируемся по локальному вектору игроков матча
				 if (allServerPlayers.count(p.netId)) {

					 if (p.netId == winnerID && winnerID != 0) {
						 allServerPlayers[p.netId].profile.wins++;
						 allServerPlayers[p.netId].profile.coins += 50;
						 std::cout << "[Match] Player " << p.netId << " WON. Stats saved.\n";
					 }
					 else {
						 allServerPlayers[p.netId].profile.losses++;

						 std::cout << "[Match] Player " << p.netId << " LOST. Stats saved.\n";
					 }

					 // Сохраняем в БД через сервер
					 server->SavePlayerToDB(allServerPlayers[p.netId]);


					 // Собираем данные профиля вручную в сообщение
					 olc::net::message<GameMsg> syncMsg;
					 syncMsg.header.id = GameMsg::Server_SyncPlayerProfile;

					 auto& sProf = allServerPlayers[p.netId].profile;

					 // 1. Записываем имя (будет в самом низу)
					 syncMsg << sProf.name;

					 // 2. Записываем базовые числа
					 syncMsg << sProf.coins << sProf.wins << sProf.losses;

					 // 3. Записываем список Unlocked
					 for (auto& id : sProf.unlocked) syncMsg << id;
					 syncMsg << (uint32_t)sProf.unlocked.size(); // Размер идет ПОСЛЕ элементов (чтобы считаться ПЕРВЫМ)

					 // 4. Записываем список Friends
					 for (auto& id : sProf.friends) syncMsg << id;
					 syncMsg << (uint32_t)sProf.friends.size(); // Последнее, что записали — размер друзей

					 server->MessageClient(server->GetConnectionByPlayer(p.netId), syncMsg); //по постоянному id находим id временного соединения с сокетом
				 }
			 }
			 bStatsProcessed = true;
		 }

		 endMatchTimer -= dt;
		 if (endMatchTimer <= 0.0f) {

			 // ТАЙМЕР ВЫШЕЛ: Рассылаем финальное сообщение
			 sMatchEnd data;
			 data.winnerID = this->winnerID;
			 data.matchDuration = 3.0f;



			 olc::net::message<GameMsg> msg;
			 msg.header.id = GameMsg::Server_MatchEnded;
			 msg << data;

			 MessageAllMatchClients(msg);

			 // Рассылка Server_MatchEnded и установка флага для удаления матча
		 }
		 //return; // Пропускаем остальную логику апдейта
	 }



		// 2. Движение блоков (оптимизировано)
		//tileMoveTimer += dt;
		//if (tileMoveTimer >= 1.0f) { // Можно чаще, так как теперь это влияет на геймплей
		//tileMoveTimer = 0.0f;

		//// Собираем позиции живых игроков
		//std::vector<glm::vec2> activePlayerPos;
		//for (const auto& p : entities) {
		//	if (p.active && p.character != nullptr) {
		//		if (!p.character->IsDead()) {
		//			activePlayerPos.push_back(p.character->position);
		//		}
		//	}
		//}

		//// Двигаем тайлы только рядом с ними
		//level.TryMoveTilesNearPlayers(activePlayerPos, 5);
	//}

		windChangeTimer -= dt;
		if (windChangeTimer <= 0.0f) {
			// Меняем ветер раз в 10 секунд
			windAngle = (float)(rand() % 360) * 3.14159f / 180.0f;
			windStrength = (uint8_t)(rand() % 3) + 1;
			windChangeTimer = 10.0f;

			// Рассылаем клиентам
			sEnvironmentData data;
			data.windAngle = windAngle;
			data.windForce = windStrength;
			data.dayProgress = dayProgress;
			olc::net::message<GameMsg> msg;
			msg.header.id = GameMsg::Game_EnvironmentUpdate;
			msg << data;
			MessageAllMatchClients(msg);

		}
			// 1️⃣ ЛОГИКА
			level.Update(dt);
			spawnerTick -= dt;
			if (spawnerTick <= 0) {
				UpdateSpawners(0.5f); // Проверяем спавн всего 2 раза в секунду
				spawnerTick = 0.5f;
			}
			spellManager.Update(dt, this);
			


			// 2️⃣ ОЧИСТКА GRID
			ClearSpatialGrid();

			// 2. Заполняем игроками (используем индекс массива)
		   // Итерируемся по всему вектору players. Индекс i является индексом слота.
			for (int i = 0; i < entities.size(); ++i) {
				
				if (!entities[i].active || entities[i].character == nullptr|| entities[i].character->IsDead()) {
					continue;
				}

				int cellIdx = GridIndex(WorldToGridCell(entities[i].character->position));   // индекс клети y * width + x - на которой стоит персонаж
				// Проверяем границы сетки
				if (cellIdx >= 0 && cellIdx < spatialGrid.size()) {
					// В сетку кладем индекс слота (i)! 
					// Это важно, потому что в логике коллизий вы будете использовать этот индекс 
					// для доступа к элементу players[cid].
					entities[i].nextInCell = spatialGrid[cellIdx].firstCharacter; // тут мы присваиваем последнего кто в клетке к персонажу
					spatialGrid[cellIdx].firstCharacter = i; // и сразу того к кому привязали привязываем к клетке
				}
			}
			// 2. Расталкивание (Separation) — итерируем по ячейкам, а не по персонажам
			for (int y = 0; y < gridHeight; ++y) {
				for (int x = 0; x < gridWidth; ++x) {
					int currentCellIdx = y * gridWidth + x;

					// Проверяем текущую ячейку (взаимодействие внутри ячейки)
					// И соседние ячейки (только "вперед", чтобы не проверять пары дважды)
					// Соседи для исключения дублей: (0,0), (1,0), (1,1), (0,1), (-1,1)
					for (auto& n : forwardNeighbors) {
						int neighborIdx = GridIndex(glm::ivec2(x, y) + n);
						if (neighborIdx < 0) continue;

						// Итерируем персонажей в текущей ячейке и соседней
						for (int cidA = spatialGrid[currentCellIdx].firstCharacter; cidA != -1; cidA = entities[cidA].nextInCell) {
							for (int cidB = spatialGrid[neighborIdx].firstCharacter; cidB != -1; cidB = entities[cidB].nextInCell) {
								if (cidA == cidB) continue; // Пропуск самого себя

								auto& A = entities[cidA].character;
								auto& B = entities[cidB].character;


								if (!A || A->IsDead() || !B || B->IsDead()) continue;
								//if (A->entityType != B->entityType) continue;

								glm::vec2 diff = B->position - A->position;
								float distSq = glm::dot(diff, diff);
								float minDist = A->radius + B->radius;

								if (distSq < minDist * minDist) {
									float dist = std::sqrt(distSq);
									// Если координаты совпали идеально, выбираем случайное направление
									glm::vec2 dir = (dist < 0.0001f) ? glm::vec2(1.0f, 0.0f) : diff / dist;
									float overlap = minDist - dist;

									// Расталкиваем обоих на половину глубины проникновения
									A->position -= dir * (overlap * 0.5f);
									B->position += dir * (overlap * 0.5f);
								}
							}
						}
					}
				}
			}
			for (uint16_t pid : activeProjectileIndices) {
				auto& projSlot = projectiles[pid];
				if (!projSlot.active || projSlot.data.bPendingDestroy || !projSlot.collisionEnabled) continue;

				int cellIdx = GridIndex(WorldToGridCell(projSlot.data.vPos));
				if (cellIdx >= 0 && cellIdx < spatialGrid.size()) {
					projSlot.nextInCell = spatialGrid[cellIdx].firstProjectile;
					spatialGrid[cellIdx].firstProjectile = (int)pid;
				}
			}


			// 4️⃣ КОЛЛИЗИИ (Итерация по объектам)
			for (uint16_t pid : activeProjectileIndices)
			{
				auto& slot = projectiles[pid];
				if (!slot.active || slot.data.bPendingDestroy ||!slot.collisionEnabled) continue;

				auto& proj = slot.data;
				const auto& projRules = slot.cachedRules;
				float projR = proj.fRadius;

				bool canHitChar = projRules.dealsDamage || projRules.blocksCharacters;
				bool canHitProj = projRules.dealsDamage;
				if (!canHitChar && !canHitProj) continue;

				auto baseCell = WorldToGridCell(proj.vPos);

				for (auto& n : neighbors)
				{
					int nIdx = GridIndex(baseCell + n);
					if (nIdx < 0) continue;

					auto& cell = spatialGrid[nIdx];

					// ============================================
					// 🧍 PROJECTILE vs CHARACTER (Связанный список)
					// ============================================
					if (canHitChar) {
						// Начинаем с первого игрока в ячейке и идем по цепочке nextInCell
						for (int cid = cell.firstCharacter; cid != -1; cid = entities[cid].nextInCell)
						{
							if (proj.nOwnerID == entities[cid].netId) continue;

							auto& pl = entities[cid];
							// pl.active проверять не обязательно, так как в сетку попали только активные,
							// но проверка на смерть персонажа может быть нужна
							if (pl.character == nullptr || pl.character->IsDead()) continue;

							auto& ch = pl.character;
							glm::vec2 diff = ch->position - proj.vPos;
							float distSq = glm::dot(diff, diff);
							float minDist = projR + ch->radius;

							if (distSq < minDist * minDist + 0.12f) {
								if (projRules.blocksCharacters) {
									float dist = std::sqrt(distSq);
									glm::vec2 dir = (dist < 0.001f) ? glm::vec2(1, 0) : diff / dist;
									ch->position += dir * (minDist - dist);
								}
								if (projRules.dealsDamage) {
									OnProjectileHit(pid, (uint32_t)cid, projRules);
									if (proj.bPendingDestroy) break;
								}
							}
						}
					}
					if (proj.bPendingDestroy) break;

					// ============================================
					// 💥 PROJECTILE vs PROJECTILE (Связанный список)
					// ============================================
					if (canHitProj) {
						// Начинаем с первого снаряда в ячейке и идем по цепочке nextInCell
						for (int otherPid = cell.firstProjectile; otherPid != -1; otherPid = projectiles[otherPid].nextInCell)
						{
							// Самопроверка и предотвращение двойных расчетов
							if (pid == (uint16_t)otherPid) continue; // Не сталкиваемся сами с собой

							auto& otherSlot = projectiles[otherPid];
							if (!otherSlot.active || otherSlot.data.bPendingDestroy) continue;

							// Используем кэшированные правила другой сущности
							const auto& otherRules = otherSlot.cachedRules;

							// ЛОГИКА: Наш снаряд (pid) дамажит, а другой снаряд (otherPid) является твердым
							// Или если оба дамажат и должны взорваться при встрече
							if (!otherRules.blocksCharacters) continue;

							auto& otherProj = otherSlot.data;
							glm::vec2 diff = otherProj.vPos - proj.vPos;
							float distSq = glm::dot(diff, diff);
							float minDist = projR + otherProj.fRadius;

							if (distSq < minDist * minDist) {
								// Вызываем обработку блокировки
								OnProjectileBlocked(pid, (uint16_t)otherPid, projRules);

								// Если наш снаряд должен погибнуть при столкновении
								if (proj.bPendingDestroy) break;
							}
						}
					}
					
				}
			}


			// 3️⃣ РЕЗОЛВ КОЛЛИЗИЙ И ВЗАИМОДЕЙСТВИЙ (Используем свежую сетку)
			HandleTileInteractions(dt); // Теперь плитки видят игроков там, где они стоят СЕЙЧАС
			UpdateProjectiles(dt);      // Снаряды летят и проверяют попадания по сетке

			// 4️⃣ ФИНАЛИЗАЦИЯ ДВИЖЕНИЯ
			UpdateCharacters(dt, lightIntensity);

		
			BroadcastSnapshot();



			// РАБОТАЕТ МУСОРЩИК
			for (auto id : garbageProjectiles) {
				// 1. Сообщение клиентам об удалении
				olc::net::message<GameMsg> msgRemove;
				msgRemove.header.id = GameMsg::Game_RemoveProjectile;
				msgRemove << id;
				
				MessageAllMatchClients(msgRemove);

				// 2. Полная очистка слота
				auto& slot = projectiles[id];
				slot.active = false;
				slot.collisionEnabled = false;
				slot.data = {}; // Сбрасываем все данные, включая bPendingDestroy

				// 3. Возвращаем индекс в очередь для новых выстрелов
				freeProjectileIds.push((uint16_t)id);

				// 4. Удаляем из списка активных для итерации в Update
				auto it = std::find(activeProjectileIndices.begin(), activeProjectileIndices.end(), id);
				if (it != activeProjectileIndices.end()) {
					// Быстрое удаление (swap с хвостом)
					*it = activeProjectileIndices.back();
					activeProjectileIndices.pop_back();
				}
			}
			garbageProjectiles.clear(); // Очищаем сет для следующего кадра

			for (uint32_t netId : garbageEntities) {
				auto it = idToIndex.find(netId);
				if (it == idToIndex.end()) continue;

				uint32_t idx = it->second;

				// 1. Уведомляем клиентов
				olc::net::message<GameMsg> msg;
				msg.header.id = GameMsg::Game_RemovePlayer; // или Game_RemoveEntity
				msg << netId;
				MessageAllMatchClients(msg);

				// 2. Очищаем данные (важно занулить всё, чтобы SpawnAI нашел этот слот)
				entities[idx].active = false;
				entities[idx].netId = 0;
				entities[idx].character.reset(); // Уничтожаем объект Character

				// 3. Удаляем из мапы поиска по ID
				idToIndex.erase(it);
			}
			garbageEntities.clear();



			FlushTileUpdates();

			// --- ОТПРАВКА УРОНА КЛИЕНТАМ ---
			if (!damageQueue.empty()) {
				olc::net::message<GameMsg> msgDmg;
				msgDmg.header.id = GameMsg::Game_DamageEvent;

				// Сначала пишем количество отчетов
				uint16_t count = (uint16_t)damageQueue.size();

				// В olc::net данные заталкиваются в буфер задом наперед (LIFO), 
				// поэтому при чтении на клиенте порядок будет важен.
				for (auto& report : damageQueue) {
					msgDmg << report;
				}
				msgDmg << count;

				MessageAllMatchClients(msgDmg);
				damageQueue.clear(); // Обязательно чистим!
			}

}
uint32_t Match::SpawnAI(ArchetypeId type, glm::vec2 pos, int home)
{

	// Сдвигаем позицию на ближайшую свободную клетку
	// 1. Находим чистый центр клетки (например, 5.5, 10.5)
	glm::vec2 emptyCenter = FindNearestEmptySpace(level, pos);

	// 2. Добавляем небольшой разброс (от -0.25 до +0.25)
	// Это удержит моба внутри найденной клетки '.'
	float offsetX = ((rand() % 100) / 100.0f - 0.5f) * 0.5f;
	float offsetY = ((rand() % 100) / 100.0f - 0.5f) * 0.5f;

	glm::vec2 spawnPos = emptyCenter + glm::vec2(offsetX, offsetY);

	// 1. Генерируем ID через сервер (или свой внутренний счетчик матча) пока что костыль 
	uint32_t mobNetId = server->GeneratePlayerID();

	// 2. Создаем структуру Player (в твоем Match это обертка над Character)
	Player mobActor;
	mobActor.netId = mobNetId;
	mobActor.active = true;
	// Для моба нет сессии и состояния "готовности" как у игрока
	mobActor.state = PlayerState::INGAME;
	
	
	// 3. Создаем самого персонажа
	auto zombie = CharacterFactory::CreateAI(type,mobNetId, this, spawnPos,home);
	mobActor.character = std::move(zombie);

	// 4. Ищем свободный слот в векторе (чтобы не раздувать массив)
	uint32_t targetIdx = 0xFFFF;
	for (uint32_t i = 0; i < (uint32_t)entities.size(); ++i) {
		if (!entities[i].active) {
			entities[i] = std::move(mobActor);
			targetIdx = i;
			break;
		}
	}
	
	// Если свободных слотов нет — расширяем
	if (targetIdx == 0xFFFF) {
		entities.push_back(std::move(mobActor));
		targetIdx = (uint32_t)entities.size() - 1;
	}

	// 5. Регистрируем в мапе поиска (чтобы ApplyDamage и прочее работало)
	idToIndex[mobNetId] = (uint8_t)targetIdx;

	//// 6. Оповещаем SpatialGrid (чтобы моба можно было найти через FindNearestEnemy)
	//glm::ivec2 cell = WorldToGridCell(pos);
	//int gridIdx = GridIndex(cell);
	//if (gridIdx != -1) {
	//	players[targetIdx].nextInCell = spatialGrid[gridIdx].firstCharacter;
	//	spatialGrid[gridIdx].firstCharacter = targetIdx;
	//}

	return mobNetId;
	
}
uint32_t Match::SpawnCompanion(ArchetypeId type, glm::vec2 pos, Character* owner)
{

	uint32_t netId = server->GeneratePlayerID();

	Player compActor;
	compActor.netId = netId;
	compActor.active = true;
	compActor.state = PlayerState::INGAME;

	// Создаем именно компаньона (нужно добавить логику в Factory)
	auto companion = CharacterFactory::CreateCompanion(type, netId, owner, this);
	companion->position = GetSpawnPoint(netId); // Спавним рядом с хозяином

	compActor.character = std::move(companion);

	//даём нашей тиме позиции 
	
	//uint8_t currentIdx = (uint8_t)(owner->companionSquad.size() % 8);
//	owner->companionSquad[netId] = currentIdx;
	//owner->AddCompanion(compActor.character.get());
	owner->GetSquad()->AddCompanion(compActor.character.get());


	// Помещаем в общий список сущностей (entities)
	uint32_t targetIdx = 0xFFFF;
	for (uint32_t i = 0; i < (uint32_t)entities.size(); ++i) {
		if (!entities[i].active) {
			entities[i] = std::move(compActor);
			targetIdx = i;
			break;
		}
	}
	if (targetIdx == 0xFFFF) {
		entities.push_back(std::move(compActor));
		targetIdx = (uint32_t)entities.size() - 1;
	}

	idToIndex[netId] = (uint8_t)targetIdx;
	return netId;
}
void Match::TrySpawnCompanionForPlayer(uint32_t ownerId, glm::vec2 position)
{
	uint32_t idx = GetPlayerIndex(ownerId);
	Character* owner = GetCharacterByIdx(idx);


	if (owner && !owner->IsDead()) {
		auto squad = owner->GetSquad();

		if (squad->CanAddMore()) {
			// Определяем тип компаньона в зависимости от уровня или рандомно
			ArchetypeId companionType = ArchetypeId::Companion_Warrior;
			if (owner->GetProgression()->GetLevel() >= 5) {
				// На высоких уровнях алтарь может дать мага
				companionType = (rand() % 2 == 0) ? ArchetypeId::Companion_Hunter : ArchetypeId::Companion_Priest;
			}

			SpawnCompanion(companionType, position, owner);

			// Можно отправить сообщение игроку "Новый спутник присоединился!"
		}
		else {
			// Если слотов нет, можно дать бонусный опыт вместо спутника
			owner->GetProgression()->AddExperience(50.0f);
			// И отправить уведомление: "Ваш отряд полон! Получен бонусный опыт."
		}
	}
}
void Match::OnPlayerMessage(uint32_t player, olc::net::message<GameMsg>& msg)
{
	
	if (idToIndex.find(player) == idToIndex.end()) return;

	uint8_t slotIndex = idToIndex[player]; // Получаем индекс в векторе players
	auto& pl = entities[slotIndex];

	// Создаем копию сообщения для чтения (т.к. msg const&)
	olc::net::message<GameMsg> msgCopy = msg;

	switch (msg.header.id)
	{
	case GameMsg::Game_UpdatePlayer:
	{
		glm::vec2 inputDir, lookDir;
		msgCopy >> inputDir; // Если слали (input, look), то читаем (look, input)
		msgCopy >> lookDir;  // Библиотека olc::net читает с конца! 

		if (glm::length(inputDir) > 1.0f) inputDir = glm::normalize(inputDir);

		pl.character->inputVel = inputDir;
		pl.character->direction = lookDir;
	
		break;


	}case(GameMsg::Game_CastSpell):
	{

		//Player& pl = players[idToSlot[pid]];
		//if (!pl.active || pl.character->IsDead()) break;

		//ActionSlot slot;
		//msg >> slot;
		//// Сохраняем слот в персонажа
		//pl.character->SetLastCastSlot(slot); // для охотника, что бы знать левая или праввая стрела 

		//SpellId spell = GetSpellForSlot(pl.character->GetClass(), slot,*pl.character);
		//if (spell == SpellId::None) break;

		//HandleCastSpell(player, spell, 0);
		//break;
		
		if (!pl.active ||pl.character == nullptr || pl.character->IsDead()) break;
		ActionSlot slot;
		msg >> slot;
		
		pl.character->SetLastCastSlot(slot); // для охотника, что бы знать левая или праввая стрела 
		pl.character->StartCharging(slot); // Включаем таймер в Character
		


		// Сразу создаем спелл. Он войдет в состояние Appear и будет ждать Release.
		SpellId spellId = pl.character->GetBoundSpell(slot);
		HandleCastSpell(player, spellId, 0);
		break;
	}
	case(GameMsg::Game_ReleaseSpell):
	{
		if (!pl.active  ||pl.character == nullptr || pl.character->IsDead() || !pl.character->IsCharging()) break;
		// Просто выключаем зарядку. 
		// Спелл в своем UpdateAppear увидит, что зарядка окончена, и полетит.
		pl.character->StopCharging();
		break;
	}
	case(GameMsg::chat_message): {

		uint32_t pid;
		std::string text;

		msg >> pid;  // Читаем ID (так как он был последним)
		msg >> text; // Читаем текст

		olc::net::message<GameMsg> out;
		out.header.id = GameMsg::chat_message;
		out << text;
		out << pid; // Снова записываем так: ID будет последним
		MessageAllMatchClients(out);
		break;
	}
	case GameMsg::Client_SelectClass:
	{



		PlayerClass cls;
		msg >> cls;

		//auto& pl = players[idToSlot[pid]];
		pl.session.selectedclass = cls;
		pl.session.ready = true;

		CheckAllReady();
		break;
	}

	
case GameMsg::Game_TriggerTeleport:
	{
		auto& sm = spellManager;
		// Ищем активный телепорт этого игрока
		if (auto* spell = sm.FindActive(SpellId::Teleport, player))
		{
			// Вызываем Cast (для постановки 2-го портала) 
			// или ForceFinish (для закрытия), в зависимости от вашей логики.
			spell->Cast(*entities[idToIndex[player]].character, this);
		}
		break;
	}
	case(GameMsg::Game_CraftArrow):
	{
		ArrowType type;
		msg >> type;

		//Player& pl = players[idToSlot[pid]];
		if (pl.character->GetArchetypeId() == ArchetypeId::Player_Hunter) {
			
			pl.character->CraftArrow(type); // Вызываем вашу функцию крафта
		}
		break;
	}
	}

}
void Match::HandlePlayerTileChange(uint32_t playerId, glm::ivec2 newPos, glm::ivec2 oldPos)
{
	// 1. Проверяем, что координаты в границах уровня
	if (!level.IsValid(newPos)) return;
	// 2. Получаем тайл, на который наступил игрок
	Tile& t = level.GetTile(newPos);

	// 1. УДАЛЯЕМ старые эффекты клетки (те, из которых вышли)
	Tile& oldTile = level.GetTile(oldPos);
	auto* character = GetEntities()[GetPlayerIndex(playerId)].character.get();
	auto* effectMgr = character->GetEffects();
	if (!effectMgr) return;

	if (oldTile.type == TileType::Fire || oldTile.type == TileType::Sand) {

		

		//character->RemoveEffect(StatusEffectType::Burn); // Нужно реализовать этот метод
		//character->RemoveEffect(StatusEffectType::Slow);
		effectMgr->Remove(StatusEffectType::Burn);
		effectMgr->Remove(StatusEffectType::Slow);

	}
	


	// 3. Логика эффектов
	if (t.type == TileType::Sand) {
		// Замедляем игрока
	
		ProjectileRules rules = GetProjectileRules(ProjectileType::IceRoots);
		StatusEffect effect;
		effect.type = rules.effectToApply;
		effect.timeLeft = 999;
		effect.value = 0.5f;
		effect.nOwnerNetID =-1;

		if (playerId != -1)
			effectMgr->Add(effect);
		//	effectMgr->AddStatusEffect(effect);
			
	}
	if (t.type == TileType::Fire) {
		
		StatusEffect effect;
		effect.type = StatusEffectType::Burn;
		effect.timeLeft = 999;
		effect.value = 1.5f;
		effect.nOwnerNetID = -1;

		if (playerId != -1)
			effectMgr->Add(effect);
	}
	//else if (t.type == TileType::Healer && !t.destroyed) {
	//	// Лечим, если блок еще цел
	//}
}
void Match::AddPlayer(uint32_t id)
{
	
	 uint8_t index = static_cast<uint8_t>(entities.size());

    Player pl{};
    pl.netId = id; // эта id матча в сервере 
    pl.active = true;
    pl.state = PlayerState::ROOM; // пока в комнате

	
    entities.push_back(std::move(pl));
    idToIndex[id] = index;
	realPlayerIds.push_back(id);
	if (entities.size() == 3) {
		
        StartRoom();
	}


}
void Match::RemovePlayer(uint32_t id) // сюда приходит вечный ip
{
	if (idToIndex.find(id) == idToIndex.end()) return;

	uint8_t indexToRemove = idToIndex[id]; // берём индекс для players внутри матча

	// Эффективное удаление из вектора (Swap with last)
	if (indexToRemove < entities.size() - 1) {
		Player& lastPlayer = entities.back();
		entities[indexToRemove] = std::move(lastPlayer);
		idToIndex[entities[indexToRemove].netId] = indexToRemove;
	}

	entities.pop_back();
	idToIndex.erase(id);
	// 1. Удаляем из списка сетевой рассылки
	realPlayerIds.erase(
		std::remove(realPlayerIds.begin(), realPlayerIds.end(), id),
		realPlayerIds.end()
	);
	// Рассылаем всем в ЭТОМ матче, что игрок ушел
	olc::net::message<GameMsg> msg;
	msg.header.id = GameMsg::Game_RemovePlayer;
	msg << id;

	// Используем серверный метод рассылки именно по участникам этого матча
	server->SendToMatch(*this, GameMsg::Game_RemovePlayer, id);

	// Если игра шла — проверяем, не остался ли один победитель
	if (state == MatchState::INGAME) {
		CheckVictoryCondition();
	}
	OnCharacterDied(id);
}
bool Match::IsReadyToDelete() const
{
	// Матч готов к удалению, если он завершен и таймер показа результатов истек
	return (state == MatchState::FINISHED && endMatchTimer <= 0.0f);
}
Character* Match::GetCharacterByIdx(uint32_t idx)
{
	if (idx >= entities.size() || !entities[idx].active) return nullptr;
	return entities[idx].character.get();
}
bool Match::HasLineOfSight(glm::vec2 start, glm::vec2 end)
{
	glm::vec2 dir = end - start;
	float dist = glm::length(dir);
	if (dist < 0.1f) return true;

	dir /= dist; // нормализуем
	float step = 0.5f; // шаг проверки (в единицах GRID_CELL_SIZE или пикселях)

	for (float i = 0.0f; i < dist; i += step) {
		glm::vec2 checkPos = start + dir * i;
		if (level.IsSolid(checkPos)) return false; // Предполагаем, что у level есть метод IsWall
	}
	return true;
}
void Match::SendTileUpdate(const glm::ivec2& cell, char value)
{
	olc::net::message<GameMsg> msg;
	msg.header.id = GameMsg::Game_TileUpdate;

	uint16_t index = cell.y * level.levelWidth + cell.x;
	msg << index;
	msg << value;
	MessageAllMatchClients(msg);
	
}
void Match::SendTileMove(const glm::ivec2& from, const glm::ivec2& to, float duration)
{
	olc::net::message<GameMsg> msg;
	msg.header.id = GameMsg::Game_TileMove;

	uint16_t fromIdx = from.y * level.levelWidth + from.x;
	uint16_t toIdx = to.y * level.levelWidth + to.x;

	msg << fromIdx;
	msg << toIdx;
	msg << duration;
	MessageAllMatchClients(msg);
}
void Match::ApplyDamage(DamageContext ctx)
{
	if (ctx.cancelled) return;
	ctx.finalDamage = ctx.baseDamage;
	float damageBefore = ctx.baseDamage;

	// 1️⃣ WORLD TILE
	if (ctx.targetType == DamageTargetType::WorldTile)
	{
		level.DamageTile(ctx.targetCell, (int)ctx.finalDamage, ctx.attackerId);
		return;
	}

	// 2️⃣ PROJECTILE
	if (ctx.targetType == DamageTargetType::Projectile)
	{
		auto& slot = projectiles[ctx.targetId];

		

			slot.cachedRules.effectValue -= ctx.finalDamage;
			
		if (slot.active && slot.data.type == ProjectileType::MageCrystal)
		{

			

			// РЕЗОНАНС: если атакующий — владелец кристалла
			if (ctx.attackerId == slot.data.nOwnerID && ctx.source == DamageSource::Projectile)
			{
				if (ctx.type == DamageType::Magical) { // Лёд (Iceball)

					int idx = GetPlayerIndex(slot.data.nOwnerID);

				
					
					if (idx < GetEntities().size() && GetEntities()[idx].character !=nullptr && GetEntities()[idx].active)
					{
						auto& owner = GetEntities()[idx].character;
						if (owner) {
							owner->Heal(20.0f);
							
						}
						// Визуальный эффект резонанса
						//SendEffectToClients(slot.data.vPos, "IceResonance");
					}
					
				}
				//else if (ctx.type == DamageType::Fire) { // Огонь (Fireball)
				//	// Мощный взрыв и удаление кристалла
				//	ProcessAreaDamage(slot.data.vPos, 4.5f, 50.0f, ctx.attackerId, false, GetProjectileRules(ProjectileType::Explosion));
				//	slot.cachedRules.effectValue = -1.0f; // Принудительно ломаем
				//}
			}

		}


		if (slot.cachedRules.effectValue <= 0) {
			MarkProjectileForRemoval(ctx.targetId);
			spellManager.ForceFinishProjectile(ctx.targetId, this); // ОБЯЗАТЕЛЬНО!
		}
		return;

	}

	// 3️⃣ CHARACTER (Старая логика, теперь она в безопасности)
	if (ctx.targetId >= (uint32_t)entities.size()) return;

	auto& pl = entities[ctx.targetId];
	if (!pl.active || pl.character == nullptr) return;
	

	
	 Character& target = *pl.character;
	//HandleShield(ctx, target);
	target.OnProcessIncomingDamage(ctx); // адаптация воина 
	HandleStatusModifiers(ctx, target); // 

	if (!ctx.cancelled && ctx.finalDamage > 0)
	{
		target.TakeDamage(ctx.finalDamage);

		// Проверка связи через эффект (O(1))
		if (ctx.source != DamageSource::LinkSecondary)
		{
			
			if (auto* link = target.GetEffects()->GetEffect(StatusEffectType::Linked))
			{
				if (link->linkedEntityId != -1)
				{
					DamageContext linkedCtx = ctx; // Копируем базу (аттакера, тип урона)
					linkedCtx.targetId = link->linkedEntityId;
					linkedCtx.baseDamage = ctx.finalDamage;
					linkedCtx.source = DamageSource::LinkSecondary;
					
					ApplyDamage(linkedCtx); // Передаем урон
				}
			}
		}
		float xpToReport = 0.0f; // Переменная для записи в отчет
		if (target.IsDead())
		{
			OnCharacterDied(target.GetId());

			// 1. Ищем атакующего (кто нанес урон)
	   // Предполагаем, что ctx.attackerId — это уникальный ID сущности
			int attackerIdx = GetPlayerIndex(ctx.attackerId);

			if (attackerIdx != -1 && attackerIdx < entities.size())
			{
				auto& attackerEnt = entities[attackerIdx];
				if (attackerEnt.active && attackerEnt.character)
				{
					// 2. Получаем количество опыта из убитого (моба)
					// Добавьте метод GetExperienceReward() в класс Character
					xpToReport = 100.0f; // target.GetExperienceReward();

					attackerEnt.character->GetProgression()->AddExperience(xpToReport);
				}
			}
		}

		sDamageReport report;  // тут нужны постоянный id
		report.nVictimId = target.GetId();
		report.nAttackerId = ctx.attackerId;
		report.nType = (uint8_t)ctx.type;
		report.Amount = ctx.finalDamage;
		report.ExperienceGained = xpToReport;

		report.bResisted = (ctx.finalDamage < damageBefore);
	

		damageQueue.push_back(report);

	}

	// 4️⃣ Notify (later: combat log / floating dmg)
}/*
void Match::BroadcastMatchmakingStatus()
{
	olc::net::message<GameMsg> msg;
	msg.header.id = GameMsg::Server_MatchmakingStatus;

	uint8_t count = sessionPlayers.size();
	msg << count;

	MessageAllClients(msg);

	if (count == 3) {
		StartRoom();
	}
}*/
void Match::MessageAllMatchClients(olc::net::message<GameMsg>& msg)
{
	
	for (const auto& pl : realPlayerIds) {
		
			// Используем метод сервера для получения указателя на соединение
			auto client = server->GetConnectionByPlayer(pl);
			if (client && client->IsConnected()) {
				server->MessageClient(client, msg);
			}
		
	}
}

void Match::SendAreaDamageToClient(olc::net::message<GameMsg>& msg, glm::vec2 epicentersPos)
{
	// Рассчитываем текущий радиус видимости на основе времени суток (lightIntensity)
  // Добавим запас в 2.0f единицы, чтобы объекты плавно входили в кадр
	float minR = MIN_RADIUS / (float)BRICK_SIZE; // 192 / 32 = 6.0 тайлов
	float maxR = MAX_RADIUS / (float)BRICK_SIZE; // 384 / 32 = 12.0 тайлов

	// Считаем текущий радиус в зависимости от освещения
	float currentViewRadius = minR + (maxR - minR) * lightIntensity;

	// Итоговый радиус для рассылки (с запасом 2.0 тайла)
	float viewRadius = currentViewRadius + 2.0f;
	float viewRadiusSq = viewRadius * viewRadius;


	for (const auto& pl : realPlayerIds) {
		
		auto it = idToIndex.find(pl);
		if (it == idToIndex.end()) continue;

		auto& ent = entities[it->second];
		if (!ent.active || !ent.character) continue;

		// Используем метод сервера для получения указателя на соединение
		if (glm::distance2(epicentersPos, ent.character->position) < viewRadiusSq) {

			auto client = server->GetConnectionByPlayer(pl);
			if (client && client->IsConnected()) {
				server->MessageClient(client, msg);
			}
		}
	}
//
//	for (auto& recipient : entities) {
//		if (!recipient.active || !recipient.session.isOnline) continue;
//
//		//olc::net::message<GameMsg> msgUpdate;
//	//	msgUpdate.header.id = GameMsg::Game_UpdateWorld;
//
//	//	glm::vec2 playerPos = recipient.character->position;
//
//	//	msgUpdate << matchTime;
//
//		// 2. СНАРЯДЫ
//	//	uint16_t visibleProjectiles = 0;
//	//	for (uint16_t pid : activeProjectileIndices) {
//	//		auto& proj = projectiles[pid].data;
//	//		if (glm::distance2(playerPos, proj.vPos) < viewRadiusSq) {
//	//			msgUpdate << proj;
//	//			visibleProjectiles++;
//	//		}
////		}
//	//	msgUpdate << visibleProjectiles;
//
//
//
//		// 1. ПЕРСОНАЖИ (Игроки + Мобы)
//		uint16_t visibleEntities = 0;
//		for (auto& target : entities) {
//			if (!target.active) continue;
//
//			if (glm::distance2(playerPos, target.character->position) < viewRadiusSq) {
//				msgUpdate << ToNet(*target.character);
//				visibleEntities++;
//			}
//		}
//		msgUpdate << visibleEntities;
//
//
//		// Отправка конкретному игроку
//		auto client = server->GetConnectionByPlayer(recipient.netId);
//		if (client && client->IsConnected()) {
//			server->MessageClient(client, msgUpdate);
//		}
//	}
}
void Match::BroadcastSnapshot()
{
	// Рассчитываем текущий радиус видимости на основе времени суток (lightIntensity)
  // Добавим запас в 2.0f единицы, чтобы объекты плавно входили в кадр
	float minR = MIN_RADIUS / (float)BRICK_SIZE; // 192 / 32 = 6.0 тайлов
	float maxR = MAX_RADIUS / (float)BRICK_SIZE; // 384 / 32 = 12.0 тайлов

	// Считаем текущий радиус в зависимости от освещения
	float currentViewRadius = minR + (maxR - minR) * lightIntensity;

	// Итоговый радиус для рассылки (с запасом 2.0 тайла)
	float viewRadius = currentViewRadius + 2.0f;
	float viewRadiusSq = viewRadius * viewRadius;

	for (auto& recipient : entities) {
		if (!recipient.active || !recipient.session.isOnline) continue;

		olc::net::message<GameMsg> msgUpdate;
		msgUpdate.header.id = GameMsg::Game_UpdateWorld;

		glm::vec2 playerPos = recipient.character->position;

		msgUpdate << matchTime;

		// 2. СНАРЯДЫ
		uint16_t visibleProjectiles = 0;
		for (uint16_t pid : activeProjectileIndices) {
			auto& proj = projectiles[pid].data;
			if (glm::distance2(playerPos, proj.vPos) < viewRadiusSq) {
				msgUpdate << proj;
				visibleProjectiles++;
			}
		}
		msgUpdate << visibleProjectiles;



		// 1. ПЕРСОНАЖИ (Игроки + Мобы)
		uint16_t visibleEntities = 0;
		for (auto& target : entities) {
			if (!target.active) continue;

			if (glm::distance2(playerPos, target.character->position) < viewRadiusSq) {
				msgUpdate << ToNet(*target.character);
				visibleEntities++;
			}
		}
		msgUpdate << visibleEntities;


		// Отправка конкретному игроку
		auto client = server->GetConnectionByPlayer(recipient.netId);
		if (client && client->IsConnected()) {
			server->MessageClient(client, msgUpdate);
		}
	}
}
//void Match::CheckProjectileCharacterCollision(uint16_t pid, ProjectileSlot& slot, float dt)
//{
//	auto& proj = slot.data;
//
//	float radius = proj.fRadius;
//
//	for (auto& p : players) {
//		if (!p.character) continue;
//		if (p.character->GetId() == proj.nOwnerID) continue;
//
//		float dist = glm::distance(p.character->position, proj.vPos);
//
//		if (dist < radius + p.character->radius) {
//			spellManager.OnProjectileHitCharacter(pid, p.character->GetId(), this);
//			return;
//		}
//	}
//}
void Match::StartRoom()
{
	
	state = MatchState::ROOM;
	roomTimer = 60.0f;

	for (auto& id : entities)
		id.state = PlayerState::ROOM;

	olc::net::message<GameMsg> msg;
	msg.header.id = GameMsg::Server_RoomStarted;
	MessageAllMatchClients(msg);
	
	InitSpatialGrid();
	
}
void Match::CheckAllReady()
{
	if (entities.empty()) return;

	bool allReady = true;
	for (auto& player : entities)
	{
		if (!player.session.ready ||
			player.session.selectedclass == PlayerClass::None)
		{
			allReady = false;
			break;
		}
	}

	if (allReady)
	{
		StartGame();
	}
}

void Match::StartGame()
{
	state = MatchState::INGAME;

	

	for (auto& pl : entities) //  тут создаётся Character !!!!
	{
		
		
			// 1. Создаем объект через фабрику
			pl.character = CharacterFactory::Create(
				pl.session.selectedclass,
				pl.netId,
				server->GetPlayerName(pl.netId),
				this
			);

		pl.character->position = GetSpawnPoint(pl.netId);
		pl.character->teamId = pl.netId; // определяем fraction
		pl.character->FullReset();
		pl.character->Revive();
		//pl.character->ClearEffects();
		//pl.character->ClearCooldowns();

		pl.state = PlayerState::INGAME;
		pl.character->onTileChangedPos = [this](uint32_t id, glm::ivec2 n, glm::ivec2 o) {
			this->HandlePlayerTileChange(id, n, o);
			};
		sPlayerInfo info{
			pl.netId,
			pl.character->GetName(),
			pl.session.selectedclass
		};

		server->SendToMatch(*this, GameMsg::Game_PlayerInfo, info);


		sEntityDescription desc = ToNet(*pl.character); // Используем общую функцию!
		
		server->SendToMatch(*this, GameMsg::Game_AddPlayer, desc); // Рассылаем всем в матче
	}
	// 2. Потужно обрабатываем очередь спавна
	for (const auto& req : pendingSpawns) {
		this->SpawnCompanion(req.type, req.pos, req.owner);
	}


	olc::net::message<GameMsg> msg;
	msg.header.id = GameMsg::Server_GameStarted;
	msg << level.seed;    // uint32_t
	msg << level.levelHeight; // int
	msg << level.levelWidth; // int

	MessageAllMatchClients(msg);


	// 3. Очищаем очередь для следующего кадра/события
	pendingSpawns.clear();
}
void Match::EndMatch(uint32_t WinnerId)
{
	if (state != MatchState::INGAME) return;
	state = MatchState::FINISHED;

	
	this->winnerID = WinnerId;
	
	

	// 2. Важный момент: мы не удаляем Match сразу. 
	// Даем игрокам 5 секунд посмотреть на экран статистики (endMatchTimer).
}
void Match::UpdateCharacters(const float& dt, const float& lightIntensity)
{
	float acceleration = 12.0f; // Насколько быстро игрок набирает скорость

	for (auto& p : entities)
	{
		if (!p.active || p.character == nullptr) continue;

		Character& ch = *p.character;
	
		

		ch.OnUpdate(dt, lightIntensity); // тут таимер для мобов что бы удалиться 
		if (ch.IsDead())
			continue;
		auto* effects = ch.GetEffects();

		effects->Update(dt);  // время жизни эффектов
		effects->RefreshModifiers(); // что эти модификаторы делают

		// !!! ИЗМЕНЕНО: Блокируем ввод, если персонаж в стане или залочен заклинанием
		bool inputBlocked = !ch.CanMove();

		if (inputBlocked) {
			// Если движение заблокировано, плавно гасим текущую скорость ходьбы
			ch.currentMovingVel += (glm::vec2(0) - ch.currentMovingVel) * 10.0f * dt;
		}

		// --- 1. ТРЕНИЕ (FRICTION) ---
		// Плавное затухание сил отталкивания
		float friction = 8.0f;
		if (glm::length(ch.knockbackVel) > 0.01f) {
			ch.knockbackVel -= ch.knockbackVel * friction * dt;
		}
		else {
			ch.knockbackVel = { 0, 0 };
		}

		// --- 2. ИНЕРЦИЯ ДВИЖЕНИЯ (SMOOTH MOVEMENT) ---
		// Вместо мгновенного totalVel, мы плавно разгоняем персонажа
		// Создаем переменную ch.currentMovingVel (добавьте в класс Character)
		if (!inputBlocked) {

		float finalMaxSpeed = PLAYER_VELOCITY * ch.GetCurSpeedModifier();

		glm::vec2 targetInputVel = ch.inputVel * finalMaxSpeed;

		// Интерполяция текущей скорости ходьбы к желаемой
		ch.currentMovingVel += (targetInputVel - ch.currentMovingVel) * acceleration * dt ;
		}

		// --- 3. ИТОГОВЫЙ ВЕКТОР ---
		glm::vec2 totalVel = ch.currentMovingVel + ch.knockbackVel;

		if (glm::length(totalVel) > 0.001f) {
			// 1. Считаем количество шагов только если скорость реально высокая
			float frameDistSq = glm::length2(totalVel * dt); // Используем квадрат длины (быстрее) За один кадр персонаж пролетит 1000 * 0,016 = 16; единиц.
			float maxStepDist = ch.radius * 0.5f; //Мы хотим, чтобы за один микро - шаг персонаж сдвинулся не более чем на половину своего тела

			int numSteps = 1;
			if (frameDistSq > maxStepDist * maxStepDist) {
				numSteps = std::max(1, (int)std::ceil(sqrtf(frameDistSq) / maxStepDist));
			}

			float subDt = dt / (float)numSteps;

			for (int i = 0; i < numSteps; ++i) {
				glm::vec2 prevPos = ch.position;
				ch.position += totalVel * subDt;
				glm::vec2 normal;
				bool Kill = true;
				// 2. Если столкновение произошло, ResolveCircleWorldCollision изменит ch.position
				if (ResolveCircleWorldCollision(ch.position, ch.radius, level, Kill,&normal)) {
					if (Kill)
					{
						ch.TakeDamage(ch.health);
						ch.IsDead();
						OnCharacterDied(ch.GetId());
					}

					float impactStrength = glm::dot(ch.knockbackVel, -normal);

					// Если мы летим В стену (сила удара положительная)
					if (impactStrength > 1.0f) {
						// Формула отскока: V_new = V_old + 2 * impactStrength * normal
						// 0.5f — коэффициент "прыгучести" (bounciness). 1.0 - идеальный мячик.
						float bounciness = 0.5f;
						ch.knockbackVel += normal * impactStrength * (1.0f + bounciness);

						// Опционально: если удар ОЧЕНЬ сильный, можно нанести урон
						// if (impactStrength > 50.0f) ch.ApplyDamage(impactStrength * 0.1f);
					}

					// Опционально: если персонаж "застрял", можно обнулить скорость 
					// или слегка погасить её для реализма
				}

				// 3. Границы мира
				ch.position = glm::clamp(ch.position,
					glm::vec2(ch.radius),
					glm::vec2(level.levelWidth - ch.radius, level.levelHeight - ch.radius));

				// 4. Оптимизация: если после коррекции мы почти не сдвинулись, выходим
				if (numSteps > 1 && glm::distance2(prevPos, ch.position) < 0.00001f) {
					break;
				}
			}
		}
	}


}

void Match::UpdateProjectiles(float dt)
{
	// Рассчитываем вектор ветра один раз за апдейт
	glm::vec2 windForce = { cosf(windAngle), sinf(windAngle) };
	windForce *= (float)windStrength * 2.5f; // Настройте множитель силы

	for (uint16_t pid : activeProjectileIndices)
	{
		auto& slot = projectiles[pid];
		if (!slot.active || !slot.collisionEnabled) continue;
		// Тогда в цикле:
		const ProjectileRules& rules = slot.cachedRules; // Мгновенно
	

		auto& proj = slot.data;
		// --- ОПТИМИЗАЦИЯ ДЛЯ ЗАСТРЯВШИХ СНАРЯДОВ ---
		if (proj.bStuck) {
			// Если снаряд застрял, мы НЕ двигаем его и НЕ проверяем коллизии со стенами.
			// Но мы ДОЛЖНЫ проверить коллизии с персонажами (через Spatial Grid), 
			// чтобы сработал "сбор информации" Hunter-а.

			// Тут вызываем только проверку столкновения с игроками (если она у тебя отдельно)
			// Если проверка с игроками идет ниже по коду, просто делаем:
			// continue; // или идем к блоку проверки персонажей
		

			continue;
		}
		
		

		// Влияние ветра: чем выше "парусность" снаряда, тем сильнее его сносит
	 // Добавьте в ProjectileRules поле float windResistance (0.0 - 1.0)
		//windForce = windForce* rules.windResistance; // Для теста;
		//// Если скорость почти нулевая (фаза роста), ветер не должен сносить позицию
		//if (glm::length(slot.data.vVel) > 0.1f) {
		//	// Только летящие снаряды получают боковой снос от ветра
		//	slot.data.vVel += windForce * dt;
		//}


		if (!proj.bStuck && rules.homingStrength > 0.0f) {
			float maxDist = 6.0f; // Радиус поиска цели
			uint16_t targetId = FindNearestEnemy(proj.vPos, proj.nOwnerID, maxDist);

			if (targetId != 0xFFFF) {
				auto& target = entities[targetId].character;
				glm::vec2 toTarget = glm::normalize(target->position - proj.vPos);
				float speed = glm::length(proj.vVel);

				// Плавно подмешиваем направление на цель к текущему вектору
				// rules.homingStrength может быть ~2.0 - 5.0f
				proj.vVel = glm::normalize(proj.vVel + toTarget * rules.homingStrength * dt) * speed;
			}
		}

		glm::vec2 newPos = proj.vPos + proj.vVel * dt;
		if (rules.ignoresWorld == true) {
			proj.vPos = newPos;
			continue;
		}

		glm::ivec2 hitCell;
		glm::vec2 normal;
		bool kill;
		if (ResolveCircleWorldCollision(newPos, proj.fRadius, level, kill, &normal, &hitCell))
		{
			if (rules.dealsWorldDamage)
			{
				DamageContext ctx;
				ctx.attackerId = proj.nOwnerID;
				ctx.targetType = DamageTargetType::WorldTile;
				ctx.targetCell = hitCell;
				ctx.baseDamage = rules.damageToWorld;
				ctx.source = DamageSource::Projectile;

				ApplyDamage(ctx);
			}
			
			if (proj.type == ProjectileType::StickyBomb || proj.type == ProjectileType::Hook ||  proj.type == ProjectileType::Shoot || proj.type == ProjectileType::BindShot || proj.type == ProjectileType::Fireball)
			{
				proj.bStuck = true;
				proj.vVel = { 0,0 };

				// Передаем информацию спеллу через SpellManager Notify WorldHit
			   // Мы знаем pid (индекс снаряда), значит можем найти спелл, который им владеет
			
				spellManager.OnProjectileHitWorld(pid, hitCell, this);
				continue;
			}

			if (rules.diesOnWorldCollision)
			{
				
				MarkProjectileForRemoval(pid);
				spellManager.ForceFinishProjectile(pid, this);
				continue;
			}
		}

		proj.vPos = newPos;
	}
}

uint32_t Match::FindNearestEnemy(glm::vec2 pos, uint32_t ownerNetId, float maxDist)
{
	float bestDistSq = maxDist * maxDist;
	uint32_t bestTargetIdx = -1;

	// Определяем, кто выпустил снаряд (игрок или моб
	auto it = idToIndex.find(ownerNetId);
	if (it == idToIndex.end()) return -1; // На всякий случай, если ID невалиден

	//EntityType ownerType = entities[it->second].character->entityType;
	uint32_t   ownerTeam = entities[it->second].character->teamId;

	// Вычисляем охват ячеек на основе дистанции
	// Предполагаем, что CELL_SIZE — это размер стороны вашей ячейки
	int cellRadius = std::ceil(maxDist / GRID_CELL_SIZE);
	glm::ivec2 baseCell = WorldToGridCell(pos);

	// Проходим по квадрату ячеек от -cellRadius до +cellRadius
	for (int y = -cellRadius; y <= cellRadius; ++y) {
		for (int x = -cellRadius; x <= cellRadius; ++x) {

			int nIdx = GridIndex(baseCell + glm::ivec2(x, y));
			if (nIdx < 0 || nIdx >= (int)spatialGrid.size()) continue;

			for (int cid = spatialGrid[nIdx].firstCharacter; cid != -1; cid = entities[cid].nextInCell) {

				if (entities[cid].netId == ownerNetId ||entities[cid].character == nullptr || entities[cid].character->IsDead()) continue;

				auto& targetCh = *entities[cid].character;


				// 2. ЛОГИКА ФРАКЦИЙ:
	
				//if (ownerType == EntityType::Mob && targetCh.entityType == EntityType::Mob) continue; // если моб ищет моба - пропускаем
				// Если у нас одна команда — пропускаем 
				if (ownerTeam == targetCh.teamId) continue;
				

				glm::vec2 diff = entities[cid].character->position - pos;
				float dSq = glm::dot(diff, diff);

				if (dSq < bestDistSq) {
					bestDistSq = dSq;
					bestTargetIdx = (uint32_t)cid;  // Возвращаем индекс в массиве players
				}
			}
		}
	}
	return bestTargetIdx;
}

glm::vec2 Match::FindNearestEmptySpace(const gameLevel& level, glm::vec2 startPos)
{
	glm::ivec2 startCell = glm::ivec2(std::floor(startPos.x), std::floor(startPos.y));

	// Если точка уже свободна, возвращаем её (с центровкой)
	if (level.IsValid(startCell) && !level.IsSolid(startCell)) {
		return glm::vec2(startCell) + 0.5f;
	}

	// Ищем кругами (радиус r от 1 до 5, например)
	for (int r = 1; r <= 5; ++r) {
		for (int x = -r; x <= r; ++x) {
			for (int y = -r; y <= r; ++y) {
				// Нам нужны только клетки на периметре текущего квадрата
				if (std::abs(x) != r && std::abs(y) != r) continue;

				glm::ivec2 candidate = startCell + glm::ivec2(x, y);

				if (level.IsValid(candidate) && !level.IsSolid(candidate)) {
					// Возвращаем центр найденной пустой клетки
					return glm::vec2(candidate) + 0.5f;
				}
			}
		}
	}

	return startPos; // Если совсем ничего не нашли (тупик), возвращаем оригинал
}

void Match::NotifyMobDied(int homeIdx)
{
	

	auto it = activeSpawners.find(homeIdx);
	if (it != activeSpawners.end()) {
		it->second.currentCount--;

		// Защита: счетчик не должен быть отрицательным
		if (it->second.currentCount < 0) {
			it->second.currentCount = 0;
		}
	}
}

void Match::CleanupCharacter(uint32_t netId)
{
	std::cout << "in Garbage " << netId <<"  entities size: " << entities.size() << std::endl;
	garbageEntities.push_back(netId);
	

}

PlayerID Match::GeneratePlayerID()
{
	return server->GeneratePlayerID();
}

void Match::FlushTileUpdates()
{
	if (pendingTileUpdates.empty()) return;

	olc::net::message<GameMsg> msg;
	msg.header.id = GameMsg::Game_TileUpdate;

	// Сначала записываем количество измененных тайлов
	uint16_t count = (uint16_t)pendingTileUpdates.size();

	
	for (const auto& update : pendingTileUpdates) {
		msg << update.value; // символ
		msg << update.index;  // индекс
		msg << update.respawnTime;  // время
	}
	msg << count;

	MessageAllMatchClients(msg);
	pendingTileUpdates.clear();
}

void Match::UpdateSpawners(float dt)
{

	for (auto& [tileIdx, sp] : activeSpawners) { //исключать из activespawners когда убили и включать когда появился 
		// Проверка, не разрушили ли могилу (сменили тип тайла)
		if (level.GetTile(sp.gridPos).type != TileType::Graveyard) continue;

		sp.nextSpawnTime -= dt;
		if (sp.currentCount < sp.currentLimit && sp.nextSpawnTime <= 0) {
			// Спавним, передаем tileIdx как "дом"
			ArchetypeId npcType;
			if (rand() % 2 == 0)
				npcType = ArchetypeId::Mob_Melee; else
				npcType = ArchetypeId::Mob_Ranged;
			//
			SpawnAI(npcType, glm::vec2(sp.gridPos) + 0.5f, tileIdx);
			sp.currentCount++;
			
			sp.nextSpawnTime = 10.0f + (rand() % 5);
		}
	}
}

glm::ivec2 Match::WorldToGridCell(const glm::vec2& pos) const
{
	return {
	  int(pos.x / GRID_CELL_SIZE),
	  int(pos.y / GRID_CELL_SIZE)
	};
}

int Match::GridIndex(const glm::ivec2& c) const
{

	if (c.x < 0 || c.y < 0 || c.x >= gridWidth || c.y >= gridHeight)
		return -1;

	return c.y * gridWidth + c.x;
}

//void Match::DestroyWall(const glm::vec2& cellPos)
//{
//	/*auto& tile = level.GetTile(cellPos);
//	if (tile != 0xffff) {
//		level.DamageTile(cellPos,100);
//
//	}*/
//
//}

void Match::HandleTileInteractions(float dt)
{
	for (int idx : level.GetActiveTileIndices())
	{
		const Tile& tile = level.GetTileByIdx(idx);
		if (tile.type != TileType::Block || tile.destroyed) continue;

		// 1. Определяем центр и ячейку сетки для активного тайла
		glm::vec2 tileCenter;
		glm::ivec2 gridCell;

		if (tile.moving) {
			glm::vec2 startPos = glm::vec2(tile.from) + 0.5f;
			glm::vec2 endPos = glm::vec2(tile.to) + 0.5f;
			tileCenter = glm::mix(startPos, endPos, tile.moveT);
		}
		else {
			tileCenter = level.GetTileCenter(idx);
		}
		gridCell = WorldToGridCell(tileCenter);
		float tileEffectRadius = 0.5f * tile.growInterp;

		// 2. Проверяем только соседей в Spatial Grid
		for (const auto& n : neighbors) // Используем твой массив смещений
		{
			int nIdx = GridIndex(gridCell + n);
			if (nIdx < 0 || nIdx >= (int)spatialGrid.size()) continue;

			// 3. Итерируемся по списку персонажей в ячейке
			int pIdx = spatialGrid[nIdx].firstCharacter;
			while (pIdx != -1)
			{
				Player& p = entities[pIdx];
				// Переходим к следующему сразу, чтобы не потерять итератор
				int nextP = p.nextInCell;

				float dist = glm::distance(p.character->position, tileCenter);
				float combinedRadius = p.character->radius + tileEffectRadius;

				if (dist < combinedRadius)
				{
					glm::vec2 pushDir = (dist > 0.001f) ?
						glm::normalize(p.character->position - tileCenter) :
						glm::vec2(0.0f, -1.0f);

					// Импульс (Knockback)
					if (tile.growInterp < 0.2f || (tile.moving && tile.moveSpeed > 2.0f)) {
						p.character->knockbackVel += pushDir * 10.0f * dt * 60.0f;
					}

					// Коррекция позиции
					float overlap = combinedRadius - dist;
					p.character->position += pushDir * overlap;
				}
				pIdx = nextP;
			}
		}
	}
}

void Match::InitSpatialGrid()
{
	
	gridWidth = level.levelWidth;
	gridHeight = level.levelHeight;

	spatialGrid.clear();
	spatialGrid.resize(gridWidth * gridHeight);
}

void Match::ClearSpatialGrid()
{
	// 1. Очищаем сетку (просто массив интов - это мгновенно)
	for (auto& cell : spatialGrid) cell.Clear();
}

void Match::SendCooldownToClient(uint32_t playerId, SpellId spell,float cd)
{

	auto it = idToIndex.find(playerId);
	if (it == idToIndex.end()) return;
	
	auto& pl = entities[it->second];
	

	olc::net::message<GameMsg> msg;
	msg.header.id = GameMsg::Server_SpellCooldown;

	// ВАЖНО: Используйте matchTime, чтобы клиент мог синхронизироваться
	msg << sSpellCooldown{
		(uint8_t)spell,
		cd,
		matchTime
	};

	// Отправляем только тому, кто кастанул
	auto client = server->GetConnectionByPlayer(playerId);
	if (client && client->IsConnected()) {
		server->MessageClient(client, msg);
	}
}

void Match::OnCharacterDied(uint32_t id)
{
	if (state != MatchState::INGAME)
		return;

	// Находим сущность
	auto it = idToIndex.find(id);
	if (it != idToIndex.end()) {
		auto& p = entities[it->second];
		if (p.character) {
			// Вызываем виртуальный метод — каждый объект сам решит, что делать
			
			p.character->OnBeforeDestroy();
		}
	}

	CheckVictoryCondition();
}

void Match::ApplyBindEffect(Character* target, glm::vec2 center, int linkedId, bool isDynamic)
{
	StatusEffect e;
	e.type = StatusEffectType::BindingChain;
	e.timeLeft = 2.5f;
	e.vCenterPos = center;       // Точка, куда тянет
	e.linkedEntityId = linkedId; // Если тянет к другому игроку
	e.value = 180.0f;            // Сила натяжения
	//target->AddStatusEffect(e);
	target->GetEffects()->Add(e);


	olc::net::message<GameMsg> msg;
	msg.header.id = GameMsg::Game_BindLink;
	msg << target->GetId();
	msg << center;
	msg << (int32_t)linkedId; // К кому привязан (-1 если к стене)
	msg<< 2.5f; // Кто, Куда, Как долго
	//MessageAllMatchClients(msg);
	SendAreaDamageToClient(msg, center);
}
//
//void Match::BroadcastProjectile(uint16_t idx)
//{
//	auto& slot = projectiles[idx];
//
//	olc::net::message<GameMsg> msg;
//	msg.header.id = GameMsg::Game_CastSpell; // Используем тот ID, который клиент ждет
//	msg << slot.data; // Записываем структуру sProjectileDescription
//
//	// Отправляем ВСЕМ игрокам, чтобы они увидели снаряд
//	server->MessageAllClients(msg);
//}

glm::vec2 Match::GetSpawnPoint(uint32_t id)
{
	auto it = idToIndex.find(id);
	size_t playerIdx = (it != idToIndex.end()) ? it->second : 0;

	// Распределяем игроков по зонам (0, 1, 2, 3)
	size_t spawnIdx = playerIdx % actualSpawns.size();

	glm::ivec2 cell = actualSpawns[spawnIdx];

	// Возвращаем центр тайла
	return glm::vec2(cell.x + 0.5f, cell.y + 0.5f);
}

void Match::SyncEntityStats(uint32_t entityId)
{
	uint32_t autoIdx = GetPlayerIndex(entityId);
	if (autoIdx == -1) return;

	

	auto& character = entities[autoIdx].character;

	olc::net::message<GameMsg> msg;
	msg.header.id = GameMsg::Game_EntityStats;

	msg << character->maxHealth; // Можно заодно синхронить макс. ХП
	msg << entityId;
	msg << character->GetProgression()->GetLevel();
	msg << character->GetProgression()->GetXP();
	MessageAllMatchClients(msg);
}

//void Match::GetMinionsOf(uint32_t casterId)
//{
//	auto it = idToIndex.find(casterId);
//	if (it == idToIndex.end()) return nullptr;
//
//	////сперва находим его в очереди для entities - затем  вызываем у него метод и возвращаем результат
//	//auto it = idToIndex.find(casterId);
//	//size_t playerIdx = (it != idToIndex.end()) ? it->second : 0; // либо индекс либо 0
//
//	
//	Character* MinionsOwner = entities[it->second].character.get();
//
//	return MinionsOwner->GetMinions();
//
//
//}

void Match::CalculateValidSpawns()
{
	actualSpawns.clear();

	// Используем сид уровня для генератора, чтобы спавны были разными в каждом матче
	std::mt19937 spawnRng(level.seed); //совершенно безумное случайное число (от 0 до 4 миллиардов).

	struct Zone { int xS, xE, yS, yE; };
	std::vector<Zone> zones = {
		{1, 10, 1, 10},                                 // Топ-лево
		{level.levelWidth - 11, level.levelWidth - 2, 1, 10}, // Топ-право
		{1, 10, level.levelHeight - 11, level.levelHeight - 2}, // Бот-лево
		{level.levelWidth - 11, level.levelWidth - 2, level.levelHeight - 11, level.levelHeight - 2} // Бот-право
	};

	for (auto& z : zones) {
		std::vector<glm::ivec2> freeCellsInZone;

		// Собираем все пустые клетки в зоне
		for (int y = z.yS; y <= z.yE; ++y) {
			for (int x = z.xS; x <= z.xE; ++x) {
				if (IsCellFree({ x, y })) {
					freeCellsInZone.push_back({ x, y });
				}
			}
		}

		if (!freeCellsInZone.empty()) {  // 2. Если нашли хотя бы одну — выбираем случайную из них

			std::uniform_int_distribution<size_t> dist(0, freeCellsInZone.size() - 1); //  «Слушай, мне нужны числа строго от Мин до Макс».
			size_t randomIndex = dist(spawnRng);
			actualSpawns.push_back(freeCellsInZone[randomIndex]);
		}
		else {
			// Если зона забита блоками (редко), берем центр зоны как запасной вариант
			actualSpawns.push_back({ (z.xS + z.xE) / 2, (z.yS + z.yE) / 2 });
		}
	}
}

Spell* Match::HandleCastSpell(uint32_t playerId, SpellId spell, uint32_t ConnectionId)
{
	if (idToIndex.find(playerId) == idToIndex.end()) return nullptr;
	auto& pl = entities[idToIndex[playerId]];
	if (!pl.active ||pl.character == nullptr || pl.character->IsDead()) return nullptr;
	auto& ch = pl.character;
	// Категория Б: Многоэтапные (Teleport, StickyBomb, BindArrow)
		// Второй клик активирует вторую фазу (прыжок, взрыв)
	bool isMultiStage = (spell == SpellId::Teleport || spell == SpellId::StickyBomb || spell == SpellId::BindArrow ||    spell == SpellId::GhostArrow || spell == SpellId::MagicConservation);
	bool isFireballType = (spell == SpellId::Fireball || spell == SpellId::Iceball || spell == SpellId::FireDash || spell == SpellId::IceRoots || spell == SpellId::InfuseArrow || spell == SpellId::Shoot);
	// --- 1. ПРОВЕРКА КУЛДАУНА ---
	if (!ch->CanCast(spell, matchTime)) return nullptr;

	// --- 2. ЛОГИКА АКТИВНЫХ СПЕЛЛОВ ---
	if (auto* activeSpell = spellManager.FindActive(spell, playerId))
	{
		// Категория А: Прерываемые (Fireball, Iceball)
		// Если он еще только "заряжается" (Appear), второй клик его отменяет
		if (activeSpell->CanBeCancelled()) {
			activeSpell->Cancel(this);
			return activeSpell; // КД не ставим, маг свободен
		}

		

		if (isMultiStage) {
			activeSpell->Cast(*ch, this);

			// Если после клика спелл завершился (например, телепортнулся) — ставим КД
			if (activeSpell->IsFinished()) {
				float cd = activeSpell->GetAdjustedCooldown();
				SendCooldownToClient(playerId, spell, cd);
				ch->GetCooldown()->Set(spell, cd, matchTime);
				//ch->SetCooldown(spell, cd, matchTime);
			}
			return activeSpell;
		}

		// Если это летящий Fireball (Active), мы его игнорируем здесь.
		// Но так как КД уже висит на персонаже, новый не создастся (отсечет CanCast выше).
	}

	// --- 3. СОЗДАНИЕ НОВОГО СПЕЛЛА ---
	auto s = SpellFactory::Create(spell);
	// ГАРАНТИЯ БЕЗОПАСНОСТИ:
	if (!s) {
	//	std::cout << "Error: SpellFactory returned nullptr for ID: " << (int)spell << std::endl;
		return nullptr; // Просто выходим, не пытаясь кастовать
	}
	if (s->Cast(*ch, this)) {
		s->SetOwner(playerId);

		// Для Мага: OnSpellCast вызываем только если каст прошел
		// (Для Fireball это залочит персонажа и запустит стадию Appear)
	}
	else {
		return nullptr; // Каст не удался (маг в стане или занят)
	}

	float cd = s->GetAdjustedCooldown();
	
	// СОХРАНЯЕМ УКАЗАТЕЛЬ ТУТ (до move)
	Spell* ptr = s.get();
	// Добавляем в менеджер
	spellManager.Add(std::move(s));

	// --- 4. КД ДЛЯ ОБЫЧНЫХ СПЕЛЛОВ ---
	// КД сразу ставится только на мгновенные спеллы (не мульти-стейдж и не фаербол)
	// У Фаербола КД поставится САМ в FireballSpell::UpdateAppear при t >= 1.0f
	if (!isMultiStage && !isFireballType) {
		SendCooldownToClient(playerId, spell, cd);
		ch->GetCooldown()->Set(spell, cd, matchTime);
	}
	return 	ptr;
}

void Match::HandleCancelSpell(uint32_t playerId, SpellId id)
{
	spellManager.Cancel(playerId, id, this);
}
	
SpellId Match::GetSpellForSlot(PlayerClass cls, ActionSlot slot, const Character& pl)
{
	// Маг — единственное исключение из-за механики баланса (две способности на одном слоте)
	// Но даже это можно перенести в Mage::GetBoundSpell
	return pl.GetBoundSpell(slot);
}

ActionSlot Match::GetSlotForSpell(PlayerClass& cls, SpellId& spell, const Character& pl)
{
	return pl.GetSlotForSpell(spell);
}

void Match::RemoveActiveProjectile(uint16_t id)
{
	// Находим индекс 'id' в векторе activeProjectileIndices
	auto it = std::find(activeProjectileIndices.begin(), activeProjectileIndices.end(), id);
	if (it != activeProjectileIndices.end()) {
		// Эффективное удаление из вектора:
		// Переносим последний элемент на место удаляемого
		*it = activeProjectileIndices.back();
		// Удаляем последний элемент
		activeProjectileIndices.pop_back();

		// Освобождаем сам слот
		projectiles[id].active = false;
		freeProjectileIds.push(id);
	}
}

void Match::CheckVictoryCondition()
{
	int alive = 0;
	PlayerID lastAlive = 0;
	
	for (uint32_t netIdx : realPlayerIds) {
		
		auto& p = entities[GetPlayerIndex(netIdx)];

		if (!p.character->IsDead())
		{
			alive++;
			lastAlive = p.netId;
		}
	}
	/*for (auto& p : entities)
	{
		if (!p.character->IsDead())
		{
			alive++;
			lastAlive = p.netId;
		}
	}*/
	
	if (alive <= 1)
		EndMatch(lastAlive);
}

void Match::MarkProjectileForRemoval(uint32_t id)
{
	if (id >= MAX_PROJECTILES) return;

	auto& slot = projectiles[id];

	// Если снаряд уже неактивен или уже помечен на удаление — ничего не делаем
	if (!slot.active || slot.data.bPendingDestroy) return;

	// Помечаем флаг, чтобы системы (коллизии, апдейт) игнорировали его в этом кадре
	slot.data.bPendingDestroy = true;


	// Добавляем в сет (если он там уже есть, set просто проигнорирует)
	garbageProjectiles.insert(id);
}

void Match::OnProjectileHit(uint32_t projId, uint32_t targetId, const ProjectileRules& rules)
{
	

		auto& slot = projectiles[projId];
		auto& targetPl = entities[targetId];
		if (!slot.active || !targetPl.active || targetPl.character == nullptr) return;


		uint32_t casterIdx = GetPlayerIndex(slot.data.nOwnerID);
		if (casterIdx == 0xFFFF) return; // Владелец уже вышел из игры

		auto& casterCh = entities[casterIdx].character;
		Character& target = *targetPl.character;

		if (!casterCh) return;

		// --- ПРОВЕРКА КОМАНДЫ (Friendly Fire Check) ---
		if (casterCh->teamId == target.teamId) {
			// Если это снаряд, который должен пролетать сквозь своих, просто выходим.
			// Если он должен исчезать при касании своих (но не дамажить) - вызываем удаление и выходим.
			if (rules.diesOnCharacterCollision) {
				// Опционально: раскоментируйте, если снаряды должны "биться" о союзников
				// MarkProjectileForRemoval(projId);
			}
			return;
		}

		
		slot.data.nVictimId = targetPl.netId;   // Сохраняем NetID жертвы, чтобы PullSpell мог его найти Netid -> idToIndex -> players
		auto& proj = slot.data;
		
		if (proj.type == ProjectileType::Shoot || proj.type == ProjectileType::Hook || proj.type == ProjectileType::BindShot || proj.type == ProjectileType::Fireball || proj.type == ProjectileType::GhostArrow) {
			// Если стрела уже в стене (isStuck), значит это "сбор информации"
			// Нам нужно найти спелл Shoot, который владеет этим снарядом
			spellManager.OnProjectileHitCharacter(projId, targetId, this);
		}
		// --- ЛОГИКА ИМПАКТА (KNOCKBACK) ---
		if (rules.knockbackForce > 0.01f && !target.IsDead())
		{
			glm::vec2 impactDir = target.position - proj.vPos;
			float dist = glm::length(impactDir);
	
			if (dist < 0.001f) impactDir = glm::vec2(0, 1);
			else impactDir /= dist;
	
			// --- РАСЧЕТ ИНЕРЦИИ (МАССЫ) ---
	  // Если радиус 0.8 (день), масса = 1.6. Если 0.4 (ночь), масса = 0.8.
			float mass = target.radius / 0.5f;
			// Чем больше масса, тем меньше импульс
			float finalForce = rules.knockbackForce / mass;
			// 1. Мгновенное смещение (Impact Pop) с учетом массы
			target.position += impactDir * (finalForce * 0.1f);

			// 2. Добавление импульса (Knockback Velocity)
			target.knockbackVel += impactDir * finalForce * 20.0f;
	
			// Опционально: можно заблокировать движение игрока на 0.1 сек (HitStun)
			// ch.Stun(0.1f); 
		}

		// --- НАЛОЖЕНИЕ ЭФФЕКТОВ ---
		if (rules.effectToApply != StatusEffectType::None)
		{
			// Создаем объект эффекта
			StatusEffect effect;
			effect.type = rules.effectToApply;
			effect.timeLeft = rules.effectDuration;
			effect.value = rules.effectValue; 
			effect.nOwnerNetID = slot.data.nOwnerID;
			// Если это щит, задаем HP
			if (effect.type == StatusEffectType::Shield) {
				effect.shieldHP = (int)rules.effectValue;
			}
			
			target.GetEffects()->Add(effect);
			//target.AddStatusEffect(effect); // перекидываем с правил на таргета этот статус эффект
			
		}
	
		DamageContext ctx;
		ctx.attackerId = slot.data.nOwnerID;
		ctx.targetType = DamageTargetType::Character;
		ctx.targetId = targetId;
	
		// Используем правила напрямую из аргумента (максимально быстро)
		ctx.baseDamage = rules.damageToPlayers;
	
		ctx.type = rules.type;
		ctx.source = DamageSource::Projectile;
	
		ApplyDamage(ctx);
	
		if (rules.diesOnCharacterCollision) {
			MarkProjectileForRemoval(projId);
			spellManager.ForceFinishProjectile(projId, this);
		}
		
}

void Match::ProcessAreaDamage(glm::vec2 pos, float radius, float damage, uint32_t attackerId, bool hitAttacker, ProjectileRules rules)
{
	// Определяем диапазон ячеек, которые попадают в радиус
	glm::ivec2 minCell = WorldToGridCell(pos - glm::vec2(radius));
	glm::ivec2 maxCell = WorldToGridCell(pos + glm::vec2(radius));

	// Ограничиваем в пределах сетки
	minCell = glm::clamp(minCell, glm::ivec2(0), glm::ivec2(gridWidth - 1, gridHeight - 1));
	maxCell = glm::clamp(maxCell, glm::ivec2(0), glm::ivec2(gridWidth - 1, gridHeight - 1));


	uint32_t attackerIdx = GetPlayerIndex(attackerId);
	Character* attackerCh = (attackerIdx != -1) ? entities[attackerIdx].character.get() : nullptr;
	//if (!attackerCh) return;

	float radiusSq = radius * radius;

	// Проходим только по затронутым ячейкам
	for (int y = minCell.y; y <= maxCell.y; ++y) {
		for (int x = minCell.x; x <= maxCell.x; ++x) {
			int cellIdx = y * gridWidth + x;

			// Итерируемся по связанному списку персонажей в ячейке
			for (int cid = spatialGrid[cellIdx].firstCharacter; cid != -1; cid = entities[cid].nextInCell) {
				auto& pl = entities[cid];

				// Базовые проверки
				if (!pl.active || !pl.character || pl.character->IsDead()) continue;
				if (!hitAttacker && pl.character->GetId() == attackerId) continue;
				
				if (attackerId != -1 && pl.character->teamId == attackerCh->teamId) continue;

				// Быстрая проверка дистанции через квадрат (без sqrt)
				glm::vec2 diff = pl.character->position - pos;
				float distSq = glm::dot(diff, diff);
				if (distSq > radiusSq) continue;

				// --- ЛОГИКА ПРИМЕНЕНИЯ УРОНА (как в вашем коде) ---
				auto& ch = pl.character;

				// Отброс
				if (rules.knockbackForce > 0.01f) {
					float dist = std::sqrt(distSq);
					glm::vec2 impactDir = (dist < 0.001f) ? glm::vec2(0, 1) : diff / dist;
					ch->position += impactDir * (rules.knockbackForce * 0.1f);
					ch->knockbackVel += impactDir * rules.knockbackForce * 20.0f;
				}

				// Статусы
				if (rules.effectToApply != StatusEffectType::None) {
					StatusEffect eff;
					eff.type = rules.effectToApply;
					eff.timeLeft = rules.effectDuration;
					eff.value = rules.effectValue;
					eff.nOwnerNetID = attackerId;
					ch->GetEffects()->Add(eff);
					//ch->AddStatusEffect(eff);
				}

				DamageContext ctx;
				ctx.attackerId = attackerId;
				ctx.targetId = (uint32_t)cid;
				ctx.baseDamage = damage;
				ctx.type = rules.type;
				ctx.source = DamageSource::Projectile;
				ApplyDamage(ctx);
			}
		}
	}

	// --- 2. РАЗРУШЕНИЕ ОКРУЖЕНИЯ (Новое) ---
	if (rules.dealsWorldDamage)
	{
		// Определяем границы поиска в сетке уровня
		int minX = (int)std::floor(pos.x - radius);
		int maxX = (int)std::ceil(pos.x + radius);
		int minY = (int)std::floor(pos.y - radius);
		int maxY = (int)std::ceil(pos.y + radius);

		for (int y = minY; y <= maxY; ++y) {
			for (int x = minX; x <= maxX; ++x) {
				glm::ivec2 cell(x, y);
				// Проверяем, попадает ли центр клетки в радиус взрыва
				float dist = glm::distance(glm::vec2(x + 0.5f, y + 0.5f), pos);
				if (dist <= radius) {
					DamageContext worldCtx;
					worldCtx.attackerId = attackerId;
					worldCtx.targetType = DamageTargetType::WorldTile;
					worldCtx.targetCell = cell;
					worldCtx.baseDamage = (float)rules.damageToWorld;
					worldCtx.source = DamageSource::Projectile;
					ApplyDamage(worldCtx);
				}
			}
		}
	}

	// --- 3. ВИЗУАЛЬНЫЙ ЭФФЕКТ ---
	olc::net::message<GameMsg> msg;
	msg.header.id = GameMsg::Game_Explosion;
	msg << pos << radius;
	//MessageAllMatchClients(msg);
	SendAreaDamageToClient(msg, pos);
}

//void Match::HandleShield(DamageContext& ctx, Character& target)
//{
//	auto* shield = target.GetEffect(StatusEffectType::Shield);
//	if (!shield)
//		return;
//
//	int absorbed = std::min(shield->shieldHP, ctx.finalDamage);
//	shield->shieldHP -= absorbed;
//	ctx.finalDamage -= absorbed;
//
//	ctx.absorbedByShield = absorbed > 0;
//
//	if (shield->shieldHP <= 0)
//		target.RemoveEffect(StatusEffectType::Shield);
//
//	if (ctx.finalDamage <= 0)
//		ctx.cancelled = true;
//}

void Match::HandleStatusModifiers(DamageContext& ctx, Character& target)
{
	auto* effectMgr = target.GetEffects();
	if (!effectMgr) return;

	// 1. SHIELD (Логика поглощения урона)
	if (auto* shield = effectMgr->GetEffect(StatusEffectType::Shield))
	{
		int absorbed = std::min(shield->shieldHP, ctx.finalDamage);
		shield->shieldHP -= absorbed;
		ctx.finalDamage -= absorbed;
		ctx.absorbedByShield = absorbed > 0;

		if (shield->shieldHP <= 0)
			effectMgr->Remove(StatusEffectType::Shield);

		if (ctx.finalDamage <= 0) {
			ctx.cancelled = true;
			return; // Дальше проверять эффекты нет смысла, урон обнулен
		}
	}
	if (auto* shield = effectMgr->GetEffect(StatusEffectType::IceShield))
	{
		int absorbed = std::min(shield->shieldHP, ctx.finalDamage);
		shield->shieldHP -= absorbed;
		ctx.finalDamage -= absorbed;
		ctx.absorbedByShield = absorbed > 0;

		if (shield->shieldHP <= 0)
			effectMgr->Remove(StatusEffectType::IceShield);

		if (ctx.finalDamage <= 0) {
			ctx.cancelled = true;
			return; // Дальше проверять эффекты нет смысла, урон обнулен
		}
		if (ctx.attackerId != 0) {
			StatusEffect slow;
			slow.type = StatusEffectType::Slow;
			slow.timeLeft = 1.5f;
			
			if (auto& attacker = entities[idToIndex[ctx.attackerId]].character)
				effectMgr->Add(slow);
		}
	}
	// Проверяем, есть ли на цели эффект "Стигийского пронзателя"
	if (auto* chill = effectMgr->GetEffect(StatusEffectType::StygianChill))
	{
		// Если это Огненное Копье (SunforgeGreatlance)
		// Допустим, мы передаем тип снаряда через ctx или проверяем по урону
		if (ctx.baseDamage >= 20.0f) // Условный порог урона копья T3
		{
			ctx.finalDamage *= 2.0f;     // Тройной урон!
			ctx.type = DamageType::Pure; // Урон становится чистым (игнор брони)

			// Создаем визуальный взрыв "Температурного Шока"
			// server->BroadcastEffect(target.position, EffectType::ThermalShock);

			// Снимаем ледяной эффект (он поглощен взрывом)
			effectMgr->Remove(StatusEffectType::StygianChill);

			// Можно добавить стан на 0.5с от шока
			StatusEffect stun;
			stun.type = StatusEffectType::Stun;
			stun.timeLeft = 0.5f;
			effectMgr->Add(stun);
		}
	}
	bool hasChill = effectMgr->GetEffect(StatusEffectType::StygianChill);
	bool hasBrand = effectMgr->GetEffect(StatusEffectType::SunBrand);

	// Если попал огнем по льду ИЛИ льдом по огню
	if ((ctx.type == DamageType::Magical && hasChill) ||
		(ctx.type == DamageType::Pure && hasBrand))
	{
		ctx.finalDamage *= 3.0f;     // x3 урон
		ctx.type = DamageType::Pure; // Пробивает всё

		effectMgr->Remove(StatusEffectType::StygianChill);
		effectMgr->Remove(StatusEffectType::SunBrand);

		// Доп. микровзрыв для красоты
		//this->ProcessAreaDamage(target.position, 2.5f, 20.0f, ctx.attackerId, false, rules);
	}
	//if (target.HasEffect(StatusEffectType::Silence))
	//{

	//}
	//if (target.HasEffect(StatusEffectType::Slow)) // можно усилить замедление 
	//{

	//}
	//if (target.HasEffect(StatusEffectType::Stun))
	//{

	//}
	//if (target.HasEffect(StatusEffectType::Burn)) // можно усилить  горение
	//{

	//}
}

void Match::OnProjectileBlocked(uint32_t projId, uint32_t blockerProjId, const ProjectileRules& rules)
{
	if (projId >= MAX_PROJECTILES || blockerProjId >= MAX_PROJECTILES) return;

	auto& slot = projectiles[projId];
	auto& blockerSlot = projectiles[blockerProjId];

	//if (slot.data.nOwnerID == blockerSlot.data.nOwnerID) return;
	if (!slot.active || !blockerSlot.active) return;

	// === ЛОГИКА УРОНА ПО СТЕНЕ/ПРЕГРАДЕ ===
	// Если у преграды есть прочность (durability), уменьшаем её
	if (blockerSlot.cachedRules.hasDurability) {
		// Вы можете добавить поле fDurability в sProjectileDescription
		// или вызвать метод SpellManager, чтобы он уменьшил время жизни стены


		DamageContext ctx;
		ctx.attackerId = slot.data.nOwnerID;
		ctx.targetType = DamageTargetType::Projectile; // Новая цель!
		ctx.targetId = blockerProjId;
		ctx.baseDamage = rules.damageToWorld; // Берем урон по строениям
		ctx.source = DamageSource::Projectile;
		ctx.type = rules.type;

		std::cout << "We in Projectile BLocked" << std::endl;
		

		// Если хозяин шара и хозяин стены — один и тот же человек
		if (slot.data.nOwnerID == blockerSlot.data.nOwnerID) {
			// Если это стена или кристалл, шар должен пролететь СКВОЗЬ
			if (blockerSlot.data.type == ProjectileType::Wall)
			{

				return; // ВЫХОДИМ: не наносим урон и НЕ удаляем шар ниже
			}

			// Если мы ударили по своему щиту
			if (blockerSlot.data.type == ProjectileType::Shield)
			{
				//std::cout << "SHIELD BURST TRIGGERED!" << std::endl;

				//// 1. Создаем объект спелла взрыва
				//auto burst = SpellFactory::Create(SpellId::ShieldBurst);
				//
				//// 2. Находим персонажа владельца
				//auto& pl = players[idToIndex[slot.data.nOwnerID]];

				//// 3. Вызываем каст (он возьмет позицию воина и его адаптацию)
				//if (burst->Cast(*pl.character, this)) {
				//	// Удаляем сам щит (он взорвался)
				//	MarkProjectileForRemoval(blockerProjId);
				//	spellManager.ForceFinishProjectile(blockerProjId, this);
				//}
				
				// 4. Удаляем атакующий снаряд (кулак/меч), которым били
				MarkProjectileForRemoval(blockerSlot.data.nOwnerID);
				spellManager.ForceFinishProjectile(blockerSlot.data.nOwnerID, this);
				return;
			}
		
		}

		ApplyDamage(ctx);
	
	}

	// === ЛОГИКА УНИЧТОЖЕНИЯ АТАКУЮЩЕГО СНАРЯДА ===
	if (rules.diesOnWorldCollision || rules.diesOnCharacterCollision) {
		// Проверяем, не удалили ли его уже (например, если был встречный урон)
		if (slot.active && !slot.data.bPendingDestroy) {
			MarkProjectileForRemoval(projId);
			spellManager.ForceFinishProjectile(projId, this);
		}
	}
}

bool Match::IsCellFree(const glm::ivec2& cell) const
{

	// Используем метод IsSolid, который вы уже написали.
		// Если клетка НЕ твердая (!IsSolid), значит она свободная.
	return !level.IsSolid(cell);
}

uint16_t Match::CreateProjectile(ProjectileType type, const ProjectileParams& params, glm::vec2 pos, glm::vec2& baseVel, float& baseRadius)
{
	if (freeProjectileIds.empty()) return 0xFFFF;

	// 2. Логика влияния ветра при "вылете"
 // --- ЛОГИКА ВЕТРА ---
	if (glm::length(baseVel) > 0.1f) {
		glm::vec2 windVec = { cosf(windAngle), sinf(windAngle) };
		glm::vec2 shootDir = glm::normalize(baseVel);
		float dot = glm::dot(shootDir, windVec);
		float windEff = (float)windStrength / 255.0f;
		// Коэффициент влияния (для каждого типа снаряда можно брать из правил)
		   // Например, стрелы (Arrow) сносит сильнее, чем Камни (Rock)
		float windSensitivity = 0.5f;

		if (dot > 0.6f) { // По ветру
			baseVel *= (1.0f + windSensitivity * windEff);
			baseRadius *= 1.5f; // "Раздуваем" эффект
		}
		else if (dot < -0.6f) { // Против ветра
			baseVel *= (1.0f - windSensitivity * 0.5f * windEff);
			baseRadius *= 0.7f; // Сжимаем
		}
	
	}



	uint16_t idx = freeProjectileIds.front();
	freeProjectileIds.pop();

	auto& slot = projectiles[idx];
	slot.data = {}; // Сброс старых данных cоздание нового проджектайла 
	slot.data.nUniqueID = idx;
	slot.data.type = type;
	slot.data.nOwnerID = params.ownerId;
	
	slot.data.vPos = pos;
	slot.data.vVel = { 0, 0 }; // Для ваших Spell-стадий роста оставляем 0
	slot.data.fRadius = 0.0f;   // Начнет расти с нуля в UpdateAppear
	slot.data.bPendingDestroy = false;

	// Кэшируем правила прямо здесь, так как GameServer знает о GetProjectileRules
	slot.cachedRules = GetProjectileRules(type);
	slot.cachedRules.damageToPlayers *= params.damageMod; // учитываем усиление или ослобления от класс к базовому урону 
	slot.cachedRules.effectValue *= params.effectMod; //усиливаем горение/замедление и тд от модификаторов мага и других
	slot.active = true;
	

	// Если по ветру — удваиваем урон в кэше правил этого конкретного снаряда
	if (glm::dot(glm::normalize(baseVel), { cosf(windAngle), sinf(windAngle) }) > 0.6f) {

		slot.cachedRules.damageToWorld *= 1.2;
		slot.cachedRules.damageToPlayers *= 1.2;
	}

	activeProjectileIndices.push_back(idx);



	

	return idx;
}
