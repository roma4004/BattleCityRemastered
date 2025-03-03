#pragma once

#include "../BaseObj.h"
#include "../Point.h"
#include "../application/Window.h"

#include <memory>

struct ObjRectangle;
class EventSystem;

class WaterTile final : public BaseObj
{
	std::shared_ptr<Window> _window{nullptr};
	std::shared_ptr<EventSystem> _events{nullptr};
	std::string _name;
	int _id{0};

	void Subscribe() const;
	void Unsubscribe() const;

	void Draw() const override;

	void SendDamageStatistics(const std::string& author, const std::string& fraction) override;

public:
	WaterTile(const ObjRectangle& rect, std::shared_ptr<Window> window, std::shared_ptr<EventSystem> events, int id);

	~WaterTile() override;

	[[nodiscard]] std::string GetName() const;
	[[nodiscard]] int GetId() const;
};
