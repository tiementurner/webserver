#ifndef RequestParser_HPP
# define RequestParser_HPP

#include <string>
#include <map>
#include <vector>

class RequestParser
{
private:
    const char*                         _request;

    std::string                         _method;
    std::string                         _uri;
    std::string                         _protocol;

    std::map<std::string, std::string>  _headers;

    std::vector<char>                   _body;

    int                                 _status_code;

	int									_content_remaining;
	size_t								_header_length;

    bool                                _ParsingCompleted;

    void        add_header(std::string key, std::string value);
    void        consume_request();
    void        parse_error(const std::string &str, int code);
    void        tokenize(const std::string& str, std::vector<std::string>& tokens, char delimiter);
    void        validate_request_line();
    void        decode_uri();
    void        validate_header(std::string line);
    bool        validate_content(std::string line);
	void 		set_content_disposition(const char *request);
    void        print_request() const;

public:
    RequestParser(char * request);
    RequestParser();
    RequestParser(const RequestParser &src);
    ~RequestParser();
    RequestParser &operator=(const RequestParser &src);

	void 		fill_body(const char* temp_body, int bytesReceived);
	void		unchunk_body();
    bool        parsingCompleted() const;
	std::string get_content_disposition() const;
    std::string get_method() const;
    std::string get_uri() const;
    std::string get_protocol() const;
    std::vector<char> get_body() const;
    std::string get_content_type() const;
    size_t      get_content_length() const;
	int 		get_header_length() const;
	int	        get_content_remaining() const;
    int         get_status_code() const;

    std::string find_header(std::string key) const;
};

enum ParseState {
    RequestLineParsing,
    HeadersParsing,
    BodyParsing
};

// enum httpMethod
// {
//     GET,            // Read only: The HTTP GET request method is used to request a resource from the server. 
//     POST,           // Send data to server, create new resource: create new data entry
//     DELETE
// };

#endif