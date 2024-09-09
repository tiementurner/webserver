#include "WebServer/ConfigParser/ServerConfig.hpp"
#include "WebServer/ConfigParser/ConfigParser.hpp"
#include "WebServer/ServerManager.hpp"
#include "WebServer/Server.hpp"

#include <iostream>
#include <csignal>

int main(int argc, char **argv){
    if (argc > 2) {
        std::cout << "Provide one config file or none for the default configuration" << std::endl;
        return (0);
    }
    std::string configPath;
    if (argc == 2) {
        configPath = argv[1];
    }
    if (configPath.empty()) {
        configPath = "WebServer/ConfigParser/DefaultConfig/default.conf";
    }
    signal(SIGCHLD, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);
    try{
        ConfigParser parsed(configPath);
        parsed.read_and_parse_config();
        std::vector<ServerConfig> servers = parsed.get_serverconfig();
        ServerManager manager(servers);
    }
    catch(const ConfigParserException& e) {
        std::cerr << "ConfigParser Error: " << e.what() << std::endl;
    }
}
