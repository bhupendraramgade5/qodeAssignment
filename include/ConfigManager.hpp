#ifndef CONFIG_MANAGER_HPP
#define CONFIG_MANAGER_HPP


#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <filesystem>
#include <cinttypes>
#include <iostream>
#include <string>


constexpr int SUCCESS = 0;
constexpr int FAIL = 1;

static constexpr int MAX_THREADS=8;
static constexpr int MAXSYMBOLS=100;
static constexpr double MAXDRIFT=0.05;
static constexpr double EPS=1e-6;
static constexpr uint16_t MAX_SYMBOL_ID=500;
static constexpr uint16_t MIN_SYMBOL_ID=1;
static constexpr double MSGQuoteRatio = 0.70;
static constexpr double MSGTradeRatio = 0.30;
enum class RunMode{
    Random, Manual
};
// using boost_pt = boost::property_tree::ptree;
namespace boost_ptree = boost::property_tree;


class ConfigManager{
    public:
    ConfigManager()=default;
    ConfigManager(ConfigManager&&) = default;
    ConfigManager& operator=(const ConfigManager&)=default;
    ConfigManager& operator=(ConfigManager&&)=default;

    explicit ConfigManager(const std::string& configFilePath){
        
        try {
            boost_ptree::ini_parser::read_ini(configFilePath, m_ptree);
        }
        catch (const boost_ptree::ini_parser_error& e){
            std::cerr << "Failed to read config: " << e.what() << "\n";
        }
        LoadParameters();
        ValidateConfigParameters();
    }

    void LoadParameters(){
        m_numOfThreads = m_ptree.get<int>("SERVER.THREADS", 4);
        m_numOfSymbols = m_ptree.get<int>("EXCHANGE.SYMBOLS", 100);
        m_port = m_ptree.get<int>("SERVER.PORT",9876);
        m_ipadd = m_ptree.get<std::string>("SERVER.SERVER_IP_ADD", "0.0.0.0");
        m_marketDrift = m_ptree.get<double>("MARKET.DRIFT", 0.0);


        m_spreadMax = m_ptree.get<double>("MARKET.SPREADMAX",0.0020);
        m_spreadMin=m_ptree.get<double>("MARKET.SPREADMIN", 0.0005);

        m_priceMax = m_ptree.get<double>("EXCHANGE.PRICEMAX", 100.00);
        m_priceMin = m_ptree.get<double>("EXCHANGE.PRICEMIN", 5000.00);

        m_volatilityMax = m_ptree.get<double>("MARKET.VOLATILITYMAX", 0.01);
        m_volatilityMin =m_ptree.get<double>("MARKET.VOLATILITYMIN", 0.06);

        m_tickRateMax = m_ptree.get<uint32_t>("TICKS.RATEMAX", 10000);
        m_tickRateMin = m_ptree.get<uint32_t>("TICKS.RATEMIN", 500000);
        m_ticksRate = m_ptree.get<uint32_t>("TICKS.TICKSRATE", 50000);

        m_runDurationSec = m_ptree.get<uint64_t>("TICKS.m_runDurationSec", 1);
        m_dt = m_ptree.get<double>("TICKS.dT", 0.001);
    }
    
    void setRandomMode(char runMode){
        if(runMode=='R'){
            m_runMode=runMode;
        }
        else{
            std::cerr<<"Invalid Run  Mode"<<"\n";
            exit(FAIL);
        }
    }

    void setManualRunMode(char runMode, const std::string& manualFilePath){
        m_runMode = runMode;

        if(m_runMode=='M' || m_runMode=='R'){
            std::cout<<"Run Mode : "<<m_runMode<<"\n";
        }
        else{
            std::cerr<<"Invalid Run Mode"<<"\n";
            exit(FAIL);
        }

        if(m_runMode=='M'){
            m_manualFilePath = manualFilePath; 
            std::cout<<"Manual File Path : "<<m_manualFilePath<<"\n";
        }
    }

    private:
    void ValidateConfigParameters(){
        //thread validation
        if(m_numOfThreads>MAX_THREADS){
        std::cout<<"Reduce the Number of threads in INI"<<"\n";
        }
        else if(m_numOfThreads==0){
        m_numOfThreads=4;
        std::cout<<"Fall back TO default number of threads"<<"\n";
        }

        if(m_numOfSymbols>0 && m_numOfSymbols<=MAXSYMBOLS){
            std::cout<<"Symbols : "<<m_numOfSymbols<<"\n";
        }
        else{
            std::cerr<<"Limit exceded : "<<m_numOfSymbols<<"\n";
            exit(FAIL);
        }

        if(m_marketDrift<=MAXDRIFT && m_marketDrift>=-MAXDRIFT){
            std::cout<<"Market Drift : "<<m_marketDrift<<"\n";
        }
        else{
            std::cerr<<"Invalid Market Drift"<<"\n";
            exit(FAIL);
        }

        if(m_spreadMax>m_spreadMin){
            std::cout<<"Max Spread: "<<m_spreadMax<<"\n"<< "Min Spread: "<<m_spreadMin<<"\n";
        }
        else{
            std::cerr<<" Invalid Spread "<<"\n";
            exit(FAIL);
        }

        if(m_volatilityMax>m_volatilityMin){
            std::cout<<"Max Volatility: "<<m_volatilityMax<<"\n"<<"Min Volatility: "<<m_volatilityMin<<"\n";
        }
        else{
            std::cerr<<" Invalid Volatility"<<"\n";
            exit(FAIL);
        }

        if(m_tickRateMax>m_tickRateMin){
            std::cout<<"Max Ticks: "<<m_tickRateMax<<"\n"<<"Min Ticks: "<<m_tickRateMin<<"\n";
        }
        else {
            std::cerr<<"Invalid Ticks "<<"\n";
        }
        std::cout<<"Stop Time: "<<m_runDurationSec<<"\n";
        // if(m_msgQuoteRatio+m_msgTradeRatio-1>=EPS){
        //     std::cerr<<"Invalid Ratio's"<<"\n";
        // }
        // else{
        //     std::cout<<"Quote Ratio : "<<m_msgQuoteRatio<<"\n"<<"Trade Ratio : "<<m_msgTradeRatio<<"\n";
        // }
    }

    private:

        std::string m_configFilePath;
        std::string m_manualFilePath; //may remain unused depending upon the RUN MODE
    public:

        double m_spreadMin;
        double m_spreadMax;
        std::string m_ipadd;
        int m_port;

        int m_numOfThreads;
        int m_numOfSymbols;

        uint32_t m_tickRateMin;
        uint32_t m_tickRateMax;
        uint32_t m_ticksRate;
        double m_dt;

        char m_runMode;

        boost_ptree::ptree m_ptree;
        const uint64_t m_seed=12345;

        double m_priceMin;
        double m_priceMax;

        double m_volatilityMin;
        double m_volatilityMax;

        double m_marketDrift;
        uint64_t m_runDurationSec ;
        // uint32_t m_rng_seed;

};



#endif
