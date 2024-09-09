#include "ResponseBuilder.hpp"
#include "CgiHandler/CgiHandler.hpp"
#include "Utilities/Utilities.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>

ResponseBuilder::ResponseBuilder(RequestParser &request, ServerConfig config): _request(request), _config(config) {
    this->_cgiPipeFd = 0; //default to 0 for not set
    this->_status_code = this->_request.get_status_code();
    this->_matched_location = match_location(this->_request.get_uri());
	if (_request.find_header("Transfer-Encoding") == " chunked")
		_request.unchunk_body();
	build_response();
}

ResponseBuilder::ResponseBuilder(const ResponseBuilder &src) {
    *this = src;
}

ResponseBuilder::~ResponseBuilder() {}

ResponseBuilder &ResponseBuilder::operator=(const ResponseBuilder &src)
{
    if (this != &src) {
        this->_request = src._request;
        this->_header = src._header;
		this->_body = src._body;
		this->_response = src._response;
        this->_status_code = src._status_code;
        this->_cgiPipeFd = src._cgiPipeFd;
    }
    return *this;
}

//build a directory listing for when autoindex is on
void        ResponseBuilder::build_dir_listing(std::string uri) {
    //build html string and set to this->_body at the end
    DIR             *directory;

    uri = uri.substr(0, uri.find_last_of('/'));
    directory = opendir(uri.c_str());
    if (!directory) {
        std::cout << "Could not open directory" << std::endl;
        this->_status_code = 500;
        uri = "error.html";
        std::ifstream htmlFile = open_error_page();
        if (!htmlFile.good()) {
            htmlFile.close();
            std::cout << "error page can't be opened for some reason" << std::endl;
            build_header("");
            this->_response = this->_header;
            return;
        }
        std::stringstream buffer;
        buffer << htmlFile.rdbuf();
	    htmlFile.close();
        this->_body = buffer.str();
        build_header(uri);
	    this->_response = this->_header;
	    this->_response.append(this->_body);
        return;
    }
    this->_body = "<html><head><title> Index of " + this->_request.get_uri();
    this->_body.append("</title></head><body><h1> Index of " + this->_request.get_uri());
    this->_body.append("</h1><br>");
    for (struct dirent *entry = readdir(directory); entry; entry = readdir(directory)) {
        this->_body.append("<a href=\"./" + std::string(entry->d_name) + "\">" + std::string(entry->d_name) + "</a></br>");
    }
    this->_body.append("</body></html>");
    build_header("autoindex.html");
    this->_response = this->_header;
	this->_response.append(this->_body);
    closedir(directory);
}

//if a config file has 2 locations with returns pointing at each other
//that would likely cause a stack overflow because of the recursion here
//maybe try to catch that somehow?
Location    ResponseBuilder::match_location(std::string uri) {
    Location empty{};
    std::vector<Location> locations = this->_config.get_locations();
    for (std::vector<Location>::iterator it = locations.begin(); it != locations.end(); it++) {
        if (it->path != "/") {
            if (uri.find(it->path, 0) != std::string::npos) {
                if (it->returnPath.empty()) {
                    return (*it);
                }
                else {
                    return (match_location(it->returnPath));
                }
            }
        }
    }
    //loop again and find a potential match for just a '/' which should always match if it exists
    for (std::vector<Location>::iterator it = locations.begin(); it != locations.end(); it++) {
        if (it->path == "/") {
            if (it->returnPath.empty()) {
                return (*it);
            }
            else {
                return (match_location(it->returnPath));
            }
        }
    }
    //if there are no locations or no matches, return an empty location struct
    return (empty);
}

