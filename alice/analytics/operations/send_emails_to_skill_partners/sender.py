# coding: utf-8

import requests
import json
import argparse
import locale
import datetime
import sys

class Recipient(object):
    def __init__(self, email, dau, requests, skill_name):
        if not isinstance(dau, int):
            raise TypeError("DAU should be integer")
        if not isinstance(requests, int):
            raise TypeError("Requests should be integer")

        self._args = {
            'skill_name': skill_name,
            'dau': '{0:n}'.format(dau),
            'requests': '{0:n}'.format(requests),
            'date': None
        }

        self.email = email

    def set_date(self, date):
        self._args['date'] = datetime.datetime.strftime(date, '%x')

    def jsonify_args(self):
        return json.dumps(self._args, ensure_ascii=False, indent=4)


class MultiRequester(object):
    def __init__(self, user, campaign_slug, date):
        self.url = 'https://sender.yandex-team.ru/api/0/yandex.dialogs/transactional/{campaign_slug}/send?to_email='.format(
            campaign_slug=campaign_slug
        )
        self.user = user
        self.date = date
        self._queue = []

    def add_recipient(self, recipient):
        recipient.set_date(self.date)
        self._queue.append(recipient)

    def perform_all(self):
        for recipient in self._queue:
            try:
                r = requests.post(
                    self.url + recipient.email,
                    auth=(self.user, ''),
                    data={
                        'args': recipient.jsonify_args(),
                        'async': False
                    }
                )
                if r.status_code == requests.status_codes.ok:
                    print >>sys.stderr, "Email succesfully sent"
                elif r.status_code == requests.status_codes.accepted:
                    print >>sys.stderr, "Email was not sent, but this behaviour is intended"
                else:
                    print >>sys.stderr, "Email was not sent, code is {code}".format(code=r.status_code)
            except requests.exceptions.RequestException as ex:
                print >>sys.stderr, "Email is not sent due to network problems"
                print >>sys.stderr, ex
            except KeyboardInterrupt:
                print >>sys.stderr, "The process is interrupted"
                raise
            except:
                print >>sys.stderr, "Email is not sent due to unspecified error"
            finally:
                print >>sys.stderr, "Recepient is " + recipient.email
                print >>sys.stderr, ""


def main():
    '''
    doc: https://github.yandex-team.ru/sendr/sendr/blob/master/docs/transaction-api.md
    '''
    locale.setlocale(locale.LC_ALL, '')

    parser = argparse.ArgumentParser()
    parser.add_argument('--user', required=True, help='Identifier from sender')
    parser.add_argument('--partners-data', required=True, help='Information about dau/requests for every partner')
    parser.add_argument('--fielddate', required=True, help='Fielddate in format YYYY-MM-DD')
    parser.add_argument('--test', action='store_true', help='Test only (ignores partners email and sends to robot-voice-qa@yandex-team.ru')
    parser.add_argument('--test-email', help='Use this email instead of default while testing')
    parser.add_argument('--campaign-slug', required=True)

    args = parser.parse_args()

    test_email = 'robot-voice-qa@yandex-team.ru' if args.test_email is None else args.test_email

    requester = MultiRequester(
        user=args.user,
        campaign_slug=args.campaign_slug,
        date=datetime.datetime.strptime(args.fielddate, '%Y-%m-%d')
    )

    with open(args.partners_data, 'r') as f:
        partners = json.loads(f.read())

    for partner in partners:
        for partner_email in partner['partner_emails']:
            email = test_email if args.test else partner_email
            r = Recipient(
                email=email,
                dau=partner['dau'],
                requests=partner['requests'],
                skill_name=partner['skill_name']
            )
            requester.add_recipient(r)

    requester.perform_all()


if __name__ == '__main__':
    main()
