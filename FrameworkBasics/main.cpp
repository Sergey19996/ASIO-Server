#include <olc_net.h>
#include <unordered_map>
#include "../NetServer/gameLevel.h"
#include <GLFW/glfw3.h>

gameLevel* GameLevel;
const float PLAYER_VELOCITY(10.0f);

#define SCREENWIDTH 800
#define SCREENHEIGHT 608
const int BRICK_SIZE = 32;
class GameServer : public olc::net::server_interface<GameMsg>
{
public:
	GameServer(uint16_t nPort) : olc::net::server_interface<GameMsg>(nPort)
	{
		GameLevel = new gameLevel(SCREENWIDTH / BRICK_SIZE, SCREENHEIGHT / BRICK_SIZE);


	}

	std::unordered_map<uint32_t, sPlayerDescription> m_mapRoster;
	std::unordered_map<uint32_t, sProjectileDescription> mapProjectiles;
	std::vector<uint32_t> m_vGarbageIDs;

	float felapsedTime = 0.0f;

	void UpdatePositions() {

		
		// Update objects locally
		for (auto& object : m_mapRoster)
		{
			// ѕотенциальна€ позици€ после движени€
			glm::vec2 vPotentialPosition = object.second.vPos + object.second.vVel * felapsedTime * PLAYER_VELOCITY;

			// учитываем центр круга
			glm::vec2 vCircleCenter = vPotentialPosition + glm::vec2(object.second.fRadius);

			// “екуща€ и целева€ клетка
			glm::ivec2 vCurrentCell = glm::floor(object.second.vPos);  // ѕреобразуем float -> int через floor
			glm::ivec2 vTargetCell = glm::floor(vPotentialPosition);

			// ¬ерхний левый и нижний правый угол области проверки   1;1 лево верх      1;1 право низ дополнительные клетки 
			glm::ivec2 vAreaTL = glm::max(glm::min(vCurrentCell, vTargetCell) - glm::ivec2(1, 1), glm::ivec2(0, 0));
			glm::ivec2 vAreaBR = glm::min(glm::max(vCurrentCell, vTargetCell) + glm::ivec2(1, 1), GameLevel->ScreenSize);


			// ѕеребор всех клеток в области
			for (int y = vAreaTL.y; y <= vAreaBR.y; ++y)
			{
				for (int x = vAreaTL.x; x <= vAreaBR.x; ++x)
				{
					// ѕроверка коллизии с клеткой (например, символ стены)
					if (GameLevel->LevelData[y * GameLevel->ScreenSize.x + x] == '#')
					{
						glm::vec2 vNearestPoint;
						// Inspired by this (very clever btw) 
						// https://stackoverflow.com/questions/45370692/circle-rectangle-collision-response
						vNearestPoint.x = std::max(float(x), std::min(vCircleCenter.x, float(x + 1)));
						vNearestPoint.y = std::max(float(y), std::min(vCircleCenter.y, float(y + 1)));

						//glm::ivec2 vAreaTL = glm::max(glm::min(vCurrentCell, vTargetCell) - glm::ivec2(1, 1), glm::ivec2(0, 0));
					//	glm::ivec2 vAreaBR = glm::min(glm::max(vCurrentCell, vTargetCell) + glm::ivec2(1, 1), GameLevel->ScreenSize);


						// But modified to work :P

						glm::vec2 vRayToNearest = vNearestPoint - vCircleCenter;
						float fOverlap = object.second.fRadius - glm::length(vRayToNearest);
						if (std::isnan(fOverlap)) fOverlap = 0;// Thanks Dandistine!

						// If overlap is positive, then a collision has occurred, so we displace backwards by the 
						// overlap amount. The potential position is then tested against other tiles in the area
						// therefore "statically" resolving the collision
						if (fOverlap > 0)
						{
							// Statically resolve the collision
							vPotentialPosition = vPotentialPosition - glm::normalize(vRayToNearest) * fOverlap;
						}


					}


				}

			}


			// Set the objects new position to the allowed potential position
			object.second.vPos = vPotentialPosition;

		};

	

		olc::net::message<GameMsg> msgUpdate;
		msgUpdate.header.id = GameMsg::Game_UpdatePlayer;  // отдельный ID дл€ массового обновлени€

		

		for (auto& [id, desc] : m_mapRoster)
		{
	
			msgUpdate << desc;
		}
		uint16_t numPlayers = m_mapRoster.size();
		msgUpdate << numPlayers;
		MessageAllClients(msgUpdate);

	


		for (auto& object : mapProjectiles)
		{
			// ѕотенциальна€ позици€ после движени€
			glm::vec2 vPotentialPosition = object.second.vPos + object.second.vVel * felapsedTime * PLAYER_VELOCITY;


			std::cout << " Projectile Position : " << object.second.vPos.x << " Projectile Velocity : " << object.second.vVel.x << std::endl;
			// учитываем центр круга
			glm::vec2 vCircleCenter = vPotentialPosition + glm::vec2(object.second.fRadius);

			// “екуща€ и целева€ клетка
			glm::ivec2 vCurrentCell = glm::floor(object.second.vPos);  // ѕреобразуем float -> int через floor
			glm::ivec2 vTargetCell = glm::floor(vPotentialPosition);

			// ¬ерхний левый и нижний правый угол области проверки   1;1 лево верх      1;1 право низ дополнительные клетки 
			glm::ivec2 vAreaTL = glm::max(glm::min(vCurrentCell, vTargetCell) - glm::ivec2(1, 1), glm::ivec2(0, 0));
			glm::ivec2 vAreaBR = glm::min(glm::max(vCurrentCell, vTargetCell) + glm::ivec2(1, 1), GameLevel->ScreenSize);


			// ѕеребор всех клеток в области
			for (int y = vAreaTL.y; y <= vAreaBR.y; ++y)
			{
				for (int x = vAreaTL.x; x <= vAreaBR.x; ++x)
				{
					// ѕроверка коллизии с клеткой (например, символ стены)
					if (GameLevel->LevelData[y * GameLevel->ScreenSize.x + x] == '#')
					{
						glm::vec2 vNearestPoint;
						// Inspired by this (very clever btw) 
						// https://stackoverflow.com/questions/45370692/circle-rectangle-collision-response
						vNearestPoint.x = std::max(float(x), std::min(vCircleCenter.x, float(x + 1)));
						vNearestPoint.y = std::max(float(y), std::min(vCircleCenter.y, float(y + 1)));

						//glm::ivec2 vAreaTL = glm::max(glm::min(vCurrentCell, vTargetCell) - glm::ivec2(1, 1), glm::ivec2(0, 0));
					//	glm::ivec2 vAreaBR = glm::min(glm::max(vCurrentCell, vTargetCell) + glm::ivec2(1, 1), GameLevel->ScreenSize);


						// But modified to work :P

						glm::vec2 vRayToNearest = vNearestPoint - vCircleCenter;
						float fOverlap = object.second.fRadius - glm::length(vRayToNearest);
						if (std::isnan(fOverlap)) fOverlap = 0;// Thanks Dandistine!

						// If overlap is positive, then a collision has occurred, so we displace backwards by the 
						// overlap amount. The potential position is then tested against other tiles in the area
						// therefore "statically" resolving the collision
						if (fOverlap > 0)
						{
							// Statically resolve the collision
							vPotentialPosition = vPotentialPosition - glm::normalize(vRayToNearest) * fOverlap;
						}


					}


				}
			}

			// Set the objects new position to the allowed potential position
			object.second.vPos = vPotentialPosition;
		}


	}

	


protected:
	bool OnClientConnect(std::shared_ptr<olc::net::connection<GameMsg>> client) override
	{
		// For now we will allow all 
		return true;
	}

