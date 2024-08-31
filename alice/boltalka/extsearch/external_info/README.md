## Создание всех нужных файлов для HNSW индекса внешней информации

1. Сгенерировать колонку *doc_id* в таблице с колонками *embedding* и *text* 

```bash
python3 prep_index/enumerate_index.py --in-table //home/gena/artemkorenev/zeliboba_external_data/uber_index/2/index_deduplicated_postprocessed__WIKI__emb256_emb --out-table //home/gena/leshanbog/RAG/wiki-index/enumerated_wiki_16M
```

2. Сделать из таблицы .vec и .ids файлы

```bash
python3 prep_index/prepare_vec_and_ids.py --in-table //home/gena/leshanbog/RAG/wiki-index/enumerated_wiki_16M --out-local-prefix /home/leshanbog/GenServiceStuff/wiki_16M_hnsw
```

3. Подготовить файл с текстами

```bash
python3 prep_index/prepare_docs.py --in-table //home/gena/leshanbog/RAG/wiki-index/enumerated_wiki_16M --out-local-file /home/leshanbog/GenServiceStuff/wiki_16M_docs.json
```

4. Построить .index файл

```bash
cd build_index && ya make && ./build_index --vec-path /home/leshanbog/GenServiceStuff/wiki_16M_hnsw.vec --out-index-path /home/leshanbog/GenServiceStuff/wiki_16M_hnsw.index
```

5. В итоге получится 4 файла, которые затем надо сжать и загрузить на sandbox. Например, для индекса с 16M документов:

```bash
29G     /home/leshanbog/GenServiceStuff/index_artefacts/wiki_16M_docs.json
63M     /home/leshanbog/GenServiceStuff/index_artefacts/wiki_16M_hnsw.ids
3.9G    /home/leshanbog/GenServiceStuff/index_artefacts/wiki_16M_hnsw.index
16G     /home/leshanbog/GenServiceStuff/index_artefacts/wiki_16M_hnsw.vec
```


## Инференс таблички через API

1. Поднять апишку, находясь в `~/arcadia/alice/boltalka/generative/service/server/bin`

```bash
ya make -r -DCUDA_VERSION=11.4 -DCUDNN_VERSION=8.0.5 && ./generative_boltalka --config ~/arcadia/alice/boltalka/extsearch/external_info/infer_using_api/example_config_with_external_info.pb.txt --http-server-config-port 30303
```

2. Проинферить табличку

```bash
python3 infer_using_api/infer_local_api.py --yt-in-table-path //home/gena/artemkorenev/zeliboba_external_data/experiments_v2/_initial_data/boltalka/twitter/sep_delim/bucket --yt-out-table //home/gena/leshanbog/api_bucket_res --context-col context --api-endpoint http://127.0.0.1:30303/external_info_generative
```
