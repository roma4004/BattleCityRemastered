#include "components/AddressField.h"
#include "components/events/InputEvents.h"
#include "network/Endpoints.h"
#include "gtest/gtest.h"
#include <algorithm>
#include <array>
#include <string_view>

using namespace std::string_view_literals;

// digits typed in land as the field shows them: octets and groups hand the caret on, past 255 or 65535 nothing more
TEST(InputIPAddressTest, TypedDigitsLandWhereTheFieldShowsThem)
{
	struct Typed final
	{
		AddressFamily family;
		std::string_view typed;
		std::string_view shown;
	};

	constexpr std::array kTyped{
			Typed{.family = AddressFamily::IPv4, .typed = "1921681001", .shown = "192.168.100.1:auto"},
			Typed{.family = AddressFamily::IPv4, .typed = "300", .shown = "30.0.___.___:auto"},
			Typed{.family = AddressFamily::IPv4, .typed = ":65536", .shown = "___.___.___.___:6553"},
			Typed{.family = AddressFamily::IPv6, .typed = "20010db8", .shown = " 2001:0db8: :auto"},
			Typed{.family = AddressFamily::IPv6, .typed = "2001:", .shown = " 2001: :auto"}};
	for (const Typed& typed: kTyped)
	{
		SCOPED_TRACE(typed.typed);
		AddressField field{typed.family};

		std::ranges::for_each(typed.typed, [&field](const char symbol) { field.Type(symbol); });

		EXPECT_EQ(field.Shown(false).text, typed.shown);
	}
}

// a colon in IPv4, or digits past a whole IPv6, leave the address for the port
TEST(InputIPAddressTest, TypingPastTheAddressGoesToThePort)
{
	struct Typed final
	{
		AddressFamily family;
		std::string_view typed;
		std::string_view text;
	};

	constexpr std::array kTyped{
			Typed{.family = AddressFamily::IPv4, .typed = "1:5", .text = "1...:5"},
			Typed{.family = AddressFamily::IPv6,
				  .typed = "1111222233334444555566667777888812",
				  .text = "[1111:2222:3333:4444:5555:6666:7777:8888]:12"}};
	for (const Typed& typed: kTyped)
	{
		SCOPED_TRACE(typed.typed);
		AddressField field{typed.family};

		std::ranges::for_each(typed.typed, [&field](const char symbol) { field.Type(symbol); });

		EXPECT_EQ(field.Text(), typed.text);
	}
}

// back on the row, the same colon is a new one - the added one was left behind with the row
TEST(InputIPAddressTest, AColonAfterComingBackIsANewOne)
{
	AddressField field{AddressFamily::IPv6};

	std::ranges::for_each("2001"sv, [&field](const char symbol) { field.Type(symbol); });
	field.Focus();
	field.Type(':');

	EXPECT_EQ(field.Shown(false).text, " 2001:: :auto");
}

// a digit typed into a full group pushes the group's last digit over the colon into the next one
TEST(InputIPAddressTest, ADigitInAFullGroupFlowsOverTheColon)
{
	AddressField field{AddressFamily::IPv6};
	std::ranges::for_each("20010db8"sv, [&field](const char symbol) { field.Type(symbol); });
	for (int step{}; step < 6; ++step)
	{
		field.Press(TextKey::CaretLeft);
	}

	field.Type('f');

	EXPECT_EQ(field.Shown(false).text, " 2001:f0db:8 :auto");
}

// erasing the colon between two full groups would join eight digits - the digit before it goes instead
TEST(InputIPAddressTest, TheColonBetweenFullGroupsIsNotErased)
{
	AddressField field{AddressFamily::IPv6};
	std::ranges::for_each("20010db8"sv, [&field](const char symbol) { field.Type(symbol); });
	for (int step{}; step < 5; ++step)
	{
		field.Press(TextKey::CaretLeft);
	}

	field.Press(TextKey::Erase);

	EXPECT_EQ(field.Shown(false).text, " 200:0db8: :auto");
}

// a filled address goes in whole, with the caret on its port
TEST(InputIPAddressTest, AFilledAddressPutsTheCaretOnThePort)
{
	AddressField field{AddressFamily::IPv4};

	field.Fill(network::ServerAddress{.host = "10.0.0.6", .port = 4001}, false);

	const ShownAddress shown{field.Shown(true)};
	EXPECT_EQ(shown.text, "10.0.0.6:4001");
	EXPECT_EQ(shown.caret, shown.text.size());
}

// a server's own address is picked, not typed - the caret starts on the port and typing goes there
TEST(InputIPAddressTest, APickedAddressStartsTheCaretOnThePort)
{
	AddressField field{AddressFamily::IPv4, AddressInput::Picked};
	field.Fill(network::ServerAddress{.host = "192.168.0.5"}, false);

	field.CaretToStart();
	field.Type('9');

	EXPECT_EQ(field.Text(), "192.168.0.5:9");
}

// erasing past the port's start and stepping left of it leave the picked address alone
TEST(InputIPAddressTest, APickedAddressKeepsTheCaretInThePort)
{
	AddressField field{AddressFamily::IPv4, AddressInput::Picked};
	field.Fill(network::ServerAddress{.host = "192.168.0.5", .port = 50}, false);

	constexpr std::array kKeys{TextKey::Erase, TextKey::Erase, TextKey::Erase, TextKey::CaretLeft, TextKey::WordLeft};
	for (const TextKey key: kKeys)
	{
		field.Press(key);
	}

	field.Type('7');

	EXPECT_EQ(field.Text(), "192.168.0.5:7");
}
