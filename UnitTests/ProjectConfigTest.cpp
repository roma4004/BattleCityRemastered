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
//NOTE: what happens to the file is the behaviour under test, so these cases need a real one on disk
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

// point the config at a path with no file: it is written with defaults, and the absence is not an
// error the caller has to show
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
	//NOTE: boost: "key expected", line 3; the width differs from the default, so reading it would show
	constexpr std::string_view broken{"[Window]\nwidth=1024\n=nokey\n"};
	const TempIni ini{"battlecity_broken.ini", broken};

	{
		const ProjectConfig projectConfig{ini.Path()};
		EXPECT_EQ(projectConfig.LoadError().value_or(ConfigError{}).line, 3u);

		const WindowConfig windowConfig{projectConfig};
		EXPECT_EQ(windowConfig.size, (UPoint{.x = 800u, .y = 600u}));
	}

	EXPECT_EQ(ini.Read(), broken);
}

//NOTE: a fresh ini has no saved position to restore, so the window is centred on the monitor
TEST(ProjectConfigTest, MissingAndUnparseableFilesBothCountAsFresh)
{
	const TempIni missing{"battlecity_fresh_missing.ini"};
	EXPECT_TRUE(ProjectConfig{missing.Path()}.IsFreshIni());

	const TempIni broken{"battlecity_fresh_broken.ini", "[Window]\n=nokey\n"};
	EXPECT_TRUE(ProjectConfig{broken.Path()}.IsFreshIni());
}

// an ini with a saved position: not fresh, so the window is put back where it was instead of centred
TEST(ProjectConfigTest, AReadableFileIsNotFreshAndCenteringIsOptIn)
{
	const TempIni ini{"battlecity_saved.ini", "[Window]\nposX=340\nposY=180\n"};

	const ProjectConfig projectConfig{ini.Path()};
	EXPECT_FALSE(projectConfig.IsFreshIni());
	EXPECT_FALSE(projectConfig.IsCenterOnStart());

	const WindowConfig windowConfig{projectConfig};
	EXPECT_EQ(windowConfig.pos, (UPoint{.x = 340u, .y = 180u}));
}

// centerOnStart written into the file reaches IsCenterOnStart
TEST(ProjectConfigTest, CenterOnStartIsReadBackFromTheFile)
{
	const TempIni ini{"battlecity_centered.ini", "[Window]\ncenterOnStart=true\n"};

	EXPECT_TRUE(ProjectConfig{ini.Path()}.IsCenterOnStart());
}
