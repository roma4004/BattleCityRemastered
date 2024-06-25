#pragma once

class IGame
{
public:
	IGame(const IGame& other) = delete;
	IGame(IGame&& other) noexcept = delete;
	IGame() = default;
	virtual ~IGame() = default;

	virtual void MainLoop() = 0;
	[[nodiscard]] virtual int Result() const = 0;

	IGame& operator=(const IGame& other) = delete;
	IGame& operator=(IGame&& other) noexcept = delete;
};