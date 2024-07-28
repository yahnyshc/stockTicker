#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <vector>

class Config {

public:
    // Constructor
    Config(const std::string& fileName);

    // Getter methods
    std::string getToken() const;
    std::string getControlToken() const;
    std::vector<std::string> getSubsList() const;
    std::vector<std::string> getApiSubsList() const;
    std::vector<std::string> getLogoSubsList() const;
    int getLogoSize() const;
    int getChartHeight() const;

    // Setter methods
    void setSubsList(const std::vector<std::string>& subsList);
    void setApiSubsList(const std::vector<std::string>& apiSubsList);
    void setLogoSubsList(const std::vector<std::string>& logoSubsList);

private:
    std::string token_;
    std::string controlToken_;
    std::vector<std::string> subsList_;
    std::vector<std::string> apiSubsList_;
    std::vector<std::string> logoSubsList_;
    int logoSize_;
    int chartHeight_;

    bool boolRenderLogos_;
};

#endif // CONFIG_HPP
