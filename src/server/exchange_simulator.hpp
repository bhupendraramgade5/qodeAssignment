#ifndef EXCHANGE_SIMULATOR_HPP
#define EXCHANGE_SIMULATOR_HPP

#include <iostream>
#include <cstdint>
#include <array>
#include <vector>
#include <random>
#include <memory>
#include "ConfigManager.hpp"
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdio>
#include <cerrno>
#include <unordered_set>
#include <arpa/inet.h>
#include <atomic>

enum class MessageType : uint8_t {
    QUOTE,
    TRADE
};

struct MarketMessage {
    MessageType type;
    uint16_t symbol_id;
    uint64_t sequence;
    uint64_t timestamp_ns;
    
    union {
        struct {
            double bid_price;
            double ask_price;
            uint32_t bid_qty;
            uint32_t ask_qty;
        } quote;

        struct {
            double trade_price;
            uint32_t trade_qty;
            bool aggressor_buy;
        } trade;
    };
    inline static std::atomic<uint64_t> global_sequence;

    void assignSequence() {
        global_sequence.fetch_add(1, std::memory_order_relaxed) + 1;
        std::cout<<" global_sequence :"<<global_sequence;
    }
    void printinfoquote(){
        std::cout<<"-------------------------------------------------------------------------------\n";
        std::cout<<"symbol_id :"<<symbol_id<<"\nsequence :"<<sequence<<"\ntimestamp_ns :"<<timestamp_ns<<

        "\nQuote :: \nbid_price"<<quote.bid_price<<"\nbid_qty :"<<quote.bid_qty<<"\nask_price :"<<quote.ask_price<<"\nask_qty :" <<quote.ask_qty<<"\nglobal_sequence"<<global_sequence<<"\n";

        // std::cout<<"symbol_id :"<<symbol_id<<"\nsequence :"<<sequence<<"\ntimestamp_ns :"<<timestamp_ns<<
        // "\nTrade :: \ntrade_price"<<trade.trade_price<<" trade_qty :"<<trade.trade_qty<<"\naggressor_buy :"<<trade.aggressor_buy<<"\n";
    }

    void printinfotrade(){
        std::cout<<"-------------------------------------------------------------------------------\n";
        std::cout<<"symbol_id :"<<symbol_id<<"\nsequence :"<<sequence<<"\ntimestamp_ns :"<<timestamp_ns<<
        // std::cout<<"symbol_id :"<<symbol_id<<"\nsequence :"<<sequence<<"\ntimestamp_ns :"<<timestamp_ns<<
        "\nTrade :: \ntrade_price"<<trade.trade_price<<"\ntrade_qty :"<<trade.trade_qty<<"\naggressor_buy :"<<trade.aggressor_buy<<"\nglobal_sequence"<<global_sequence<<"\n";

    }
};


struct SymbolData{
    uint16_t st_symbolID;

    double st_symbolPrice;
    double st_bidPrice;
    double st_askPrice;

    double st_symbolMU;      //drift
    double st_symbolSIGMA;   //volatility
    double st_symbolSpread;  // spread in percent
    

    uint64_t st_symbolSequenceNumber;    //to be decided whether to use it as a atomic<TYPE> or keep it 
    uint64_t st_timeStamp;

    std::mt19937_64 st_rng;

    void printInfo(){
        std::cout<<"-------------------------------------------------------------------------------\n";
        std::cout<<"st_symbolID : "<<st_symbolID<<"\nst_symbolPrice : "<<st_symbolPrice<<" ,st_bidPrice : "<<st_bidPrice
        <<" ,st_askPrice : "<<st_askPrice<<" ,st_symbolMU : "<<st_symbolMU<<" ,st_symbolSIGMA : "<<st_symbolSIGMA
        <<" ,st_symbolSpread : "<<st_symbolSpread<<" ,st_symbolSequenceNumber : "<<st_symbolSequenceNumber
        <<" ,st_timeStamp : "<<st_timeStamp<<"\n";
        // std::cout<<"--------------------------------------------------------------------------------\n";
    }

};

uint64_t GetTime_ns()
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();
}

