from contextlib import contextmanager

from woz_data_collection_bot.config import Config
from woz_data_collection_bot.commands import BotCommands
from woz_data_collection_bot.env import DialogManagerEnviroment, Phrases


class TestBot:
    def __init__(self, logger: list = []):
        self.logger = logger

    def sendMessage(self, user_id: str, text: str):
        self.logger.append((user_id, text))


class TestContext:
    def __init__(self, logger):
        self.bot = TestBot(logger)


class TestFromUser:
    def __init__(self, username: str):
        self.username = username


class TestMessage:
    def __init__(self, chat_id: int, text: str, from_user: str, context: TestContext):
        self.chat_id = chat_id
        self.text = text
        self.from_user = TestFromUser(from_user)

        self.context = context

    def reply_text(self, text: str):
        self.context.bot.sendMessage(str(self.chat_id), text)


class TestUpdate:
    def __init__(self, chat_id: int,  context: TestContext, text: str = ""):
        self.message = TestMessage(chat_id, text, str(chat_id), context)


def mock_check_user(self, username: str):
    return self.checked_user.get(username, False)


def mock_set_unique_links(self, table_path):
    self.unique_links = ["1"]


def mock_set_tasks(self, table_path):
    self.tasks = [("story", "task")]


def mock_yt_write_table(table_path, object):
    pass


def mocker_wrapper(function):
    """
    Mocking several methods and functions:
    - **check_user** -- goes to Staff API. Change to check with provided list.
    - **set_unique_links** -- goes to YT. Change to return a constant list.
    - **set_tasks** -- goes to YT. Change to return a constant list.
    - **yt_write_table** -- goes to YT. Change to do nothing.
    """
    def wrapped_function(mocker):
        mocker.patch('woz_data_collection_bot.env.DialogManagerEnviroment.check_user', mock_check_user)
        mocker.patch('woz_data_collection_bot.env.DialogManagerEnviroment.set_tasks', mock_set_tasks)
        mocker.patch('woz_data_collection_bot.env.DialogManagerEnviroment.set_unique_links', mock_set_unique_links)
        mocker.patch('yt.wrapper.write_table', mock_yt_write_table)
        function()
    return wrapped_function


def get_test_config():
    return Config(
        timeout=10,
        starting_dialog_timeout=7,
        next_phrase_timeout=5,
        start_text="start_text\n",
        user_base_text="user_base_text\n",
        alice_base_text="alice_base_text\n",
        admins=["10"],
    )


@contextmanager
def init_bot():
    config = get_test_config()
    env = DialogManagerEnviroment(config)
    env.checked_user = {"0": True, "1": True}
    phrases = Phrases()
    bot_commands = BotCommands(env, config, phrases)

    context_logger = list()
    context = TestContext(logger=context_logger)

    exit_event, threads = bot_commands.run_parallel_jobs(context.bot)
    try:
        yield context, context_logger, bot_commands, config, phrases
    finally:
        exit_event.set()
        for thread in threads:
            thread.join()
