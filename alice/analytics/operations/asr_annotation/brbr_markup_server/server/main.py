import argparse
import http.server
import json
import os
import shutil
import socket
import time
import urllib.parse

import library.python.resource

import voicetech.common.lib.utils as utils


from http.server import HTTPServer

logger = utils.initialize_logging(__name__)


def factory(fpaths):
    paths = []
    with open(fpaths) as fin:
        for line in fin:
            paths.append(line.strip())

    class CustomHandler(http.server.BaseHTTPRequestHandler):
        def _set_response(self, code, content_type=None, location=None, cookies=None, content_disposition=None):
            self.send_response(code)
            if content_type:
                self.send_header('Content-type', content_type)
            if content_disposition:
                self.send_header('Content-disposition', content_disposition)
            if location:
                self.send_header('Location', location)
            if cookies:
                for key, value in cookies.items():
                    logger.info('Passing cookie: %s=%s', key, value)
                    self.send_header('Set-Cookie', '{}={};path=/'.format(key, value))
            self.end_headers()

        def download_check(self, attribute):
            logger.info('Downloading check.tsv')
            self._set_response(
                code=200, content_type='text/tab-separated-values', content_disposition='attachment; filename=check.tsv'
            )
            result = self._backend.dump_check_file(attribute)
            for uttid, value in result.items():
                self.wfile.write('{}\t{}\n'.format(uttid, '1' if value else '0').encode('utf-8'))

        def return_main_page(self):
            logger.info('Returning main page')
            self._set_response(200, content_type='text/html')
            with open('main.html', encoding='utf-8') as fin:
                html = fin.read()
            self.wfile.write(html.encode('utf-8'))

        def return_finish_page(self, name):
            logger.info('Returning main page')
            self._set_response(200, content_type='text/html')
            with open('finish.html', encoding='utf-8') as fin:
                html = fin.read()
            html = html.replace('DUMP_VALUE', name)
            self.wfile.write(html.encode('utf-8'))

        def return_favicon(self):
            logger.info('Returning favicon')
            if self._favicon is None:
                logger.info('Loading favicon from resource')
                resource = library.python.resource.find('/favicon')
                if resource is None:
                    raise KeyError("Can't find favicon. Check ya.make")
                self._favicon = resource
            self._set_response(200, content_type='image/png')
            self.wfile.write(self._favicon)

        def return_markup_page(self, name, id):
            logger.info('Returning main page')
            self._set_response(200, content_type='text/html')
            with open(paths[id], encoding='utf-8') as fin:
                html = fin.read()
            html = html.replace('USEFUL_VALUE', '{}|{}|useful'.format(name, id))
            html = html.replace('USELESS_VALUE', '{}|{}|not_useful'.format(name, id))
            html = html.replace('DUMP_VALUE', name)
            self.wfile.write(html.encode('utf-8'))

        def _get_markup_for_name(self, name):
            fname = os.path.join('markup', name + '.json')
            if os.path.exists(fname):
                backup_fname = os.path.join('backup', '{}_{}.json'.format(name, int(time.time())))
                shutil.copy(fname, backup_fname)
                with open(fname) as fin:
                    markup_data = json.load(fin)
            else:
                markup_data = {}
            return markup_data

        def return_markup(self, name):
            markup_data = self._get_markup_for_name(name)
            self._set_response(
                200,
                content_type='application/json',
                content_disposition='attachment; filename={}'.format(name + '.json'),
            )
            self.wfile.write(json.dumps(markup_data, ensure_ascii=False).encode('utf-8'))

        def _find_the_smallest_not_used_id(self, name):
            markup_data = self._get_markup_for_name(name)
            logger.info('%d ids already markuped, finding something new in %d ids', len(markup_data), len(paths))
            for i in range(len(paths)):
                if str(i) not in markup_data:
                    return i
            return None

        def dispatch(self, path, query_args):
            logger.info('Dispatching request:')
            logger.info('path=%s', path)
            for k, v in query_args.items():
                logger.info('args[%s] = "%s"', k, v)

            if 'login' in query_args:
                name = query_args['login'][0]
                logger.info('Starting markup for %s', name)
                id = self._find_the_smallest_not_used_id(name)
                logger.info('Smallest not markuped id: %d', id)
                if id is None:
                    id = 'all'
                self._set_response(302, location='?name={}&id={}'.format(name, id))
                return

            if 'name' in query_args and 'id' in query_args:
                name = query_args['name'][0]
                id = query_args['id'][0]
                try:
                    if int(id) >= len(paths):
                        id = 'all'
                except ValueError:
                    pass
                if id == 'all':
                    self.return_finish_page(name)
                else:
                    self.return_markup_page(name, int(id))
                return

            if path == '/':
                self.return_main_page()
                return

        def do_GET(self):
            logger.info('Received GET request')
            request_params = urllib.parse.urlparse(self.path)
            self.dispatch(request_params.path, urllib.parse.parse_qs(request_params.query))

        def _process_markup_result(self, data):
            logger.info("Processing markup result")
            markup_tokens = data['markup'][0].split('|')
            name = markup_tokens[0]
            id = int(markup_tokens[1])

            markup_data = self._get_markup_for_name(name)
            markup_data[id] = {'result': markup_tokens[2]}
            if 'comment' in data:
                markup_data[id]['comment'] = data['comment']
            markup_data[id]['brbr'] = 'brbr' in data
            markup_data[id]['sidespeech'] = 'sidespeech' in data
            markup_data[id]['context'] = 'context' in data

            fname = os.path.join('markup', name + '.json')
            with open(fname, 'w') as fout:
                json.dump(markup_data, fout, indent=2, ensure_ascii=False)

            self._set_response(302, location='?name={}&id={}'.format(markup_tokens[0], int(markup_tokens[1]) + 1))

        def do_POST(self):
            logger.info('Received POST request')
            logger.info('path: %s', self.path)
            length = int(self.headers['Content-length'])
            data = urllib.parse.parse_qs(self.rfile.read(length).decode('utf-8'))
            logger.info('Posted data: "%s"', data)

            if 'markup' in data:
                self._process_markup_result(data)
            elif 'dump' in data:
                self.return_markup(data['dump'][0])

    return CustomHandler


class HTTPServerV6(HTTPServer):
    address_family = socket.AF_INET6


def _parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('paths')
    parser.add_argument('--port', default=8123, type=int, help='port to bind server to')
    parser.add_argument('--ipv6', action='store_true', help='use ipv6')
    return parser.parse_args()


def main():
    args = _parse_args()

    logger.info('Initializing server')
    if args.ipv6:
        httpd = HTTPServerV6(('::', args.port), factory(args.paths))
    else:
        httpd = HTTPServer(('', args.port), factory(args.paths))

    try:
        logger.info('Starting server on port %d', args.port)
        httpd.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        logger.info('Halting backend')
    httpd.server_close()
    logger.info('Server closed')
