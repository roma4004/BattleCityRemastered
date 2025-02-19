#include "../../headers/input/InputProviderForPlayerOneNet.h"
#include "../../headers/EventSystem.h"

InputProviderForPlayerOneNet::InputProviderForPlayerOneNet(std::string name, std::shared_ptr<EventSystem> events)
	: _name{std::move(name)}, _events{std::move(events)}
{
	_events->AddListener("ServerReceive_W_Pressed", _name, [&btn = playerKeys]() { btn.up = true; });
	_events->AddListener("ServerReceive_W_Released", _name, [&btn = playerKeys]() { btn.up = false; });
	_events->AddListener("ServerReceive_A_Pressed", _name, [&btn = playerKeys]() { btn.left = true; });
	_events->AddListener("ServerReceive_A_Released", _name, [&btn = playerKeys]() { btn.left = false; });
	_events->AddListener("ServerReceive_S_Pressed", _name, [&btn = playerKeys]() { btn.down = true; });
	_events->AddListener("ServerReceive_S_Released", _name, [&btn = playerKeys]() { btn.down = false; });
	_events->AddListener("ServerReceive_D_Pressed", _name, [&btn = playerKeys]() { btn.right = true; });
	_events->AddListener("ServerReceive_D_Released", _name, [&btn = playerKeys]() { btn.right = false; });
	_events->AddListener("ServerReceive_Space_Pressed", _name, [&btn = playerKeys]() { btn.shot = true; });
	_events->AddListener("ServerReceive_Space_Released", _name, [&btn = playerKeys]() { btn.shot = false; });
}

InputProviderForPlayerOneNet::~InputProviderForPlayerOneNet()
{
	_events->RemoveListener("ServerReceive_W_Pressed", _name);
	_events->RemoveListener("ServerReceive_W_Released", _name);
	_events->RemoveListener("ServerReceive_A_Pressed", _name);
	_events->RemoveListener("ServerReceive_A_Released", _name);
	_events->RemoveListener("ServerReceive_S_Pressed", _name);
	_events->RemoveListener("ServerReceive_S_Released", _name);
	_events->RemoveListener("ServerReceive_D_Pressed", _name);
	_events->RemoveListener("ServerReceive_D_Released", _name);
	_events->RemoveListener("ServerReceive_Space_Pressed", _name);
	_events->RemoveListener("ServerReceive_Space_Released", _name);
}
