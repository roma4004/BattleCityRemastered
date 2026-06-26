#pragma once
#include <boost/property_tree/ptree.hpp>

class GameConfig
{
public:
	GameConfig(const std::string& filePath);
	~GameConfig();

	void LoadIni(const std::string& filePath);
	void DefaultInitIni(const std::string& filePath);
	void SaveIni(const std::string& filePath) const;
	boost::property_tree::ptree pTreeIni;
	
private:
	//TODO make pTreeIni private + Getter's Setter's for it
};
