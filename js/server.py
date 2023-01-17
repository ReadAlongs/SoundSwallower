#!/usr/bin/env python
#
# Small web server that serves
# pocketsphinx.wasm with the
# correct MIME type
#

import http.server
import socketserver

PORT = 8000
Handler = http.server.SimpleHTTPRequestHandler
Handler.extensions_map[".wasm"] = "application/wasm"
socketserver.TCPServer.allow_reuse_address = True
httpd = socketserver.TCPServer(("", PORT), Handler)
print("serving at port {}".format(PORT))
httpd.serve_forever()
