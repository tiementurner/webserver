#ifndef SERVER_HPP
# define SERVER_HPP

#include "ConfigParser/ServerConfig.hpp"

#include <sys/socket.h>
#include <string>

class Server
{

public:
    Server(ServerConfig config);
    Server(const Server &src);
    Server();
    ~Server();

    Server & operator=(Server const &rhs);

	int					get_socket() const;
	struct sockaddr_in	get_sock_addr() const;
    ServerConfig        get_config() const;

private:

    ServerConfig			        _config;
    int                             _socket;

    struct sockaddr_in				_socketAddr;

    void             	start_server();
	void    			exit_error(const std::string &str);
};

#endif
