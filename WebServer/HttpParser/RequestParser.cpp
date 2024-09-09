#include "RequestParser.hpp"
#include "../Utilities/Utilities.hpp"
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cstdio>
#include <string.h>

// CHECK: Check if everything looks well with the whitespaces trim, or more chars have to be added.

RequestParser::RequestParser(char * request): _request(request), _status_code(200), _ParsingCompleted(false) {
    consume_request();
    print_request();                // FOR TESTING
}

RequestParser::RequestParser(): _request(""), _status_code(200), _ParsingCompleted(false) {}

RequestParser::RequestParser(const RequestParser &src) {
    *this = src;
}

RequestParser::~RequestParser() {}

void    RequestParser::add_header(std::string key, std::string value) {
    this->_headers[key] = value;
}

RequestParser &RequestParser::operator=(const RequestParser &src)
{
    if (this != &src) {
        this->_request = src._request;
        this->_method = src._method;
        this->_uri = src._uri;
        this->_protocol = src._protocol;
        this->_headers = src._headers;
        this->_body = src._body;
        this->_ParsingCompleted = src._ParsingCompleted;
        this->_status_code = src._status_code;
		this->_content_remaining = src._content_remaining;
    }
    return *this;
}

bool    RequestParser::parsingCompleted() const {
    return this->_ParsingCompleted;
}

std::string RequestParser::get_method() const {
    return (this->_method);
}

std::string RequestParser::get_uri() const {
    return (this->_uri);
}

std::string RequestParser::get_protocol() const {
    return (this->_protocol);
}

int 		RequestParser::get_header_length() const {
	return(this->_header_length);
}

std::vector<char> RequestParser::get_body() const {
    return (this->_body);
}
int RequestParser::get_content_remaining() const {
    return (this->_content_remaining);
}

std::string RequestParser::get_content_type() const {
    if (_headers.count("Content-Type") > 0) {
        return _headers.at("Content-Type");
    } else {
        return "";
    }
}


size_t RequestParser::get_content_length() const {
    if (_headers.count("Content-Length") > 0) {
        return std::stoi(_headers.at("Content-Length"));
    } else {
        return 0;
    }
}

std::string RequestParser::get_content_disposition() const {
	if (_headers.count("Content-Disposition") > 0) {
        return (_headers.at("Content-Disposition"));
    } else {
        return "";
    }
}

int RequestParser::get_status_code() const {
    return (this->_status_code);
}

std::string RequestParser::find_header(std::string key) const{
    if (_headers.find(key) != _headers.end()) {
        return (_headers.at(key));
    } else {
        return "";
    }
}

void    RequestParser::parse_error(const std::string &str, int code) {
    this->_status_code = code;
    std::cerr << code << " " << str << std::endl;
}

void    RequestParser::tokenize(const std::string& str, std::vector<std::string>& tokens, char delimiter) {
    std::istringstream iss(str);
    std::string token;
    while (std::getline(iss, token, delimiter)) {
        tokens.push_back(token);
    }
}

void    RequestParser::validate_request_line(){
    if (this->_method != "GET" && this->_method != "POST" && this->_method != "DELETE")
        parse_error("Method Not Allowed", 405);
    // not sure what to check here
    // else if ()
    //         parse_error("Bad Request", 400);
    else if (this->_protocol != "HTTP/1.1")
            parse_error("HTTP Version Not Supported", 505);

}

void    RequestParser::decode_uri(){

    std::string     decoded;
    size_t          len = this->_uri.length();

    for (size_t i = 0; i < len; i++)
    {
        // check if % is available and at least two more ints behind it
        if (_uri[i] == '%' && i + 2 < len && isxdigit(_uri[i + 1]) && isxdigit(_uri[i + 2]))
        {
            int hex;
            // check if its a valid hex representation
            if (sscanf(this->_uri.substr(i + 1, 2).c_str(), "%x", &hex) == 1) 
            {
                decoded += static_cast<char>(hex);
                i += 2;
            } else 
                decoded += this->_uri[i];
        }
        // check for spaces
        else if (this->_uri[i] == '+')
            decoded += ' ';
        else
            decoded += this->_uri[i];
    }
    this->_uri = decoded;
}

void    RequestParser::validate_header(std::string line){

    size_t colon_pos = line.find(':');


    if (colon_pos != std::string::npos)
    {
        std::string header_name = line.substr(0, colon_pos);
        std::string header_value = line.substr(colon_pos + 1);

        this->_headers.insert(std::make_pair(header_name, header_value));
    } else
    {
        parse_error("Invalid Header Format", 400);
    }
}

bool    RequestParser::validate_content(std::string line){
    if (line.empty()){
    	return(0);
	}
	
	if (line.find("Content-Disposition:") != std::string::npos || line.find("Content-Type:") != std::string::npos)
	{
		size_t colon_pos = line.find(':');

		if (colon_pos != std::string::npos)
		{
			std::string header_name = line.substr(0, colon_pos);
			std::string header_value = line.substr(colon_pos + 1);

			this->_headers.insert(std::make_pair(header_name, header_value));
		}
		return(1);
	}
	
    return(0);
}

