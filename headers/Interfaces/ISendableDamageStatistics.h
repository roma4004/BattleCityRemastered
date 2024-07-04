#pragma once
#include <string>

class ISendableDamageStatistics
{
public:
	virtual ~ISendableDamageStatistics() = default;
	virtual void SendDamageStatistics(const std::string& author, const std::string& fraction) = 0;
};
