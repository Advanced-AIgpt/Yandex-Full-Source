import json
import logging
import re
import requests
from retry import retry
from urllib.parse import quote

from alice.tests.evo_parser.lib.evo_parser_data_pb2 import TEvoParserRequest, TEvoParserResponse


logger = logging.getLogger(__name__)


class ApiHandlers(object):
    ARCANUM = 'https://a.yandex-team.ru/api/tree'
    SANDBOX = 'https://sandbox.yandex-team.ru/api/v1.0'
    STAFF = 'https://staff-api.yandex-team.ru/v3'
    GAP = 'https://staff.yandex-team.ru'


BAD_NAME_PATTERN = re.compile(r'\[.*fail.*\] <span .*>(?P<full_name>(?P<class_name>.*?)<\/span>::(?P<method_name>.*?)\[.*?\]) .*')
PARENT_CLASSNAME_PATTERN = re.compile(r'^class (?P<name>.*?)(\((?P<parent>.*)?\))?:')
OWNER_PATTERN = re.compile(r'owners = \((?P<owners>.*)\)')
SETRACE_PATTERN = re.compile(r'Uuid of fake-user: \b[a-z0-9]+\b')


class ClassPropertiesHolder:

    def __init__(self, parents=(), owners=[], path_code=''):
        self._parents = parents
        self._owners = owners
        self._path_code = path_code

    @property
    def parents(self):
        return self._parents

    @property
    def owners(self):
        return self._owners

    @property
    def path_code(self):
        return self._path_code


@retry(tries=3, delay=1, backoff=2)
def _get_requests_retry(url, headers={}):
    return requests.get(url=url, headers=headers)


def _get_sandbox_task_context(task_id):
    response = _get_requests_retry(url=f'{ApiHandlers.SANDBOX}/task/{str(task_id)}/context')
    return response.json()


def _get_report_content(task_context):
    results_url = task_context['html_result_path']
    return _get_requests_retry(results_url).text


def _find_owners(classes, current_class):

    if current_class not in classes:
        return []

    if classes[current_class].owners:
        return classes[current_class].owners

    for parent in classes[current_class].parents:
        classes[current_class].owners.extend(_find_owners(classes, parent))

    return classes[current_class].owners


def _is_good(line):
    return line.startswith('[<span style="color:#457b23;">good</span>]')


def _is_bad(line):
    return line.startswith('[<span style="color:#b7141e;">fail</span>]')


def _is_link(link_type, line):
    return line.startswith(f'<span style="color:#424242;font-weight:bold;">{link_type}:') and line.endswith('</a>')


def _is_error_snippet(line):
    return line.startswith('E ') or line.startswith('<span style="color:#b7141e;">E ')


def _rebuild_error_snippet(line):
    line = re.sub(r'<span.*?>', ' ', line).replace('</span>', ' ')
    line = filter(lambda x: x.strip(), line.split('E '))
    return u'\n'.join(map(lambda x: u'E ' + x.strip(), line))


def _update_log_link(line):
    index = line.rfind('.')
    if line[index:] == '.log':
        return line
    return line[:index]


def _get_link_from_html(line):
    line = line[:-4]
    return line[line.rfind('>')+1:]


def _parse_runs_lines(report):
    started = False
    runs = []
    current_lines = []

    for line in report.split('\n'):
        line = line.strip()
        if _is_bad(line) or _is_good(line):
            if not started:
                started = True
            if current_lines:
                runs.append(current_lines)
            current_lines = [line]
        else:
            if started:
                current_lines.append(line)
    if current_lines:
        runs.append(current_lines)
    return runs


def _make_setrace_url(log_url):
    def _find_uuid(text):
        expr = SETRACE_PATTERN.findall(text)
        return expr[0].strip().split()[-1] if expr else None

    try:
        log_uuid = _find_uuid(_get_requests_retry(log_url, headers={'Accept': 'text/html'}).text)
    except:
        return 'bad url given'

    if not log_uuid:
        return 'setrace url not found'
    return 'https://setrace.yandex-team.ru/ui/alice/sessionsList?trace_by={uuid}'.format(uuid=log_uuid)


def _parse_bad_names(runs):
    bad_names = []
    release_bad_names = set()

    for run in runs:
        if not _is_bad(run[0]):
            continue
        m = BAD_NAME_PATTERN.match(run[0])

        error_snippet = ''
        links = {}
        full_name = m['full_name'].replace('</span>', '')
        for line in run:
            if _is_link('Log', line) and 'Log' not in links:
                links['Log'] = _get_link_from_html(line.strip())
            elif _is_link('Logsdir', line) and 'Logsdir' not in links:
                links['Logsdir'] = _get_link_from_html(line.strip())
            elif _is_error_snippet(line) and 'Snippet' not in links:
                error_snippet += line + '\n'
            elif error_snippet:
                links['Snippet'] = error_snippet
                error_snippet = ''
            if 'ReleaseBugError' in line:
                release_bad_names.add(full_name)

        if 'Snippet' in links:
            links['Snippet'] = _rebuild_error_snippet(links['Snippet'])
        if 'Log' in links:
            links['Log'] = _update_log_link(links['Log'])
            links['Setrace'] = _make_setrace_url(links['Log'])
        bad_names.append((m['class_name'], m['method_name'], full_name, links))

    return bad_names, release_bad_names


