import logging
import os
import re
import time
import ydb


class YdbQueue(object):
    def __init__(self):
        driver_config = ydb.DriverConfig(endpoint=os.getenv('YDB_ENDPOINT'), database=os.getenv('YDB_DATABASE'), auth_token=os.getenv('YDB_TOKEN'))
        try:
            self._driver = ydb.Driver(driver_config)
            self._driver.wait(timeout=5)
        except TimeoutError:
            logging.error('Connect failed to YDB')
            logging.error('Last reported errors by discovery: %s' % str(self._driver.discovery_debug_details()))

    @staticmethod
    def _get_chat_title(update):
        chat = update.message.chat
        return chat.title or chat.username

    @staticmethod
    def _get_target_username_and_text(update):
        def text_filter(text):
            return ' '.join([w for w in text.split() if not (re.match(r'^@[^\s]+$', w) or w.startswith('/'))])

        text = update.message.text
        for word in text.split():
            if re.match(r'^@[^\s]+$', word):
                return word, text_filter(text)
        return ('@' + update.message.from_user.username, text_filter(text))

    def _get_order_rows(self, update):
        session = self._driver.table_client.session().create()
        result_sets = session.transaction(ydb.SerializableReadWrite()).execute(
            """
            PRAGMA TablePathPrefix("{path}");
            SELECT *
            FROM bot_queue
            WHERE chat_id = {chat_id}
            ORDER BY created;
            """.format(path=os.getenv('YDB_DATABASE'), chat_id=update.message.chat.id),
            commit_tx=True,
        )
        return result_sets[0].rows

    def _get_order(self, update):
        rows = self._get_order_rows(update)
        order = []
        for row in rows:
            order.append((row.username.decode('utf-8'), row.theme.decode('utf-8')))
        return order

    def show_queue(self, update):
        order = self._get_order(update)
        msg = '⌛ Состояние очереди в чате *{}*\n\n'.format(self._get_chat_title(update))
        cnt = 1
        for username, theme in order:
            msg += '{}. `{}` в очереди для: `{}` '.format(cnt, username, theme)
            msg += '\n'
            cnt += 1
        if cnt == 1:
            msg += 'Очередь пустая'
        return msg

    def enqueue(self, update):
        username, text = self._get_target_username_and_text(update)
        if not text:
            return 'Задайте тему очереди (для чего хотите ее занять).'

        session = self._driver.table_client.session().create()
        session.transaction(ydb.SerializableReadWrite()).execute(
            """
            PRAGMA TablePathPrefix("{path}");
            UPSERT INTO bot_queue (chat_id, created, theme, username) VALUES
                ({chat_id}, {created}, "{theme}", "{username}");
            """.format(path=os.getenv('YDB_DATABASE'), chat_id=update.message.chat.id, created=int(time.time()), theme=text, username=username),
            commit_tx=True,
        )

        msg = 'В очередь встал {}, тема `{}`.\n\n'.format(username.replace('_', '\\_'), text)

        order = self._get_order(update)
        if len(order) == 1:
            msg += 'Вы один в очереди. Можно катить.'
        else:
            username, theme = order[0]
            msg += 'Перед вами еще *{}* человек. Сейчас `{}` катит `{}`.'.format(len(order) - 1, username, theme)
        return msg

    def unqueue(self, update):
        username, text = self._get_target_username_and_text(update)

        session = self._driver.table_client.session().create()
        result_sets = session.transaction(ydb.SerializableReadWrite()).execute(
            """
            PRAGMA TablePathPrefix("{path}");
            SELECT *
            FROM bot_queue
            WHERE chat_id = {chat_id}
            ORDER BY created;
            """.format(path=os.getenv('YDB_DATABASE'), chat_id=update.message.chat.id),
            commit_tx=True,
        )

        row_to_delete = None
        is_first_row = True
        rows = result_sets[0].rows
        for row in rows:
            if row.username.decode('utf-8') == username:
                row_to_delete = row
                break
            is_first_row = False

        if not row_to_delete:
            return 'Пользователь `{}` не стоял в очереди 🤪'.format(username)

        session.transaction(ydb.SerializableReadWrite()).execute(
            """
            PRAGMA TablePathPrefix("{path}");
            DELETE FROM bot_queue
            WHERE chat_id = {chat_id} AND created = {created};
            """.format(path=os.getenv('YDB_DATABASE'), chat_id=update.message.chat.id, created=row_to_delete.created),
            commit_tx=True,
        )

        msg = 'Из очереди вышел {}, тема `{}`.\n\n'.format(username.replace('_', '\\_'), row_to_delete.theme.decode('utf-8'))
        if is_first_row and len(rows) > 1:
            msg += 'Ваша очередь, {}.\n'.format(rows[1].username.decode('utf-8').replace('_', '\\_'))
            msg += self.show_queue(update)
        return msg

    def swap(self, update):
        s = update.message.text.split(' ')
        num1 = int(s[1]) - 1
        num2 = int(s[2]) - 1

        rows = self._get_order_rows(update)

        if max(num1, num2) >= len(rows):
            return 'Выбрано слишком большое место! Длина очереди *{}*.'.format(len(rows))

        session = self._driver.table_client.session().create()
        for created, theme, username in [
            (rows[num1].created, rows[num2].theme, rows[num2].username),
            (rows[num2].created, rows[num1].theme, rows[num1].username),
        ]:
            session.transaction(ydb.SerializableReadWrite()).execute(
                """
                PRAGMA TablePathPrefix("{path}");
                UPSERT INTO bot_queue (chat_id, created, theme, username) VALUES
                    ({chat_id}, {created}, "{theme}", "{username}");
                """.format(path=os.getenv('YDB_DATABASE'), chat_id=update.message.chat.id, created=created, theme=theme.decode('utf-8'), username=username.decode('utf-8')),
                commit_tx=True,
            )

        msg = 'Места *{}* и *{}* поменялись.\n\n'.format(num1 + 1, num2 + 1)
        if min(num1, num2) == 0:
            # Note: rows list not updated, only ydb updated
            msg += '{}, вам уступили первое место, можно катить.\n\n'.format(rows[max(num1, num2)].username.decode('utf-8').replace('_', '\\_'))

        msg += self.show_queue(update)
        return msg

    # methods for run_evo_tests
    def get_sandbox_task_ids(self):
        session = self._driver.table_client.session().create()
        result_sets = session.transaction(ydb.SerializableReadWrite()).execute(
            """
            PRAGMA TablePathPrefix("{path}");
            SELECT *
            FROM bot_evo_runs;
            """.format(path=os.getenv('YDB_DATABASE')),
            commit_tx=True,
        )

        res = {}
        for row in result_sets[0].rows:
            res[row['test_id']] = row['sandbox_task_id']
        return res

    def put_sandbox_task_id(self, test_id, task_id):
        session = self._driver.table_client.session().create()
        session.transaction(ydb.SerializableReadWrite()).execute(
            """
            PRAGMA TablePathPrefix("{path}");
            UPSERT INTO bot_evo_runs (test_id, sandbox_task_id) VALUES
                ({test_id}, {sandbox_task_id});
            """.format(path=os.getenv('YDB_DATABASE'), test_id=test_id, sandbox_task_id=task_id),
            commit_tx=True,
        )
