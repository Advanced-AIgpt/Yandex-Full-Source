# Item Selector

## Video Item Selector

Ниже описывается пайплайн обучения модельки для продакшена, которая прописывается в этом [ya.make](https://a.yandex-team.ru/arc/trunk/arcadia/alice/nlu/data/ru/models/alice_item_selector/video_gallery/ya.make)

1. Разметить какой-то набор заданий толокерами. Есть [специальный проект](https://toloka.yandex.ru/requester/project/40554)

   Толокерскую разметку можно получать примерно [вот так](https://nirvana.yandex-team.ru/flow/f3521ece-e3da-426b-a6f2-22212569dda8/b9ff6ede-7813-422a-9bda-54110fd83a78/graph)

2. Агреггированная разметка должна быть преобразована в обучающий пул для BERTa.
   
   [Такой граф](https://nirvana.yandex-team.ru/flow/92a9bad9-bef9-4439-b8d0-9e6ce0e28a2c/5097d56d-e4aa-40cc-8323-19462d7453bd/graph) по (project_id, pool_id) умеет его (обучающий пул BERTа) получать

   Уже собран [вот такой пул](https://yt.yandex-team.ru/hahn/navigation?path=//home/voice/volobuev/tickets/DIALOG-6896/pools/bert/model_input).

3. Дистилляция BERTа в LSTM, используемую в продакшене, осуществляется при помощи [вот такого графа](https://nirvana.yandex-team.ru/flow/2f5831f8-fc15-486d-ab59-d106682b5dd3/15bedb67-02b9-4a03-8ecb-8cba501de751/graph)
   Тут стоит обратить внимание на два входа:
    * Если хочется сделать побольше табличку дистилляции, то нужно подредактировать скрипт кубика __Collect Requests with Video Gallery__ и, возможно, настроить семплинг в кубике __Sample Table__
    * Если есть какой-то _более хороший_ пул c шага 2, то его стоит подать в __Get Bert Training Pool__

   Файлы полученной модели будут находиться в выходе __state__ кубика __Train LSTM__
   Для удобства, ресурс сразу загружается кубиком __sandbox upload__

Корзинка для оценки качества собиралась в тикете https://st.yandex-team.ru/DIALOG-6802

## Action Recognizer

1. Разметить пул толокерами. Есть специальное толокерское задание.