void        ResponseBuilder::build_header(std::string uri) {
    std::ostringstream ss;
    if (this->_request.get_protocol().empty())
        ss << "HTTP/1.1";
    else
        ss << this->_request.get_protocol();
    ss << " " << this->_status_code;
    switch(this->_status_code) {
        case 200:
            ss << " OK\n";
            break;
        case 201:
            ss << " Created\n";
            break;
        case 400:
            ss << " Bad Request\n";
            break;
        case 401:
            ss << " Unauthorized\n";
            break;
        case 403:
            ss << " Forbidden\n";
            break;
        case 404:
            ss << " Not Found\n";
            break;
        case 405:
            ss << " Method Not Allowed\n";
            break;
		case 409:
			ss << " Conflict\n";
			break;
		case 411:
			ss << " Length Required\n";
			break;
        case 413:
            ss << " Payload Too Large\n";
            break;
		case 415:
			ss << " Unsupported Media Type\n";
			break;
        case 500:
            ss << " Internal Server Error\n";
            break;
        case 501:
            ss << " Not Implemented\n";
            break;
        case 502:
            ss << " Bad Gateway\n";
            break;
        case 503:
            ss << " Service Unavailable\n";
            break;
        case 504:
            ss << " Gateway Timeout\n";
            break;
        default:
            ss << " Some Other Status\n";
    }
    ss << "Content-Type: " << utility::getMIMEType(uri) << "\n";
    ss << "Content-Length: " << this->_body.size() << "\n\n"; //header ends with an empty line
	this->_header = ss.str();
}

std::string ResponseBuilder::process_uri() {
    std::string uri = this->_request.get_uri();
    if (!this->_matched_location.path.empty()) {
        std::cout << "matched a location " << this->_matched_location.path << "!" << std::endl;
        if (uri.find(this->_matched_location.path, 0) == std::string::npos) {
            //if uri and path don't match, this was a redirect
            //and we should change the uri accordingly
            //erase until a slash then add path instead
            //if there are no slashes, replace whole uri with path
            if (uri.front() == '/') {
                uri.erase(0, 1);
            }
            if (uri.find_first_of('/') != std::string::npos) {
                uri.erase(0, uri.find_first_of('/'));
                uri.insert(0, this->_matched_location.path);
            }
            else {
                uri = this->_matched_location.path;
            }
        }
        if (this->_matched_location.root.empty()) {
            uri.insert(0, this->_config.get_rootdirectory());
        }
        uri.insert(0, this->_matched_location.root);
        if (!this->_matched_location.cgiExtensions.empty()) {
            std::cout << "cgi found in matched location!" << std::endl;
            CgiHandler cgi(this->_matched_location, this->_request);
            //the output of the cgi script can be read from pipe_out[0]
            this->_cgiPipeFd = cgi.pipe_out[0];
            return ("CGI_MATCHED");
        }
    }
    else {
        //no location match
        uri.insert(0, this->_config.get_rootdirectory());
    }
    struct stat s;
    if (lstat(uri.c_str(), &s) == 0) {
        if (S_ISDIR(s.st_mode)) {
            std::cout << "is a directory" << std::endl;
            if (this->_request.get_method() == "GET" && uri.back() != '/') {
                uri.append("/");
            }
        }
    }
    std::cout << "final processed uri: " << uri << std::endl;
    return (uri);
}

std::ifstream   ResponseBuilder::open_error_page() {
    std::ifstream errorPageFile;
    if (this->_config.get_errorpages().count(this->_status_code)) {
        errorPageFile.open(this->_config.get_errorpages().at(this->_status_code));
    }
    else {
        switch(this->_status_code) {
            case 400:
                errorPageFile.open("WebServer/ConfigParser/DefaultErrorPages/400BadRequest.html");
                break;
            case 401:
                errorPageFile.open("WebServer/ConfigParser/DefaultErrorPages/401Unauthorized.html");
                break;
            case 404:
                errorPageFile.open("WebServer/ConfigParser/DefaultErrorPages/404NotFound.html");
                break;
            case 405:
                errorPageFile.open("WebServer/ConfigParser/DefaultErrorPages/405MethodNotAllowed.html");
                break;
            case 500:
                errorPageFile.open("WebServer/ConfigParser/DefaultErrorPages/500InternalServer.html");
                break;
            case 502:
                errorPageFile.open("WebServer/ConfigParser/DefaultErrorPages/502BadGateway.html");
                break;
            case 503:
                errorPageFile.open("WebServer/ConfigParser/DefaultErrorPages/503ServiceUnavailable.html");
                break;
            case 504:
                errorPageFile.open("WebServer/ConfigParser/DefaultErrorPages/504GatewayTimeout.html");
                break;
            default:
                errorPageFile.open("WebServer/ConfigParser/DefaultErrorPages/DefaultError.html");
        }
    }
    return (errorPageFile);
}

