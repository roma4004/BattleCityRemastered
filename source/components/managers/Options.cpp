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
, _name{std::string("Options")}
{
	Subscribe();

	std::shared_ptr<EventSystem> _events{nullptr};
	_windowHeight = static_cast<int>(windowSize.y);
}

Options::~Options()
{
	Unsubscribe();
}

void Options::Subscribe()
{
	
}

void Options::Unsubscribe() const { _events->RemoveAllListeners(_name); }

void Options::DisplayOptions(bool isDisplayed)
{
	_isOptionsDisplayed = isDisplayed;

	if (_isOptionsDisplayed)
	{
		_events->AddListener("DrawOptions", _name, [this]() { this->Draw(); });
	}
	else
	{
		_events->RemoveListener("DrawOptions", _name);
	}
}

void Options::DrawTextLine(Point& posText, std::string text) const
{
	constexpr unsigned int color = {0xffffffffu};
	_events->EmitEvent("RenderText", posText, color, text);
	posText.y += 30;
}

void Options::Draw()
{
	// first time animation, slow scrolling from bottom corner to vertical center
	if (constexpr int yOffsetEnd = 0; _yOffsetStart > yOffsetEnd)
	{
		_yOffsetStart -= 3;
		_pos.y = _padding + _yOffsetStart;
		_events->EmitEvent("MenuPosChanged", _pos);
	}

	_events->EmitEvent("RenderMenuBackground", _pos);
}
