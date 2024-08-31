# coding=utf-8
from __future__ import print_function, unicode_literals, division
import sys
import argparse
import json
import requests
import ast
import numpy as np
from telegram.ext import Updater, CommandHandler, MessageHandler, Filters
import telegram
from collections import Counter
import requests
import yaml
from alice.boltalka.telegram_bot.lib.instance import BoltalkaInstance

import logging
logging.basicConfig(format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
                     level=logging.INFO)


class BoltalkaHandler:
    def __init__(self, config):
        self.config = config

    def get_instance(self, chat_data):
        if not 'instance' in chat_data:
            chat_data['instance'] = BoltalkaInstance(self.config)
        instance = chat_data['instance']
        return instance

    def __call__(self, bot, update, chat_data):
        instance = self.get_instance(chat_data)
        text = update.message.text
        if instance.wait_variant and ":" in text:
            text = text.rsplit(":", 1)[-1]
        instance.context.push(text)
        if instance.wait_variant:
            instance.wait_variant.delete()
            update.message.reply_text(text, reply_markup=telegram.ReplyKeyboardRemove())
            update.message.delete()
            instance.wait_variant = None
        else:
            if instance.use_variants:
                self.reply_variants(bot, update, chat_data)
            else:
                self.reply(bot, update, chat_data)

    def candidates(self, bot, update, args, chat_data):
        instance = self.get_instance(chat_data)
        TOP_SIZE = 10

        def sorted_candidates(ranker_pos):
            arr = zip(map(lambda x: x['text'], instance.candidates), map(lambda x: x['score_explain'][ranker_pos],
                                                                         instance.candidates))
            return sorted(arr, key=lambda x: x[1], reverse=True)

        def print_overall(candidates):
            return '\n'.join([
                el['text'] + ' ' + ', '.join(map(str, el['score_explain'])) for el in candidates
            ])

        def print_special(candidates):
            return '\n'.join([
                el[0] + ' ' + str(el[1]) for el in candidates
            ])

        try:
            if len(args) == 0:
                update.message.reply_text('{}\n-----\n{}'.format(print_overall(instance.candidates[:TOP_SIZE]),
                                                                 print_overall(instance.candidates[-TOP_SIZE:])))

            elif len(args) == 1:
                ranker_pos = instance.replier.rankers.index(str(args[0]))
                sorted_candidates = sorted_candidates(ranker_pos)
                update.message.reply_text('{}\n-----\n{}'.format(print_special(sorted_candidates[:TOP_SIZE]),
                                                                 print_special(sorted_candidates[-TOP_SIZE:])))

            elif len(args) == 2:
                ranker_pos = instance.replier.rankers.index(str(args[0]))
                TOP_SIZE = int(args[1])
                if TOP_SIZE < 1:
                    raise ValueError
                sorted_candidates = sorted_candidates(ranker_pos)
                update.message.reply_text('{}\n-----\n{}'.format(print_special(sorted_candidates[:TOP_SIZE]),
                                                                 print_special(sorted_candidates[-TOP_SIZE:])))
            else:
                update.message.reply_text("Invalid number of arguments")
                return

        except (KeyError, ValueError):
            update.message.reply_text("Invalid arguments")
            return

    def set_args(self, bot, update, args, chat_data):
        instance = self.get_instance(chat_data)
        if len(args) != 3:
            update.message.reply_text("Invalid number of arguments")
            return
        try:
            instance.modules[args[0]].set_option(instance.module_options[args[0]], args[1], ast.literal_eval(args[2]))
            update.message.reply_text("Option set")
        except (KeyError, ValueError):
            update.message.reply_text("Invalid arguments")

    def get_args(self, bot, update, chat_data):
        instance = self.get_instance(chat_data)
        for el in instance.get_options_recursive():
            update.message.reply_text(el)

    def reset(self, bot, update, chat_data):
        instance = self.get_instance(chat_data)
        instance.context = Context()
        instance.wait_variant = False
        instance.candidates = []
        update.message.reply_text('-----------------------')

    @staticmethod

    def reply(self, bot, update, chat_data):
        instance = self.get_instance(chat_data)
        instance.get_candidates()
        reply = np.random.choice(instance.candidates[:10])
        reply = reply['text']
        instance.context.push(reply)
        update.message.reply_text(reply)

    def reply_variants(self, bot, update, chat_data):
        instance = self.get_instance(chat_data)
        instance.get_candidates()
        sources = list(set([el['source'] for el in instance.candidates]))
        sources.sort()
        source_candidates = []
        for source in sources:
            count = 0
            for el in instance.candidates:
                if el['source'] == source:
                    source_candidates.append(f"{source}:{el['text']}")
                    count += 1
                if count >= instance.variants_per_source:
                    break
        reply_markup = telegram.ReplyKeyboardMarkup([[el] for el in source_candidates], one_time_keyboard=False, resize_keyboard=True)
        instance.wait_variant = bot.send_message(update.message.chat_id, text="Выберите один из вариантов ответа Алисы или напишите свой:\n" + "\n".join(source_candidates),
                                                 reply_markup=reply_markup)

    def reply_context(self, bot, update, chat_data):
        instance = self.get_instance(chat_data)
        instance.context._context = update.message.text[len("/reply "):].split(";")
        self.reply(bot, update, chat_data)
    
    def choose_movie(self, bot, update, chat_data):
        instance = self.get_instance(chat_data)
        name = update.message.text[len("/movie "):]
        movie_id, movie_name = instance.select_movie(name)
        if movie_id:
            update.message.reply_text(f'Selected movie: {movie_name}, kinopoisk id: {movie_id}')
        else:
            update.message.reply_text('Movie not found :(')

if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--telegram-token', required=True)
    parser.add_argument('--config', default='config.yaml')
    args = parser.parse_args()
    updater = Updater(token=args.telegram_token, use_context=False)
    config = yaml.load(open(args.config))
    dispatcher = updater.dispatcher
    handler = BoltalkaHandler(config)
    dispatcher.add_handler(CommandHandler("candidates", handler.candidates, pass_args=True, pass_chat_data=True))
    dispatcher.add_handler(CommandHandler("set", handler.set_args, pass_args=True, pass_chat_data=True))
    dispatcher.add_handler(CommandHandler("get", handler.get_args, pass_chat_data=True))
    dispatcher.add_handler(CommandHandler("reset", handler.reset, pass_chat_data=True))
    dispatcher.add_handler(CommandHandler("auto", handler.reply, pass_chat_data=True))
    dispatcher.add_handler(CommandHandler("reply", handler.reply_context, pass_chat_data=True))
    dispatcher.add_handler(CommandHandler("movie", handler.choose_movie, pass_chat_data=True))
    dispatcher.add_handler(MessageHandler(Filters.text, handler, pass_chat_data=True))
    updater.start_polling()
    updater.idle()
