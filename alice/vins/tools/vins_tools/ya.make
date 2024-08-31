PY2_LIBRARY()

OWNER(g:alice)

PEERDIR(
    alice/vins/apps/personal_assistant
    alice/vins/core
)

RESOURCE_FILES(
    PREFIX vins_tools/
    nlu/ner/requirements.txt
    nlu/ner/ru/data/fio/ru.first.txt
    nlu/ner/ru/data/fio/ru.second.txt
    nlu/ner/ru/data/currency/currency.json
    nlu/ner/ru/data/site/site.json
    nlu/ner/ru/data/example
    nlu/ner/ru/data/geo/countries_capitals
    nlu/ner/ru/data/geo/moscow_streets.txt
    nlu/ner/ru/data/poi_category_ru/poi_categories.json
    nlu/ner/ru/data/soft/soft.json
)

PY_SRCS(
    NAMESPACE vins_tools
    nlu/__init__.py
    nlu/inspection/dataset_iteration.py
    nlu/inspection/interactive_app_analyzer.py
    nlu/inspection/nlu_processing_on_dataset.py
    nlu/inspection/nlu_result_info.py
    nlu/inspection/results_comparison.py
    nlu/inspection/taggers_recall.py
    nlu/ner/__init__.py
    nlu/ner/compile_custom_entities.py
    nlu/ner/fst_base.py
    nlu/ner/fst_custom.py
    nlu/ner/fst_utils.py
    nlu/ner/normbase.py
    nlu/ner/ru/fst_calc.py
    nlu/ner/ru/fst_date.py
    nlu/ner/ru/fst_datetime_range.py
    nlu/ner/ru/fst_datetime.py
    nlu/ner/ru/fst_fio.py
    nlu/ner/ru/fst_float.py
    nlu/ner/ru/fst_geo.py
    nlu/ner/ru/fst_num.py
    nlu/ner/ru/fst_time.py
    nlu/ner/ru/fst_units.py
    nlu/ner/ru/fst_weekdays.py
    nlu/ner/tr/fst_datetime.py
    nlu/ner/tr/fst_num.py
    utils/chunks.py
)

END()

RECURSE_FOR_TESTS(tests)
