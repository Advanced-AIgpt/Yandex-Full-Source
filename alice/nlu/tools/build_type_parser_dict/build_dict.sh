#!/usr/bin/env bash

set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
[[ "$ARCADIA" ]] || { ARCADIA="${SCRIPT_DIR%/alice/nlu*}"; }

if [[ -z "$1" ]]; then
    echo "Usage: $0 <dictionary type>"
    exit 1
fi

dictionary_type="$1"
resource_name="type_parser_${dictionary_type}_rus.dict"
tmp_dict_resource=".${dictionary_type}.tmp"

ya m -r --checkout $ARCADIA/alice/nlu/tools/build_type_parser_dict && \
python $ARCADIA/alice/nlu/tools/build_type_parser_dict/build_dict.py --entity-type $dictionary_type --out $tmp_dict_resource && \
$ARCADIA/alice/nlu/tools/build_type_parser_dict/filters/filters --input $tmp_dict_resource && \
$ARCADIA/alice/nlu/tools/build_type_parser_dict/converter/converter --input $tmp_dict_resource --output $resource_name && \
ya upload --ttl inf $resource_name && \
rm $tmp_dict_resource && rm $resource_name
