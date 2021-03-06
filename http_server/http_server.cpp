#include "http_server/http_server.h"

#include <stdio.h>
#include <stdlib.h>

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <chrono>
#include <fstream>
#include <memory>

namespace beast = boost::beast;    // from <boost/beast.hpp>
namespace http = beast::http;      // from <boost/beast/http.hpp>
namespace net = boost::asio;       // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;  // from <boost/asio/ip/tcp.hpp>

std::function<void(std::ostream &)> OnProvideImageJpeg;
std::function<void(std::ostream &)> OnProvideJson;
std::function<void(std::string key)> OnKeyPress;

class http_connection : public std::enable_shared_from_this<http_connection> {
 public:
  http_connection(tcp::socket socket, const std::filesystem::path &root)
      : socket_(std::move(socket)), root_(root) {}

  // Initiate the asynchronous operations associated with the connection.
  void start() {
    read_request();
    check_deadline();
  }

 private:
  // The socket for the currently connected client.
  tcp::socket socket_;

  const std::filesystem::path &root_;

  // The buffer for performing reads.
  beast::flat_buffer buffer_{8192};

  // The request message.
  http::request<http::dynamic_body> request_;

  // The response message.
  http::response<http::dynamic_body> response_;

  // The timer for putting a deadline on connection processing.
  net::steady_timer deadline_{socket_.get_executor(), std::chrono::seconds(60)};

  // Asynchronously receive a complete request message.
  void read_request() {
    auto self = shared_from_this();

    http::async_read(
        socket_, buffer_, request_,
        [self](beast::error_code ec, std::size_t bytes_transferred) {
          boost::ignore_unused(bytes_transferred);
          if (!ec) self->process_request();
        });
  }

  // Determine what needs to be done with the request message.
  void process_request() {
    response_.version(request_.version());
    response_.keep_alive(false);

    switch (request_.method()) {
      case http::verb::get:
        response_.result(http::status::ok);
        response_.set(http::field::server, "Beast");
        create_response();
        break;

      default:
        // We return responses indicating an error if
        // we do not recognize the request method.
        response_.result(http::status::bad_request);
        response_.set(http::field::content_type, "text/plain");
        beast::ostream(response_.body())
            << "Invalid request-method '"
            << std::string(request_.method_string()) << "'";
        break;
    }

    write_response();
  }

  // Construct a response message based on the program state.
  void create_response() {
    if (request_.target().starts_with("/press?k=")) {
      if (OnKeyPress) {
        OnKeyPress(std::string(
            request_.target().substr(std::string("/press?k=").size())));
      }
      response_.set(http::field::content_type, "application/json");
      beast::ostream(response_.body()) << "{}";
    } else if (request_.target().starts_with("/img")) {
      if (OnProvideImageJpeg) {
        response_.set(http::field::content_type, "image/jpeg");
        auto os = beast::ostream(response_.body());
        OnProvideImageJpeg(os);
      } else {
        response_.set(http::field::content_type, "text/html");
        beast::ostream(response_.body()) << "OnProvideImageJpeg is not set!";
      }
    } else if (request_.target().starts_with("/json")) {
      if (OnProvideJson) {
        response_.set(http::field::content_type, "application/json");
        auto os = beast::ostream(response_.body());
        OnProvideJson(os);
      } else {
        response_.set(http::field::content_type, "text/html");
        beast::ostream(response_.body()) << "OnProvideJson is not set!";
      }
    } else {
      response_.set(http::field::content_type, "text/html");
      std::ifstream t(root_ / "index.html");
      t.seekg(0, std::ios::end);
      size_t size = t.tellg();
      std::string buffer(size, ' ');
      t.seekg(0);
      t.read(&buffer[0], size);
      beast::ostream(response_.body()) << buffer;
    }
  }

  // Asynchronously transmit the response message.
  void write_response() {
    auto self = shared_from_this();

    response_.content_length(response_.body().size());

    http::async_write(socket_, response_,
                      [self](beast::error_code ec, std::size_t) {
                        self->socket_.shutdown(tcp::socket::shutdown_send, ec);
                        self->deadline_.cancel();
                      });
  }

  // Check whether we have spent enough time on this connection.
  void check_deadline() {
    auto self = shared_from_this();

    deadline_.async_wait([self](beast::error_code ec) {
      if (!ec) {
        // Close socket to cancel any outstanding operation.
        self->socket_.close(ec);
      }
    });
  }
};

void http_server(tcp::acceptor &acceptor, tcp::socket &socket,
                 const std::filesystem::path &root) {
  acceptor.async_accept(socket, [&](beast::error_code ec) {
    if (!ec)
      std::make_shared<http_connection>(std::move(socket), root)->start();
    http_server(acceptor, socket, root);
  });
}

void run_server(std::filesystem::path root, unsigned short port) {
  net::io_context ioc{1};
  auto const address = net::ip::make_address("0.0.0.0");
  tcp::acceptor acceptor{ioc, {address, port}};
  tcp::socket socket{ioc};
  http_server(acceptor, socket, root);

  ioc.run();
}