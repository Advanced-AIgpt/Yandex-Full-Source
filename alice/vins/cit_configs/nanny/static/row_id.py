#!/usr/bin/env python

import sys


def main():
    while True:
        line = sys.stdin.readline()
        if line == '':
            break
        secret, row_id, offset, message = line.split(";", 3)
        line = "{};{};{};{}".format(
            secret, row_id, offset,
            message.replace("{", '{' + "\"pushclient_row_id\":{},".format(row_id), 1)
        ).strip()
        print(line)


if __name__ == '__main__':
    main()
