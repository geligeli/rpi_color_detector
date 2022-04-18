#!/usr/bin/env python3

import os
import socket
import shutil
import glob
from http import HTTPStatus
from http.server import SimpleHTTPRequestHandler, HTTPServer
from socketserver import ThreadingMixIn
from collections import defaultdict
import threading

from urllib.parse import urlparse, parse_qs


BASE_DIR = '/nfs/general/shared'
ALLOWED_PATHS = set(['/style.css', '/js.js', '/orientation'])

A_FILES = set([x.replace(BASE_DIR, '')
               for x in glob.glob('{}/KeyA/*.jpg'.format(BASE_DIR))])
D_FILES = set([x.replace(BASE_DIR, '')
               for x in glob.glob('{}/KeyD/*.jpg'.format(BASE_DIR))])
DIR_FILES = set([x.replace(BASE_DIR, '')
                 for x in glob.glob('{}/pos/*/*.jpg'.format(BASE_DIR))])


# for f in DIR_FILES:
#     if int(f.split('/')[3].split('_')[0]) >= 1650270649087:
#         os.rename(BASE_DIR+f, BASE_DIR+f+'.bad')
# DIR_FILES = set([x for x in DIR_FILES if int(x.split('/')[3].split('_')[0]) < 1650270649087])



DIR_FILES_DICT=defaultdict(lambda: [])
for x in DIR_FILES:
    DIR_FILES_DICT[int(x.split('/')[2])%80].append(x)

print(DIR_FILES_DICT)

class ThreadedHTTPServer(ThreadingMixIn, HTTPServer):
    """Handle requests in a separate thread."""


def parseImagePaths():
    global A_FILES
    global D_FILES
    A_FILES = set([x.replace(BASE_DIR, '')
                   for x in glob.glob('{}/KeyA/*.jpg'.format(BASE_DIR))])
    D_FILES = set([x.replace(BASE_DIR, '')
                   for x in glob.glob('{}/KeyD/*.jpg'.format(BASE_DIR))])


class MyServer(SimpleHTTPRequestHandler):
    def send_head(self):
        print(self)
        if self.path.startswith('/labelImage?'):
            p = urlparse(self.path)
            qs = parse_qs(p.query)
            self.moveImage(qs["className"][0], qs["imageName"][0])
            return False
        path = self.translate_path(self.path)
        print(path)
        if path == '/':
            self.indexHtml()
            return False
        elif path == '/orientation':
            self.orientationIndexHtml()
            return False
        elif path == '/style.css':
            self.serveStyle()
            return False
        elif path == '/js.js':
            self.serveJs()
            return False
        else:
            return super().send_head()

    def translate_path(self, path):
        print(path)
        if self.path in A_FILES or self.path in D_FILES or self.path in DIR_FILES:
            return BASE_DIR + self.path
        elif self.path in ALLOWED_PATHS:
            return self.path
        return '/'
        # return super().translate_path(path)

    def serveStyle(self):
        s = '''
        .flexbox-container {
            display: flex;
        }
        '''
        self.serveString(s, "test/css")

    def serveJs(self):
        s = '''
        function labelImage(className, imageName) {
            xhttp=new XMLHttpRequest();
            xhttp.open("GET", `labelImage?className=${encodeURIComponent(className)}&imageName=${encodeURIComponent(imageName)}`);
            xhttp.send();
        }
        '''
        self.serveString(s, "test/javascript")

    def serveString(self, s, mimeType="text/html"):
        self.send_response(HTTPStatus.OK)
        self.send_header("Content-type", mimeType)
        self.send_header("Content-Length", str(len(s)))
        self.send_header("Cache-Control", "no-cache")
        self.end_headers()
        self.wfile.write(s.encode('UTF-8', 'replace'))

    def imageList(self, title, images):
        l = '<div>{}<br>'.format(title)
        for i in images:
            splitl = i.split('/')
            l += '<img src="{}"/ onclick="labelImage(\'{}\', \'{}\')"><br/>'.format(
                i, splitl[1], splitl[2])
        l += '</div>'
        return l

    def orientationIndexHtml(self):
        s = '''
        <html>
        <head>
        <link rel="stylesheet" href="/style.css">
        <title>Orientation index</title>
        </head>
        <body>
        <div class="flexbox-container">
        '''
        for i in range(800):
            s += self.imageList(str(i), sorted(DIR_FILES_DICT[i]))
        s += '''
        </div>
        </body>
        </html>
        '''
        self.serveString(s)

    def indexHtml(self):
        parseImagePaths()
        s = '''
        <html>
        <head>
        <link rel="stylesheet" href="/style.css">
        <script src="/js.js"></script>
        <title>Foo Bar</title>
        </head>
        <body>
        <div class="flexbox-container">
        '''
        s += self.imageList('KeyA', A_FILES)
        s += self.imageList('KeyD', D_FILES)
        s += '''
        </div>
        </body>
        </html>
        '''
        self.serveString(s)

    def moveImage(self, className, imageName):
        fromName = '{}/{}/{}'.format(BASE_DIR, className, imageName)
        if className == 'KeyA':
            className = 'KeyD'
        else:
            className = 'KeyA'
        toName = '{}/{}/{}'.format(BASE_DIR, className, imageName)
        shutil.move(fromName, toName)

    def do_HEAD(self):
        """Serve a HEAD request."""
        f = self.send_head()
        if f:
            f.close()

    def do_GET(self):
        f = self.send_head()
        if f:
            try:
                self.copyfile(f, self.wfile)
            finally:
                f.close()


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
