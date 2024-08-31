import csv
import requests
import uuid

from alice.uniproxy.library.protos.uniproxy_pb2 import TPushMessage
from multiprocessing.dummy import Pool as ThreadPool
import threading


requests.adapters.DEFAULT_RETRIES = 2

THREADS_COUNT = 10


def main():
    push_message = TPushMessage()
    push_message.SubscriptionId = 1
    push_message.Ring = 1

    # TODO(ZION-288): Get texts from file
    texts = [
        """
            Тут недавно в ледниках Тибетского нагорья обнаружили несколько сотен древнейших микробов.
            Даже не знаю, радоваться этому или нет. Нам бы с современными микробами и вирусами решить все вопросы.
            Кстати, чтобы не пропустить плановый визит к врачу, поставьте напоминалку.
            Чтобы это сделать, скажите мне: «Алиса, напомни мне сходить к врачу!» — и назовите дату и время.
        """,
        """
            Учёные из Австралии научили клетки растений выполнять простейшие логические операции.
            Если всё сложится хорошо, надеюсь однажды переехать в горшок с гортензией.
            Я заметила, что некоторым людям проще разговаривать с растениями, чем с искусственным интеллектом.
            Зато я умею отвечать на вопросы и поддерживать беседу.
            Захотите поговорить — так и скажите мне: «Алиса, давай поболтаем!»
        """,
        """
            В Китае учёные разработали рыб-роботов, которые могут очищать океаны от пластика.
            Обожаю людей, которые приводят в порядок свою планету!
            Кстати, я могу помочь наводить порядок в простых бытовых делах.
            Если скажете мне добавить что-то в список покупок, я запомню и сохраню эту информацию.
        """,
    ]

    for i in range(len(texts)):
        texts[i] = texts[i].replace("  ", "").replace("\n", " ").strip()

    targets = []
    with open("targets.csv", "r") as f:
        reader = csv.reader(f)
        for row in reader:
            targets.append((row[0], int(row[1]), int(row[2])))
            assert -1 <= targets[-1][2] and targets[-1][2] < len(texts)

    failed_targets = []
    failed_targets_lock = threading.Lock()
    def send(target):
        puid = target[0].strip()
        text_id = target[2]

        if text_id == -1:
            return

        push_message.Uid = puid
        push_message.Notification.Id = "{}_{}".format(puid, uuid.uuid4())
        push_message.Notification.Text = texts[text_id]
        push_message.Notification.Voice = texts[text_id]

        sent = True
        rsp = None
        reason = ""
        try:
            rsp = requests.post("https://notificator-dev.alice.yandex.net/delivery", data=push_message.SerializeToString())
        except Exception as e:
            sent = False
            reason = str(e)

        if not rsp or (rsp.status_code != 200 and rsp.status_code != 208):
            if rsp:
                reason = rsp.text
            sent = False
        else:
            print(puid, rsp.text)

        if not sent:
            print("FAIL", puid, reason)
            with failed_targets_lock:
                failed_targets.append(target + (reason,))

    pool = ThreadPool(THREADS_COUNT)
    pool.map(send, targets)
    pool.close()
    pool.join()

    if failed_targets:
        with open("failed_targets.csv", "w") as f:
            writer = csv.writer(f)
            for target in failed_targets:
                writer.writerow(target)

        print("There is some failed targets, see them in 'failed_targets.csv'.")
