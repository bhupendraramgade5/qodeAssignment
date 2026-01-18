
#include <iostream>
#include <filesystem>
#include <optional>
#include "ConfigManager.hpp"
#include "exchange_simulator.hpp"

// constexpr int SUCCESS = 0;
// constexpr int FAIL = 1;

using namespace std;

int main(int argc, char* argv[]){
    std::string argv_configPath;    //Config Path
    char argv_mode= 'R';            //default Mode in ehich the application would run 
    std::string arg_manualPath;     //File for Manual Symbol Parameters

    //-------------Parsing the Arguments------------------------------------------------

    if(argc==1){
        std::cout<<"1.  Config Path"<<"\n";
        std::cout<<"2.  Mode for Symbol Parameters, M: manual(from file) , R: Random(Auto Generated)"<<"\n";
        std::cout<<"3.  Manual File Path"<<"\n";
        std::cout<<"3.  Rest of the Parameters are yet to be decided"<<"\n\n\n";

        std::cout<<"No arguments Provided :: Using the default path for the Config"<<"\n";

        argv_configPath="./configs/ServerConfig.ini";   //"configs/ServerConfig.ini"
        
    }
    else if(argc>=2){
        argv_configPath=argv[1];        //if Config Path was Provided through Program argumrnts
        cout<<"Test : "<<argv_configPath<<"\n";
    }
    if(argc>=3){
        if(argv[2]==nullptr || argv[2][0]=='\0' || argv[2][1]!='\0'){
            std::cerr << "Invalid mode argument. Use 'M' or 'R'\n";
            return (FAIL);
        }

        argv_mode = std::toupper(argv[2][0]);
        
        if (argv_mode != 'M' && argv_mode != 'R') {
            std::cerr << "Invalid mode. Allowed values: M or R\n";
            return FAIL;
        }
    }

    if (argv_mode == 'M') {
        if (argc >= 4) {
            arg_manualPath = argv[3];
        } else {
            std::cerr << "Manual mode requires manual file path\n";
            std::cout<<"Using Default Manual Path"<<"\n";

            arg_manualPath = "Default_manual_path";
        }
    }
    //------------------------Config Intialisation-------------------------------------
    std::filesystem::path configPath(argv_configPath);
    std::error_code errConfigFile {};
    unique_ptr<ConfigManager> cfg;
    if(!errConfigFile && std::filesystem::exists(configPath, errConfigFile)){
        //intialise the Config Parameters 
        cfg = make_unique<ConfigManager>(static_cast<const std::string>(argv_configPath));
    }
    else{
        //print the error captured while checking the file
        std::cerr<<"filesystem error(Config): Path doesnot exist ,"<<errConfigFile.message()<<configPath<<"\n";
        //Since Config  Cannot be accessed the Program should exit with a status Code 1
        return (FAIL);
    }
    
    //---------------------------------------Runnning Application : Object Intialisation -----------------------------------
    std::filesystem::path manualFilePath(arg_manualPath);
    std::error_code errManualFile {};

    if(argv_mode =='M' && !errManualFile && std::filesystem::exists(manualFilePath, errManualFile)){
        //Now we execute the Manual Intialsation of the Exchange
    }
    else if(argv_mode=='R'){
        //Now Run the Random MOODE of application
        ExchangeSimulator obj_exchangeSimulator(9876, cfg->m_numOfSymbols);
        cout<<"Running in the Random Mode"<<"\n";

        obj_exchangeSimulator.InitialiseSymbols(std::move(cfg));
        // obj_exchangeSimulator.printSymbolData();
        obj_exchangeSimulator.start();

    }else{
        //print the error captured while checking the file
        std::cerr<<"filesystem error(Manual): Path doesnot exist ,"<<manualFilePath<<"\n";
        //Since Config  Cannot be accessed the Program should exit with a status Code 1
        return (FAIL);
    }
    return (SUCCESS);
}
