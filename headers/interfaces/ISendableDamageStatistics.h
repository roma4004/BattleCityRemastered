#pragma once

#include <string>

class ISendableDamageStatistics
{
protected:
	virtual ~ISendableDamageStatistics() = default;

public:
	virtual void SendDamageStatistics(const std::string& author, const std::string& fraction) = 0;
};