	void OnClientValidated(std::shared_ptr<olc::net::connection<GameMsg>> client) override
	{
		// Client passed validation check, so send them a message informing
		// them they can continue to communicate
		olc::net::message<GameMsg> msg;
		msg.header.id = GameMsg::Client_Accepted;
		client->Send(msg);
	}

	void OnClientDisconnect(std::shared_ptr<olc::net::connection<GameMsg>> client) override
	{
		if (client)
		{
			if (m_mapRoster.find(client->GetID()) == m_mapRoster.end())
			{
				// client never added to roster, so just let it disappear
			}
			else
			{
				auto& pd = m_mapRoster[client->GetID()];
				std::cout << "[UNGRACEFUL REMOVAL]:" + std::to_string(pd.nUniqueID) + "\n";
				m_mapRoster.erase(client->GetID());
				m_vGarbageIDs.push_back(client->GetID());
			}
		}

	}

	void OnMessage(std::shared_ptr<olc::net::connection<GameMsg>> client,  olc::net::message<GameMsg>& msg) override
	{
		if (!m_vGarbageIDs.empty())
		{
			for (auto pid : m_vGarbageIDs)
			{
				olc::net::message<GameMsg> m;
				m.header.id = GameMsg::Game_RemovePlayer;
				m << pid;
				std::cout << "Removing " << pid << "\n";
				MessageAllClients(m);
			}
			m_vGarbageIDs.clear();
		}



		switch (msg.header.id)
		{
		case GameMsg::Client_RegisterWithServer:
		{
			sPlayerDescription desc;
			msg >> desc;
			desc.nUniqueID = client->GetID();
			m_mapRoster.insert_or_assign(desc.nUniqueID, desc);

			olc::net::message<GameMsg> msgSendID;
			msgSendID.header.id = GameMsg::Client_AssignID;
			msgSendID << desc.nUniqueID;
			MessageClient(client, msgSendID);

			olc::net::message<GameMsg> msgAddPlayer;
			msgAddPlayer.header.id = GameMsg::Game_AddPlayer;
			msgAddPlayer << desc;
			MessageAllClients(msgAddPlayer);

			for (const auto& player : m_mapRoster)
			{
				olc::net::message<GameMsg> msgAddOtherPlayers;
				msgAddOtherPlayers.header.id = GameMsg::Game_AddPlayer;
				msgAddOtherPlayers << player.second;
				MessageClient(client, msgAddOtherPlayers);
			}

			break;
		}

		case GameMsg::Client_UnregisterWithServer:
		{
			break;
		}

		case GameMsg::Game_UpdatePlayer:
		{
		
			sPlayerDescription desc; //  создаЄм пустой проджектайл
			glm::vec2 vel; // создаЄм пустую vel
			msg >> vel; // записываем изменени€ по vtk
			m_mapRoster[client->GetID()].vVel = vel; // у прибывшего клиента измен€ем vel
		//	std::cout << desc.vVel.x << " \t" << desc.vVel.y << std::endl;

			//.insert_or_assign(client->GetID(), desc); // так же внедр€ем в мапу проджектайл
			
			
		
			break;
		}
		case(GameMsg::chat_message): {
			/*uint32_t senderID;
			std::string text;

			msg >> text;
			msg >> senderID;

			sChatMessage

			std::cout << "chatMessage comming" << std::endl;*/
			MessageAllClients(msg, client);
			break;
		}
		case(GameMsg::Game_AddProjectile):
		{
			sProjectileDescription desc;  // создаем проджектайл пустышку
			msg >> desc;  //  полуенный пакет записываем в пустышку там уже есть данные по idowner но нет idserver


			desc.nUniqueID = nProjectileIDCounter; // устанавливаем id сервера
			nProjectileIDCounter++; //двигаем счЄтчик дл€ след присваивани€

			mapProjectiles.insert_or_assign(desc.nUniqueID, desc); // по нему внедр€ем в map на сервере

			olc::net::message<GameMsg> msgProjId;  // cоздаем пустое сообщение дл€ всех клиентов
			msgProjId.header.id = GameMsg::Game_AddProjectile; // говорим куда попадет
			msgProjId << desc;  // в пустышку записываем наши данные об проджектайле

			std::cout << "Projectile added" << desc.nUniqueID << std::endl;

			MessageAllClients(msgProjId); //рассылаем всем

			break;
		}
		}

	}

};



int main()
{
	GameServer server(60000);
	server.Start();
	glfwInit(); // дл€   glfwGetTime()


	float timeValue = glfwGetTime(); // ѕолучаем начальное врем€

	//const float AnimationTimer = 0.2f;
	float timer = 0.0f;
	
	while (1)
	{
		float LastTime = timeValue;  // —охран€ем предыдущее врем€
		timeValue = glfwGetTime(); // ѕолучаем текущее врем€
		server.felapsedTime = timeValue - LastTime;
		
		timer += server.felapsedTime;
		
		server.Update(-1, true);
	//	server.UpdatePositions(felapsedTime);
		server.UpdatePositions();  // просчитывем коллизию с локацией  и обнавл€ем позицию


		

	}

		return 0;
	
}