import requests
import json
import logging
from os import getenv


def initialize_logging(name=None):
    """Initialize logging with reasonable format."""
    log = logging.getLogger(name)
    log.setLevel(logging.INFO)
    _stream_handler = logging.StreamHandler()
    _stream_handler.setLevel(logging.DEBUG)
    _stream_handler.setFormatter(logging.Formatter(
        u'%(filename)s[LINE:%(lineno)d]# %(levelname)-8s [%(asctime)s]  %(message)s'
    ))
    log.addHandler(_stream_handler)
    log.propagate = False

    return log


logger = initialize_logging(__name__)


def main():
    try:
        logger.info('Start mega bot experiment poster')

        if getenv('STATE') != 'RUNNING':
            logger.info('Wont Run')
            exit(0)

        # get summary
        ticket = getenv('TICKET_NUMBER')
        logger.info('Ticket is ' + ticket)

        url = 'https://st-api.yandex-team.ru/v2/issues/EXPERIMENTS-' + ticket
        r = requests.get(url, headers={'Authorization': 'OAuth ' + getenv('ST_TOKEN')})

        logger.info('Startrek request status code: ' + str(r.status_code))
        if r.status_code == 200:
            summary = json.loads(r.text)['summary']
            logger.info('Got summary! Look: ' + summary)
        else:
            logger.info('Some fails with startrek! Look: ' + r.text)

        # send message to chat
        url = 'https://api.telegram.org/bot' + getenv('BOT_TOKEN') + '/sendMessage'
        r = requests.post(url, headers={'Content-Type': 'application/json'}, params={
            'chat_id': getenv('CHAT_ID'),
            'text': 'Сейчас запускается <a href="https://st.yandex-team.ru/EXPERIMENTS-' + ticket + '">EXPERIMENTS-' + ticket + '</a>\n\n<b>' + summary + '</b>',
            'parse_mode': 'HTML',
        })

        logger.info('Telegram request status code: ' + str(r.status_code))
        if r.status_code == 200:
            logger.info('Telegram message success!')
        else:
            logger.info('Some fails with telegram! Look: ' + r.text)

    except Exception as e:
        logger.info('Everything went wrong: ' + str(e))
