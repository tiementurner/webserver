#include "Utilities.hpp"

#include <iostream>
#include <map>
#include <sys/time.h>
#include <sstream>
#include <fstream>
#include <iostream>
#include <exception>
#include <unistd.h>
#include <algorithm>

namespace utility {
	struct invalid_size: public std::exception {
	const char * what () const throw () {
		return "";
	}
	};

	unsigned long long		get_max_size(std::string size)
	{
		int multiplier = 0;
		if(size.size() == 0)
			return(1000000);
		size_t i = 0;
		for(; i < size.length() - 1; i++)
		{
			if (!isdigit(size[i]))
				throw invalid_size();
		}
		if (isdigit(size[i]))
			multiplier = 1;
		else if (size[i] == 'k' || size[i] == 'K')
			multiplier = 1000;
		else if (size[i] == 'm' || size[i] == 'M')
			multiplier = 1000000;
		else if (size[i] == 'g' || size[i] == 'G')
			multiplier = 1000000000;
		unsigned long long size_in_bytes = stoull(size) * multiplier;

		return size_in_bytes;
	}

    void    stringTrim(std::string &str, const char *charset) {

        size_t firstNonSpace = str.find_first_not_of(charset);

        if (firstNonSpace == std::string::npos) {
            str.clear();
        } else {
            str.erase(0, firstNonSpace);
            size_t lastNonSpace = str.find_last_not_of(charset);
            if (lastNonSpace != std::string::npos) {
                str.erase(lastNonSpace + 1);
                }
            }
    }

    long	getCurrentTimeinSec(void) {
        struct timeval	current_time;

        gettimeofday(&current_time, NULL);
        return (current_time.tv_sec);
    }

    std::string getMIMEType(std::string filepath) {
        const static std::map<std::string, std::string> lookup {
            {"html", "text/html"},
            {"htm", "text/html"},
            {"css", "text/css"},
            {"js", "text/javascript"},
            {"txt", "text/plain"},
            {"ico", "image/x-icon"},
            {"jpg", "image/jpeg"},
            {"jpeg", "image/jpeg"},
            {"gif", "image/gif"},
            {"png", "image/png"},
            {"bmp", "image/bmp"},
            {"webm", "video/webm"},
            {"mp4", "video/mp4"},
            {"mp3", "audio/mp3"},
            {"wav", "audio/wav"},
            {"zip", "application/zip"},
            {"pdf", "application/pdf"},
            {"xml", "application/xml"},
        };
        std::size_t pos = filepath.find_last_of(".");
        if (pos == std::string::npos) {
            return ("text/plain");
        }
        std::string extension = filepath.substr(pos + 1);
        if (lookup.count(extension)) {
            return (lookup.at(extension));
        }
        return ("text/plain");
    }
	
	std::string getContentInfo(std::string header, std::string info)
	{
		std::stringstream ss(header);
		std::string line;
		int info_found = 0;
		while (getline(ss, line, ' '))
		{
			if (line.find(info) != std::string::npos){
				info_found = 1;
			}
		}
		if (info_found == 0)
			return ("");
		line.erase(0, info.length());
		return (line);
	
	}

	//CURL REQUESTS DOEN
	int upload_file(RequestParser *post_request, std::string uri, unsigned long long maxSize)
	{
		
		std::vector<char> body = post_request->get_body();
		std::vector<char>::iterator it = body.begin();
		std::string filename;
		std::string boundary;
		int newlines_found = 0;
		int i = 0;
		int length;

		if (post_request->get_content_length() > maxSize)
			return (413);

		//skip the first few lines because they are content-type, disposition and boundary.
		//only do this if 
		if (!post_request->get_content_disposition().empty() && post_request->get_content_type() != "multipart/form-data")
		{
			while (it != body.end())
			{
				if (*it == '\n')
					newlines_found++;
				it++;
				i++;	
				if (newlines_found == 6)
					break;
			}
			//get the filename and boundary
			filename = getContentInfo(post_request->get_content_disposition(), "filename=");
			if (!filename.empty())//remove first and last quote
			{
				filename.erase(0,1);
				for (int i = filename.size(); i > 0; i--)
				{
					if (filename[i] == '\"')
					{
						filename.erase(i, filename.size());
						break;
					}
				}
			} else {
				return (415);
			}
			boundary = getContentInfo(post_request->get_content_type(), "boundary=");
			if (!boundary.empty())
			{
				boundary = "--" + boundary + "--";
				length = post_request->get_content_length() - boundary.size();
			}
		}
		else {// means not enough info is given
			return (415);
		}
		std::ofstream newfile(uri + filename, std::ios::out | std::ios::binary);
		//write all bytes to file except final boundary

		if (newfile.is_open()) {
			while (i < length)
			{
				i++;
				newfile << *it;
				it++;
			}
			newfile.close();
			return (201);
		}
		else
			return(500);
	}

	int delete_resource(std::string uri)
	{

		if (access(uri.c_str(), F_OK) != 0)
			return (404);//check if exists
		if (access(uri.c_str(), W_OK) != 0)
			return (403);//check for write permissions

		std::ofstream file(uri);
		if (file.is_open())
			file.close();//check if file is not in use
		else
			return (409);//send 409 Conflict
		
		const int result = remove(uri.c_str());// try to remove the resource
		if (result != 0)
			return (500);//internal server error
		return(200);//complete!
	}
};
