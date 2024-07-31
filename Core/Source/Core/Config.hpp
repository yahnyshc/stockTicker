#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <vector>

class Config {

public:
    static Config* getInstance(const std::string& fileName);

    // Getter methods
    std::string getToken() const;
    std::string getControlToken() const;
    std::vector<std::string> getSubsList() const;
    std::vector<std::string> getApiSubsList() const;
    std::vector<std::string> getLogoSubsList() const;
    int getLogoSize() const;
    int getChartHeight() const;
    int getSwitchTime() const;

    // Setter methods
    void setSubsList(const std::vector<std::string>& subsList);
    void setApiSubsList(const std::vector<std::string>& apiSubsList);
    void setLogoSubsList(const std::vector<std::string>& logoSubsList);

    void setSwitchTime(int switchTime);

private:
    // Constructor
    Config(const std::string& fileName);
    Config(const Config&);
    Config& operator=(const Config&);

    static Config* instance_;   // The one, single instance
    std::string token_;
    std::string controlToken_;
    std::vector<std::string> subsList_;
    std::vector<std::string> apiSubsList_;
    std::vector<std::string> logoSubsList_;
    int logoSize_;
    int chartHeight_;
    int switchTime_;

    bool boolRenderLogos_;
};

#endif // CONFIG_HPP
