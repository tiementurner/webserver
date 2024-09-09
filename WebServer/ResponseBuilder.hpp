#ifndef RESPONSEBUILDER_HPP
# define RESPONSEBUILDER_HPP

#include "HttpParser/RequestParser.hpp"
#include "ConfigParser/ServerConfig.hpp"

#include <string>

class ResponseBuilder
{
private:
    RequestParser                       _request;
    ServerConfig                        _config;
    std::string                         _header;
    std::string                         _body;
    std::string                         _response;
    int                                 _status_code;
    int                                 _cgiPipeFd;
    Location                            _matched_location;

	std::string		getContentInfo(std::string content_type, std::string info);
    std::ifstream   open_error_page();
    void        	build_header(std::string uri);
    void        	build_response();
    std::string     process_uri();
    Location        match_location(std::string uri);
    void            build_dir_listing(std::string uri);

public:
    ResponseBuilder(RequestParser &request, ServerConfig config);
    ResponseBuilder(const ResponseBuilder &src);
    ~ResponseBuilder();
    ResponseBuilder &operator=(const ResponseBuilder &src);

    std::string     get_response();
    std::string     get_header();
    int             get_cgiPipeFd();
};

#endif
