#include "Options.h"
#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/RenderUIEvents.h"

/*
 * План меню опций:
 * Юзаем фон менюхи и тайлы для навигации + вывод текста для визуала
 * Добавляем новую cтроку Options в Меню +
 * Добавляем новый тайл для интеракта мышью, увеличиваем высоту общего бэкплэйтТайла +
 * При транзишене в Опции паузим игру в демоГеймМоде и фолсим флаг показа меню и статистики
 * Отрисовываем опции и визуальные тайлы свитчеров
 * Добавляем функциональные методы каждой опции и подписки на них
 * Добавляем навигацию по опциям как в менюхе
 * Добавляем кнопку выхода для мыши и выход по эскейпу с клавы/пада
 * При транзишене дисэйблим отрисовку Опций и врубаем отрисовку меню и снимаем паузу с демоГеймМода +
 */

struct EnterToTheOptions;

Options::Options(UPoint windowSize, const std::shared_ptr<EventSystem>& events)
	: _pos{.x = 25, .y = 25}
	  , _windowHeight{static_cast<int>(windowSize.y)}
	  , _yOffsetStart{static_cast<int>(windowSize.y)}
	  , _name{std::string("Options")}
	  , _selectedGameMode{GameMode::EndIterator}
	  , _events{events}
{
	Subscribe();
	_windowHeight = static_cast<int>(windowSize.y);
}

Options::~Options()
= default;

void Options::Subscribe()
{
	//TODO: replace this listener from "...changedTo" to menu items as in Menu

	_subs.push_back(_events->AddListener(this, &Options::OnOptionsShown));
	_subs.push_back(_events->AddListener(this, &Options::OnMenuShown));
}

void Options::OnOptionsShown(const EnterToTheOptions& event)
{
	DisplayOptions(true);
}

void Options::OnMenuShown(const MenuShownEvent& event)
{
	if (event.isShown)
	{
		DisplayOptions(false);
	}
}

void Options::DisplayOptions(const bool isDisplayed)
{
	_isOptionsDisplayed = isDisplayed;
}

void Options::DrawTextLine(Point& posText, std::string text) const
{
}

void Options::Draw() const
{
	if (_isOptionsDisplayed)
	_events->EmitEvent(RenderMenuBackgroundEvent{});
	//Here must be added points of options
}
