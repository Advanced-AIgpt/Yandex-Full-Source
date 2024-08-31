#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import http.server
import os
import socket
import subprocess
import sys
import tempfile

class ServeLocalError(RuntimeError):
    pass

def serve(port):
    class HTTPServerV6(http.server.HTTPServer):
        address_family = socket.AF_INET6

    server = HTTPServerV6(("::", port), http.server.SimpleHTTPRequestHandler)
    print("http://{fqdn}:{port}/ or http://localhost:{port}".format(fqdn=socket.getfqdn(), port=port), file=sys.stderr)
    server.serve_forever()



def main():
    if sys.version_info < (3, 6):
        raise ServeLocalError("Required Python >= 3.7, actual python: {}".format(sys.version))

    with tempfile.TemporaryDirectory() as tmpdir:
        os.chdir(os.path.dirname(os.path.realpath(__file__)))
        subprocess.check_call(["../../ya", "make", "--output", tmpdir])
        os.chdir(tmpdir)
        os.mkdir("html")
        subprocess.check_call(["tar", "xzf", "yt/docs/docs.tar.gz", "-C", "html"])
        os.chdir("html")
        serve(8080)

if __name__ == "__main__":
    main()
