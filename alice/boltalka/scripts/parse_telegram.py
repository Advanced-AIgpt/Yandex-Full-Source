#!/usr/bin/python
import sys
from datetime import datetime
import argparse
from collections import deque, defaultdict
from copy import copy
import codecs
sys.stdin = codecs.getreader('utf-8')(sys.stdin)
sys.stdout = codecs.getwriter('utf-8')(sys.stdout)

TIME_FORMAT = '%Y-%m-%d %H:%M:%S'

class Reply(object):
    def __init__(self, line):
        chat, user, time, text = line.rstrip().split('\t')
        time = datetime.strptime(time.strip('"'), TIME_FORMAT)
        self.chat = chat
        self.user = user
        self.time = time
        self.text = text

    def output(self, args, line_sep='\n'):
        line = []
        if args.print_chat: line.append(self.chat)
        if args.print_user: line.append(self.user)
        if args.print_time: line.append(datetime.strftime(self.time, TIME_FORMAT))
        line.append(self.text)
        sys.stdout.write(args.column_sep.join(line) + line_sep)

    def __copy__(self):
        cls = self.__class__
        result = cls.__new__(cls)
        result.__dict__.update(self.__dict__)
        return result

def parse_dialogs(args):
    prev_reply = None
    for line in sys.stdin:
        reply = Reply(line)

        if prev_reply and (prev_reply.chat != reply.chat or \
                (reply.time - prev_reply.time).seconds > args.dialog_threshold * 60):
            if prev_reply:
                prev_reply.output(args)
            if prev_reply:
                print
            prev_reply = None

        if prev_reply and prev_reply.user == reply.user:
            prev_reply.text += ' ' + reply.text
            prev_reply.time = reply.time
        else:
            if prev_reply:
                prev_reply.output(args)
            prev_reply = reply

    if prev_reply:
        prev_reply.output(args)

def output_edges(args, mem, replies, edges, key_users):
    """
    print 'v'*30
    """
    for key_user in key_users:
        for user, key_user_reply in edges[key_user]:
            mem_key_l = (key_user_reply.time, key_user_reply.user, key_user_reply.text)
            mem_key_r = (replies[user].time, replies[user].user, replies[user].text)
            mem_key = (mem_key_l, mem_key_r)
            if key_user_reply.time > replies[user].time:
                mem_key = mem_key[::-1]
            if mem_key in mem:
                continue
            mem.add(mem_key)

            if key_user_reply.time <= replies[user].time:
                key_user_reply.output(args, line_sep='\t')
                replies[user].output(args)
            else:
                replies[user].output(args, line_sep='\t')
                key_user_reply.output(args)
    """
    print '^'*30
    """

def generate_pairs(args):
    mem = set()
    replies = {} # user -> Reply
    queue = deque()
    edges = defaultdict(list) # user -> [user_0, ...]
    inv_edges = defaultdict(list) # user -> [user_0, ...]
    for line in sys.stdin:
        """
        print '='*256
        """
        reply = Reply(line)

        """
        print line.rstrip()
        print queue
        print replies
        print edges
        """

        if queue and reply.chat != replies[queue[-1]].chat:
            output_edges(args, mem, replies, edges, queue)
            output_edges(args, mem, replies, inv_edges, queue)
            mem.clear()
            queue.clear()
            replies.clear()
            edges.clear()
            inv_edges.clear()

        same_message = False
        for prev_user in reversed(queue):
            prev_reply = replies[prev_user]

            if prev_reply.user == reply.user:
                same_message = True
                prev_reply.text += ' ' + reply.text
                prev_reply.time = reply.time
                break

            response_time = (reply.time - prev_reply.time).seconds
            if response_time > args.dialog_threshold * 60 or response_time < len(reply.text) / 200.0 * 60.0 + args.min_response_time:
                continue

            edges[prev_reply.user].append((reply.user, copy(prev_reply)))
            inv_edges[reply.user].append((prev_reply.user, copy(reply)))
            break

        if not same_message:
            output_edges(args, mem, replies, edges, [reply.user])
            output_edges(args, mem, replies, inv_edges, [reply.user])
            edges[reply.user] = []
            inv_edges[reply.user] = []
            replies[reply.user] = reply

        if reply.user in queue:
            queue.remove(reply.user)

        queue.append(reply.user)

def main(args):
    if args.generate_pairs:
        generate_pairs(args)
    else:
        parse_dialogs(args)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--print-chat', action='store_true')
    parser.add_argument('--print-user', action='store_true')
    parser.add_argument('--print-time', action='store_true')
    parser.add_argument('--generate-pairs', action='store_true', help='Generate two column dataset "phrase \\t response"')
    parser.add_argument('--column-sep', default='\t', help='Delimeter for reply columns: chat, user, time and text.')
    parser.add_argument('--dialog-threshold', type=int, default=20, help='After this time in minutes a new dialog starts.')
    parser.add_argument('--min-response-time', type=int, default=5, help='Min time in seconds to respond.')
    args = parser.parse_args()
    main(args)
