# -*-coding: utf8 -*-
#!/usr/bin/env python
from os.path import basename as path_basename
import time
import logging
import copy
import yt.wrapper as yt
from yt.wrapper.ypath import TablePath
from utils.nirvana.op_caller import call_as_operation
from utils.yt.reader import to_utf8


def read_basket(basket_path, assistant):
    """
    :param basket_path: path for accept-sample basket in accept-like ue2e format
    :param assistant: generates right table column name with query text
        (use empty for yandex, google for google column, marusya for marusya column)
    :return: list with sorted by session_id data
    """
    basket_with_order = list()
    yt.config["proxy"]["url"] = "hahn.yt.yandex.net"
    # не сортировать текущую табличку, делать tmp?
    yt.run_sort(basket_path, sort_by=["session_id", "session_sequence"])

    text_column = (assistant + "_" if assistant != "alice" else "") + "text"
    rows = yt.read_table(
        TablePath(
            basket_path,
            columns=["request_id", "session_id", "session_sequence", "toloka_intent"] + [text_column]
        ),
        format="json")

    prev_session = ""
    cur_data = dict()
    for row in rows:
        if not prev_session or prev_session != row["session_id"]:
            if cur_data:
                basket_with_order.append(cur_data)

            cur_data = {
                "session_id": row["session_id"],
                "request_id": row["request_id"],
            }
            prev_session = row["session_id"]

        cur_data["context_" + str(row["session_sequence"])] = to_utf8(row[text_column])
        if row["toloka_intent"]:
            cur_data["toloka_intent"] = row["toloka_intent"]

    return basket_with_order


def get_screenshot_key(filename):
    return path_basename(filename).split('-')[1]


def main(mds_screenshots, basket_path, assistant):
    """
    :param mds_screenshots: json after MDS upload operation
    :param basket_path: path to table with queries
    :param assistant: alice for yandex, google for google assistant, marusya for Mail.Ru marusya assistant
    :return: json with tasks for Toloka upload
    """
    logging.basicConfig(
        format="%(asctime)s %(levelname)-8s %(message)s",
        level=logging.INFO,
        datefmt="%Y-%m-%d %H:%M:%S"
    )

    screenshots = dict((get_screenshot_key(item["initialFileName"]), item["downloadUrl"]) for item in mds_screenshots)
    basket_with_order = read_basket(basket_path, assistant)
    tasks = []
    for ind, item in enumerate(basket_with_order):
        item_copy = item.copy()
        item_copy['assistant'] = assistant
        if str(ind) in screenshots:
            tasks.append({
                "url": screenshots[str(ind)],
                "action": "Ответ показан на экране",
                "info": item_copy
            })
        else:
            logging.warning("No screenshot for input tasks. Session id: %s" % item["session_id"])
    return tasks


if __name__ == '__main__':
    call_as_operation(main,
                      input_spec={'mds_screenshots': {'link_name': 'mds', 'required': True, 'parser': 'json'}}
                      )
