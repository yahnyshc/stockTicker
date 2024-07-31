#include <boost/program_options.hpp>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

#include "Config.hpp"

namespace po = boost::program_options;

Config* Config::instance_ = nullptr;

Config* Config::getInstance(const std::string& fileName) {
   if (instance_ == nullptr) {
      instance_ = new Config(fileName);
   }
   return instance_;
}


Config::Config(const std::string& fileName) {
    po::options_description config("Configuration");
    config.add_options()
        ("API_Token", po::value<std::string>()->default_value(""), "API Key assigned by Finnhub")
        ("Control_API_Token", po::value<std::string>()->default_value(""), "API Key assigned by controlling app")
        ("Subs_list", po::value<std::string>(), "List of Symbols to be subscribed")
        ("Api_Subs_list", po::value<std::string>(), "List of API names of symbols to be subscribed for")
        ("Logo_Subs_list", po::value<std::string>(), "Symbol to Icon name translation")
        ("Logo_Size", po::value<int>()->default_value(22), "Size of logo")
        ("Chart_Height", po::value<int>()->default_value(17), "Height of chart")
        ("Switch_Time", po::value<int>()->default_value(5), "How often to switch between subscribed symbols");

    po::variables_map vm;

    std::ifstream ifs(fileName);
    if (ifs) {
        po::store(po::parse_config_file(ifs, config), vm);
        po::notify(vm);
    } else {
        std::cerr << "Could not open configuration file: " << fileName << std::endl;
        return;
    }

    if (vm.count("API_Token")) {
        token_ = vm["API_Token"].as<std::string>();
    } else {
        std::cerr << "API_Token is not defined in the configuration file" << std::endl;
        return;
    }

    if (vm.count("Control_API_Token")) {
        controlToken_ = vm["Control_API_Token"].as<std::string>();
    } else {
        std::cerr << "Control_API_Token is not defined in the configuration file" << std::endl;
        controlToken_ = "";
    }

    if (vm.count("Subs_list")) {
        std::istringstream iss(vm["Subs_list"].as<std::string>());
        std::string symbol;
        while (iss >> symbol) {
            subsList_.push_back(symbol);
        }
    } else {
        std::cerr << "Subs_list is not defined in the configuration file" << std::endl;
    }

    if (vm.count("Api_Subs_list")) {
        std::istringstream iss(vm["Api_Subs_list"].as<std::string>());
        std::string symbol;
        while (iss >> symbol) {
            apiSubsList_.push_back(symbol);
        }
    } else {
        std::cerr << "Api_Subs_list is not defined in the configuration file" << std::endl;
    }

    if (vm.count("Logo_Subs_list")) {
        std::istringstream iss(vm["Logo_Subs_list"].as<std::string>());
        std::string symbol;
        while (iss >> symbol) {
            logoSubsList_.push_back(symbol);
        }
    } else {
        std::cerr << "Logo_Subs_list is not defined in the configuration file" << std::endl;
    }

    if (vm.count("Logo_Size")) {
        logoSize_ = vm["Logo_Size"].as<int>();
    } else {
        std::cerr << "Logo_Size is not defined in the configuration file" << std::endl;
        return;
    }

    if (vm.count("Chart_Height")) {
        chartHeight_ = vm["Chart_Height"].as<int>();
    } else {
        std::cerr << "Chart_Height is not defined in the configuration file" << std::endl;
        return;
    }

    if (vm.count("Switch_Time")) {
        switchTime_ = vm["Switch_Time"].as<int>();
    } else {
        std::cerr << "Switch_Time is not defined in the configuration file" << std::endl;
        return;
    }
}

std::string Config::getToken() const {
    return token_;
}

std::string Config::getControlToken() const {
    return controlToken_;
}

std::vector<std::string> Config::getSubsList() const {
    return subsList_;
}

std::vector<std::string> Config::getApiSubsList() const {
    return apiSubsList_;
}

std::vector<std::string> Config::getLogoSubsList() const {
    return logoSubsList_;
}

void Config::setSubsList(const std::vector<std::string>& subsList) {
    subsList_ = subsList;
}

void Config::setApiSubsList(const std::vector<std::string>& apiSubsList) {
    apiSubsList_ = apiSubsList;
}

void Config::setLogoSubsList(const std::vector<std::string>& logoSubsList) {
    logoSubsList_ = logoSubsList;
}

int Config::getLogoSize() const {
    return logoSize_;
}

int Config::getChartHeight() const {
    return chartHeight_;
}

int Config::getSwitchTime() const {
    return switchTime_;
}

void Config::setSwitchTime(int switchTime) {
    switchTime_ = switchTime;
}
