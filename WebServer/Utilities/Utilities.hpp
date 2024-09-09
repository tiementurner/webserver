#ifndef UTILITY_HPP
#define UTILITY_HPP

#include <string>
#include "../HttpParser/RequestParser.hpp"
#include "../ConfigParser/ServerConfig.hpp"

namespace utility {
	unsigned long long	    get_max_size(std::string size);
    void                    stringTrim(std::string &str, const char *charset);
    long                    getCurrentTimeinSec(void);
    std::string             getMIMEType(std::string path);
	int		                upload_file(RequestParser *post_request, std::string uri, unsigned long long maxSize);
	int 					delete_resource(std::string uri);
}

#endif
