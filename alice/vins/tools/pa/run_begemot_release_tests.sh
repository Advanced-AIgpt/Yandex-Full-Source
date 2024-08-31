#!/usr/bin/env bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
[[ "$ARCADIA" ]] || { ARCADIA="${SCRIPT_DIR%/alice/*}"; }
ya() {
    "$ARCADIA/ya" "$@"
}

if [[ $# < 3 ]]; then
    echo "Usage: $0 BG_URL RESULT_ON_PROD RESULT_ON_RC"
    echo "Examples:"
    echo "  $0 \\"
    echo "    http://begemot-megamind-9-1.yappy.yandex.ru/wizard \\"
    echo "    bg-9-1-diff-on-vins-26-16-prod \\"
    echo "    bg-9-1-diff-on-vins-27-12-rc"
    echo "  $0 \\"
    echo "    http://w7ap7okmbn7kv7ho.man.yp-c.yandex.net/wizard \\"
    echo "    my-prod \\"
    echo "    my-rc \\"
    echo "    --test-prefix test_music"
    exit 1
fi

BG_URL=$1
RESULT_ON_PROD=$2
RESULT_ON_RC=$3

ya make --checkout "$ARCADIA/alice/vins/tools/pa"

run_test() {
  VINS_URL=$1
  RESULT=$2

  $ARCADIA/alice/vins/tools/pa/pa_tools run_integration_diff_tests \
    --placeholders "city_dialog_id:672f7477-d3f0-443d-9bd5-2487ab0b6a4c" \
    --fast \
    --trace \
    --experiments "mm_dont_defer_apply:1" \
    --vins-url "$VINS_URL" \
    --req-wizard-url-v2 "$BG_URL" \
    --json-dump "$RESULT.json" \
    --report "$RESULT.txt" \
    --summary "$RESULT.summary.txt" \
    "${@:3}"
}

# Compare Begemot-RC with Begemot-prod using Megamind-prod
run_test "http://vins.alice.yandex.net/speechkit/app/pa/" $RESULT_ON_PROD "${@:4}"

# Compare Begemot-RC with Begemot-prod using Megamind-RC
run_test "http://megamind-rc.alice.yandex.net/speechkit/app/pa/" $RESULT_ON_RC "${@:4}"

echo
echo "===================================================================="
echo
echo "$RESULT_ON_PROD:"
cat "$RESULT_ON_PROD.summary.txt"
echo
echo "$RESULT_ON_RC:"
cat "$RESULT_ON_RC.summary.txt"
echo
echo "===================================================================="
