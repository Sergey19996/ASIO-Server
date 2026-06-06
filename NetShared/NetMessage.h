#pragma once
#include <vector>
#include <string>

struct sChatMessage
{
	uint32_t nSenderID;            // ID отправителя (тот же, что в sPlayerDescription.nUniqueID)
	std::string sText;             // Сам текст
};