void RequestParser::set_content_disposition(const char *request)
{
	std::string line;
	std::istringstream raw_body(request);
	while (std::getline(raw_body, line))
	{
		if (line.find("Content-Disposition:") != std::string::npos)
		{
			size_t colon_pos = line.find(':');

			if (colon_pos != std::string::npos)
			{
				std::string header_name = line.substr(0, colon_pos);
				std::string header_value = line.substr(colon_pos + 1);

				this->_headers.insert(std::make_pair(header_name, header_value));
			}
		}
	}
}

void RequestParser::fill_body(const char *_request, int bytesReceived)
{

	const char * temp_body;
	//if its the first read of the request, skip to the body part 
	if (_body.size() == 0)
	{
		temp_body = strstr(_request, "\r\n\r\n");
		if (temp_body == NULL)
			return;
		this->_content_remaining += 4;//adding this up because the seperators are included in bytesreceived
	}else
		temp_body = _request;
	
	set_content_disposition(_request);

	//copy amount of bytes read into the body
	size_t length = bytesReceived;
	for(size_t i = 0; i < length; i++)
	{
		_body.push_back(temp_body[i]);
	}
	this->_content_remaining -= bytesReceived;

}

void	RequestParser::unchunk_body()
{
	std::string 		result;
	std::stringstream 	stream;
	size_t 				total_size = 0;

	for (size_t i = 4; i < this->_body.size(); i++)
		stream << this->_body[i];
	while (true)
	{
  		std::string chunkSizeLine;
        std::getline(stream, chunkSizeLine); // Read the chunk size line
        if (chunkSizeLine.empty()) {
            break; // End of chunked data
        }

        // Parse the chunk size from the line
        size_t chunkSize = std::stoul(chunkSizeLine, 0, 16);
		total_size += chunkSize;
        // Read the chunk data
        std::vector<char> chunkData(chunkSize);
        stream.read(&chunkData[0], chunkSize);

        // Skip the CRLF after the chunk data
        char crlf[2];
        stream.read(crlf, 2);

        // Append the chunk data to the result
        result.append(chunkData.begin(), chunkData.end());
    }
	this->_body.clear();
	this->_body.push_back('\r');
	this->_body.push_back('\n');
	this->_body.push_back('\r');
	this->_body.push_back('\n');
	for(size_t i = 0; i < total_size; i++)
		this->_body.push_back(result[i]);

}

//TO DO: body parsen van deze functie los maken zodat hij multiparts ook via curl kan doen.
void RequestParser::consume_request(){

    std::string                 raw_request = this->_request;
    std::vector<std::string>    parsed_request;
    std::istringstream          iss(raw_request);
    std::string                 line;
    std::vector<std::string>    words;
    //size_t                      colon_pos;

    ParseState                  state = RequestLineParsing;


    if (raw_request.empty())
        parse_error("Bad Request", 400);
    //char first_char = line[0];

    while (std::getline(iss, line)) {
        parsed_request.push_back(line);
        utility::stringTrim(line, "\r");
        
        switch(state) {
            case RequestLineParsing:
            tokenize(line, words, ' ');

            if (words.size() >= 3) {
                this->_method = words[0];
                this->_uri = words[1];
                this->_protocol = words[2];
                decode_uri();
                validate_request_line();
            } else {
                parse_error("Invalid Request Format", 400);
				return ;
            }

            state = HeadersParsing;
            break;

            case HeadersParsing:
                if (line.empty()) {
                    state = BodyParsing;
                    break;
                } else
                    validate_header(line);
                break;

            case BodyParsing:
                if (validate_content(line))
                    break;
                break;
         }
    }
	this->_header_length = raw_request.find("\r\n\r\n");
	this->_content_remaining = this->get_content_length();
    this->_ParsingCompleted = true;
}

void RequestParser::print_request() const {
    //std::cout << "Request: " << _request << std::endl;
    std::cout << "--Incoming request--" << std::endl;
    std::cout << "Method: " << _method << std::endl;
    std::cout << "URI: " << _uri << std::endl;
    std::cout << "Protocol: " << _protocol << std::endl;

    std::cout << "Headers:" << std::endl;
    for (const auto& header : _headers) {
        std::cout << "  " << header.first << ": " << header.second << std::endl;
    }

	// ONLY FOR WHEN POST METHOD
	// std::cout << "Body: " << std::endl;
	// std::vector<char> body = get_body();
	// std::vector<char>::iterator it = body.begin();

	// for (size_t i = 0; i < this->get_content_length(); i++)
	// {
    // 	std::cout << *it;
	// 	it++;
	// }
	//std::cout << std::endl;
	
	std::cout << "Header length: " << _header_length << std::endl;
    std::cout << "Content length: " << this->get_content_length() << std::endl;
    std::cout << "Content type: " << this->get_content_type() << std::endl;

}
