#include "../../headers/input/InputProviderForPlayerOne.h"
#include "../../headers/EventSystem.h"

InputProviderForPlayerOne::InputProviderForPlayerOne(std::string name, std::shared_ptr<EventSystem> events)
	: _name{std::move(name)}, _events{std::move(events)}
{
	_events->AddListener("W_Pressed", _name, [&btn = playerKeys]() { btn.up = true; });
	_events->AddListener("W_Released", _name, [&btn = playerKeys]() { btn.up = false; });
	_events->AddListener("A_Pressed", _name, [&btn = playerKeys]() { btn.left = true; });
	_events->AddListener("A_Released", _name, [&btn = playerKeys]() { btn.left = false; });
	_events->AddListener("S_Pressed", _name, [&btn = playerKeys]() { btn.down = true; });
	_events->AddListener("S_Released", _name, [&btn = playerKeys]() { btn.down = false; });
	_events->AddListener("D_Pressed", _name, [&btn = playerKeys]() { btn.right = true; });
	_events->AddListener("D_Released", _name, [&btn = playerKeys]() { btn.right = false; });
	_events->AddListener("Space_Pressed", _name, [&btn = playerKeys]() { btn.shot = true; });
	_events->AddListener("Space_Released", _name, [&btn = playerKeys]() { btn.shot = false; });
}

InputProviderForPlayerOne::~InputProviderForPlayerOne()
{
	_events->RemoveListener("W_Pressed", _name);
	_events->RemoveListener("W_Released", _name);
	_events->RemoveListener("A_Pressed", _name);
	_events->RemoveListener("A_Released", _name);
	_events->RemoveListener("S_Pressed", _name);
	_events->RemoveListener("S_Released", _name);
	_events->RemoveListener("D_Pressed", _name);
	_events->RemoveListener("D_Released", _name);
	_events->RemoveListener("Space_Pressed", _name);
	_events->RemoveListener("Space_Released", _name);
}
