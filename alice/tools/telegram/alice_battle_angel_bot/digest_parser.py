import re

from .common import (get_latest_vins_release_st_ticket, get_st_ticket, get_st_ticket_comments,
                        get_comment_about_manual_testing, get_staff_tg_logins)


class DigestParser(object):

    @staticmethod
    def _is_ok(line):
        return '!!(зел)ОК!!' in line

    @staticmethod
    def _is_fail(line):
        return '!!НЕОК!!' in line

    def _parse_table(self):
        msg = ''
        for line in self._ticket['description'].split('\n'):
            if not line.startswith('||'):
                continue

            if self._is_ok(line) or self._is_fail(line):
                if self._is_ok(line) and self._is_fail(line):
                    msg += '⚠️ ок/неок'
                elif self._is_ok(line):
                    msg += '✅ ок'
                else:
                    msg += '⛔️ неок'

                msg += ' — *{}*\n'.format(re.search(r'\|\| (.*?) \|', line).group(1))
        return msg

    def _parse_manual_testing(self):
        if not self._manual_testing_comment:
            return ''

        msg = '*Состояние регресса*\n'
        ticket_keys = re.findall(r'https://st.yandex-team.ru/(.*?)\s', self._manual_testing_comment['text'] + '\n')
        tickets = [get_st_ticket(key) for key in ticket_keys]
        tickets = [ticket for ticket in tickets if 'key' in ticket]
        assignees = [ticket['assignee']['id'] for ticket in tickets if 'assignee' in ticket]

        tg_logins = get_staff_tg_logins(assignees)
        for ticket in tickets:
            key = ticket['key']
            info = '[{}](https://st.yandex-team.ru/{}) - *{}*'.format(key, key, ticket['status']['display'])
            if 'assignee' in ticket:
                assignee = ticket['assignee']['id']
                info += ' (исполнитель [{}](https://staff.yandex-team.ru/{}), \\[`@{}`])'.format(
                    assignee.replace('_', '\\_'),
                    assignee.replace('_', '\\_'),
                    tg_logins[assignee].replace('_', '\\_')
                )
            info += '\n'
            info += '`' + ticket['summary'] + '`'
            msg += info + '\n\n'

        return msg

    def _prepare_msg(self):
        msg = 'Общее состояние релиза *{}* {}\n'.format(self._ticket['summary'], '([тикет](https://st.yandex-team.ru/' + self._ticket['key'] + '))')
        msg += self._parse_table() + '\n\n'
        msg += self._parse_manual_testing() + '\n'
        return msg

    def parse(self):
        tickets = get_latest_vins_release_st_ticket(count=2)
        msg = ''
        for ticket in tickets:
            comments = get_st_ticket_comments(ticket['key'])
            manual_testing_comment = get_comment_about_manual_testing(comments)

            self._ticket= ticket
            self._manual_testing_comment = manual_testing_comment

            msg += self._prepare_msg()
        return msg