class ExchangeSimulator{
    public:
    ExchangeSimulator(uint16_t port, size_t symbols = 100):m_port(port),m_activeSymbolCounts(symbols){
        m_activeSymbols.reserve(m_activeSymbolCounts);
    }

    void InitialiseSymbols(const std::unique_ptr<ConfigManager>& cfg){
        // Seed scheduler RNG
        m_scheduler_rng.seed(cfg->m_seed ^ 0xABCDEF);
        m_ticks_per_second = cfg->m_ticksRate;
        m_dt = cfg->m_dt;
        // Prepare uniform distribution ONCE
        m_symbol_dist = std::uniform_int_distribution<size_t>(0, m_activeSymbols.size() - 1);

        // Tick timing
        m_tick_interval_ns = 1'000'000'000ULL / m_ticks_per_second;
        m_last_tick_ns = GetTime_ns();

        m_activeSymbols.clear();

        m_runDurationSec=cfg->m_runDurationSec;
        std::vector<uint16_t> allIds;
        allIds.reserve(500);
        //all symbol id are in range {1-500}
        for (uint16_t i = MIN_SYMBOL_ID;i <= MAX_SYMBOL_ID; ++i) {
            allIds.push_back(i);
        }

        //Now Shuffling the Id in the array
        std::mt19937_64 rng(cfg->m_seed);
        std::shuffle(allIds.begin(), allIds.end(), rng);

        m_activeSymbols.assign(allIds.begin(),allIds.begin() + m_activeSymbolCounts);
        //Now Generate The Random Parameterised SymbolData for every symbol
        for (uint16_t symbolId : m_activeSymbols) {
            m_symbolState[symbolId] = GenerateSymbol(symbolId, cfg);
        }

    }

