import logging
import threading
import traceback
import yt.wrapper as yt

from time import time, sleep
from hashlib import sha256
from queue import Queue, Empty

from woz_data_collection_bot.config import Config
from woz_data_collection_bot.env import DialogManagerEnviroment, DialogState, Phrases

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)


class BotCommands:
    def __init__(self, env: DialogManagerEnviroment, config: Config, phrases: Phrases):
        self.env = env
        self.phrases = phrases
        self.dump_dialog_queue = Queue(maxsize=16)
        self.exit_event = threading.Event()
        self.config = config

    def check_unauth_user(self, message):
        if message is None:
            return True
        if message.from_user is None or not self.env.check_user(message.from_user.username):
            message.reply_text(self.phrases.no_access)
            return True
        return False

    def broadcast(self, dialog, bot, user_text, alice_text=None):
        user = dialog.user
        alice = dialog.alice
        bot.sendMessage(user, text=user_text)
        bot.sendMessage(alice, text=alice_text or user_text)

    def _dumps_dialog(self):
        while not self.exit_event.is_set():
            try:
                dialog = self.dump_dialog_queue.get(timeout=10)
                yt.write_table(
                    yt.TablePath(self.config.result_table_yt_path, append=True),
                    [
                        {
                            "alice_id": dialog.alice,
                            "user_id": dialog.user,
                            "dialog_id": dialog.dialog_id,
                            "dialog": dialog.log,
                            "debug_log": dialog.debug_log,
                            "user_story": dialog.user_story,
                            "user_task": dialog.user_task,
                        }
                    ],
                )
            except Empty:
                pass

    def finalize_dialog(self, dialog, bot, logging_dialog: bool = True, send_code: bool = True):
        if send_code:
            bot.sendMessage(dialog.alice, text=self.phrases.write_code.format(dialog.dialog_id, dialog.alice))
            bot.sendMessage(dialog.user, text=self.phrases.write_code.format(dialog.dialog_id, dialog.user))
        dialog.state = DialogState.FINISHED
        self.broadcast(dialog, bot, self.phrases.dialog_ended)
        if dialog.user in self.env.user_to_dialog: 
            del self.env.user_to_dialog[dialog.user]
        if dialog.alice in self.env.user_to_dialog:
            del self.env.user_to_dialog[dialog.alice]

        if dialog.log and logging_dialog:
            self.dump_dialog_queue.put(dialog)

    def start_dialog(self, dialog_id, context, role_id):
        dialog = self.env.dialogs[dialog_id]
        dialog.user_story = self.env.tasks[role_id][0]
        dialog.user_task = self.env.tasks[role_id][1]
        dialog.state = DialogState.STARTED
        dialog.start_msg_time = time()
        dialog.last_msg_time = time()
        dialog.last_user_msg_time = time()
        dialog.last_alice_msg_time = time()
        dialog.dialog_id = dialog_id
        user_text = self.phrases.user_start_text.format(
            self.config.start_text, self.config.user_base_text, dialog.user_story, dialog.user_task
        )
        alice_text = self.phrases.alice_start_text.format(
            self.config.start_text, self.config.alice_base_text, dialog.user_story, dialog.user_task
        )
        self.broadcast(dialog, context.bot, user_text, alice_text)


    def _check_all_dialogs(self, bot):
        while not self.exit_event.is_set():
            try:
                for dialog_id, dialog in self.env.dialogs.items():
                    if dialog.state == DialogState.FINISHED:
                        continue
                    if dialog.state == DialogState.STARTED:
                        if time() - dialog.last_msg_time > self.config.timeout:
                            logger.info(f"Dialog {dialog_id} has been finished by timeout")
                            self.broadcast(dialog, bot, self.phrases.finished_by_timeout)
                            self.finalize_dialog(dialog, bot, send_code=False)
                            continue
                    all_msg_alices = all(phrase[0] == self.phrases.alice_name for phrase in dialog.log)
                    all_msg_users = all(phrase[0] == self.phrases.user_name for phrase in dialog.log)
                    if len(dialog.log) == 0 or all_msg_alices or all_msg_users:
                        if time() - dialog.start_msg_time > self.config.starting_dialog_timeout:
                            logger.info(f"Dialog {dialog_id} has been finished by inactivity")
                            self.broadcast(dialog, bot, self.phrases.finished_by_inactivity)
                            self.finalize_dialog(dialog, bot, send_code=False)
                            continue
                sleep(1)
            except KeyboardInterrupt:
                raise
            except Exception as e:
                logger.error(traceback.format_exc())
                pass

    def run_parallel_jobs(self, bot):
        threads = [
            threading.Thread(target=self._check_all_dialogs, args=(bot,)),
            threading.Thread(target=self._dumps_dialog),
        ]
        for thread in threads:
            thread.start()
        return self.exit_event, threads

    def check_active(self, bot, user_id):
        if user_id not in self.env.user_to_dialog:
            bot.sendMessage(user_id, text=self.phrases.have_no_opponent)
            return False
        dialog = self.env.dialogs[self.env.user_to_dialog[user_id]]
        if dialog.state not in (DialogState.STARTED, DialogState.ENOUGH):
            bot.sendMessage(user_id, text=self.phrases.dialog_not_started)
            return False
        return True

    def try_make_pair(self, dialog_id, context, user_id, role=None):
        next_user_id, next_dialog_id = None, None
        with self.env.user_queue_lock():
            if self.env.waiting_user_queue is None:
                self.env.waiting_user_queue = (user_id, dialog_id, role)
            else:
                next_user_id, next_dialog_id, next_role = self.env.waiting_user_queue
                self.env.waiting_user_queue = None

        if next_user_id is not None:
            new_dialog_id = "-".join(map(str, [dialog_id, user_id, next_dialog_id, next_user_id, time()]))
            m = sha256()
            m.update(new_dialog_id.encode("utf-8"))
            new_dialog_id = m.hexdigest()
            dialog = self.env.dialogs[new_dialog_id]
            self.env.user_to_dialog[user_id] = new_dialog_id
            self.env.user_to_dialog[next_user_id] = new_dialog_id
            if role == next_role:
                alice = user_id
                user = next_user_id
            elif role == "alice":
                alice = user_id
                user = next_user_id
            elif role == "user":
                alice = next_user_id
                user = user_id
            else:
                if next_role == "alice":
                    alice = next_user_id
                    user = user_id
                else: 
                    alice = user_id
                    user = next_user_id
            dialog.alice = alice
            logger.info(f"User {alice} joined as Alice in dialog {new_dialog_id}")
            dialog.user = user
            logger.info(f"User {user} joined as User in dialog {new_dialog_id}")
            role_id = int(new_dialog_id, base=16) % len(self.env.tasks)
            self.start_dialog(new_dialog_id, context, role_id)
            self.env.remove_active(user_id)
            self.env.remove_active(next_user_id)
        else:
            context.bot.sendMessage(user_id, text=self.phrases.waiting_opponent)

    def send_message(self, update, context, prefix="", is_cmd: bool = False):
        user_id = update.message.chat_id
        dialog = self.env.get_dialog(user_id)
        is_user = dialog.user == user_id
        text = update.message.text
        if is_cmd:
            try:
                text = text.split(" ", 1)[1]
            except (IndexError, ValueError):
                context.bot.sendMessage(user_id, text=self.phrases.wrong_command)
                return
        message = (
            self.phrases.user_name if is_user else self.phrases.alice_name,
            prefix + text,
            time()
        )
        if is_user:
            context.bot.sendMessage(dialog.alice, text=f"{message[0]}: {message[1]}")
        else:
            context.bot.sendMessage(dialog.user, text=f"{message[0]}: {message[1]}")
        return message

    def start(self, update, context):
        if self.check_unauth_user(update.message):
            return
        user_id = update.message.chat_id
        if user_id in self.env.user_to_dialog:
            context.bot.sendMessage(user_id, text=self.phrases.no_enter_have_active)
            return
        if self.env.check_active(user_id):
            context.bot.sendMessage(user_id, text=self.phrases.no_enter_until_opponent)
            return
        try:
            dialog_id = update.message.text.split()[1]
            if dialog_id.endswith("@alice"):
                role = "alice"
                dialog_id = dialog_id[:-6]
            elif dialog_id.endswith("@user"):
                role = "user"
                dialog_id = dialog_id[:-5]
            else:
                role = None
            if dialog_id not in self.env.unique_links:
                raise ValueError
        except (IndexError, ValueError):
            context.bot.sendMessage(user_id, text=self.phrases.wrong_link)
            self.env.remove_active(user_id)
            return
        self.try_make_pair(dialog_id, context, user_id, role)

    def send(self, update, context):
        if self.check_unauth_user(update.message):
            return
        user_id = update.message.chat_id
        if not self.check_active(context.bot, user_id):
            return
        message = self.send_message(update, context)
        dialog_id = self.env.user_to_dialog.get(user_id, None)
        dialog = self.env.dialogs[dialog_id]
        dialog.last_msg_time = time()
        if dialog.user == user_id:
            dialog.last_user_msg_time = time()
        elif dialog.alice == user_id:
            dialog.last_alice_msg_time = time()
        dialog.log.append(message)
        dialog.debug_log.append(message)
        if dialog.state == DialogState.STARTED and len(dialog.log) >= self.config.dialog_enough_limit:
            dialog.state = DialogState.ENOUGH
            self.broadcast(dialog, context.bot, self.phrases.dialog_optimal_size)

    def stop(self, update, context):
        if self.check_unauth_user(update.message):
            return
        user_id = update.message.chat_id
        dialog_id = self.env.user_to_dialog.get(user_id, None)
        if dialog_id is None:
            if self.env.check_active(user_id):
                context.bot.sendMessage(user_id, text=self.phrases.no_dialog_stop)
                return
            else:
                context.bot.sendMessage(user_id, text=self.phrases.no_dialog)
                return
        dialog = self.env.dialogs[dialog_id]
        if dialog.alice == user_id:
            if time() - dialog.last_user_msg_time > self.config.next_phrase_timeout:
                logger.info(f"User {user_id} stopped dialog {dialog_id}")
                self.finalize_dialog(dialog, context.bot)
                return
            context.bot.sendMessage(
                user_id, text=self.phrases.wait_until.format(self.config.next_phrase_timeout // 60)
            )
        if dialog.user == user_id:
            if len(dialog.log) < self.config.dialog_lower_limit:
                context.bot.sendMessage(user_id, text=self.phrases.short_dialog)
                if time() - dialog.last_alice_msg_time < self.config.next_phrase_timeout:
                    context.bot.sendMessage(
                        user_id, text=self.phrases.wait_until.format(self.config.next_phrase_timeout // 60)
                    )
                    return
            logger.info(f"User {user_id} stopped dialog {dialog_id}")
            self.finalize_dialog(dialog, context.bot)

    def stop_search(self, update, context):
        if self.check_unauth_user(update.message):
            return
        user_id = update.message.chat_id
        dialog_id = self.env.user_to_dialog.get(user_id, None)
        if dialog_id:
            context.bot.sendMessage(user_id, text=self.phrases.no_dialog_stop_search)
            return
        with self.env.user_queue_lock():
            if self.env.waiting_user_queue is None:
                context.bot.sendMessage(user_id, text=self.phrases.dialog_have_founded)
                return
            self.env.waiting_user_queue = None
        self.env.remove_active(user_id)
        context.bot.sendMessage(user_id, text=self.phrases.quit_dialog_search)

    def send_private(self, update, context):
        if self.check_unauth_user(update.message):
            return
        user_id = update.message.chat_id
        if not self.check_active(context.bot, user_id):
            return
        dialog_id = self.env.user_to_dialog.get(user_id, None)
        if not dialog_id:
            context.bot.sendMessage(user_id, text=self.phrases.no_dialog)
            return
        message = self.send_message(
            update, context, prefix=self.phrases.private_msg, is_cmd=True
        )
        self.env.dialogs[dialog_id].debug_log.append(message)

    def get_dialog_id(self, update, context):
        if self.check_unauth_user(update.message):
            return
        user_id = update.message.chat_id
        if not self.check_active(context.bot, user_id):
            context.bot.sendMessage(user_id, text=self.phrases.no_dialog)
        else:
            context.bot.sendMessage(
                user_id, text=self.phrases.get_dialog_id.format(self.env.user_to_dialog[user_id])
            )

    def add_user(self, update, context):
        if not self.env.check_admin(update.message.from_user.username):
            return
        try:
            username = update.message.text.split(" ")[1]
        except:
            update.message.reply_text(self.phrases.wrong_command)
            return
        self.env.checked_user[username.lower()] = True
        context.bot.sendMessage(
            update.message.chat_id, text=self.phrases.admin_add_user.format(username)
        )

    def current_num_dialogs(self, update, context):
        if not self.env.check_admin(update.message.from_user.username):
            return
        num_dialogs = sum(d.state != DialogState.FINISHED for d in self.env.dialogs.values())
        context.bot.sendMessage(
            update.message.chat_id, text=self.phrases.admin_num_dialogs.format(num_dialogs)
        )

    def change_unique_links_table_path(self, update, context):
        if not self.env.check_admin(update.message.from_user.username):
            return
        try:
            new_path = update.message.text.split(" ")[1]
        except:
            update.message.reply_text(self.phrases.wrong_command)
            return
        self.env.set_unique_links(new_path)
        context.bot.sendMessage(
            update.message.chat_id, text=self.phrases.admin_unique_links.format(new_path)
        )

    def change_task_table_path(self, update, context):
        if not self.env.check_admin(update.message.from_user.username):
            return
        try:
            new_path = update.message.text.split(" ")[1]
        except:
            update.message.reply_text(self.phrases.wrong_command)
            return
        self.env.set_tasks(new_path)
        context.bot.sendMessage(
            update.message.chat_id, text=self.phrases.admin_task_table.format(new_path)
        )

    def stop_dialog(self, update, context):
        if not self.env.check_admin(update.message.from_user.username):
            return
        try:
            dialog_id = update.message.text.split(" ")[1]
        except:
            update.message.reply_text(self.phrases.wrong_command)
            return
        if dialog_id in self.env.dialogs:
            dialog = self.env.dialogs[dialog_id]
            self.broadcast(dialog, context.bot, user_text=self.phrases.admin_dialog_ended)
            self.finalize_dialog(dialog, context.bot, False, False)
            context.bot.sendMessage(update.message.chat_id, text=self.phrases.dialog_ended)
            logger.info(f"Dialog {dialog_id} has been finished by admin")
        else:
            context.bot.sendMessage(
                update.message.chat_id, text=self.phrases.admin_dialog_not_founded.format(dialog_id)
            )
