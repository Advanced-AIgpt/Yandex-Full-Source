import json
import logging
import os
import traceback
from flask import Flask
from telegram.ext import Updater, MessageHandler, CommandHandler, Filters, run_async
from threading import Thread

from .common import (get_staff_info_by_tg_login, slice_long_message, get_st_ticket,
                     get_test_ids, create_sandbox_task_from_template, run_sandbox_task,
                     validate_st_ticket_name, build_duty_summon)
from .branch_parser import BranchParser
from .digest_parser import DigestParser
from .evo_fails_parser import EvoFailsParser
from .ydb_queue import YdbQueue


QUEUE_CHAT_ID = -1001285806271
MOVE_TO_QUEUE_CHAT_MSG = "Очередь выкаток вынесена в отдельный чат. Пожалуйста, используйте его.\n\n🚀 https://t.me/joinchat/JczMzkc7zgA4Yjky 🚀"


class BotHandler(object):
    def __init__(self):
        self._ydb_queue = YdbQueue()

    # HELPER METHODS
    @staticmethod
    def is_debug_message(update):
        msg = update.message
        chat_id = msg.chat_id
        return msg.chat.type == 'private' and str(chat_id) == os.environ['BOT_MASTER']

    @staticmethod
    def is_secure(update):
        msg = update.message
        if msg.chat.type == 'private':
            if not get_staff_info_by_tg_login(update.message.chat.username):
                return False
        return True

    def _send_parsed_info(self, bot, update, parser):
        msg = update.message
        for part in slice_long_message(parser.parse()):
            bot.send_message(chat_id=msg.chat_id, text=part, parse_mode='Markdown')

    # COMMAND METHODS
    @run_async
    def echo(self, bot, update):
        if self.is_debug_message(update):
            msg = update.message
            bot.send_message(chat_id=msg.chat_id, text=msg.text)

    @run_async
    def info(self, bot, update):
        if not self.is_secure(update):
            return
        pass

    @run_async
    def duty(self, bot, update):
        if not self.is_secure(update):
            return
        msg = update.message
        for part in slice_long_message(build_duty_summon(msg.text)):
            bot.send_message(chat_id=msg.chat_id, text=part, parse_mode='Markdown')

    @run_async
    def evo_tests(self, bot, update):
        """
            Show EVO tests info
        """
        if not self.is_secure(update):
            return
        dummy = bot.send_message(chat_id=update.message.chat_id, text='Пожалуйста, подождите около минуты, отчет готовится... 🔍')
        self._send_parsed_info(bot, update, parser=EvoFailsParser(update))
        bot.delete_message(chat_id=update.message.chat_id, message_id=dummy.message_id)

    @run_async
    def evo_tests_raw(self, bot, update):
        """
            Show raw EVO tests info
        """
        if not self.is_secure(update):
            return
        dummy = bot.send_message(chat_id=update.message.chat_id, text='Пожалуйста, подождите около минуты, отчет готовится... 🔍')
        self._send_parsed_info(bot, update, parser=EvoFailsParser(update, raw=True))
        bot.delete_message(chat_id=update.message.chat_id, message_id=dummy.message_id)

    @run_async
    def digest(self, bot, update):
        """
            Show digest info
        """
        if not self.is_secure(update):
            return
        self._send_parsed_info(bot, update, parser=DigestParser())

    @run_async
    def branch(self, bot, update):
        """
            Show branch calendar info
        """
        if not self.is_secure(update):
            return
        self._send_parsed_info(bot, update, parser=BranchParser())

    @run_async
    def show_queue(self, bot, update):
        """
            Show chat's queue
        """
        if not self.is_secure(update):
            return
        chat_id = update.message.chat_id
        if chat_id != QUEUE_CHAT_ID:
            bot.send_message(chat_id=chat_id, text=MOVE_TO_QUEUE_CHAT_MSG, parse_mode='Markdown')
            return
        for part in slice_long_message(self._ydb_queue.show_queue(update)):
            bot.send_message(chat_id=chat_id, text=part, parse_mode='Markdown')

    @run_async
    def enqueue(self, bot, update):
        """
            Enqueue user with his task
        """
        if not self.is_secure(update):
            return
        chat_id = update.message.chat_id
        if chat_id != QUEUE_CHAT_ID:
            bot.send_message(chat_id=chat_id, text=MOVE_TO_QUEUE_CHAT_MSG, parse_mode='Markdown')
            return
        for part in slice_long_message(self._ydb_queue.enqueue(update)):
            bot.send_message(chat_id=chat_id, text=part, parse_mode='Markdown')

    @run_async
    def unqueue(self, bot, update):
        """
            Unqueue user with his task
        """
        if not self.is_secure(update):
            return
        chat_id = update.message.chat_id
        if chat_id != QUEUE_CHAT_ID:
            bot.send_message(chat_id=chat_id, text=MOVE_TO_QUEUE_CHAT_MSG, parse_mode='Markdown')
            return
        for part in slice_long_message(self._ydb_queue.unqueue(update)):
            bot.send_message(chat_id=chat_id, text=part, parse_mode='Markdown')

    @run_async
    def swap(self, bot, update):
        """
            Swap users
        """
        if not self.is_secure(update):
            return
        chat_id = update.message.chat_id
        if chat_id != QUEUE_CHAT_ID:
            bot.send_message(chat_id=chat_id, text=MOVE_TO_QUEUE_CHAT_MSG, parse_mode='Markdown')
            return
        for part in slice_long_message(self._ydb_queue.swap(update)):
            bot.send_message(chat_id=chat_id, text=part, parse_mode='Markdown')

    @run_async
    def run_evo_tests(self, bot, update):
        """
            Creates Sandbox task from test-ticket and runs EVO tests.
            Sample ticket: https://st.yandex-team.ru/ABR-13
        """

        if not self.is_secure(update):
            return

        ticket_name = validate_st_ticket_name(update.message.text)
        if not ticket_name:
            bot.send_message(chat_id=update.message.chat_id, text='Не могу распознать название тикета', parse_mode='Markdown')
        logging.info('Ticket name: "%s"' % ticket_name)

        ticket = get_st_ticket(ticket_name)
        test_ids = get_test_ids(ticket)
        if not test_ids:
            bot.send_message(chat_id=update.message.chat_id, text='Не могу распознать test ids из тикета {}'.format(ticket_name), parse_mode='Markdown')

        msg = '🛠️ *Запущенные таски*:\n\n'
        for i in test_ids:
            task_id = create_sandbox_task_from_template(ticket_name, i)
            run_sandbox_task(task_id)
            msg += 'https://sandbox.yandex-team.ru/task/{}/view (test\\_id = `{}`)\n'.format(task_id, i)
        bot.send_message(chat_id=update.message.chat_id, text=msg, parse_mode='Markdown')


