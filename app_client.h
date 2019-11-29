#ifndef _app_client_h
#define _app_client_h

#include <boost/asio.hpp>
#include <queue>
#include <memory>

#include "url_parse.h"
#include "buffer.h"
#include "app_request.h"

class AppClient : public std::enable_shared_from_this<AppClient>
{
public:
    AppClient(boost::asio::io_service &ios, const std::string &url, const std::string &task_id);
    ~AppClient();

    void write_block(std::shared_ptr<OutputBuffer> buf);
    void write_block(const std::string &key, const std::string &value, bool truncate = false);
    
private:

    enum class Status {
	    Disabled,
	    Live,
	    InProgress,
	    Dead
	    };
		
    friend std::ostream &operator<<(std::ostream &os, Status s);
	
    void sync_resolve_url(const url_utilities::parsed_url &p);

    void process_queue();

    void on_success(const AppRequest::response_type &response, std::shared_ptr<OutputBuffer> buf);
    void on_failure(const AppRequest::response_type &response, std::shared_ptr<OutputBuffer> buf);

    boost::asio::io_service &ios_;
    boost::asio::ip::tcp::resolver resolver_;
    std::string url_;
    std::string task_id_;
    url_utilities::parsed_url parsed_url_;
    
    boost::asio::ip::tcp::resolver::results_type resolved_host_;
    Status status_;
    std::shared_ptr<AppRequest> current_request_;

    boost::asio::ssl::context ssl_context_;

    std::queue<std::shared_ptr<OutputBuffer>> buffer_queue_;
};

inline std::ostream &operator<<(std::ostream &os, AppClient::Status s)
{
    switch (s) {
    case AppClient::Status::Disabled:
	os << "Disabled";
	break;
    case AppClient::Status::Live:
	os << "Live";
	break;
    case AppClient::Status::InProgress:
	os << "InProgress";
	break;
    case AppClient::Status::Dead:
	os << "Dead";
	break;
    }
    return os;
}

#endif
