#ifndef Config_HPP
#define Config_HPP

class Config {

public:
    Config(const std::string fileName);

    std::string get_token();
    std::vector<std::string> get_symbols();
    std::vector<std::string> get_icon_mappings();
private:
    std::string token_;
    std::vector<std::string> symbols_;
    std::vector<std::string> icon_mappings_;
};

#endif // Config_HPP