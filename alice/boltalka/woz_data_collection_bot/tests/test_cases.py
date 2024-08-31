import logging
import parse
import pytest
import time

from .common import TestUpdate, init_bot, mocker_wrapper, mock_check_user, mock_set_tasks, mock_set_unique_links, mock_yt_write_table

logger = logging.getLogger(__name__)


@mocker_wrapper
def test_start():
    with init_bot() as resources:
        context, context_logger, bot_commands, config, phrases = resources
        user_0 = lambda msg: TestUpdate(chat_id="0", text=msg, context=context)
        user_1 = lambda msg: TestUpdate(chat_id="1", text=msg, context=context)

        bot_commands.send(update=user_0("test_text"), context=context)
        assert context_logger[-1] == ("0", phrases.have_no_opponent)

        bot_commands.start(update=user_0("/start 0"), context=context)
        assert context_logger[-1] == ("0", phrases.wrong_link)

        bot_commands.start(update=user_0("/start 1"), context=context)
        assert context_logger[-1] == ("0", phrases.waiting_opponent)

        bot_commands.start(update=user_0("/start 1"), context=context)
        assert context_logger[-1] == ("0", phrases.no_enter_until_opponent)

        bot_commands.send(update=user_0("test_text"), context=context)
        assert context_logger[-1] == ("0", phrases.have_no_opponent)

        bot_commands.start(update=user_1("/start 1"), context=context)
        bot_commands.start(update=user_0("/start 1"), context=context)
        first_msg = phrases.user_start_text.format(config.start_text, config.user_base_text, "story", "task")
        second_msg = phrases.alice_start_text.format(config.start_text, config.alice_base_text, "story", "task")

        assert context_logger[-3:] == [("0", first_msg), ("1", second_msg), ("0", phrases.no_enter_have_active)]


@mocker_wrapper
def test_early_stop():
    with init_bot() as resources:
        context, context_logger, bot_commands, config, phrases = resources
        user_0 = lambda msg: TestUpdate(chat_id="0", text=msg, context=context)
        user_1 = lambda msg: TestUpdate(chat_id="1", text=msg, context=context)

        bot_commands.start(update=user_0("/start 1"), context=context)
        assert context_logger[-1] == ("0", phrases.waiting_opponent)

        bot_commands.stop(update=user_0("/stop"), context=context)
        assert context_logger[-1] == ("0", phrases.no_dialog_stop)

        bot_commands.stop_search(update=user_0("/stop_search"), context=context)
        assert context_logger[-1] == ("0", phrases.quit_dialog_search)

        bot_commands.start(update=user_0("/start 1"), context=context)
        bot_commands.start(update=user_1("/start 1"), context=context)
        bot_commands.stop_search(update=user_0("/stop_search"), context=context)
        assert context_logger[-1] == ("0", phrases.no_dialog_stop_search)