    void printSymbolData(){
        for(auto symbolid: m_activeSymbols){
            auto temp = m_symbolState[symbolid];
            temp.printInfo();
        }
    }
    //start accepting connections 
    //So create a single instance of the epoll which the clients are going to connect
    void start(){

        m_listen_fd = socket(AF_INET, SOCK_STREAM, 0);

        if(m_listen_fd<0){
            perror("socket ");
            return ;
        }
        // fcntl(m_listen_fd, F_SETFL, O_NONBLOCK);
        int flags = fcntl(m_listen_fd, F_GETFL, 0);
        if (flags < 0 || fcntl(m_listen_fd, F_SETFL, flags | O_NONBLOCK) < 0) {
            perror("fcntl ");
            close(m_listen_fd);
            return;
        }


        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        // addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port= htons(m_port);

        if(m_bind_IP.empty()){
            addr.sin_addr.s_addr = INADDR_ANY;
        }
        else if (inet_pton(AF_INET, m_bind_IP.c_str(), &addr.sin_addr) <= 0) {
            perror("inet_pton ");
            close(m_listen_fd);
            return;
        }
        int opt = 1;
        setsockopt(m_listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        if(bind(m_listen_fd, (sockaddr*)&addr, sizeof(addr))){
            perror("bind ");
            close(m_listen_fd);
            return ;
        }
        
        if(listen(m_listen_fd, SOMAXCONN)<0){
            perror("Listen ");
            close(m_listen_fd);
            return ;
        }
        
        
        m_epollFD = epoll_create1(0);
        if(m_epollFD<0){
            perror("epoll_create1 ");
            close(m_listen_fd);
            return ;
        }

        epoll_event ev{};
        ev.events = EPOLLIN;
        ev.data.fd = m_listen_fd;


        if (epoll_ctl(m_epollFD, EPOLL_CTL_ADD, m_listen_fd, &ev) < 0) {
            perror("epoll_ctl ");
            close(m_listen_fd);
            close(m_epollFD);
            return;
        }


        m_last_tick_ns = GetTime_ns();
        m_running = true;

        m_start_time_ns = GetTime_ns();
        m_end_time_ns   = m_start_time_ns + m_runDurationSec * 1'000'000'000ULL;
        run();
        close(m_listen_fd);

    }
    //Event based loop meaning every time a  client is connected 
    //this what will provide the data to every connected client as required
    void run(){
        constexpr int MAX_EVENTS=64;
        epoll_event events[MAX_EVENTS];

        while(m_running){

            if (GetTime_ns() >= m_end_time_ns) {
                m_running = false;
                break;
            }
            int n = epoll_wait(m_epollFD, events, MAX_EVENTS, 0);
            for(int i=0;i<n;i++){
                int temp_fd = events[i].data.fd;
                if(temp_fd==m_listen_fd){
                    handle_new_connection();
                }
                else if(events[i].events &(EPOLLHUP | EPOLLERR)){
                    handle_client_disonnect(temp_fd);
                }
            }
            //TICKS GENERATION
            uint64_t time_now = GetTime_ns();

            while(time_now-m_last_tick_ns>=m_tick_interval_ns){
                // m_last_tick_ns=time_now;
                m_last_tick_ns += m_tick_interval_ns;
                uint16_t symbolID = PickSymbol();
                generate_ticks(symbolID);
            }
        }

    }
    void set_tick_rate(uint32_t ticksPerSeconds){

    }

    void enable_fault_injection(){

    }

    private:
    uint16_t PickSymbol(){
        assert(!m_activeSymbols.empty());

        std::uniform_int_distribution<size_t> dist(
            0, m_activeSymbols.size() - 1
        );

        size_t idx = dist(m_scheduler_rng);
        return m_activeSymbols[idx];
    }

    
    // void generate_ticks(uint16_t symbolId){
    //     SymbolData& tempSymbolData= m_symbolState[symbolId];
    //     static thread_local std::normal_distribution<double> normal(0.0, 1.0);
    //     /*Geometric Brownian Motion Tick Generator:*/
    //     double dW = normal(tempSymbolData.st_rng);
    //     double dt = m_dt;
    //     double mu = tempSymbolData.st_symbolMU;
    //     double sigma = tempSymbolData.st_symbolSIGMA;
    //     //calculate Symbol Price 
    //     tempSymbolData.st_symbolPrice *= std::exp((mu - 0.5 * sigma * sigma) * dt + sigma * std::sqrt(dt) * dW);

    //     //Calculate Ask and Bid Component here
    //     double halfSpread = tempSymbolData.st_symbolSpread *0.5;
    //     tempSymbolData.st_bidPrice = tempSymbolData.st_symbolPrice * (1.0 - halfSpread);
    //     tempSymbolData.st_askPrice = tempSymbolData.st_symbolPrice * (1.0 + halfSpread);

    //     tempSymbolData.st_symbolSequenceNumber++;
    //     tempSymbolData.st_timeStamp = GetTime_ns();


    //     MarketMessage msg = build_message(tempSymbolData);
    //     broadcast_message(&msg, sizeof(msg));
    // }

    void generate_ticks(uint16_t symbol_id){
        SymbolData& tempSymbolData = m_symbolState[symbol_id];

        // 1. Evolve price ONCE
        EvolvePrice(tempSymbolData);

        // 2. Update bid / ask
        Update_Quote_Prices(tempSymbolData);

        // 3. Decide message type (70/30)
        if (Is_Quote_Message()) {
            BuildAndSendQuote(tempSymbolData);
        } else {
            BuildAndSendTrade(tempSymbolData);
        }
    }
    bool Is_Quote_Message(){
        static thread_local std::uniform_real_distribution<double> dist(0.0, 1.0);
        double r = dist(m_scheduler_rng);
        return r < MSGQuoteRatio;  // e.g. 0.70
    }

    void BuildAndSendQuote(SymbolData& temp_symbolData) {
        MarketMessage msg{};
        msg.type = MessageType::QUOTE;
        msg.symbol_id = temp_symbolData.st_symbolID;
        msg.sequence = ++temp_symbolData.st_symbolSequenceNumber;
        msg.timestamp_ns = GetTime_ns();

        msg.quote.bid_price = temp_symbolData.st_bidPrice;
        msg.quote.ask_price = temp_symbolData.st_askPrice;

        // Random but stable quote sizes
        msg.quote.bid_qty = Random_Quote_Qty(temp_symbolData);
        msg.quote.ask_qty = Random_Quote_Qty(temp_symbolData);
        msg.assignSequence();
        // msg.printinfoquote();
        // broadcast_message(&msg, sizeof(msg));
    }

    uint32_t Random_Quote_Qty(SymbolData& temp_symbolData){
        // Log-normal gives realistic size distribution
        static thread_local std::lognormal_distribution<double> qty_dist(
            3.5,   // mean (log space)
            0.8    // stddev
        );

        uint32_t qty = static_cast<uint32_t>(qty_dist(temp_symbolData.st_rng));

        // Clamp to reasonable bounds
        if (qty < 10)    qty = 10;
        if (qty > 10000) qty = 10000;

        return qty;
    }

    void Update_Quote_Prices(SymbolData& temp_symbolData){
        double halfSpread = temp_symbolData.st_symbolSpread * 0.5;

        temp_symbolData.st_bidPrice = temp_symbolData.st_symbolPrice * (1.0 - halfSpread);
        temp_symbolData.st_askPrice = temp_symbolData.st_symbolPrice * (1.0 + halfSpread);

        // Safety invariant
        if (temp_symbolData.st_bidPrice >= temp_symbolData.st_askPrice) {
            temp_symbolData.st_bidPrice = temp_symbolData.st_symbolPrice * 0.999;
            temp_symbolData.st_askPrice = temp_symbolData.st_symbolPrice * 1.001;
        }
    }

    void EvolvePrice(SymbolData& temp_symbolData){
        // Normal distribution for Wiener process
        static thread_local std::normal_distribution<double> normal(0.0, 1.0);

        double dW = normal(temp_symbolData.st_rng);

        double mu    = temp_symbolData.st_symbolMU;
        double sigma = temp_symbolData.st_symbolSIGMA;
        double dt    = m_dt;

        // Geometric Brownian Motion
        temp_symbolData.st_symbolPrice *= std::exp((mu - 0.5 * sigma * sigma) * dt + sigma * std::sqrt(dt) * dW);

        // Safety guard (prices should never go negative)
        if (temp_symbolData.st_symbolPrice < 0.01) {
            temp_symbolData.st_symbolPrice = 0.01;
        }
    }

    void BuildAndSendTrade(SymbolData& temp_symbolData) {
        MarketMessage msg{};
        msg.type = MessageType::TRADE;
        msg.symbol_id = temp_symbolData.st_symbolID;
        msg.sequence = ++temp_symbolData.st_symbolSequenceNumber;
        msg.timestamp_ns = GetTime_ns();

        // Trade executes at bid or ask
        bool aggressor_buy = Random_Bool(temp_symbolData);
        msg.trade.aggressor_buy = aggressor_buy;

        msg.trade.trade_price = aggressor_buy ? temp_symbolData.st_askPrice : temp_symbolData.st_bidPrice;

        msg.trade.trade_qty = Random_Trade_Qty(temp_symbolData);
        msg.assignSequence();
        // msg.printinfotrade();
        // broadcast_message(&msg, sizeof(msg));
    }

    bool Random_Bool(SymbolData& s){
        static thread_local std::bernoulli_distribution dist(0.5);
        return dist(s.st_rng);
    }

    uint32_t Random_Trade_Qty(SymbolData& temp_symbolData){
        // Trades are usually smaller than quote depth
        static thread_local std::uniform_int_distribution<uint32_t> trade_qty_dist(10, 2000);

        return trade_qty_dist(temp_symbolData.st_rng);
    }

    // void broadcast_message(const void* data, size_t len){

    // }
    void broadcast_message(const void* data, size_t len)
    {
        for (auto it = clients.begin(); it != clients.end(); ) {
            int fd = *it;

            ssize_t sent = send(fd, data, len, MSG_DONTWAIT | MSG_NOSIGNAL);

            if (sent < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // Client is slow → drop it (simple policy)
                    handle_client_disonnect(fd);
                    it = clients.erase(it);
                    continue;
                } else {
                    // Fatal error
                    handle_client_disonnect(fd);
                    it = clients.erase(it);
                    continue;
                }
            }

            // Partial send is treated as slow consumer
            if (static_cast<size_t>(sent) != len) {
                handle_client_disonnect(fd);
                it = clients.erase(it);
                continue;
            }

            ++it;
        }
    }


