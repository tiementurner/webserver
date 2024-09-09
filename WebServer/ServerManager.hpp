#ifndef SERVERMANAGER_HPP
# define SERVERMANAGER_HPP

#include "HttpParser/RequestParser.hpp"
#include "ResponseBuilder.hpp"
#include "Server.hpp"
#include "ConfigParser/ServerConfig.hpp"

#include <string>
#include <vector>
#include <map>
#include <poll.h>

class ServerManager
{
public:

    ServerManager(const std::vector<ServerConfig> &configs);
    ~ServerManager();

    void    exit_error(const std::string &str);

private:

    std::vector<Server>                 _servers;

    std::vector<struct pollfd>          _pollfds;
    std::map<int, Server>               _requestServerIndex;
    std::map<int, RequestParser>        _requests;
    std::map<int, std::string>          _responses;
    std::map<int, long>                 _timeOutIndex;
    std::map<int, int>                  _cgiIndex;
    std::map<int, std::string>          _cgiResponseIndex;


    int             start_server();
    void            close_server();

    void            start_listen();
    void            accept_connection(int incoming);
    void            close_connection(int client_socket);

    int             send_response(int socket_fd);
    void            check_timeout(void);
    std::string     build_cgi_error(int pipe_fd);
};

#endif
