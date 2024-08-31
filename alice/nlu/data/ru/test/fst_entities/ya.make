OWNER(
    egor-suhanov
)

EXECTEST()

SIZE(MEDIUM)
FORK_SUBTESTS()


RUN(
    NAME test_calc
    test_fst_entities parse --fst-name calc --fst-type CALC --dataset alice5v3.tsv --out ${TEST_CASE_ROOT}/out.tsv
    CANONIZE_LOCALLY ${TEST_CASE_ROOT}/out.tsv
)

RUN(
    NAME test_date
    test_fst_entities parse --fst-name date --fst-type DATE --dataset alice5v3.tsv --out ${TEST_CASE_ROOT}/out.tsv
    CANONIZE_LOCALLY ${TEST_CASE_ROOT}/out.tsv
)

RUN(
    NAME test_datetime
    test_fst_entities parse --fst-name datetime --fst-type DATETIME --dataset alice5v3.tsv --out ${TEST_CASE_ROOT}/out.tsv
    CANONIZE_LOCALLY ${TEST_CASE_ROOT}/out.tsv
)

RUN(
    NAME test_datetime_range
    test_fst_entities parse --fst-name datetime_range --fst-type DATETIME_RANGE --dataset alice5v3.tsv --out ${TEST_CASE_ROOT}/out.tsv
    CANONIZE_LOCALLY ${TEST_CASE_ROOT}/out.tsv
)

RUN(
    NAME test_fio
    test_fst_entities parse --fst-name fio --fst-type FIO.NAME --dataset alice5v3.tsv --out ${TEST_CASE_ROOT}/out.tsv
    CANONIZE_LOCALLY ${TEST_CASE_ROOT}/out.tsv
)

RUN(
    NAME test_float
    test_fst_entities parse --fst-name float --fst-type FLOAT --dataset alice5v3.tsv --out ${TEST_CASE_ROOT}/out.tsv
    CANONIZE_LOCALLY ${TEST_CASE_ROOT}/out.tsv
)

RUN(
    NAME test_geo
    test_fst_entities parse --fst-name geo --fst-type GEO --dataset alice5v3.tsv --out ${TEST_CASE_ROOT}/out.tsv
    CANONIZE_LOCALLY ${TEST_CASE_ROOT}/out.tsv
)

RUN(
    NAME test_num
    test_fst_entities parse --fst-name num --fst-type NUM --dataset alice5v3.tsv --out ${TEST_CASE_ROOT}/out.tsv
    CANONIZE_LOCALLY ${TEST_CASE_ROOT}/out.tsv
)

RUN(
    NAME test_time
    test_fst_entities parse --fst-name time --fst-type TIME --dataset alice5v3.tsv --out ${TEST_CASE_ROOT}/out.tsv
    CANONIZE_LOCALLY ${TEST_CASE_ROOT}/out.tsv
)

RUN(
    NAME test_units_time
    test_fst_entities parse --fst-name units_time --fst-type UNITS_TIME --dataset alice5v3.tsv --out ${TEST_CASE_ROOT}/out.tsv
    CANONIZE_LOCALLY ${TEST_CASE_ROOT}/out.tsv
)

RUN(
    NAME test_weekdays
    test_fst_entities parse --fst-name weekdays --fst-type WEEKDAYS --dataset alice5v3.tsv --out ${TEST_CASE_ROOT}/out.tsv
    CANONIZE_LOCALLY ${TEST_CASE_ROOT}/out.tsv
)

DEPENDS(
    alice/nlu/data/ru/test/pool
    alice/nlu/tools/test_fst_entities
)

DEPENDS(
    alice/begemot/lib/fst/data/fst/calc
    alice/begemot/lib/fst/data/fst/date
    alice/begemot/lib/fst/data/fst/datetime
    alice/begemot/lib/fst/data/fst/datetime_range
    alice/begemot/lib/fst/data/fst/fio
    alice/begemot/lib/fst/data/fst/float
    alice/begemot/lib/fst/data/fst/geo
    alice/begemot/lib/fst/data/fst/num
    alice/begemot/lib/fst/data/fst/time
    alice/begemot/lib/fst/data/fst/units_time
    alice/begemot/lib/fst/data/fst/weekdays
)
    
END()
