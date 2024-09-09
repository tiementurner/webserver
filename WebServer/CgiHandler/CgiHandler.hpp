#ifndef CGIHANDLER_HPP
# define CGIHANDLER_HPP

#include "../ConfigParser/ServerConfig.hpp"
#include "../HttpParser/RequestParser.hpp"

#include <string>
#include <map>

class CgiException : public std::runtime_error {
public:
    CgiException(const std::string& message) : std::runtime_error(message) {}
};

class CgiHandler
{
private:
    std::map<std::string, std::string>	    _environment;
    std::vector<char*>                      _childEnvp;
    char**                                  _argv;
    const char*                             _runLoc;

    static const size_t CGI_BUFSIZE = 4096;  // Adjust the size as needed

    void    initialize_environment(Location const &loc, RequestParser const &httprequest);
    void    print_env();
    void    execute_script(RequestParser const &httprequest);

public:
    CgiHandler(Location const &loc, RequestParser const &httprequest);
    ~CgiHandler();
    CgiHandler(const CgiHandler &src);
    CgiHandler &operator=(const CgiHandler &src);

    int                                     pipe_in[2];
    int                                     pipe_out[2];
    std::map<std::string, std::string>      get_environment() const;
};


#endif