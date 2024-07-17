#include <boost/program_options.hpp>
#include <fstream>
#include <iostream>

#include "Config.hpp"

namespace po = boost::program_options;

Config::Config(const std::string fileName) {
    po::options_description config("Configuration");
    config.add_options()
        ("API_Token", po::value<std::string>()->default_value(""), "API Key assigned by Finnhub");
    config.add_options()
        ("Symbol_List", po::value<std::vector<std::string>>()->multitoken(), "List of Symbols to be subscribed");
    config.add_options()
        ("Icon_Mapping", po::value<std::vector<std::string>>()->multitoken(), "Symbol to Icon name translation");
    config.add_options()
        ("Logo_Size", po::value<int>()->default_value(20), "Size of logo");
    config.add_options()
        ("Chart_Height", po::value<int>()->default_value(17), "Height of chart");
    config.add_options()
        ("Render_Logos", po::value<bool>()->default_value(true), "Whether to render logos or not");
        
    
    boost::program_options::variables_map vm;

    std::ifstream ifs(fileName);
    if (ifs) {
        boost::program_options::store(boost::program_options::parse_config_file(ifs, config), vm);
        boost::program_options::notify(vm);
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

    if (vm.count("Symbol_List")) {
        symbols_ = vm["Symbol_List"].as<std::vector<std::string>>();
    } else {
        std::cerr << "Symbol_List is not defined in the configuration file" << std::endl;
    }

    if (vm.count("Icon_Mapping")) {
        icon_mappings_ = vm["Icon_Mapping"].as<std::vector<std::string>>();
    } else {
        std::cerr << "Icon_Mapping is not defined in the configuration file" << std::endl;
    }

    if (vm.count("Logo_Size")) {
        logo_size_ = vm["Logo_Size"].as<int>();
    } else {
        std::cerr << "Logo_Size is not defined in the configuration file" << std::endl;
        return;
    }

    if (vm.count("Chart_Height")) {
        chart_height_ = vm["Chart_Height"].as<int>();
    } else {
        std::cerr << "Chart_Height is not defined in the configuration file" << std::endl;
        return;
    }

    if (vm.count("Render_Logos")) {
        bool_render_logos_ = vm["Render_Logos"].as<bool>();
    } else {
        std::cerr << "Render_Logos is not defined in the configuration file" << std::endl;
        return;
    }
}

std::string Config::symbol_to_logo(const std::string& symbol){
    std::string logo_identifier = symbol;

    for ( std::string mapping : c.get_icon_mappings() ){
        std::string delimiter = " -> ";
        size_t pos = mapping.find(delimiter);
        std::string token = mapping.substr(0, pos);
        if (token == symbol){
            mapping.erase(0, pos + delimiter.length());
            logo_identifier = mapping;
        }
    }
    return logo_identifier;
}
std::string Config::get_token() {
    return token_;
}

std::vector<std::string> Config::get_symbols() {
    return symbols_;
}

std::vector<std::string> Config::get_icon_mappings() {
    return icon_mappings_;
}

int Config::get_logo_size() {
    return logo_size_;
}

int Config::get_chart_height() {
    return chart_height_;
}

bool Config::get_bool_render_logos() {
    return bool_render_logos_;
}