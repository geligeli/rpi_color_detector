#!/usr/bin/env python3

import datetime
import email
import io
import json
import logging
import os
import socket
from http import HTTPStatus
from http.server import HTTPServer, SimpleHTTPRequestHandler
from socketserver import ThreadingMixIn
from urllib.parse import parse_qs, urlparse

import tensorflow as tf

import image_provider

HTML_DIR = os.path.join(os.path.dirname(os.path.realpath(__file__)), 'html')

class ThreadedHTTPServer(ThreadingMixIn, HTTPServer):
    """Handle requests in a separate thread."""

class MyServer(SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        self.image_provider = image_provider.ImageProvider()
        self.cgi_methods = {method[4:]: self.__getattribute__(
            method) for method in dir(type(self)) if method.startswith('CGI_')}
        super().__init__(*args, directory=HTML_DIR, **kwargs)

    def serve_file_path(self, path):
        f = None
        if os.path.isdir(path):
            self.send_error(HTTPStatus.NOT_FOUND, "File not found")
            return None
        if path.endswith("/"):
            self.send_error(HTTPStatus.NOT_FOUND, "File not found")
            return None
        try:
            f = open(path, 'rb')
        except OSError:
            self.send_error(HTTPStatus.NOT_FOUND, "File not found")
            return None
        ctype = self.guess_type(path)
        try:
            fs = os.fstat(f.fileno())
            # Use browser cache if possible
            if ("If-Modified-Since" in self.headers
                    and "If-None-Match" not in self.headers):
                # compare If-Modified-Since and time of last file modification
                try:
                    ims = email.utils.parsedate_to_datetime(
                        self.headers["If-Modified-Since"])
                except (TypeError, IndexError, OverflowError, ValueError):
                    # ignore ill-formed values
                    pass
                else:
                    if ims.tzinfo is None:
                        # obsolete format with no timezone, cf.
                        # https://tools.ietf.org/html/rfc7231#section-7.1.1.1
                        ims = ims.replace(tzinfo=datetime.timezone.utc)
                    if ims.tzinfo is datetime.timezone.utc:
                        # compare to UTC datetime of last modification
                        last_modif = datetime.datetime.fromtimestamp(
                            fs.st_mtime, datetime.timezone.utc)
                        # remove microseconds, like in If-Modified-Since
                        last_modif = last_modif.replace(microsecond=0)

                        if last_modif <= ims:
                            self.send_response(HTTPStatus.NOT_MODIFIED)
                            self.end_headers()
                            f.close()
                            return None

            self.send_response(HTTPStatus.OK)
            self.send_header("Content-type", ctype)
            self.send_header("Content-Length", str(fs[6]))
            self.send_header("Last-Modified",
                             self.date_time_string(fs.st_mtime))
            self.end_headers()
            return f
        except:
            f.close()
            raise

    def CGI_image(self, query_string):
        image_path = self.image_provider.filePath(query_string["imageName"][0])
        if not image_path:
            self.send_error(HTTPStatus.NOT_FOUND, "File not found")
            return None
        return self.serve_file_path(image_path)

    def CGI_listImages(self, query_string):
        r = self.image_provider.listImages(query_string["type"][0])
        return self.serveString(json.dumps(r), 'application/json')

    def CGI_relabelImage(self, query_string):
        sourceImage = query_string["sourceImage"][0]
        targetClass = query_string["targetClass"][0]
        self.image_provider.relabelImage(sourceImage, targetClass)
        return self.serveString('{}', 'application/json')

    def send_head(self):
        try:
            if self.path.startswith('/cgi-bin?'):
                p = urlparse(self.path)
                qs = parse_qs(p.query)
                return self.cgi_methods[qs["app"][0]](qs)
        except Exception as error:
            error_string = repr(error)
            logging.warning(error_string)
            return self.serveString(error_string, responseCode=HTTPStatus.INTERNAL_SERVER_ERROR)
        return super().send_head()

    def serveString(self, s, mimeType="text/html", responseCode=HTTPStatus.OK):
        self.send_response(responseCode)
        self.send_header("Content-type", mimeType)
        self.send_header("Content-Length", str(len(s)))
        self.send_header("Cache-Control", "no-cache")
        self.end_headers()
        return io.BytesIO(s.encode('utf-8'))

def localIp():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.connect(("8.8.8.8", 80))
    return s.getsockname()[0]


if __name__ == '__main__':
    PORT = 8000
    IP = localIp()
    webServer = ThreadedHTTPServer((IP, PORT), MyServer)
    print("Server started http://%s:%s" % (IP, PORT))

    try:
        webServer.serve_forever()
    except KeyboardInterrupt:
        pass

    webServer.server_close()
    print("Server stopped.")
