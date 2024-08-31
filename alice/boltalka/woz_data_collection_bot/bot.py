import argparse

from telegram.ext import CommandHandler, Filters, MessageHandler, Updater

import yt.wrapper as yt

from woz_data_collection_bot.config import Config
from woz_data_collection_bot.commands import BotCommands
from woz_data_collection_bot.env import DialogManagerEnviroment, Phrases


def init(args):
    config = Config.from_yaml_file(args.config)
    phrases = Phrases()
    if args.phrases:
        phrases = Phrases.from_yaml_file(args.phrases)

    yt.config["proxy"]["url"] = config.yt_proxy

    env = DialogManagerEnviroment(config)
    bot_commands = BotCommands(env, config, phrases)

    updater = Updater(config.tg_token, workers=8)
    dp = updater.dispatcher
    dp.add_handler(CommandHandler("start", bot_commands.start))
    dp.add_handler(CommandHandler("stop", bot_commands.stop))
    dp.add_handler(CommandHandler("stop_search", bot_commands.stop_search))
    dp.add_handler(CommandHandler("private", bot_commands.send_private))
    dp.add_handler(CommandHandler("dialog_id", bot_commands.get_dialog_id))
    dp.add_handler(CommandHandler("add_user", bot_commands.add_user))
    dp.add_handler(CommandHandler("current_num_dialogs", bot_commands.current_num_dialogs))
    dp.add_handler(CommandHandler("change_unique_links_table_path", bot_commands.change_unique_links_table_path))
    dp.add_handler(CommandHandler("change_task_table_path", bot_commands.change_task_table_path))
    dp.add_handler(CommandHandler("stop_dialog", bot_commands.stop_dialog))
    dp.add_handler(MessageHandler(Filters.text, bot_commands.send))
    return updater, env, bot_commands


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--config", type=str, default="config.yaml")
    parser.add_argument("--phrases", type=str, default=None)
    return parser.parse_args()


def main():
    args = parse_args()
    updater, env, bot_commands = init(args)
    updater.start_polling(timeout=4)

    exit_event, threads = bot_commands.run_parallel_jobs(updater.bot)

    updater.idle()
    exit_event.set()
    for thread in threads:
        thread.join()


if __name__ == "__main__":
    main()
