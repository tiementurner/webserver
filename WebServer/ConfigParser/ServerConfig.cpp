#include "ServerConfig.hpp"

#include <netinet/in.h>
#include <iostream>

ServerConfig::ServerConfig():
    _port(80),
    _host(0),
    _serverName(""),
    _maxSize(1000000),
    _rootDirectory("") {}


ServerConfig::~ServerConfig() {}

ServerConfig::ServerConfig(const ServerConfig &src) {
    *this = src;
}

ServerConfig &ServerConfig::operator=(const ServerConfig &src) {
    if (this != &src) {
        this->_port = src._port;
        this->_host = src._host;
        this->_serverName = src._serverName;
        this->_maxSize = src._maxSize;
        this->_errorPages = src._errorPages;
        this->_rootDirectory = src._rootDirectory;
        this->_index = src._index;
        this->_locations = src._locations;
    }
    return *this;
}

void ServerConfig::set_port(int port) {
    this->_port = port;
}

void ServerConfig::set_host(in_addr_t &host) {
    this->_host = host;
}

void ServerConfig::set_servername(const std::string serverName) {
    this->_serverName = serverName;
}

void ServerConfig::set_maxsize(size_t maxSize) {
    this->_maxSize = maxSize;
}

void ServerConfig::set_error_pages(int code, std::string &errorPage) {
    this->_errorPages[code] = errorPage;
}

void ServerConfig::set_rootdirectory(const std::string rootDirectory) {
    this->_rootDirectory = rootDirectory;
}

void ServerConfig::set_index(std::vector<std::string> &index) {
    this->_index = index;
}

void ServerConfig::set_location(const Location& location) {
    _locations.push_back(location);
}

void ServerConfig::set_cgiExtensions(const std::string ext, const std::string program) {
    _cgiExtensions[ext] = program;
}

int ServerConfig::get_port() const {
    return (this->_port);
}

in_addr_t   ServerConfig::get_host() const {
    return (this->_host);
}

std::string   ServerConfig::get_servername() const {
    return (this->_serverName);
}

size_t    ServerConfig::get_maxsize() const {
    return (this->_maxSize);
}

std::map<int, std::string>    ServerConfig::get_errorpages() const {
    return (this->_errorPages);
}

std::string   ServerConfig::get_rootdirectory() const {
    return (this->_rootDirectory);
}

std::vector<std::string>    ServerConfig::get_index() const {
    return (this->_index);
}

std::vector<Location>   ServerConfig::get_locations() const {
    return (this->_locations);
}