void	ResponseBuilder::build_response() {
    bool location_matched = true;
    if (this->_matched_location.path.empty())
        location_matched = false;
    std::string uri("");
    std::ifstream htmlFile;
    if (this->_status_code == 200) {
        uri = process_uri();
        if (uri == "CGI_MATCHED") { //cgi handles response
            return ;
        }
    }
    if (this->_request.get_method() == "POST"){
        if (location_matched) {
            if (!this->_matched_location.methods[POST_METHOD]) {
                this->_status_code = 405;
            }
        }
        if (this->_status_code == 200) {
            if (!this->_request.find_header("Content-Length").empty())
				this->_status_code = utility::upload_file(&this->_request, uri, this->_config.get_maxsize());
			else 
				this->_status_code = 411;
            if (this->_status_code == 201) {
                build_header(uri);
                this->_response = this->_header;
                return;
            }
        }
    }
    else if (this->_request.get_method() == "GET") {
        if (location_matched) {
            if (!this->_matched_location.methods[GET_METHOD]) {
                this->_status_code = 405;
            }
        }
        if (this->_status_code == 200) {
            std::string dflt_index;
            if (location_matched) {
                dflt_index = this->_matched_location.index;
            }
            if (dflt_index.empty()) {
                if (!this->_config.get_index().empty()) {
                    dflt_index = this->_config.get_index().front();
                }
            }
            if (dflt_index.empty()) {
                dflt_index = "index.html";
            }
            if (uri.back() == '/') {
                uri.append(dflt_index);
                htmlFile.open(uri.c_str());
                if (!htmlFile.good() && this->_status_code == 200) {
                    htmlFile.close();
                    std::cout << "no index file found" << std::endl;
                    //if autoindex is on for the matched location, send directory listing instead
                    if (location_matched) {
                        if (this->_matched_location.autoindex) {
                            build_dir_listing(uri);
                            return;
                        }
                    }
                    this->_status_code = 403;
                }
            }
            else {
                htmlFile.open(uri.c_str());
                if (!htmlFile.good() && this->_status_code == 200) {
                    htmlFile.close();
                    std::cout << "file can't be opened" << std::endl;
                    this->_status_code = 404;
                }
            }
        }
    }
    else if (this->_request.get_method() == "DELETE") {
        if (location_matched) {
            if (!this->_matched_location.methods[DELETE_METHOD]) {
                this->_status_code = 405;
            }
        }
        if (this->_status_code == 200) {
            this->_status_code = utility::delete_resource(uri);
		    build_header(uri);
		    this->_response = this->_header;
            return;
        }
    }
    else {
        this->_status_code = 501; //method not implemented
    }
    if (this->_status_code != 200) {
        htmlFile.close();
        uri = "error.html";
        htmlFile = open_error_page();
    }
	
    if (!htmlFile.good()) {
        htmlFile.close();
        std::cout << "error page can't be opened for some reason" << std::endl;
        build_header("");
        this->_response = this->_header;
        return ;
    }
    std::stringstream buffer;
    buffer << htmlFile.rdbuf();
	htmlFile.close();
    this->_body = buffer.str();
    build_header(uri);
	this->_response = this->_header;
	this->_response.append(this->_body);
}

std::string ResponseBuilder::get_response() {
	return (this->_response);
}

std::string ResponseBuilder::get_header() {
	return (this->_header);
}

int         ResponseBuilder::get_cgiPipeFd() {
	return (this->_cgiPipeFd);
}
