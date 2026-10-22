#pragma once

#include "components/UiTable.h"
#include "enums/BonusType.h"
#include "enums/UiIcon.h"
#include "geometry/Point.h"
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

struct MenuShownEvent
{
	bool isShown;
};

//NOTE: the keyboard types while it is up
struct ServerScreenShownEvent
{
	bool isShown;
};

struct ShowMenuEvent
{
	bool isShown;
};

//NOTE: how far below its resting place the panel still is - only the menu slides in
struct RenderMenuBackgroundEvent
{
	int slide{};
};

//NOTE: the picked table's rows show a window over a longer list; a list that fits has no bar but keeps its room
struct PanelScroll
{
	std::size_t firstRow{};
	std::size_t rowCount{};
	//NOTE: counted in the list's own items - one may take more than a row
	std::size_t firstShown{};
	std::size_t shownCount{};
	std::size_t total{};
};

//NOTE: drawn over a picked table row's text, so it never moves it
struct PanelCaret
{
	std::size_t row{};
	std::size_t symbol{};
	//NOTE: 0 draws a bar before the symbol, more underline that many symbols - a picked column, not a caret
	std::size_t symbols{};
	std::uint8_t alpha{};
};

//NOTE: the list dropped under the picked row; while open, PanelRowsPlacedEvent reports its items and that row only
struct PanelDropDown
{
	std::size_t row{};
	UiTable items{};
	std::size_t picked{};
};

//NOTE: the arrow marks the selected row of this table, and its rows' places go out as PanelRowsPlacedEvent
struct PanelPick
{
	std::size_t table{};
	std::size_t selectedRow{};
	std::optional<PanelScroll> scroll{};
	std::optional<PanelCaret> caret{};
	std::optional<PanelDropDown> dropDown{};
};

//NOTE: tables stacked top to bottom in the middle of the panel, each centered across it, one font size for all
struct RenderPanelTablesEvent
{
	std::vector<UiTable> tables{};
	std::optional<PanelPick> pick{};
};

//NOTE: menu content only - where the panel stands and every pixel of it are the renderer's job
struct RenderMenuEvent
{
	int slide{};
	int selectedRow{};
	UiTable title{};
	UiTable modes{};
	UiTable controls{};
};

//NOTE: where the renderer drew the menu items, so a mouse click hits them; the font size is the renderer's choice
struct MenuTilesPlacedEvent
{
	//NOTE: the top left of each tile, in the order the modes stand in
	std::vector<Point> tiles{};
	Point tileSize{};
};

//NOTE: the same for the rows of a picked panel table
struct PanelRowsPlacedEvent
{
	std::vector<Point> rows{};
	Point rowSize{};
	//NOTE: the font is monospaced, so how far across a row a click is reads as a symbol of its text
	int symbolWidth{};
};


//NOTE: a plate over the field - the pause, or the end of a match nobody shows a scoreboard for
struct RenderPlateEvent
{
	UiIcon plate{};
};

//NOTE: near-black side bar text; alpha spelled out - IntToColor reads it from the top byte, and 0 is invisible
inline constexpr unsigned int kSideBarTextColor{0xff000002u};

struct SideBarSeat
{
	unsigned short lives{};
	std::optional<float> frozen{};
	std::optional<float> helmet{};
};

//NOTE: the reserve fills the enemy frame from the top; badges take the cells it left empty
inline constexpr int kEnemyFrameCells{20};

//NOTE: an enemy's helmet or free-for-all timer, in the frame cell it took when it started
struct SideBarEnemyBadge
{
	int cell{};
	BonusType type{};
	std::optional<float> spent{};
};

struct SideBarEnemies
{
	unsigned short reserve{};
	std::vector<SideBarEnemyBadge> badges{};
	std::optional<float> teamFrozen{};
};

struct SideBarPlayers
{
	std::vector<SideBarSeat> seats{};
	unsigned short stage{};
	std::optional<float> fortified{};
	//NOTE: while a bot drives the client's seat; it replaces the flag
	std::optional<UiTable> hint{};
};

struct RenderSideBarEvent
{
	SideBarEnemies enemies{};
	SideBarPlayers players{};
};

struct RenderFPSEvent
{
	unsigned int fps;
};
