#include "application/ProjectConfig.h"
#include "application/WindowConfig.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include "TestUtils.h"//NOTE: PrintTo for the Point types

namespace
{
//NOTE: the file's fate is the behaviour under test, so unlike MapLoader these cases need a real one
class TempIni final
{
	std::filesystem::path _path;

public:
	explicit TempIni(const std::string_view name, const std::string_view contents = {})
		: _path(std::filesystem::temp_directory_path() / name)
	{
		std::filesystem::remove(_path);
		if (!contents.empty())
		{
			std::ofstream{_path} << contents;
		}
	}

	TempIni(const TempIni&) = delete;
	TempIni& operator=(const TempIni&) = delete;

	~TempIni() { std::filesystem::remove(_path); }

	[[nodiscard]] const std::filesystem::path& Path() const { return _path; }
	[[nodiscard]] bool Exists() const { return std::filesystem::exists(_path); }

	[[nodiscard]] std::string Read() const
	{
		const std::ifstream file{_path};
		std::ostringstream contents;
		contents << file.rdbuf();

		return contents.str();
	}
};
}//namespace

TEST(ProjectConfigTest, MissingFileIsWrittenAndNotReported)
{
	const TempIni ini{"battlecity_missing.ini"};

	{
		const ProjectConfig projectConfig{ini.Path()};
		EXPECT_FALSE(projectConfig.LoadError().has_value());
	}

	EXPECT_TRUE(ini.Exists());
}

//NOTE: the file used to be replaced by defaults, erasing the very line that needed fixing
TEST(ProjectConfigTest, UnparseableFileIsReportedAndLeftUntouched)
{
	constexpr std::string_view broken{"[Window]\nwidth=800\n=nokey\n"};//NOTE: boost: "key expected", line 3
	const TempIni ini{"battlecity_broken.ini", broken};

	{
		const ProjectConfig projectConfig{ini.Path()};
		EXPECT_EQ(projectConfig.LoadError().value_or(ConfigError{}).line, 3u);

		const WindowConfig windowConfig{projectConfig};
		EXPECT_EQ(windowConfig.size, (UPoint{.x = 800u, .y = 600u}));//NOTE: defaults, not the file's 800
	}

	EXPECT_EQ(ini.Read(), broken);
}

//NOTE: drives the centering branch in SDL_Config::InitRender - a run with nothing saved to restore
TEST(ProjectConfigTest, MissingAndUnparseableFilesBothCountAsFresh)
{
	const TempIni missing{"battlecity_fresh_missing.ini"};
	EXPECT_TRUE(ProjectConfig{missing.Path()}.IsFreshIni());

	const TempIni broken{"battlecity_fresh_broken.ini", "[Window]\n=nokey\n"};
	EXPECT_TRUE(ProjectConfig{broken.Path()}.IsFreshIni());
}

TEST(ProjectConfigTest, AReadableFileIsNotFreshAndCenteringIsOptIn)
{
	const TempIni ini{"battlecity_saved.ini", "[Window]\nposX=340\nposY=180\n"};

	const ProjectConfig projectConfig{ini.Path()};
	EXPECT_FALSE(projectConfig.IsFreshIni());
	EXPECT_FALSE(projectConfig.IsCenterOnStart());

	const WindowConfig windowConfig{projectConfig};
	EXPECT_EQ(windowConfig.pos, (UPoint{.x = 340u, .y = 180u}));
}

TEST(ProjectConfigTest, CenterOnStartIsReadBackFromTheFile)
{
	const TempIni ini{"battlecity_centered.ini", "[Window]\ncenterOnStart=true\n"};

	EXPECT_TRUE(ProjectConfig{ini.Path()}.IsCenterOnStart());
}
