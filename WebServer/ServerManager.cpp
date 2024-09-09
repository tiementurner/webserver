#include "ServerManager.hpp"
#include "Utilities/Utilities.hpp"

#include <iostream>
#include <sstream>
#include <fstream>
#include <unistd.h>
#include <algorithm>

const int BUFFER_SIZE = 30720;

ServerManager::ServerManager(const std::vector<ServerConfig> &configs)
{
    for (std::vector<ServerConfig>::const_iterator it = configs.begin(); it != configs.end() ;it++) {
        std::cout << "server: " << it->get_servername() << ", root: " << it->get_rootdirectory() << std::endl;
        this->_servers.push_back(Server(*it));
    }
    start_listen();
}

ServerManager::~ServerManager(){}

void    ServerManager::exit_error(const std::string &str)
{
    std::cerr << "Error - " << str << std::endl;
    exit(1);
}

void ServerManager::start_listen()
{
    std::vector<int> listeners;
    for (std::vector<Server>::iterator it = this->_servers.begin(); it != this->_servers.end() ;it++) {
        if (listen(it->get_socket(), 20) < 0)
        {
            exit_error("Socket listen failed");
        }
        std::ostringstream ss;
        ss << "\n*** Listening on ADDRESS: " 
            << inet_ntoa(it->get_sock_addr().sin_addr) 
            << " PORT: " << ntohs(it->get_sock_addr().sin_port) 
            << " ***\n\n";
        std::cout << ss.str();
        struct pollfd listener;

        listener.fd = it->get_socket();
        listener.events = POLLIN;
        listeners.push_back(it->get_socket());
        this->_pollfds.push_back(listener);
    }
    // main webserv loop starts here, the program should never exit this loop
    while (true)
    {
        std::cout << "====== Waiting for a new event ======" << std::endl << std::endl;
        if (poll(&this->_pollfds[0], this->_pollfds.size(), 10000) == -1) {
            std::cout << "Error returned from poll()" << std::endl;
        }
        for (std::vector<struct pollfd>::iterator it = this->_pollfds.begin(); it < this->_pollfds.end(); it++) {
            if (it->revents & POLLIN + POLLHUP) {
                if (std::find(listeners.begin(), listeners.end(), it->fd) != listeners.end()) {
                    accept_connection(it->fd);
                    break;
                }
                else if (this->_cgiIndex.count(it->fd)) {
                    //read cgi response
                    char buffer[BUFFER_SIZE] = {0};
                    int bytesReceived = read(it->fd, buffer, BUFFER_SIZE);
                    if (bytesReceived < 0)
                    {
                        std::cout << "Failed to read bytes from pipe connection" << std::endl;
						this->_cgiResponseIndex.insert({this->_cgiIndex.at(it->fd), build_cgi_error(it->fd)});
                        for (std::vector<struct pollfd>::iterator iter = this->_pollfds.begin(); iter < this->_pollfds.end(); iter++) {
                            if (iter->fd == this->_cgiIndex.at(it->fd)) {
                                iter->events = POLLOUT;
                            }
                        }
                        //close the pipe end
                        close(it->fd);
                        it = this->_pollfds.erase(it);
                        this->_cgiIndex.erase(it->fd);
						break;
                    }
                    //add bytes read to string and add it to cgi response index
                    std::string cgiResponse = buffer;
                    if (this->_cgiResponseIndex.count(this->_cgiIndex.at(it->fd))) {
                        this->_cgiResponseIndex.at(this->_cgiIndex.at(it->fd)).append(cgiResponse);
                    }
                    else {
                        this->_cgiResponseIndex.insert({this->_cgiIndex.at(it->fd), cgiResponse});
                    }
                    //find the pollfd of cgiIndex.at(it->fd) and set events to POLLOUT
                    if (bytesReceived == 0) {
                        if (this->_cgiResponseIndex.at(this->_cgiIndex.at(it->fd)).empty()) {
                            this->_cgiResponseIndex.at(this->_cgiIndex.at(it->fd)) = build_cgi_error(it->fd);
                        }
                        for (std::vector<struct pollfd>::iterator iter = this->_pollfds.begin(); iter < this->_pollfds.end(); iter++) {
                            if (iter->fd == this->_cgiIndex.at(it->fd)) {
                                iter->events = POLLOUT;
                            }
                        }
                        //close the pipe end
                        close(it->fd);
                        it = this->_pollfds.erase(it);
                        this->_cgiIndex.erase(it->fd);
                    }
                    break;
                }
                else {
                    char buffer[BUFFER_SIZE] = {0};
                    int bytesReceived = read(it->fd, buffer, BUFFER_SIZE);
                    if (bytesReceived < 0)
                    {
                        std::cout << "Failed to read from client socket, connection closed" << std::endl;
                        close_connection(it->fd);
                        break;
                    }
                    if (bytesReceived == 0) {
                        std::cout << "Client closed the connection" << std::endl;
                        close_connection(it->fd);
                        break;
                    }
                    //if request doesn't yet exist, create it. If it does, add the remaining bytes from the socket to the request body
                    if (this->_requests.find(it->fd) != this->_requests.end()){
                        this->_requests[it->fd].fill_body(buffer, bytesReceived);
                    }
                    else {
                        RequestParser request(buffer);
						//idee : content remaining gelijk zetten aan de laatst gelezen chunk
						
                        request.fill_body(buffer, bytesReceived - request.get_header_length());
                        this->_requests.insert({it->fd, request});
                    }
                    // if all bytes are read, POLLOUT the socket
                    if (this->_requests.count(it->fd)) {
                        if (this->_requests[it->fd].get_content_remaining() <= 0) { //to do: checken voor chunked requests?
                            std::cout << "------ Received Request from client ------" << std::endl << std::endl;
                            it->events = POLLOUT;
                        }
                    }
                    if (this->_timeOutIndex.count(it->fd)) {
                        this->_timeOutIndex.at(it->fd) = utility::getCurrentTimeinSec();
                    }
                    break;
                }
            }
            else if (it->revents & POLLOUT) {
                if (send_response(it->fd)) {
                    break;
                }
                else {
                    if (this->_timeOutIndex.count(it->fd)) {
                        this->_timeOutIndex.at(it->fd) = utility::getCurrentTimeinSec();
                    }
                    if (it->events) {
                        if (!this->_responses.count(it->fd)) {
                            it->events = POLLIN;
                        }
                    }
                }
                if (this->_requests.count(it->fd)) {
                    this->_requests.erase(it->fd);
                }
                break ;
            }
        }
        check_timeout();
    }

}