def _get_author_frequency_and_unique_logins(current_errors):
    unique_logins = set()
    counter = {}

    for _, authors_list, _ in current_errors:
        author_hash = json.dumps(authors_list)
        if author_hash not in counter:
            counter[author_hash] = 0
        counter[author_hash] += 1
        unique_logins.update(filter(lambda author: not author.startswith('g:'), authors_list))
    author_frequency = sorted([(counter[author_hash], author_hash) for author_hash in counter], reverse=True)

    return author_frequency, list(unique_logins)


def _make_author_proto(author_name, dismissed_logins, absent_logins, telegram_logins):
    author_proto = TEvoParserResponse.TAuthor()
    author_proto.StaffLogin = author_name
    if author_name.startswith('g:'):
        author_proto.Link = f'https://a.yandex-team.ru/arc/trunk/arcadia/groups/{author_name[2:]}'
    else:
        author_proto.Link = f'{ApiHandlers.GAP}/{author_name}'
        if author_name in dismissed_logins:
            author_proto.AbsenseType = 'Уволился'
        elif author_name in absent_logins:
            author_proto.AbsenseType = absent_logins[author_name]
        if author_name in telegram_logins:
            author_proto.TelegramLogin = telegram_logins[author_name]
    return author_proto


def _make_test_proto(full_name, links):
    test = TEvoParserResponse.TTest()
    test.Name = full_name
    test.SetraceLink = links.get('Setrace', '')
    test.LogLink = links.get('Log', '')
    test.LogsdirLink = links.get('Logsdir', '')
    test.Snippet = links.get('Snippet', '')
    return test


def _make_test_pack_proto(current_errors, author, dismissed_logins, absent_logins, telegram_logins, is_release):
    pack_proto = TEvoParserResponse.TTestsPack()
    pack_proto.IsRelease = is_release

    for author_name in author:
        pack_proto.Authors.append(_make_author_proto(author_name, dismissed_logins, absent_logins, telegram_logins))

    for full_name, real_author, log_links in current_errors:
        if author == real_author or (not author and not real_author):
            pack_proto.Tests.append(_make_test_proto(full_name, log_links))
    return pack_proto