@mocker_wrapper
def test_failed_stop():
    with init_bot() as resources:
        context, context_logger, bot_commands, config, phrases = resources
        user_0 = lambda msg: TestUpdate(chat_id="0", text=msg, context=context)
        user_1 = lambda msg: TestUpdate(chat_id="1", text=msg, context=context)

        bot_commands.start(update=user_0("/start 1"), context=context)
        bot_commands.start(update=user_1("/start 1"), context=context)

        for i in range(5):
            bot_commands.send(update=user_0("test_text"), context=context)
            bot_commands.send(update=user_1("test_text"), context=context)

        for i in range(5):
            assert context_logger[2 * i + 3] == ("1", f"{phrases.user_name}: test_text")
            assert context_logger[2 * i + 4] == ("0", f"{phrases.alice_name}: test_text")

        bot_commands.stop(update=user_0("/stop"), context=context)
        bot_commands.stop(update=user_1("/stop"), context=context)
        timeout_msg = phrases.wait_until.format(config.next_phrase_timeout // 60)
        assert context_logger[-3] == ("0", phrases.short_dialog)
        assert context_logger[-2] == ("0", timeout_msg)
        assert context_logger[-1] == ("1", timeout_msg)


@mocker_wrapper
def test_success():
    with init_bot() as resources:
        context, context_logger, bot_commands, config, phrases = resources
        user_0 = lambda msg: TestUpdate(chat_id="0", text=msg, context=context)
        user_1 = lambda msg: TestUpdate(chat_id="1", text=msg, context=context)

        bot_commands.start(update=user_0("/start 1"), context=context)
        bot_commands.send_private(update=user_0("/private test_text"), context=context)
        assert context_logger[-1] == ("0", phrases.have_no_opponent)

        bot_commands.start(update=user_1("/start 1"), context=context)

        for i in range(15):
            bot_commands.send(update=user_0("test_text"), context=context)
            bot_commands.send(update=user_1("test_text"), context=context)

        for i in range(15):
            assert context_logger[2 * i + 4] == ("1", f"{phrases.user_name}: test_text")
            assert context_logger[2 * i + 5] == ("0", f"{phrases.alice_name}: test_text")

        assert context_logger[-2] == ("0", phrases.dialog_optimal_size)
        assert context_logger[-1] == ("1", phrases.dialog_optimal_size)

        bot_commands.send_private(update=user_0("/private test_text"), context=context)
        assert context_logger[-1] == ("1", f"{phrases.user_name}: {phrases.private_msg}test_text")

        bot_commands.stop(update=user_1("/stop"), context=context)
        timeout_msg = phrases.wait_until.format(config.next_phrase_timeout // 60)
        assert context_logger[-1] == ("1", timeout_msg)

        bot_commands.stop(update=user_0("/stop"), context=context)
        assert context_logger[-4][1].startswith(phrases.write_code[:-5])
        assert context_logger[-3][1].startswith(phrases.write_code[:-5])
        assert context_logger[-2:] == [("0", phrases.dialog_ended), ("1", phrases.dialog_ended)]

        bot_commands.stop(update=user_1("/stop"), context=context)
        assert context_logger[-1] == ("1", phrases.no_dialog)


@mocker_wrapper
def test_full_timeout():
    with init_bot() as resources:
        context, context_logger, bot_commands, config, phrases = resources
        user_0 = lambda msg: TestUpdate(chat_id="0", text=msg, context=context)
        user_1 = lambda msg: TestUpdate(chat_id="1", text=msg, context=context)

        bot_commands.start(update=user_0("/start 1"), context=context)
        bot_commands.start(update=user_1("/start 1"), context=context)

        for i in range(10):
            bot_commands.send(update=user_0("test_text"), context=context)
            bot_commands.send(update=user_1("test_text"), context=context)

        time.sleep(15)

        true_log = [
            ("0", phrases.finished_by_timeout),
            ("1", phrases.finished_by_timeout),
            ("0", phrases.dialog_ended),
            ("1", phrases.dialog_ended),
        ]
        assert context_logger[-4:] == true_log


@mocker_wrapper
def test_oneside_inactivity_timeout():
    with init_bot() as resources:
        context, context_logger, bot_commands, config, phrases = resources
        user_0 = lambda msg: TestUpdate(chat_id="0", text=msg, context=context)
        user_1 = lambda msg: TestUpdate(chat_id="1", text=msg, context=context)

        bot_commands.start(update=user_0("/start 1"), context=context)
        bot_commands.start(update=user_1("/start 1"), context=context)

        for i in range(10):
            bot_commands.send(update=user_0("/test_text"), context=context)

        time.sleep(8)

        true_log = [
            ("0", phrases.finished_by_inactivity),
            ("1", phrases.finished_by_inactivity),
            ("0", phrases.dialog_ended),
            ("1", phrases.dialog_ended)
        ]
        assert context_logger[-4:] == true_log


@mocker_wrapper
def test_oneside_sleep_timeout():
    with init_bot() as resources:
        context, context_logger, bot_commands, config, phrases = resources
        user_0 = lambda msg: TestUpdate(chat_id="0", text=msg, context=context)
        user_1 = lambda msg: TestUpdate(chat_id="1", text=msg, context=context)

        bot_commands.start(update=user_0("/start 1"), context=context)
        bot_commands.start(update=user_1("/start 1"), context=context)

        for i in range(5):
            bot_commands.send(update=user_0("test_text"), context=context)
            bot_commands.send(update=user_1("test_text"), context=context)

        time.sleep(6)
        bot_commands.stop(update=user_1("/stop"), context=context)

        assert context_logger[-4][1].startswith(phrases.write_code[:-5])
        assert context_logger[-3][1].startswith(phrases.write_code[:-5])
        assert context_logger[-2:] == [("0", phrases.dialog_ended), ("1", phrases.dialog_ended)]


@mocker_wrapper
def test_get_dialog_id():
    with init_bot() as resources:
        context, context_logger, bot_commands, config, phrases = resources
        user_0 = lambda msg: TestUpdate(chat_id="0", text=msg, context=context)
        user_1 = lambda msg: TestUpdate(chat_id="1", text=msg, context=context)

        bot_commands.start(update=user_0("/start 1"), context=context)
        bot_commands.start(update=user_1("/start 1"), context=context)

        for i in range(5):
            bot_commands.send(update=user_0("test_text"), context=context)
            bot_commands.send(update=user_1("test_text"), context=context)

        bot_commands.get_dialog_id(update=user_0("/dialog_id"), context=context)
        bot_commands.get_dialog_id(update=user_1("/dialog_id"), context=context)
        assert context_logger[-1][1] == context_logger[-2][1]


@mocker_wrapper
def test_admin_cmd():
    with init_bot() as resources:
        context, context_logger, bot_commands, config, phrases = resources
        user_0 = lambda msg: TestUpdate(chat_id="0", text=msg, context=context)
        user_1 = lambda msg: TestUpdate(chat_id="1", text=msg, context=context)
        user_10 = lambda msg: TestUpdate(chat_id="10", text=msg, context=context)

        bot_commands.start(update=user_0("/start 1"), context=context)
        bot_commands.start(update=user_1("/start 1"), context=context)

        for i in range(5):
            bot_commands.send(update=user_0("test_text"), context=context)
            bot_commands.send(update=user_1("test_text"), context=context)

        bot_commands.current_num_dialogs(update=user_0("/current_num_dialogs"), context=context)
        bot_commands.current_num_dialogs(update=user_10("/current_num_dialogs"), context=context)

        bot_commands.get_dialog_id(update=user_0("/dialog_id"), context=context)

        parsed_msg = parse.parse(phrases.get_dialog_id, context_logger[-1][1])
        dialog_id = parsed_msg[0]
        bot_commands.stop_dialog(update=user_0(f"/stop_dialog {dialog_id}"), context=context)
        bot_commands.stop_dialog(update=user_10("/stop_dialog"), context=context)
        bot_commands.stop_dialog(update=user_10("/stop_dialog 0"), context=context)
        bot_commands.stop_dialog(update=user_10(f"/stop_dialog {dialog_id}"), context=context)
        dialog_not_found = phrases.admin_dialog_not_founded.format(0)
        assert context_logger[-7] == ("10", phrases.wrong_command)
        assert context_logger[-6] == ("10", dialog_not_found)
        assert context_logger[-5] == ("0", phrases.admin_dialog_ended)
        assert context_logger[-4] == ("1", phrases.admin_dialog_ended)
        assert context_logger[-1] == ("10", phrases.dialog_ended)

        for log_msg in context_logger:
            if log_msg[0] in ("0", "1"):
                assert not log_msg[1].endswith(phrases.admin_num_dialogs.format(1))


@mocker_wrapper
def test_admin_add_user():
    with init_bot() as resources:
        context, context_logger, bot_commands, config, phrases = resources
        user_0 = lambda msg: TestUpdate(chat_id="0", text=msg, context=context)
        user_2 = lambda msg: TestUpdate(chat_id="2", text=msg, context=context)
        user_10 = lambda msg: TestUpdate(chat_id="10", text=msg, context=context)

        bot_commands.start(update=user_0("/start 1"), context=context)
        bot_commands.start(update=user_2("/start 1"), context=context)
        assert context_logger[-1] == ("2", phrases.no_access)

        bot_commands.add_user(update=user_10("/add_user"), context=context)
        assert context_logger[-1] == ("10", phrases.wrong_command)

        bot_commands.add_user(update=user_10("/add_user 2"), context=context)
        add_user_msg = phrases.admin_add_user.format(2)
        assert context_logger[-1] == ("10", add_user_msg)

        bot_commands.start(update=user_2("/start 1"), context=context)
        first_msg = phrases.user_start_text.format(config.start_text, config.user_base_text, "story", "task")
        second_msg = phrases.alice_start_text.format(config.start_text, config.alice_base_text, "story", "task")
        assert context_logger[-2:] == [("0", first_msg), ("2", second_msg)]



@pytest.mark.parametrize(
    "user_0_cmd,user_1_cmd,order",
    [
        ("/start 1", "/start 1@user", "au"),
        ("/start 1@user", "/start 1", "ua"),
        ("/start 1@user", "/start 1@user", "ua"),
        ("/start 1", "/start 1@alice", "ua"),
        ("/start 1@alice", "/start 1", "au"),
        ("/start 1@alice", "/start 1@alice", "ua"),
        ("/start 1@user", "/start 1@alice", "ua"),
        ("/start 1@alice", "/start 1@user", "au"),
    ]
)
def test_links(mocker, user_0_cmd, user_1_cmd, order):
    mocker.patch('woz_data_collection_bot.env.DialogManagerEnviroment.check_user', mock_check_user)
    mocker.patch('woz_data_collection_bot.env.DialogManagerEnviroment.set_tasks', mock_set_tasks)
    mocker.patch('woz_data_collection_bot.env.DialogManagerEnviroment.set_unique_links', mock_set_unique_links)
    mocker.patch('yt.wrapper.write_table', mock_yt_write_table)
    with init_bot() as resources:
        context, context_logger, bot_commands, config, phrases = resources

        alice_msg = phrases.user_start_text.format(config.start_text, config.user_base_text, "story", "task")
        user_msg = phrases.alice_start_text.format(config.start_text, config.alice_base_text, "story", "task")

        user_0 = lambda msg: TestUpdate(chat_id="0", text=msg, context=context)
        user_1 = lambda msg: TestUpdate(chat_id="1", text=msg, context=context)
    
        bot_commands.start(update=user_0(user_0_cmd), context=context)
        bot_commands.start(update=user_1(user_1_cmd), context=context)

        cannon = [("1", alice_msg), ("0", user_msg)] if order == "au" else [("0", alice_msg), ("1", user_msg)]

        assert context_logger[-2:] == cannon
