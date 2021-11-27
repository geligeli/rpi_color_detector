#include "http_server/http_server.h"

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <memory>
#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <jpeglib.h>

namespace beast = boost::beast;   // from <boost/beast.hpp>
namespace http = beast::http;     // from <boost/beast/http.hpp>
namespace net = boost::asio;      // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp; // from <boost/asio/ip/tcp.hpp>

std::function<const ImagePtr()> OnAquireImage;
std::function<void(const ImagePtr &)> OnReleaseImage;
std::function<void(const std::string&)> OnImageCompressed;
std::function<void(std::string key)> OnKeyPress;

namespace my_program_state
{

  struct Result
  {
    unsigned char *buf{};
    unsigned long size{};
  };

  // ..Here allocate the buffer and initialize the size
  // before using in jpeg_mem_dest.  Or it will allocate for you
  // and you have to clean up.

  std::string imgdata()
  {
    int quality = 50;
    struct jpeg_compress_struct cinfo; // Basic info for JPEG properties.
    struct jpeg_error_mgr jerr;        // In case of error.
    JSAMPROW row_pointer[1];           // Pointer to JSAMPLE row[s].
    int row_stride;                    // Physical row width in image buffer.
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);

    Result result;
    jpeg_mem_dest(&cinfo, &result.buf, &result.size);

    auto image = OnAquireImage();

    // cinfo.image_width = 500;        // |-- Image width and height in pixels.
    // cinfo.image_height = 500;       // |
    cinfo.image_width = image.w;
    cinfo.image_height = image.h;
    cinfo.input_components = 3;     // Number of color components per pixel.
    cinfo.in_color_space = JCS_RGB; // Colorspace of input image as RGB.

    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, quality, TRUE);

    // Lock image/

    unsigned char *image_buffer = image.data; //NULL;
                                              /*    image_buffer = (unsigned char *)malloc(cinfo.image_width * cinfo.image_height * cinfo.num_components);

    for (unsigned int y = 0; y < cinfo.image_height; ++y)
      for (unsigned int x = 0; x < cinfo.image_width; ++x)
      {
        unsigned int pixelIdx = ((y * cinfo.image_height) + x) * cinfo.input_components;

        if (x % 2 == y % 2)
        {
          image_buffer[pixelIdx + 0] = 255; // r |-- Set r,g,b components to
          image_buffer[pixelIdx + 1] = 0;   // g |   make this pixel red
          image_buffer[pixelIdx + 2] = 0;   // b |   (255,0,0).
        }
        else
        {
          image_buffer[pixelIdx + 0] = 255; // r |-- Set r,g,b components to
          image_buffer[pixelIdx + 1] = 255; // g |   make this pixel white
          image_buffer[pixelIdx + 2] = 255; // b |   (255,255,255).
        }
      }*/

    //## START COMPRESSION:

    jpeg_start_compress(&cinfo, TRUE);
    row_stride = cinfo.image_width * 3; // JSAMPLEs per row in image_buffer

    while (cinfo.next_scanline < cinfo.image_height)
    {
      row_pointer[0] = &image_buffer[cinfo.next_scanline * row_stride];
      (void)jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }
    // NOTE: jpeg_write_scanlines expects an array of pointers to scanlines.
    //       Here the array is only one element long, but you could pass
    //       more than one scanline at a time if that's more convenient.

    //## FINISH COMPRESSION AND CLOSE FILE:

    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
    // unlock compress.
    OnReleaseImage(image);
    std::string buffer;
    buffer.resize(result.size);
    std::copy(result.buf, result.buf + result.size, buffer.begin());
    free(result.buf);
    return buffer;
  }

} // namespace my_program_state

class http_connection : public std::enable_shared_from_this<http_connection>
{
public:
  http_connection(tcp::socket socket) : socket_(std::move(socket)) {}

  // Initiate the asynchronous operations associated with the connection.
  void start()
  {
    read_request();
    check_deadline();
  }

private:
  // The socket for the currently connected client.
  tcp::socket socket_;

  // The buffer for performing reads.
  beast::flat_buffer buffer_{8192};

