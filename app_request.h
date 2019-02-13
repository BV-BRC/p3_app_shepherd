#ifndef _app_request_h
#define _app_request_h

#include <memory>
#include <functional>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>

#include "buffer.h"

/*
 * We include both SSL and non-SSL here and pass in a
 * use-ssl flag. It's a little ugly but it is much cleaner above
 * to use a single class, and while there might be a clever means to
 * do this without the flag, it's not currently worth the
 * thought-time to come up with the optimal solution. 
 */

class AppRequest : public std::enable_shared_from_this<AppRequest>
{
public:
    typedef boost::beast::http::response<boost::beast::http::string_body> response_type;
    typedef std::function<void(const response_type &)> success_cb;
    typedef std::function<void()> failure_cb;
    
    explicit AppRequest(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx,
			success_cb on_success, failure_cb on_failure);

    void post(bool use_ssl,
	      const std::string &host,
	      boost::asio::ip::tcp::resolver::results_type &dest,
	      const std::string &target,
	      std::shared_ptr<OutputBuffer> buffer);

private:
    success_cb success_cb_;
    failure_cb failure_cb_;

    boost::asio::io_context &ioc_;

    // For SSL

    boost::asio::ssl::stream<boost::asio::ip::tcp::socket> stream_;

    // For non-SSL

    boost::asio::ip::tcp::socket socket_;
    
    boost::beast::flat_buffer buffer_; // (Must persist between reads)
    boost::beast::http::request<boost::beast::http::buffer_body> req_;

    response_type res_;

    void on_ssl_connect(boost::beast::error_code ac);
    void on_ssl_handshake(boost::beast::error_code ec);
    void on_ssl_write(boost::beast::error_code ec, std::size_t bytes_transferred);
    void on_ssl_read(boost::beast::error_code ec, std::size_t bytes_transferred);
    void on_ssl_shutdown(boost::beast::error_code ec);

    void on_nonssl_connect(boost::beast::error_code ac);
    void on_nonssl_write(boost::beast::error_code ec, std::size_t bytes_transferred);
    void on_nonssl_read(boost::beast::error_code ec, std::size_t bytes_transferred);
    void on_nonssl_shutdown(boost::beast::error_code ec);

    void fail(boost::beast::error_code ec, const std::string &what);
};

#endif