    // void handle_client_disonnect(int clientFD){

    // }
    void handle_client_disonnect(int clientFD)
    {
        epoll_ctl(m_epollFD, EPOLL_CTL_DEL, clientFD, nullptr);
        close(clientFD);

        clients.erase(clientFD);

        // Optional logging
        // std::cout << "Client disconnected: fd=" << clientFD << "\n";
    }


    // void handle_new_connection(){

    // }

    void handle_new_connection()
    {
        while (true) {
            sockaddr_in client_addr{};
            socklen_t addr_len = sizeof(client_addr);

            int client_fd = accept(m_listen_fd,
                                (sockaddr*)&client_addr,
                                &addr_len);

            if (client_fd < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // No more clients to accept
                    break;
                } else {
                    perror("accept");
                    break;
                }
            }

            // Set non-blocking
            int flags = fcntl(client_fd, F_GETFL, 0);
            if (flags < 0 || fcntl(client_fd, F_SETFL, flags | O_NONBLOCK) < 0) {
                perror("fcntl client");
                close(client_fd);
                continue;
            }

            // Register with epoll
            epoll_event ev{};
            ev.events = EPOLLERR | EPOLLHUP;  // read not required (broadcast-only)
            ev.data.fd = client_fd;

            if (epoll_ctl(m_epollFD, EPOLL_CTL_ADD, client_fd, &ev) < 0) {
                perror("epoll_ctl client");
                close(client_fd);
                continue;
            }

            clients.insert(client_fd);

            // Optional logging
            // std::cout << "Client connected: fd=" << client_fd << "\n";
        }
    }


    SymbolData GenerateSymbol(uint16_t symbolId,const std::unique_ptr<ConfigManager>& cfg){
        SymbolData temp{};  //creating a temporary SymbolData

        temp.st_symbolID=symbolId;
        temp.st_symbolSequenceNumber=0;

        temp.st_rng.seed(cfg->m_seed^symbolId);

        std::uniform_real_distribution<double> priceDist(cfg->m_priceMin, cfg->m_priceMax);
        temp.st_symbolPrice=priceDist(temp.st_rng);
        

        std::uniform_real_distribution<double> volatileDist(cfg->m_volatilityMin,cfg->m_volatilityMax);
        temp.st_symbolSIGMA=volatileDist(temp.st_rng);

        std::uniform_real_distribution<double> spreadDist(cfg->m_spreadMin,  cfg->m_spreadMax);
        temp.st_symbolSpread=spreadDist(temp.st_rng);

        temp.st_symbolMU=cfg->m_marketDrift;

        const double halfSpread = temp.st_symbolSpread * 0.5;
        temp.st_bidPrice = temp.st_symbolPrice*(1-halfSpread);
        temp.st_askPrice  = temp.st_symbolPrice*(1+halfSpread);

        temp.st_timeStamp = GetTime_ns();

        return temp;
    }

    private:
    //Symbol Generation and Storage
    std::array<SymbolData, 501> m_symbolState;
    std::vector<int16_t> m_activeSymbols;
    size_t m_activeSymbolCounts;

    //server Properties
    std::string m_bind_IP;
    uint16_t m_port;
    int m_listen_fd;
    int m_epollFD;

    //Generation Parameters
    uint32_t m_ticksPerSeconds;
    
    std::unordered_set<int> clients;  // connected client sockets


    //Ticks Scheduling
    uint64_t m_tick_interval_ns;
    uint64_t m_last_tick_ns;
    uint32_t m_ticks_per_second;
    double m_dt;
    
    std::mt19937_64 m_scheduler_rng;
    std::uniform_int_distribution<size_t> m_symbol_dist;

    uint64_t m_start_time_ns;
    uint64_t m_end_time_ns;
    bool m_running;
    uint64_t m_runDurationSec;


};

#endif