void ServerManager::accept_connection(int incoming)
{
    std::vector<Server>::iterator correctServer;
    for (std::vector<Server>::iterator it = _servers.begin(); it != _servers.end() ;it++) {
        if (it->get_socket() == incoming)
            correctServer = it;
    }
    sockaddr_in socketAddr = correctServer->get_sock_addr();
    unsigned int socketAddrLen = sizeof(socketAddr);
    int new_socket = accept4(correctServer->get_socket(), (sockaddr *)&socketAddr,
                        &socketAddrLen, SOCK_NONBLOCK);
    if (new_socket < 0)
    {
        std::cout << "Server failed to accept incoming connection from ADDRESS: "
        << inet_ntoa(socketAddr.sin_addr) << "; PORT: "
        << ntohs(socketAddr.sin_port) << std::endl;
        return ;
    }
    struct pollfd new_socket_fd;
    new_socket_fd.fd = new_socket;
    new_socket_fd.events = POLLIN;
    this->_pollfds.push_back(new_socket_fd);
    this->_requestServerIndex.insert({new_socket, *correctServer});
    this->_timeOutIndex.insert({new_socket, utility::getCurrentTimeinSec()});
    std::cout << "------ New connection established ------" << std::endl << std::endl;
}

//return 1 to close the connection after, 0 to keep it alive
int ServerManager::send_response(int socket_fd)
{
    unsigned long bytesSent;
    if (this->_cgiResponseIndex.count(socket_fd)) {
        //send cgi response instead of normal responsebuilder stuff
        this->_responses.insert({socket_fd, this->_cgiResponseIndex.at(socket_fd)});
    }
    else if (this->_requests.count(socket_fd)) {
        if (this->_requestServerIndex.count(socket_fd)) {
            ResponseBuilder response(this->_requests.at(socket_fd), this->_requestServerIndex.at(socket_fd).get_config());
            if (response.get_cgiPipeFd()) {
                //we need to read from the pipe and send that instead
                struct pollfd cgi_fd;
                cgi_fd.fd = response.get_cgiPipeFd();
                cgi_fd.events = POLLIN;
                this->_pollfds.push_back(cgi_fd);
                //map the socket_fd to send the read output to the cgi_fd
                this->_cgiIndex.insert({cgi_fd.fd, socket_fd});
                this->_requests.erase(socket_fd);
                for (std::vector<struct pollfd>::iterator it = this->_pollfds.begin(); it != this->_pollfds.end() ;it++) {
                    if (it->fd == socket_fd) {
                        it->events = 0;
                    }
                }
                return (0);
            }
            this->_responses.insert({socket_fd, response.get_response()});
            std::cout << response.get_header() << std::endl;
        }
    }
    if (!this->_responses.count(socket_fd)) {
        std::cout << "Error getting response, closing connection" << std::endl;
        close_connection(socket_fd);
        return (1);
    }
    bytesSent = write(socket_fd, this->_responses.at(socket_fd).c_str(), this->_responses.at(socket_fd).size());
    if (bytesSent <= 0) {
        std::cout << "Error sending response to client, closing connection" << std::endl;
        close_connection(socket_fd);
        return (1);
    }
    else if (bytesSent == this->_responses.at(socket_fd).size())
    {
        this->_responses.erase(socket_fd);
        std::cout << "------ Server Response sent to client ------" << std::endl << std::endl;
    }
    else {
        //write was incomplete, need to handle that!
        this->_responses.at(socket_fd) = this->_responses.at(socket_fd).substr(bytesSent, this->_responses.at(socket_fd).size());
        return (0);
    }
    if (this->_cgiResponseIndex.count(socket_fd)) {
        std::cout << "cgi response sent, closing connection" << std::endl;
        close_connection(socket_fd);
        return (1);
    }
    if (this->_requests.count(socket_fd)) {
        if (this->_requests.at(socket_fd).find_header("Connection") != " keep-alive" ) {
            std::cout << "Closing connection" << std::endl;
            close_connection(socket_fd);
			return (1);
        }
    }
    std::cout << "Keeping connection alive" << std::endl;
    return (0);
}

