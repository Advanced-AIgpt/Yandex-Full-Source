# Render

Тулза для рендера фраз ответа Алисы на входящий контекст.

Изначально поддержанны сценарии *alarm* и *weather* (актуальные смотри в [коде](https://a.yandex-team.ru/arc_vcs/alice/nlg/tools/render_context_tool/main.cpp?#L20)). При подключении новых сценариев надо подключить PEERDIR в [ya.make](https://a.yandex-team.ru/arc_vcs/alice/nlg/tools/render_context_tool/ya.make).

Тикет [HOLLYWOOD-742](https://st.yandex-team.ru/HOLLYWOOD-742).


## Usage

Входные параметры (`--help` option).

```bash
A tool for render context for HW templates NLG phrases

Usage: ./render_context_tool [OPTIONS]

Options:
  --svnrevision           print svn version
  {-?|--help}             print usage
  {-c|--config} <path>    config file path (proto-text TConfig)
  --config-json <path>    config file path (json TConfig)
  --config-text <string>  text-serialized config TConfig
  --dump-proto            dump TConfig proto and exit
  {-i|--input} <string>   local input file with context-json per line
  {-o|--output-phrases} <string>
                          local output file with phrase per line
                          (default: "output_phrases.txt")
  --output-cards <string> local output file with card per line
                          (default: "output_cards.txt")
  {-l|--language} <ELang> specify the language to redefine the language in
                          contexts, for example: L_ARA, L_ENG, L_RUS etc.
                          Default: "L_UNK".
  {-d|--debug}            make output with extra columns
  {-r|--scenario-resources-path} <string>
                          path to resources (it's a local package directory, see more about package data sources here https://docs.yandex-team.ru/ya-make/manual/common/data#union)
                          (default: "data/alice/hollywood/shards/all/prod/resources")
```

Пример запуска:
```bash
ya make -r

# скачиваем с YT входные контексты
ya tool yt read-table --proxy hahn --format="<encode_utf8=%false>json" //table/path > input.txt

# рендер ответов из nlg
./render_context_tool --input input.txt

# записываем на YT результаты
ya tool yt write-table --proxy hahn --format="<encode_utf8=%false>json" //table/path < output_phrases.txt
ya tool yt write-table --proxy hahn --format="<encode_utf8=%false>json" //table/path < output_cards.txt
```

## Input

В параметре `--input` ожидается файл, в каждой строчке которого json c полями (или строка YT таблицы с колонками):
Колонка | Тип | Значение
--- | --- | ---
**nlg_render_id** | String | id запроса, для последующей связи output с input
**scenario_name** | String | с заглавной буквы, как в [конфиге MegaMind](https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/configs/production/scenarios), сейчас это *Weather* или *Alarm*
**template_name** | String | имя nlg файла сценария для рендеринга без указания языка, например *get_weather*
**language** | String | язык в формате [NAlice::ELang](https://a.yandex-team.ru/arc/trunk/arcadia/alice/protos/data/language/language.proto?rev=r8986890#L17): *L_RUS*, *L_ENG*, *L_TUR* или *L_ARA*
**phrase_name** | String | id фразы в nlg-шаблоне ответа, например *render_weather_today*
**card_name** | String | id карточки в nlg-шаблоне ответа, например *weather__curday_v2*
**context** | Json | сценарно-специфичный json, с attentions и всем нужным для генерирования ответа
**req_info** | Json | depricated, обычно пустой {}
**form** | Json | depricated, обычно пустой {}

## Output
На выходе получаем два отдельных файла c json-ответами рендеринга фраз и карточек. По умолчанию фразы пишутся в `output_phrases.txt` (колонки **nlg_render_id**, **text**, **voice**) и карточки в `output_cards.txt` (колонки **nlg_render_id**, **card**).

Выходные пути можно переопределить через параметры `--output-phrases` и `--output-cards`.
