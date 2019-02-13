#ifndef _url_parse_h
#define _url_parse_h

#include <string>
#include <iostream>
#include <boost/regex.hpp>

namespace url_utilities {

    struct parsed_url
    {
	std::string protocol;
	std::string domain;
	std::string port;
	std::string path;
	std::string query;
	std::string fragment;
    };

    
    
    inline bool parse_url(const std::string &url, parsed_url &purl)
    {
	boost::regex ex("(http|https)://([^/ :]+):?([^/ ]*)(/?[^ #?]*)\\x3f?([^ #]*)#?([^ ]*)");
	boost::smatch what;
	if (boost::regex_match(url, what, ex))
	{
	    purl.protocol = what[1];
	    purl.domain = what[2];
	    purl.port = what[3];
	    purl.path = what[4];
	    purl.query = what[5];
	    purl.fragment = what[6];
	    return true;
	}
	else
	    return false;
    }

    inline std::ostream &operator<<(std::ostream &os, const parsed_url &p)
    {
	os
	    << "protocol=" << p.protocol
	    << " domain=" << p.domain
	    << " port=" << p.port
	    << " path=" << p.path
	    << " query=" << p.query
	    << " fragment=" << p.fragment;
	return os;
    }
    

}

#endif