std::string ServerManager::build_cgi_error(int pipe_fd)
{
    //original error thing we had: this->_cgiResponseIndex.insert({this->_cgiIndex.at(it->fd), "Error"});
    //get the client socket id from cgi index
    int client_socket = -1;
    if (this->_cgiIndex.count(pipe_fd)) {
        client_socket = this->_cgiIndex.at(pipe_fd);
    }
    //find the serverconfig
    ServerConfig serverconfig;
    if (this->_requestServerIndex.count(client_socket)) {
        serverconfig = this->_requestServerIndex.at(client_socket).get_config();
    }
    //open the server config specified or default error page for error code 500
    std::ifstream errorPageFile;
    if (serverconfig.get_errorpages().count(500)) {
        errorPageFile.open(serverconfig.get_errorpages().at(500));
    }
    else {
        errorPageFile.open("WebServer/ConfigParser/DefaultErrorPages/500InternalServer.html");
    }
    //if that can't be opened, have body empty
    std::string error_body;
    if (!errorPageFile.good()) {
        errorPageFile.close();
    }
    else {
        std::stringstream buffer;
        buffer << errorPageFile.rdbuf();
        error_body = buffer.str();
        errorPageFile.close();
    }
    //build the header, status code 500, html, etc
    std::string header = "HTTP/1.1 500 Internal Server Error\nContent-Type: text/html\nContent-Length: ";
    std::cout << error_body.length() << std::endl;
    header.append(std::to_string(error_body.length()));
    header += "\n\n";
    //add header to body, set that to cgi response on the client socket id
    std::cout << header + error_body << std::endl;
    return (header + error_body);
}

void ServerManager::close_connection(int client_socket)
{
    //close the socket
    close(client_socket);
    //remove the fd from the array passed to poll()
    for (std::vector<struct pollfd>::iterator it = this->_pollfds.begin(); it != this->_pollfds.end() ; it++) {
        if (it->fd == client_socket) {
            it = this->_pollfds.erase(it);
            break;
        }
    }
    //remove from the client-server index map
    if (this->_requestServerIndex.count(client_socket)) {
        this->_requestServerIndex.erase(client_socket);
    }
    //remove from the client-request map (also deletes the request)
    if (this->_requests.count(client_socket)) {
        this->_requests.erase(client_socket);
    }
    //remove from the client-response map (also deletes the response)
    if (this->_responses.count(client_socket)) {
        this->_responses.erase(client_socket);
    }
    //remove from the client-timeout index
    if (this->_timeOutIndex.count(client_socket)) {
        this->_timeOutIndex.erase(client_socket);
    }
    //remove from the client-cgi response map (also deletes the cgi response)
    if (this->_cgiResponseIndex.count(client_socket)) {
        this->_cgiResponseIndex.erase(client_socket);
    }
    //find and close cgi pipe if there is one
    for (std::map<int, int>::iterator it = this->_cgiIndex.begin(); it != this->_cgiIndex.end() ; it++) {
        if (it->second == client_socket) {
            close(it->first);
            it = this->_cgiIndex.erase(it);
            break;
            //maybe kill the cgi child process if it's still running?
        }
    }
    std::cout << "Connection closed" << std::endl;
}

//Close connections that are idle for 30 seconds or more
void ServerManager::check_timeout(void)
{
    long current_time = utility::getCurrentTimeinSec();
    for(std::map<int, long>::iterator it = this->_timeOutIndex.begin(); it != this->_timeOutIndex.end(); it++) {
        long time_elapsed = current_time - it->second;
        if (time_elapsed >= 30) {
            std::cout << "Connection timed out after " << time_elapsed << "seconds" << std::endl;
            close_connection(it->first);
            check_timeout();
            break;
        }
    }
}
