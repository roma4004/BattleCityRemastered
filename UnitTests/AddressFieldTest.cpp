#include "components/AddressField.h"
#include "components/events/InputEvents.h"
#include "network/Endpoints.h"
#include "gtest/gtest.h"
#include <algorithm>
#include <string_view>

using namespace std::string_view_literals;

// digits run on into the next octet once one is full - an address is typed without its dots
TEST(AddressFieldTest, FullOctetsHandTheCaretOn)
{
	AddressField field{AddressFamily::IPv4};

	std::ranges::for_each("1921681001"sv, [&field](const char symbol) { field.Type(symbol); });

	EXPECT_EQ(field.Shown(false).text, "192.168.100.1:auto");
}

// an octet past 255 keeps what fits and hands its last digit to the next one
TEST(AddressFieldTest, AnOctetPast255HandsItsLastDigitOn)
{
	AddressField field{AddressFamily::IPv4};

	std::ranges::for_each("300"sv, [&field](const char symbol) { field.Type(symbol); });

	EXPECT_EQ(field.Shown(false).text, "30.0.___.___:auto");
}

// a colon in IPv4 leaves the address for the port
TEST(AddressFieldTest, AColonInIPv4GoesToThePort)
{
	AddressField field{AddressFamily::IPv4};

	std::ranges::for_each("1:5"sv, [&field](const char symbol) { field.Type(symbol); });

	EXPECT_EQ(field.Text(), "1...:5");
}

// a port past 65535 is refused digit by digit - the one that would make it so does not go in
TEST(AddressFieldTest, APortPast65535IsRefused)
{
	AddressField field{AddressFamily::IPv4};

	std::ranges::for_each(":65536"sv, [&field](const char symbol) { field.Type(symbol); });

	EXPECT_EQ(field.Shown(false).text, "___.___.___.___:6553");
}

// a full group of four gets its colon, the way a full octet hands the caret on
TEST(AddressFieldTest, FullGroupsGetTheirColons)
{
	AddressField field{AddressFamily::IPv6};

	std::ranges::for_each("20010db8"sv, [&field](const char symbol) { field.Type(symbol); });

	EXPECT_EQ(field.Shown(false).text, " 2001:0db8: :auto");
}

// a colon typed right after the added one is the same colon, not a "::"
TEST(AddressFieldTest, AColonAfterTheAddedOneIsSwallowed)
{
	AddressField field{AddressFamily::IPv6};

	std::ranges::for_each("2001:"sv, [&field](const char symbol) { field.Type(symbol); });

	EXPECT_EQ(field.Shown(false).text, " 2001: :auto");
}

// back on the row, the same colon is a new one - the added one was left behind with the row
TEST(AddressFieldTest, AColonAfterComingBackIsANewOne)
{
	AddressField field{AddressFamily::IPv6};

	std::ranges::for_each("2001"sv, [&field](const char symbol) { field.Type(symbol); });
	field.Focus();
	field.Type(':');

	EXPECT_EQ(field.Shown(false).text, " 2001:: :auto");
}

// a digit typed into a full group pushes the group's last digit over the colon into the next one
TEST(AddressFieldTest, ADigitInAFullGroupFlowsOverTheColon)
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

// past a whole address only the port is left to type into
TEST(AddressFieldTest, DigitsPastAWholeIPv6GoToThePort)
{
	AddressField field{AddressFamily::IPv6};

	std::ranges::for_each("1111222233334444555566667777888812"sv,
						  [&field](const char symbol) { field.Type(symbol); });

	EXPECT_EQ(field.Text(), "[1111:2222:3333:4444:5555:6666:7777:8888]:12");
}

// erasing the colon between two full groups would join eight digits - the digit before it goes instead
TEST(AddressFieldTest, TheColonBetweenFullGroupsIsNotErased)
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
TEST(AddressFieldTest, AFilledAddressPutsTheCaretOnThePort)
{
	AddressField field{AddressFamily::IPv4};

	field.Fill(network::ServerAddress{.host = "10.0.0.6", .port = 4001}, false);

	const ShownAddress shown{field.Shown(true)};
	EXPECT_EQ(shown.text, "10.0.0.6:4001");
	EXPECT_EQ(shown.caret, shown.text.size());
}