class EvoFailsParser:

    ALICE_PATH = 'alice/tests/integration_tests'

    def __init__(self, request: TEvoParserRequest):
        self._task_id = request.SandboxTaskId
        self._only_release = request.OnlyRelease
        self._arcanum_token = 'OAuth ' + request.ArcanumToken

        self._blame = dict()
        self._blob = dict()
        self._node = dict()

    def _get_node(self, path):
        if path not in self._node:
            url = f'{ApiHandlers.ARCANUM}/node/trunk/arcadia/{path}'
            resp = _get_requests_retry(url=url, headers={'Authorization': self._arcanum_token})
            self._node[path] = json.loads(resp.text)
        return self._node[path]

    def _get_children_files(self, root):
        logger.info(f'Getting all python files in root {root}')
        res = []

        def dfs(current_path):
            node = self._get_node(current_path)
            if node['type'] == 'dir':
                for sub_node in node['children']:
                    dfs(current_path + '/' + sub_node['name'])
            else:
                res.append(current_path)

        dfs(root)
        return res

    def _get_blob(self, path):
        if path not in self._blob:
            url = f'{ApiHandlers.ARCANUM}/blob/trunk/arcadia/{path}'
            resp = _get_requests_retry(url=url, headers={'Authorization': self._arcanum_token})
            self._blob[path] = resp.text
        return self._blob[path]

    def _parse_classes(self, path):
        logging.info(f'Working with path {path}')

        lines = self._get_blob(path).split('\n')

        path_code = path[len(self.ALICE_PATH)+1:].replace('/', '.')

        result = {}
        def add_class(classname, parents, owners):
            if classname:
                result[classname] = ClassPropertiesHolder(parents=parents, owners=owners, path_code=path_code)

        current_classname = ''
        current_parents = ()
        current_owners = []

        for line in lines:
            m = PARENT_CLASSNAME_PATTERN.search(line)
            if m:
                add_class(current_classname, current_parents, current_owners)
                current_owners = []
                current_parents = ()
                current_classname = m['name']
                if 'parent' in m.groupdict() and m['parent']:
                    current_parents = map(lambda x: x.strip(), m['parent'].split(','))
                    current_parents = tuple(s[s.rfind('.')+1:] for s in current_parents if s != 'object')
                continue

            m = OWNER_PATTERN.search(line)
            if m:
                current_owners = list(map(lambda x: x.strip('\'" '), filter(lambda x: x, m['owners'].split(','))))

        add_class(current_classname, current_parents, current_owners)
        return result

    def _build_owners_cache(self):
        files = self._get_children_files(self.ALICE_PATH)
        py_files = [f for f in files if f.endswith('.py')]

        classes = dict()
        for py_file in py_files:
            classes.update(self._parse_classes(py_file))

        owners_dict = dict()
        for classname, class_properties in classes.items():
            owners_dict[f'{class_properties.path_code}::{classname}'] = _find_owners(classes, classname)

        return owners_dict

    def _get_staff_dismissed_logins(self, users):
        query = f'/persons?official.is_dismissed=true&login={quote(",".join(users))}&_fields=login'
        response = _get_requests_retry(url=ApiHandlers.STAFF + query, headers={'Authorization': self._arcanum_token})

        logins = set()
        for elem in response.json().get('result', []):
            login = elem.get('login')
            if login is not None:
                logins.add(login)
        return logins

    def _get_staff_absent_logins(self, users):
        if not users:
            return {}

        query = '/gap-api/api/workflows/'
        response = _get_requests_retry(url=ApiHandlers.GAP + query, headers={'Authorization': self._arcanum_token})
        workflows_desc = {}
        for wf in response.json()['workflows']:
            workflows_desc[wf['type']] = wf['verbose_name']['ru']

        query = '/gap-api/old_api/current.xml?ext=true&login_list=' + quote(','.join(users))
        response = _get_requests_retry(url=ApiHandlers.GAP + query, headers={'Authorization': self._arcanum_token})

        logins = {}

        from xml.etree import ElementTree as ET
        tree = ET.fromstring(response.text)
        for absent in tree[0]:
            attrib = absent.attrib
            logins[attrib['login']] = workflows_desc[attrib['subject_id']]
        return logins

    def _get_staff_tg_logins(self, users):
        users = set(users)
        query = f'/persons?login={quote(",".join(users))}&_fields={quote(",".join(["login", "accounts"]))}'
        response = _get_requests_retry(url=ApiHandlers.STAFF + query, headers={'Authorization': self._arcanum_token})

        if response.status_code != 200:
            logging.info(f'Respose from staff API: {response.status_code} {response.text}')
            return {}

        telegram_logins = dict()
        for row in response.json()['result']:
            staff_login = row['login']
            telegram_login = None
            for account in row['accounts']:
                if account['type'] == 'telegram':
                    telegram_login = account['value']
            telegram_logins[staff_login] = telegram_login

        return telegram_logins

    def _build_failed_test_packs(self, current_errors, telegram_logins, is_release):

        author_frequency, unique_logins = _get_author_frequency_and_unique_logins(current_errors)
        dismissed_logins = self._get_staff_dismissed_logins(unique_logins)

        try:
            absent_logins = self._get_staff_absent_logins(unique_logins)
        except:
            logging.error('Can\'t get absent staff logins')
            absent_logins = {}

        failed_test_packs = []
        for _, author_hash in author_frequency:
            failed_test_packs.append(_make_test_pack_proto(current_errors, json.loads(author_hash) or [], dismissed_logins, absent_logins, telegram_logins, is_release))
        return failed_test_packs

    def _form_response(self, errors_and_authors, release_bad_names, telegram_logins):
        response = TEvoParserResponse()

        release_errors = []
        not_release_errors = []

        for full_name, author, log_links in errors_and_authors:
            if full_name in release_bad_names:
                release_errors.append((full_name, author, log_links))
            else:
                not_release_errors.append((full_name, author, log_links))

        response.FailedTestsPacks.extend(self._build_failed_test_packs(release_errors, telegram_logins, True))

        if self._only_release:
            return response

        response.FailedTestsPacks.extend(self._build_failed_test_packs(not_release_errors, telegram_logins, False))

        return response

    def parse(self):
        owners_cache = self._build_owners_cache()

        task_context = _get_sandbox_task_context(self._task_id)
        runs = _parse_runs_lines(_get_report_content(task_context))
        bad_names, release_bad_names = _parse_bad_names(runs)

        authors = []
        errors_and_authors = []

        for class_name, method, full_name, log_links in bad_names:
            author = []
            if class_name in owners_cache:
                author = owners_cache[class_name]
                authors.extend(author)
            errors_and_authors.append((full_name, author, log_links))

        telegram_logins = self._get_staff_tg_logins(authors)
        return self._form_response(errors_and_authors, release_bad_names, telegram_logins)
