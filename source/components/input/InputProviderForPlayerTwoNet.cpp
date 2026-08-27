#include "components/input/InputProviderForPlayerTwoNet.h"
#include "components/EventSystem.h"
#include "components/events/InputEvents.h"
#include "enums/PlayerSlot.h"

InputProviderForPlayerTwoNet::InputProviderForPlayerTwoNet(const std::shared_ptr<EventSystem>& events)
	: _events{events} {}

void InputProviderForPlayerTwoNet::Subscribe()
{
	_subs.push_back(_events->AddListener(Key(PlayerSlot::P2), this, &InputProviderForPlayerTwoNet::OnMoveUp));
	_subs.push_back(_events->AddListener(Key(PlayerSlot::P2), this, &InputProviderForPlayerTwoNet::OnMoveLeft));
	_subs.push_back(_events->AddListener(Key(PlayerSlot::P2), this, &InputProviderForPlayerTwoNet::OnMoveDown));
	_subs.push_back(_events->AddListener(Key(PlayerSlot::P2), this, &InputProviderForPlayerTwoNet::OnMoveRight));
	_subs.push_back(_events->AddListener(Key(PlayerSlot::P2), this, &InputProviderForPlayerTwoNet::OnFire));
}

void InputProviderForPlayerTwoNet::OnMoveUp(const ServerInMoveUpEvent& event) { _playerKeys.up = event.isPressed; }

void InputProviderForPlayerTwoNet::OnMoveLeft(const ServerInMoveLeftEvent& event)
{
	_playerKeys.left = event.isPressed;
}

void InputProviderForPlayerTwoNet::OnMoveDown(const ServerInMoveDownEvent& event)
{
	_playerKeys.down = event.isPressed;
}

void InputProviderForPlayerTwoNet::OnMoveRight(const ServerInMoveRightEvent& event)
{
	_playerKeys.right = event.isPressed;
}

void InputProviderForPlayerTwoNet::OnFire(const ServerInFireEvent& event) { _playerKeys.shot = event.isPressed; }

void InputProviderForPlayerTwoNet::Enable()
{
	Subscribe();
}

void InputProviderForPlayerTwoNet::Disable()
{
	_subs.clear();
}
