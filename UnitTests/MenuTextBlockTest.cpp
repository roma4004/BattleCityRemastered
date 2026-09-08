#include "application/GameConfig.h"
#include "components/EventSystem.h"
#include "components/Menu.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/RenderUIEvents.h"
#include "gtest/gtest.h"
#include <memory>
#include <vector>

//NOTE: the block is built whole and the renderer clips it, so what the renderer measures to pick
//a point size is exactly what the block sent it
class MenuTextBlockTest : public testing::Test
{
protected:
	std::shared_ptr<EventSystem> _events{std::make_shared<EventSystem>()};
	GameConfig _gameConfig{};
	std::unique_ptr<Menu> _menu{nullptr};
	std::vector<RenderMenuTextBlockEvent> _blocks{};
	std::vector<EventSubscription> _subs{};

	void SetUp() override
	{
		_subs.push_back(_events->AddListener([this](const RenderMenuTextBlockEvent& event)
		{
			_blocks.push_back(event);
		}));

		_menu = std::make_unique<Menu>(_events, _gameConfig);
		_events->EmitEvent(MenuShowedEvent{.isShown = true});
	}
};

//NOTE: the first frames slide the block up from the bottom corner - settle that animation before
//asserting on steady-state positions
constexpr int kFramesToSettleSlideIn{250};

// Nothing falls off the bottom once the menu has arrived - a line added to the hints would
TEST_F(MenuTextBlockTest, AllLinesFitOnTheClassicScreenOnceSettled)
{
	for (int frame{0}; frame < kFramesToSettleSlideIn; ++frame)
	{
		_events->EmitEvent(DrawUserInterfaceEvent{});
	}

	const int logicalHeight = static_cast<int>(_gameConfig.LogicalSize().y);
	for (const TextBlockLine& line: _blocks.back().lines)
	{
		EXPECT_LT(line.pos.y, logicalHeight);
	}
}

// The menu starts below the screen. Dropping off-screen lines would fit the point size to a block
// that grows as the menu slides in, and its widest lines are the last to arrive.
TEST_F(MenuTextBlockTest, TheBlockIsWholeWhileTheMenuIsStillSlidingIn)
{
	_events->EmitEvent(DrawUserInterfaceEvent{});
	const std::size_t whileSliding = _blocks.back().lines.size();

	for (int frame{0}; frame < kFramesToSettleSlideIn; ++frame)
	{
		_events->EmitEvent(DrawUserInterfaceEvent{});
	}

	EXPECT_EQ(whileSliding, _blocks.back().lines.size()) << "the block changed size under the animation";
}