  // The request message.
  http::request<http::dynamic_body> request_;

  // The response message.
  http::response<http::dynamic_body> response_;

  // The timer for putting a deadline on connection processing.
  net::steady_timer deadline_{socket_.get_executor(), std::chrono::seconds(60)};

  // Asynchronously receive a complete request message.
  void read_request()
  {
    auto self = shared_from_this();

    http::async_read(
        socket_, buffer_, request_,
        [self](beast::error_code ec, std::size_t bytes_transferred)
        {
          boost::ignore_unused(bytes_transferred);
          if (!ec)
            self->process_request();
        });
  }

  // Determine what needs to be done with the request message.
  void process_request()
  {
    response_.version(request_.version());
    response_.keep_alive(false);

    switch (request_.method())
    {
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
          << "Invalid request-method '" << std::string(request_.method_string())
          << "'";
      break;
    }

    write_response();
  }

  // Construct a response message based on the program state.
  void create_response()
  {
    std::cout << request_.target() << std::endl;
    if (request_.target().starts_with("/press?k=")) {
      if (OnKeyPress) {
        OnKeyPress(std::string(request_.target().substr(std::string("/press?k=").size())));
      }
      std::this_thread::sleep_for(std::chrono::seconds(2));
      response_.set(http::field::content_type, "application/json");
      beast::ostream(response_.body()) << R"""(
        {
          "foo" : "bar"
        }
      )""";
    } else if (request_.target().starts_with("/img")) {
      response_.set(http::field::content_type, "image/jpeg");
      std::string s = my_program_state::imgdata(); // my_program_state::data;
      if (OnImageCompressed) {
        OnImageCompressed(s);
      }
      beast::ostream(response_.body()) << s;
    }
    else
    {
      // response_.result(http::status::not_found);
      response_.set(http::field::content_type, "text/html");
      beast::ostream(response_.body()) << R"""(
        <html>
        <head>
          <script>
            function keypress(e) {
              document.removeEventListener('keypress', keypress);
              var xhr = new XMLHttpRequest();
              xhr.open("GET", "/press?k="+e.code, true);
              xhr.setRequestHeader('Content-Type', 'application/json');
              xhr.onreadystatechange = function () {
                  if (this.readyState != 4) return;
                  if (this.status == 200) {
                      var data = JSON.parse(this.responseText);
                      document.getElementById("viewport").src = "/img?" + new Date().getTime();
                      // we get the returned data
                  }
                  document.addEventListener('keypress', keypress);
              };
              xhr.send();
            }
            document.addEventListener('keypress', keypress);
          </script>
        </head>
        <body>
          <img id="viewport" src="/img">
        </body>
        </html>
        )""";
    }
  }

  // Asynchronously transmit the response message.
  void write_response()
  {
    auto self = shared_from_this();

    response_.content_length(response_.body().size());

    http::async_write(socket_, response_,
                      [self](beast::error_code ec, std::size_t)
                      {
                        self->socket_.shutdown(tcp::socket::shutdown_send, ec);
                        self->deadline_.cancel();
                      });
  }

  // Check whether we have spent enough time on this connection.
  void check_deadline()
  {
    auto self = shared_from_this();

    deadline_.async_wait([self](beast::error_code ec)
                         {
                           if (!ec)
                           {
                             // Close socket to cancel any outstanding operation.
                             self->socket_.close(ec);
                           }
                         });
  }
};

// "Loop" forever accepting new connections.
void http_server(tcp::acceptor &acceptor, tcp::socket &socket)
{
  acceptor.async_accept(socket, [&](beast::error_code ec)
                        {
                          if (!ec)
                            std::make_shared<http_connection>(std::move(socket))->start();
                          http_server(acceptor, socket);
                        });
}

void run_server()
{
  net::io_context ioc{1};
  auto const address = net::ip::make_address("0.0.0.0");
  unsigned short port = static_cast<unsigned short>(8888);
  tcp::acceptor acceptor{ioc, {address, port}};
  tcp::socket socket{ioc};
  http_server(acceptor, socket);

  ioc.run();
}