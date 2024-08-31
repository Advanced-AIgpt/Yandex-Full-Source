from dataclasses import dataclass, field
from dataclass_wizard import YAMLWizard

from typing import List


@dataclass
class Config(YAMLWizard):
    tg_token: str = field(default_factory=lambda: "")
    result_table_yt_path: str = field(default_factory=lambda: "//tmp/alice_table")
    task_table_yt_path: str = field(default_factory=lambda: "")
    unique_links_table_yt_path: str = field(default_factory=lambda: "")
    staff_token: str = field(default_factory=lambda: "")
    yt_proxy: str = field(default_factory=lambda: "hahn")
    telegram_url_prefix: str = field(default_factory=lambda: "https://t.me/<bot_name>?start=")
    admins: List[str] = field(default_factory=lambda: [])

    timeout: int = 1800
    starting_dialog_timeout: int = 900
    next_phrase_timeout: int = 600
    dialog_lower_limit: int = 20
    dialog_enough_limit: int = 30

    start_text: str = field(default_factory=lambda: "---Диалог начинается---\n\n")
    user_base_text: str = field(default_factory=lambda: """Вы играете роль Пользователя. Поговорите с Алисой, учитывая описание вашего персонажа и цель разговора.

Поговорите в течение 20-40 реплик и завершите разговор командой /stop, когда он подойдет к логическому завершению""")
    alice_base_text: str = field(default_factory=lambda: """Вы играете роль Алисы. Поговорите с пользователем, отвечая на его вопросы и просьбы и поддерживая интересный разговор. Ваш собеседник завершит диалог в нужный момент""")
