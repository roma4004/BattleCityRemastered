#include "../../headers/input/InputProviderForPlayerOneNet.h"

InputProviderForPlayerOneNet::InputProviderForPlayerOneNet(std::string name, std::shared_ptr<EventSystem> events)
	: _name{std::move(name)}, _events{std::move(events)}
{
	_events->AddListener("Net_W_Pressed", _name, [&btn = playerKeys]() { btn.up = true; });
	_events->AddListener("Net_W_Released", _name, [&btn = playerKeys]() { btn.up = false; });
	_events->AddListener("Net_A_Pressed", _name, [&btn = playerKeys]() { btn.left = true; });
	_events->AddListener("Net_A_Released", _name, [&btn = playerKeys]() { btn.left = false; });
	_events->AddListener("Net_S_Pressed", _name, [&btn = playerKeys]() { btn.down = true; });
	_events->AddListener("Net_S_Released", _name, [&btn = playerKeys]() { btn.down = false; });
	_events->AddListener("Net_D_Pressed", _name, [&btn = playerKeys]() { btn.right = true; });
	_events->AddListener("Net_D_Released", _name, [&btn = playerKeys]() { btn.right = false; });
	_events->AddListener("Net_Space_Pressed", _name, [&btn = playerKeys]() { btn.shot = true; });
	_events->AddListener("Net_Space_Released", _name, [&btn = playerKeys]() { btn.shot = false; });
}

InputProviderForPlayerOneNet::~InputProviderForPlayerOneNet()
{
	_events->RemoveListener("Net_W_Pressed", _name);
	_events->RemoveListener("Net_W_Released", _name);
	_events->RemoveListener("Net_A_Pressed", _name);
	_events->RemoveListener("Net_A_Released", _name);
	_events->RemoveListener("Net_S_Pressed", _name);
	_events->RemoveListener("Net_S_Released", _name);
	_events->RemoveListener("Net_D_Pressed", _name);
	_events->RemoveListener("Net_D_Released", _name);
	_events->RemoveListener("Net_Space_Pressed", _name);
	_events->RemoveListener("Net_Space_Released", _name);
}