app = Flask('bot')


@app.route('/')
def flask_check():
    return 'Hello! It\'s me, Alice Battle Angel'


def launch_sandbox_task(test_id, task_id):
    ydb = YdbQueue()
    ydb.put_sandbox_task_id(test_id, task_id)
    run_sandbox_task(task_id)
    logging.info('Task {} launched'.format(task_id))


@app.route('/run_evo/<ticket_name>')
def flask_run_evo(ticket_name):
    try:
        ticket = get_st_ticket(ticket_name)
        test_ids = get_test_ids(ticket)
        if not test_ids:
            return json.dumps({'status': 'error', 'text': 'can\'t find test ids'})

        ydb = YdbQueue()
        task_ids_map = ydb.get_sandbox_task_ids()

        task_links = []
        for test_id in test_ids:
            test_id = int(test_id)
            if test_id in task_ids_map:
                task_id = task_ids_map[test_id]
            else:
                task_id = create_sandbox_task_from_template(ticket_name, str(test_id))
                Thread(target=launch_sandbox_task, args=(test_id, task_id)).start()

            task_links.append('https://sandbox.yandex-team.ru/task/{}/view'.format(task_id))

        return json.dumps({'status': 'ok', 'text': task_links})
    except Exception:
        return json.dumps({'status': 'error', 'text': str(traceback.format_exc())})


def flask_runner():
    app.run(host='::', port=os.environ['FLASK_PORT'])


if __name__ == '__main__':
    logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')

    thread = Thread(target=flask_runner)
    thread.start()

    bot_handler = BotHandler()

    updater = Updater(token=os.environ['TELEGRAM_TOKEN'], use_context=False)
    dispatcher = updater.dispatcher

    dispatcher.add_handler(MessageHandler(Filters.text & (~Filters.command), bot_handler.echo))
    dispatcher.add_handler(CommandHandler('info', bot_handler.info))
    dispatcher.add_handler(CommandHandler('duty', bot_handler.duty))

    dispatcher.add_handler(CommandHandler('run_evo_tests', bot_handler.run_evo_tests))
    dispatcher.add_handler(CommandHandler('evo_tests', bot_handler.evo_tests))
    dispatcher.add_handler(CommandHandler('evo_tests_raw', bot_handler.evo_tests_raw))
    dispatcher.add_handler(CommandHandler('digest', bot_handler.digest))
    dispatcher.add_handler(CommandHandler('branch', bot_handler.branch))

    dispatcher.add_handler(CommandHandler('queue', bot_handler.show_queue))
    dispatcher.add_handler(CommandHandler('push', bot_handler.enqueue))
    dispatcher.add_handler(CommandHandler('pop', bot_handler.unqueue))
    dispatcher.add_handler(CommandHandler('swap', bot_handler.swap))

    updater.start_polling()
    updater.idle()
    thread.join()
