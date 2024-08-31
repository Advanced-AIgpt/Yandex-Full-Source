#!/usr/bin/env bash
set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ARCADIA="${SCRIPT_DIR%/alice/nlu*}"
CURDIR="$(dirname "$BASH_SOURCE")"

ya make -r $ARCADIA/alice/nlu/tools/ar_fst_entities

$ARCADIA/alice/nlu/tools/ar_fst_entities/build/build \
    -e number:$CURDIR/full_number_fst.far \
    -d $ARCADIA/alice/nlu/tools/ar_fst_entities/dictionaries &
$ARCADIA/alice/nlu/tools/ar_fst_entities/build/build \
    -e time:$CURDIR/time_fst.far \
    -d $ARCADIA/alice/nlu/tools/ar_fst_entities/dictionaries &
$ARCADIA/alice/nlu/tools/ar_fst_entities/build/build \
    -e date:$CURDIR/date_fst.far \
    -d $ARCADIA/alice/nlu/tools/ar_fst_entities/dictionaries &
$ARCADIA/alice/nlu/tools/ar_fst_entities/build/build \
    -e weekdays:$CURDIR/weekdays_fst.far \
    -d $ARCADIA/alice/nlu/tools/ar_fst_entities/dictionaries &
$ARCADIA/alice/nlu/tools/ar_fst_entities/build/build \
    -e float:$CURDIR/float_fst.far \
    -d $ARCADIA/alice/nlu/tools/ar_fst_entities/dictionaries &
$ARCADIA/alice/nlu/tools/ar_fst_entities/build/build \
    -e datetime:$CURDIR/datetime_fst.far \
    -d $ARCADIA/alice/nlu/tools/ar_fst_entities/dictionaries &
$ARCADIA/alice/nlu/tools/ar_fst_entities/build/build \
    -e datetime_range:$CURDIR/datetime_range_fst.far \
    -d $ARCADIA/alice/nlu/tools/ar_fst_entities/dictionaries &
wait

ya upload --ttl=inf --json-output $CURDIR/full_number_fst.far > $CURDIR/full_number_upload_result.json &
ya upload --ttl=inf --json-output $CURDIR/time_fst.far > $CURDIR/time_upload_result.json &
ya upload --ttl=inf --json-output $CURDIR/date_fst.far > $CURDIR/date_upload_result.json &
ya upload --ttl=inf --json-output $CURDIR/weekdays_fst.far > $CURDIR/weekdays_upload_result.json &
ya upload --ttl=inf --json-output $CURDIR/float_fst.far > $CURDIR/float_upload_result.json &
ya upload --ttl=inf --json-output $CURDIR/datetime_fst.far > $CURDIR/datetime_upload_result.json &
ya upload --ttl=inf --json-output $CURDIR/datetime_range_fst.far > $CURDIR/datetime_range_upload_result.json &
wait

NUMBER_RESOURCE_ID="$(ya tool jq ".resource_id" $CURDIR/full_number_upload_result.json)"
TIME_RESOURCE_ID="$(ya tool jq ".resource_id" $CURDIR/time_upload_result.json)"
DATE_RESOURCE_ID="$(ya tool jq ".resource_id" $CURDIR/date_upload_result.json)"
WEEKDAYS_RESOURCE_ID="$(ya tool jq ".resource_id" $CURDIR/weekdays_upload_result.json)"
FLOAT_RESOURCE_ID="$(ya tool jq ".resource_id" $CURDIR/float_upload_result.json)"
DATETIME_RESOURCE_ID="$(ya tool jq ".resource_id" $CURDIR/datetime_upload_result.json)"
DATETIME_RANGE_RESOURCE_ID="$(ya tool jq ".resource_id" $CURDIR/datetime_range_upload_result.json)"

rm $CURDIR/*.far &
rm $CURDIR/*.json &
wait


cat >$CURDIR/ya.make <<EOF
# Please, don't edit this file, it is automatically built by build.sh script in the same folder

UNION()

OWNER(
    moath-alali
    g:alice_quality
)

FROM_SANDBOX(FILE $NUMBER_RESOURCE_ID OUT full_number_fst.far)
FROM_SANDBOX(FILE $TIME_RESOURCE_ID OUT time_fst.far)
FROM_SANDBOX(FILE $DATE_RESOURCE_ID OUT date_fst.far)
FROM_SANDBOX(FILE $WEEKDAYS_RESOURCE_ID OUT weekdays_fst.far)
FROM_SANDBOX(FILE $FLOAT_RESOURCE_ID OUT float_fst.far)
FROM_SANDBOX(FILE $DATETIME_RESOURCE_ID OUT datetime_fst.far)
FROM_SANDBOX(FILE $DATETIME_RANGE_RESOURCE_ID OUT datetime_range_fst.far)

END()
EOF
