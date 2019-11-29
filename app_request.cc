#include "app_request.h"

#include <boost/asio/dispatch.hpp>
#include <memory>
#include <functional>
#include <iostream>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

AppRequest::AppRequest(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx,
		       AppRequest::success_cb on_success, AppRequest::failure_cb on_failure)
        : ioc_(ioc)
        , stream_(ioc, ctx)
	, socket_(ioc)
	, success_cb_(on_success)
	, failure_cb_(on_failure)
	, auth_header_("")
{
    const char *token = getenv("P3_AUTH_TOKEN");
    if (!token)
	token = getenv("KB_AUTH_TOKEN");
    if (token)
    {
	auth_header_ = std::string("OAuth ") + token;
    }
}

void AppRequest::post(bool use_ssl, const std::string &host,
			 boost::asio::ip::tcp::resolver::results_type &dest,
			 const std::string &target,
			 std::shared_ptr<OutputBuffer> buffer)
{
    // Set SNI Hostname (many hosts need this to handshake successfully)
    if (use_ssl)
    {
	if(! SSL_set_tlsext_host_name(stream_.native_handle(), host.c_str()))
	{
	    beast::error_code ec{static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()};
	    fail(ec, ec.message());
	    return;
	}
    }

    req_.version(11);
    req_.method(http::verb::post);
    req_.target(target);
    auto &body = req_.body();
    body.data = buffer->data();
    body.size = buffer->size();
    body.more = false;
    req_.set(http::field::host, host);
    req_.set(http::field::user_agent, "p3x-app-shepherd");
    req_.set(http::field::content_length, std::to_string(buffer->size()));
    if (!auth_header_.empty())
	req_.set(http::field::authorization, auth_header_);

    if (use_ssl)
    {
	net::async_connect(stream_.next_layer(),
			   dest.begin(),
			   dest.end(),
			   std::bind(&AppRequest::on_ssl_connect, shared_from_this(), std::placeholders::_1))
	;
    }
    else
    {
	boost::asio::async_connect(
	    socket_,
	    dest.begin(),
	    dest.end(),
	    std::bind(
		&AppRequest::on_nonssl_connect,
		shared_from_this(),
		std::placeholders::_1));
    }
}

void AppRequest::on_ssl_connect(beast::error_code ec)
{
    if (ec)
    {
	fail(ec, "connect");
	return;
    }
    
    // std::cerr << "Got connect\n";
    // Perform the SSL handshake
    stream_.async_handshake(
	ssl::stream_base::client,
	std::bind(
	    &AppRequest::on_ssl_handshake,
	    shared_from_this(),
	    std::placeholders::_1));
}

void AppRequest::on_ssl_handshake(beast::error_code ec)
{
    if(ec)
    {
	fail(ec, "handshake");
	return;
    }
    /*
    SSL *ssl = stream_.native_handle();
    SSL_SESSION *ss = SSL_get_session(ssl);
    static int x = 0;
    char file[1024];
    sprintf(file, "sess.%03d", x++);
    FILE *fp = fopen(file, "w");
    SSL_SESSION_print_fp(fp, ss);
    fclose(fp);
    */

    // std::cerr << "got handshake\n";
    // Send the HTTP request to the remote host
    http::async_write(stream_, req_,
		      std::bind(
			  &AppRequest::on_ssl_write,
			  shared_from_this(),
			  std::placeholders::_1,
			  std::placeholders::_2));
}

void AppRequest::on_ssl_write(
    beast::error_code ec,
    std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);
    
    if(ec)
	return fail(ec, "write");

    // std::cerr << "got write\n";
    // Receive the HTTP response
    http::async_read(stream_, buffer_, res_,
		     std::bind(
			 &AppRequest::on_ssl_read,
			 shared_from_this(),
			 std::placeholders::_1,
			 std::placeholders::_2));
}

void AppRequest::on_ssl_read(
    beast::error_code ec,
    std::size_t bytes_transferred)
{
    if (0) {
	std::cerr << "read " << ec << " " << bytes_transferred << std::endl;
	for (auto x = res_.begin(); x != res_.end(); x++)
	{
	    std::cerr << x->name() << ": " << x->value() << "\n";
	}
	std::cerr << res_.body() << std::endl;
    }
    boost::ignore_unused(bytes_transferred);
    
    if(ec)
    {
	if ((ec.category() == boost::asio::error::get_ssl_category())
	    && (ERR_GET_REASON(ec.value()) == SSL_R_SHORT_READ))
	{
	    // std::cerr << "SSL short read\n";
	}
	else
	{
	    return fail(ec, "read");
	}
    }
    
    //boost::asio::dispatch(ioc_, std::bind(success_cb_, res_));
    success_cb_(res_);

    if (!ec)
    {
	// Gracefully close the stream
	stream_.async_shutdown(
	    std::bind(
		&AppRequest::on_ssl_shutdown,
		shared_from_this(),
		std::placeholders::_1));
    }
}

void AppRequest::on_ssl_shutdown(beast::error_code ec)
{
    if(ec == net::error::eof)
    {
	// std::cerr << "got eof\n";
	// Rationale:
	// http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
	ec.assign(0, ec.category());
    }
    if(ec)
	return fail(ec, "shutdown");
    
    // If we get here then the connection is closed gracefully
    // std::cerr << "Shutdown properly complete\n";
}

void AppRequest::on_nonssl_connect(boost::system::error_code ec)
{
    if(ec)
	return fail(ec, "connect");
    
    // Send the HTTP request to the remote host
    http::async_write(socket_, req_,
		      std::bind(
			  &AppRequest::on_nonssl_write,
			  shared_from_this(),
			  std::placeholders::_1,
			  std::placeholders::_2));
}

void AppRequest::on_nonssl_write(
    boost::system::error_code ec,
    std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);
    
    if(ec)
	return fail(ec, "write");
    
    // Receive the HTTP response
    http::async_read(socket_, buffer_, res_,
		     std::bind(
			 &AppRequest::on_nonssl_read,
			 shared_from_this(),
			 std::placeholders::_1,
			 std::placeholders::_2));
}

void AppRequest::on_nonssl_read(
    boost::system::error_code ec,
    std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);

    if(ec)
	return fail(ec, "read");

    
    // Write the message to standard out
    success_cb_(res_);
    
    // Gracefully close the socket
    socket_.shutdown(tcp::socket::shutdown_both, ec);
    
    // not_connected happens sometimes so don't bother reporting it.
    if(ec && ec != boost::system::errc::not_connected)
	return fail(ec, "shutdown");
    
    // If we get here then the connection is closed gracefully
}

void AppRequest::fail(beast::error_code ec, const std::string &what)
{
    std::cerr << what << ": " << ec.message() << "\n";
    boost::asio::dispatch(ioc_, std::bind(failure_cb_, res_));
}


