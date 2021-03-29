
#include "app_client.h"

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

AppClient::AppClient(boost::asio::io_service &ios, const std::string &url, const std::string &task_id)
	: ios_(ios)
	, resolver_(ios)
	, url_(url)
	, task_id_(task_id)
	, status_(Status::Disabled)
	, ssl_context_(ssl::context::sslv23_client)
	, exiting_(false)
{
    if (url_utilities::parse_url(url, parsed_url_))
    {
	// std::cout << parsed_url_ << std::endl;

	try {
	    sync_resolve_url(parsed_url_);
	    status_ = Status::Live;
	}
	catch (std::exception &e)
	{
	    std::cerr << "Failed to resolve " << e.what() << std::endl;
	}
    }

    ssl_context_.set_default_verify_paths();
    ssl_context_.set_verify_mode(ssl::verify_peer);

    /*
    SSL_CTX *s = ssl_context_.native_handle();
    SSL_CTX_set_cipher_list(s, "TLS_RSA_WITH_AES_128_GCM_SHA256");
    */
}

AppClient::~AppClient()
{
    // std::cerr << "destroy AppClient\n";
    current_request_ = 0;
}
    

void AppClient::sync_resolve_url(const url_utilities::parsed_url &p)
{
    std::string port = p.port;
    if (port.empty())
    {
	if (p.protocol == "http")
	    port = "80";
	else if (p.protocol == "https")
	    port = "443";
	else
	    throw std::runtime_error("unknown protocol " + p.protocol);
    }

    resolved_host_ = resolver_.resolve(p.domain, port);
    //for (auto x: resolved_host_)
    // std::cerr << x.endpoint() << std::endl;
}

void AppClient::write_block(const std::string &key, const std::string &value, bool truncate)
{
    auto buf = std::make_shared<OutputBuffer>(key);
    buf->set(value);
    buf->truncate(truncate);
    write_block(buf);
}

void AppClient::write_block(std::shared_ptr<OutputBuffer> buf)
{
    // std::cout << "write block: " << buf->as_string(buf->size()) << std::endl;

    buffer_queue_.push(buf);

    process_queue();
}

void AppClient::process_queue()
{
    // std::cerr << "process queue " << this << " status=" << status_ << " size=" << buffer_queue_.size() << std::endl;
    if (status_ == Status::InProgress)
	return;

    if (buffer_queue_.empty())
	return;

    if (status_ != Status::Live)
	return;

    if (exiting_)
    {
	std::cerr << "AppClient::process_queue(): Client exiting - set alarm\n";

	alarm(60);
    }

    /*
     * We are in a state where we can process the queue.
     * Create a request for the head of the queue and
     * send it off.
     *
     * We keep a pointer to the request thus generated so that
     * it does not get deallocated. Remember we're asynch here.
     */

    auto buf = buffer_queue_.front();

    status_ = Status::InProgress;
    current_request_ = std::make_shared<AppRequest>(ios_, ssl_context_,
						    std::bind(&AppClient::on_success, shared_from_this(), std::placeholders::_1, buf),
						    std::bind(&AppClient::on_failure, shared_from_this(), std::placeholders::_1, buf));
	
    bool is_ssl = (parsed_url_.protocol == "https");

    std::string path = parsed_url_.path + "/task_info/" + task_id_ + "/" + buf->key();
    if (!buf->truncate())
	path += "/data";
    
    // std::cerr << "post to: " << path << " size=" << buf->size() << std::endl;
    current_request_->post(is_ssl, parsed_url_.domain, resolved_host_, path, buf);
}

void AppClient::on_success(const AppRequest::response_type &response, std::shared_ptr<OutputBuffer> buf)
{
    // std::cerr << "Success!\n";
    // std::cerr << response.body() << std::endl;
    buffer_queue_.pop();
    current_request_ = 0;
    status_ = Status::Live;
    boost::asio::dispatch(ios_, std::bind(&AppClient::process_queue, shared_from_this()));
}

void AppClient::on_failure(const AppRequest::response_type &response, std::shared_ptr<OutputBuffer> buf)
{
    std::cerr << "Failure.\n";
    std::cerr << response.body() << std::endl;

    current_request_ = 0;
    // Just set to live for now; need to see what the error was. This will end up in a retry.
    status_ = Status::Live;
}
