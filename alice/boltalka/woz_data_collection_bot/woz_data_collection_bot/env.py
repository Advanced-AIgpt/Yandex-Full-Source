import requests
import threading
import yt.wrapper as yt

from contextlib import contextmanager
from collections import defaultdict
from dataclasses import dataclass, field
from dataclass_wizard import YAMLWizard
from enum import Enum
from typing import List

from woz_data_collection_bot.config import Config


class DialogState(Enum):
    PREPARED = 0
    STARTED = 1
    ENOUGH = 2
    FINISHED = 3


@dataclass
class Dialog:
    alice: str = None
    user: str = None
    dialog_id: str = None
    user_story: str = None
    user_task: str = None
    last_user_msg_time: int = 0
    last_alice_msg_time: int = 0
    last_msg_time: int = 0
    start_msg_time: int = 0
    log: List[str] = field(default_factory=lambda: [])
    debug_log: List[str] = field(default_factory=lambda: [])
    state: DialogState = field(default_factory=lambda: DialogState.PREPARED)


class DialogManagerEnviroment:
    def __init__(self, config: Config):
        self.waiting_user_queue = None
        self._user_queue_lock = threading.Lock()
        self._active_user_lock = threading.Lock()
        self.user_to_dialog = {}
        self.active_user = set()
        self.dialogs = defaultdict(Dialog)
        self.checked_user = dict()
        self.unique_links = []
        self.tasks = []
        
        self.config = config

        self.set_tasks(config.task_table_yt_path)
        self.set_unique_links(config.unique_links_table_yt_path)

    def set_unique_links(self, table_path):
        self.unique_links = [row["uuid"][len(self.config.telegram_url_prefix):] for row in yt.read_table(table_path)]

    def set_tasks(self, table_path):
        self.tasks = [(row["user_story"], row["user_task"]) for row in yt.read_table(table_path)]

    def check_user(self, username):
        if username is None:
            return False
        if (username.lower() not in self.checked_user) or (not self.checked_user[username.lower()]):
            params = {
                '_one': 1,
                'telegram_accounts.value_lower': username.lower(),
                'official.is_dismissed': 'false',
                'official.is_robot': 'false',
                '_fields': 'created_at',
            }
            headers = {
                'Authorization': f'OAuth {self.config.staff_token}',
            }
            response = requests.get('https://staff-api.yandex-team.ru/v3/persons', headers=headers, params=params)
            result = response.json()
            self.checked_user[username.lower()] = 'created_at' in result
        return self.checked_user[username.lower()]

    def check_admin(self, username):
        return username in self.config.admins

    @contextmanager
    def active_user_lock(self):
        self._active_user_lock.acquire()
        yield
        self._active_user_lock.release()

    @contextmanager
    def user_queue_lock(self):
        self._user_queue_lock.acquire()
        yield
        self._user_queue_lock.release()

    def check_active(self, user_id):
        with self.active_user_lock():
            if user_id not in self.active_user:
                self.active_user.add(user_id)
                return False
            return True

    def remove_active(self, user_id):
        with self.active_user_lock():
            self.active_user.remove(user_id)

    def get_dialog(self, user_id):
        return self.dialogs[self.user_to_dialog[user_id]]


@dataclass
class Phrases(YAMLWizard):
    no_access: str = field(default_factory=lambda: "У вас нет доступа")
    write_code: str = field(default_factory=lambda: "Пожалуйста, впишите этот код на странице в Янге: {}-{}")
    dialog_ended: str = field(default_factory=lambda: "Разговор завершен, спасибо")
    user_start_text: str = field(default_factory=lambda: "{}{}\n\nВаша характеристика:\n{}\nВаша задача:\n{}")
    alice_start_text: str = field(
        default_factory=lambda: "{}{}\n\nХарактеристика пользователя, с которым вы общаетесь:\n{}\nЗадача пользователя:\n{}"
    )

    finished_by_timeout: str = field(default_factory=lambda: "Диалог был завершен по таймауту.")
    finished_by_inactivity: str = field(
        default_factory=lambda: "Одна из сторон долго не начинает диалог. Диалог был завершен по таймауту."
    )

    have_no_opponent: str = field(default_factory=lambda: "У вас пока нет собеседника.")
    dialog_not_started: str = field(default_factory=lambda: "Диалог еще не идет, подождите")
    waiting_opponent: str = field(default_factory=lambda: "Подождите немного, пока подсоединится ваш собеседник")
    wrong_command: str = field(default_factory=lambda: "Неверный формат команды")
    no_enter_until_opponent: str = field(default_factory=lambda: "Вы уже ждете диалог. Не используете новые ссылки")
    no_enter_have_active: str = field(default_factory=lambda: "У вас уже есть активный диалог")
    wrong_link: str = field(default_factory=lambda: "Ссылка не валидная.")
    dialog_optimal_size: str = field(
        default_factory=lambda: "Диалог достиг оптимального размера. Постепенно завершайте диалог"
    )
    no_dialog_stop: str = field(
        default_factory=lambda: "Вы не участвуете в разговоре. Пришлите stop_search чтобы уйти из задания."
    )
    wait_until: str = field(default_factory=lambda: "Чтобы остановить диалог подождите {} минут после последнего сообщения")
    short_dialog: str = field(default_factory=lambda: "Диалог все еще короткий, пожалуйста, продолжайте общаться")
    no_dialog_stop_search: str = field(
        default_factory=lambda: "Вы участвуете в разговоре. Пришлите stop, чтобы остановить написание диалога."
    )
    dialog_have_founded: str = field(
        default_factory=lambda: "Диалог для вас нашелся. Пришлите stop, чтобы остановить написание диалога."
    )
    quit_dialog_search: str = field(default_factory=lambda: "Вы вышли из поиска собедеседника.")
    no_dialog: str = field(default_factory=lambda: "Вы не участвуете в разговоре")
    private_msg: str = field(default_factory=lambda: "(Приватное сообщение) ")
    get_dialog_id: str = field(default_factory=lambda: "Ваш диалог: {}")

    admin_add_user: str = field(default_factory=lambda: "Пользователь {} добавлен")
    admin_num_dialogs: str = field(default_factory=lambda: "Количество диалогов, которые сейчас пишутся: {}")
    admin_unique_links: str = field(default_factory=lambda: "Таблица уникальных ссылок изменена на {}")
    admin_task_table: str = field(default_factory=lambda: "Таблица задач изменена на {}")
    admin_dialog_ended: str = field(default_factory=lambda: "Ваш диалоr остановлен админом")
    admin_dialog_not_founded: str = field(default_factory=lambda: "Диалог {} не найден")

    user_name: str = field(default_factory=lambda: "Пользователь")
    alice_name: str = field(default_factory=lambda: "Алиса")
