#include "components/Options.h"
#include "components/EventSystem.h"

/* 
 * План меню опций:
 * Юзаем фон менюхи и тайлы для навигации + вывод текста для визуала
 * Добавляем новую троку Options в Меню 
 * Добавляем новый тайл для интеракта мышью, увеличиваем высоту общего бэкплэйтТайла
 * При транзишене в Опции паузим игру в демоГеймМоде и фолсим флаг показа меню и статистики
 * Отрисовываем опции и визуальные тайлы свитчеров
 * Добавляем функциональные методы каждой опции и подписки на них
 * Добавляем навигацию по опциям как в менюхе
 * Добавляем кнопку выхода для мыши и выход по эскейпу с клавы/пада
 * При транзишене дисэйблим отрисовку Опций и врубаем отрисовку меню и снимаем паузу с демоГеймМода
 */

Options::Options(UPoint windowSize, const std::shared_ptr<EventSystem>& events) 
: _events{events}
, _pos{.x = 25, .y = 25}
, _name{std::string("Options")}
{
	Subscribe();
	_windowHeight = static_cast<int>(windowSize.y);
}

Options::~Options()
{
	Unsubscribe();
}

void Options::Subscribe()
{
	//NOTE: avoid showing score and menu at the same time
	_events->AddListener("MenuShowed", _name, [this](const bool isDisplayed)
	{
		_isMenuDisplayed = isDisplayed;
	});
	_events->AddListener("SelectedGameModeChangedTo", _name, [this](const GameMode newGameMode)
	{
		this->_gameMode = newGameMode;
		if (_gameMode == GameMode::EndIterator)
		{
			this->_isOptionsDisplayed = true;
		}
		else
		{
			this->_isOptionsDisplayed = false;
		}
	});

		_events->AddListener("DrawUserInterface", _name, [this]() { this->Draw(); });
}

void Options::Unsubscribe() const { _events->RemoveAllListeners(_name); }

void Options::DrawTextLine(Point& posText, std::string text) const
{
	constexpr unsigned int color = {0xffffffffu};
	_events->EmitEvent("RenderText", posText, color, text);
	posText.y += 30;
}

void Options::Draw()
{
	if (_isMenuDisplayed == false && _isOptionsDisplayed)
	{
		_events->EmitEvent("RenderMenuBackground", _pos);
	}
}
