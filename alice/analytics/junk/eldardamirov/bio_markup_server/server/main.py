import argparse
import copy
import http.server
import json
import os
import shutil
import socket
import time
import urllib.parse

import library.python.resource

import voicetech.common.lib.utils as utils
import alice.analytics.operations.asr_annotation.context_retrieval.server.prepare_htmls as prepare_htmls


import dominate
import dominate.tags as tags


from http.server import HTTPServer

logger = utils.initialize_logging(__name__)

not_all_markup_warn_script = """
<script type="text/javascript">
      window.onload = function () {
        document.getElementById("the-form").onsubmit = function onSubmit(form) {
        
        
        var elements = document.getElementsByClassName("my-option");
        
        for(var i = 0; i < elements.length; i++) {
          if (elements[i].value == "Выбрать") {
            alert("Не все запросы размечены");
            return false;
          }
        }
        
        return true;
    }
}
</script>
"""


def factory(fpaths):
    paths = []
    cur_name_paths = {}
    family_member_names = {}

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
            for utotid, value in result.items():
                self.wfile.write('{}\t{}\n'.format(uttid, '1' if value else '0').encode('utf-8'))

        def return_main_page(self):
            logger.info('Returning main page')
            self._set_response(200, content_type='text/html')
            with open('main.html', encoding='utf-8') as fin:
                html = fin.read()
            self.wfile.write(html.encode('utf-8'))

        def build_form_page(self, num_of_family_members, name):
            doc = dominate.document(title='Bio markup, FORM')
            with doc.head:
                tags.meta(charset='utf-8')

            with doc:
                tags.h3('Сбор корзин биометрии')
                tags.p('Введите, пожалуйста, имена членов вашей семьи, их пол и возраст ("ребенок" - до 10 лет включительно, иначе - "взрослый")')
                # tags.p('Среди опций для каждой записи также будут две стандартные - "Остальные" и "Не знаю".')
                # tags.p('В случае, если Вы можете определить голос человека, но не ввели его имя в форму ниже (например, это дальний родственник, который редко бывает у Вас дома) - выберите пункт "Остальные".')
                # tags.p('В случае, если Вы не можете определить голос человека, на записи мяукает кот, голос из телевизора или запись по каким-то причинам не запускается - выберите пункт "Не знаю".')

                with tags.table():
                    
                    with tags.tr():
                        with tags.th():
                            tags.p('Имя')
                        with tags.th():
                            tags.p('Пол')
                        with tags.th():
                            tags.p('Возраст')
                    
                    # with tags.form(method='post'):
                    with tags.form(method='post'):
                        for i in range(num_of_family_members):
                            cur_name = f'member_{i}'
                            with tags.tr():
                                with tags.td():
                                    tags.label(f'#{i}:', fr=cur_name)
                                    tags.input_(type='text', id=cur_name, name=cur_name)
                                
                                with tags.td():
                                    with tags.fieldset():

                                        with tags.div():
                                            tags.label('male', fr=f'{cur_name}|female')
                                            tags.input_(type='radio', id=f'{cur_name}|male', name=f'{cur_name}|gender', value='male')

                                        with tags.div():
                                            tags.label('female', fr=f'{cur_name}|male')
                                            tags.input_(type='radio', id=f'{cur_name}|female', name=f'{cur_name}|gender', value='female')

                                with tags.td():
                                    with tags.fieldset():

                                        with tags.div():
                                            tags.label('adult', fr=f'{cur_name}|adult')
                                            tags.input_(type='radio', id=f'{cur_name}|adult', name=f'{cur_name}|age', value='adult')

                                        with tags.div():
                                            tags.label('child', fr=f'{cur_name}|child')
                                            tags.input_(type='radio', id=f'{cur_name}|child', name=f'{cur_name}|age', value='child')
                        
                                
                    # with tags.form(method='post'):
                        with tags.tr():
                            with tags.td():
                                tags.button('Далее', name='start', value=f'{name}', type='submit')

            html = doc.render()
            
            with open('form.html', 'w') as fout:
                fout.write(html)
            
            return

        def return_form_page(self):
            logger.info('Returning form page')
            self._set_response(200, content_type='text/html')
            with open('form.html', encoding='utf-8') as fin:
                html = fin.read()
            self.wfile.write(html.encode('utf-8'))

        def return_finish_page(self, name):
            logger.info('Returning main page')
            self._set_response(200, content_type='text/html')
            with open('finish.html', encoding='utf-8') as fin:
                html = fin.read()
            html = html.replace('DUMP_VALUE', name)
            self.wfile.write(html.encode('utf-8'))

        def return_error_page(self, name):
            logger.info('Returning ERROR page')
            self._set_response(200, content_type='text/html')
            with open('error.html', encoding='utf-8') as fin:
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
            print('LOG: cur_name_paths:', cur_name_paths)
            with open(cur_name_paths[name][id], encoding='utf-8') as fin:
                html = fin.read()
            html = html.replace('DUMP_VALUE', name)

            print('LOG: family_member_names:', family_member_names)
            to_ins = ''
            for cur_member in family_member_names[name]:
                cur_name = cur_member['name']
                to_ins += str(tags.option(cur_name, value=cur_name, type='submit')) + '\n\t\t'

            html = html.replace('<p>OPTIONS_LIST</p>', to_ins) 
            html = html.replace('<p>NOT_ALL_MARKUP_WARN</p>', not_all_markup_warn_script)

            
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
                # content_disposition='attachment; filename={}'.format(name + '.json'),
                content_disposition='attachment; filename={}'.format('bio_markup_dump' + '.json'),
            )
            self.wfile.write(json.dumps(markup_data, ensure_ascii=False).encode('utf-8'))

        def dispatch(self, path, query_args):
            logger.info('Dispatching request:')
            logger.info('path=%s', path)
            for k, v in query_args.items():
                logger.info('args[%s] = "%s"', k, v)

            if 'login' in query_args:
                name = query_args['login'][0]
                if 'num_of_family_members' in query_args:
                    family_member_qnt = int(query_args['num_of_family_members'][0])
                else:
                    family_member_qnt = 1
                self.build_form_page(family_member_qnt, name)

                logger.info('Starting markup for %s', name)

                tmp_path_storage = []
                for path in paths:
                    t = path.split('/')
                    if t[3] == name:
                        tmp_path_storage.append(path)

                cur_name_paths[name] = [''] * len(tmp_path_storage)
                for path in tmp_path_storage:
                    t = path.split('/')
                    cur_page_id = t[-1].split('.')[0]
                    cur_name_paths[name][int(cur_page_id)] = path

                print('LOG: cur_name_paths:', cur_name_paths[name])
                self._set_response(302, location=f'?form_for={name}'.format(name))

                return

            if 'form_for' in query_args:
                self.return_form_page()
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
                    if len(cur_name_paths[name]) == 0:
                        self.return_error_page(name)
                    elif cur_name_paths[name][int(id)] != '':
                        self.return_markup_page(name, int(id))
                    else:
                        self.return_finish_page(name)
                return

            if path == '/':
                self.return_main_page()
                return

        def do_GET(self):
            logger.info('Received GET request')
            request_params = urllib.parse.urlparse(self.path)
            self.dispatch(request_params.path, urllib.parse.parse_qs(request_params.query))

        def _process_bio_markup_result(self, data, set_response=True):
            logger.info("Processing bio markup result")
            print('LOG: data:', data)

            name = ''
            if 'next' in data:
                name = data['next'][0].split('|')[1]
            elif 'prev' in data:
                name = data['prev'][0].split('|')[1]
            elif 'dump' in data:
                name = data['dump'][0].split('|')[1]

            markup_data = self._get_markup_for_name(name)
            print('LOG: markup_data:', markup_data, name, '!!!!')
            for key, value in data.items():
                if key == 'prev' or key == 'next' or key == 'dump':
                    continue

                msg_type = key.split('|')[0]
                message_id = key.split('|')[1]
                name = key.split('|')[2]

                if msg_type == 'bio_markup':
                    if value[0].split('|')[0] != 'Выбрать':
                        if message_id not in markup_data:
                            markup_data[message_id] = {'result': value[0].split('|')[0]}
                        else:
                            markup_data[message_id]['result'] = value[0].split('|')[0]
                        
                if msg_type == 'comment':
                    if message_id not in markup_data:
                        markup_data[message_id] = {'comment': value[0]}
                    else:
                        markup_data[message_id]['comment'] = value[0]

            fname = os.path.join('markup', name + '.json')

            with open(fname, 'w') as fout:
                json.dump(markup_data, fout, indent=2, ensure_ascii=False)

            if set_response:
                if 'next' in data:
                    name = data['next'][0].split('|')[1]
                    cur_page_id = data['next'][0].split('|')[2]
                    print('LOG: ', name, '| opened page', cur_page_id)
                    self._set_response(302, location='?name={}&id={}'.format(name, int(cur_page_id) + 1))
                elif 'prev' in data:
                    name = data['prev'][0].split('|')[1]
                    cur_page_id = data['prev'][0].split('|')[2]
                    print('LOG: ', name, '| opened page', cur_page_id)
                    self._set_response(302, location='?name={}&id={}'.format(name, int(cur_page_id) - 1))
                else:
                    self._set_response(302, location='?name={}&id={}'.format(name, 0))


        def _process_markup_result(self, data):
            logger.info("Processing markup result")
            markup_tokens = data['next'][0].split('|')
            name = markup_tokens[1]

            markup_data = self._get_markup_for_name(name)
            fname = os.path.join('markup', name + '.json')
            with open(fname, 'a+') as fout:
                json.dump(markup_data, fout, indent=2, ensure_ascii=False)

            self._set_response(302, location='?login={}'.format(markup_tokens[1]))

        def do_POST(self):
            logger.info('Received POST request')
            logger.info('path: %s', self.path)
            length = int(self.headers['Content-length'])
            data = urllib.parse.parse_qs(self.rfile.read(length).decode('utf-8'))
            logger.info('Posted data: "%s"', data)
            
  
            if 'start' in data:
                family_members_storage = []
                for i in range(len(data) - 1):
                    cur_name = f'member_{i}'
                    cur_data = {}
                    if cur_name in data:
                        cur_data['name'] = data[cur_name][0]
                        if cur_name + '|gender' in data:
                            cur_data['gender'] = data[str(cur_name + '|gender')][0]
                        if cur_name + '|age' in data:
                            cur_data['age'] = data[str(cur_name + '|age')][0]
                        family_members_storage.append(cur_data)
                    else:
                        break

                # USER_NAME FROM data['start'] passed as VALUE:
                user_name = data['start'][0]

                print('LOG: ', family_members_storage)
                nonlocal family_member_names
                family_member_names[user_name] = family_members_storage

                fname = os.path.join('family_members', user_name + '_familymembers.json')
                with open(fname, 'w') as fout:
                    json.dump(family_members_storage, fout, indent=2, ensure_ascii=False)
                self._set_response(302, location=f'?name={user_name}&id={0}')
            elif 'dump' in data:
                user_name = data['dump'][0].split('|')[1]
                print('LOG: dumping for:', user_name)
                self._process_bio_markup_result(data, set_response=False)
                self.return_markup(user_name)
            else:
                self._process_bio_markup_result(data)


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

