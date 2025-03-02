#pragma once

#include "../BaseObj.h"
#include "../Point.h"
#include "../application/Window.h"

#include <memory>

struct ObjRectangle;
class EventSystem;

//TODO: implement that steel wall can be destroyer when player have 3 bonus stars
class SteelWall final : public BaseObj
{
	std::shared_ptr<Window> _window{nullptr};
	std::shared_ptr<EventSystem> _events{nullptr};
	std::string _name;
	int _id{0};

	void Subscribe() const;
	void Unsubscribe() const;

	void SetPixel(size_t x, size_t y, int color) const;

	void Draw() const override;

	void SendDamageStatistics(const std::string& author, const std::string& fraction) override;

public:
	SteelWall(const ObjRectangle& rect, std::shared_ptr<Window> window, std::shared_ptr<EventSystem> events,
	          int id);

	~SteelWall() override;

	[[nodiscard]] std::string GetName() const;
	[[nodiscard]] int GetId() const;
};
