#!/usr/bin/env bash
set -eu
set -o pipefail

if [ -z "${COMMIT:-}" ]; then
    COMMIT=`git log -n 1 --pretty="tformat:%h"`
fi

VERSION="${BUILD_NUMBER}.${COMMIT}"

if echo "${BRANCH:-master}" | grep pull >/dev/null; then
    VERSION="${VERSION}.pr"
fi

if [ -n "${POSTFIX:-}" ]; then
    VERSION="${VERSION}.${POSTFIX}"
fi
TOKEN=${SANDBOX_TOKEN}

rm -f uniproxy.tar.gz
git-archive-all --prefix='' uniproxy.tar.gz

#rm -f re_upload.py && svn export --non-interactive svn+ssh://arcadia.yandex.ru/arc/trunk/arcadia/junk/and42/lingware_update/re_upload.py ./

RESOURCE_ID=`python re_upload.py -q -o VOICETEVCH_YALDI_RELEASERS_DOCKER -T "${TOKEN}" -t OTHER_RESOURCE -d 'uniproxy-docker-test' -D - uniproxy.tar.gz`

echo "Resource: $RESOURCE_ID"

TASK_ID_DATA=$(curl -v -fs -X POST --data "{\"requirements\": {\"cpu_model\": \"\", \"platform\": \"\", \"host\": \"\", \"disk_space\": 10485760000}, \"description\": \"uniproxy ${VERSION}\", \"notifications\": [ {\"recipients\": [ \"VOICETEVCH_YALDI_RELEASERS_DOCKER\"], \"transport\": \"email\", \"statuses\": [ \"FAILURE\",\"EXCEPTION\",\"NO_RES\",\"SUCCESS\",\"TIMEOUT\"]}], \"max_restarts\": 0, \"kill_timeout\": 10800, \"priority\": {\"class\": \"SERVICE\", \"subclass\": \"NORMAL\"}, \"source\": null, \"tasks_archive_resource\": null, \"important\": false, \"fail_on_any_error\": true, \"owner\": \"VOICETEVCH_YALDI_RELEASERS_DOCKER\", \"hidden\": false, \"se_tag\": null, \"type\": \"BUILD_DOCKER_IMAGE_V6\", \"children\": {}, \"custom_fields\": [ {\"name\":\"docker_file_url\",\"value\":\"\"},{\"name\":\"packaged_resource_id\",\"value\":\"${RESOURCE_ID}\"},{\"name\":\"docker_resource_type\",\"value\":\"tarball\"},{\"name\":\"docker_package_checkout_arcadia_from_url\",\"value\":\"\"},{\"name\":\"docker_package_arcadia_patch\",\"value\":\"\"},{\"name\":\"docker_package_json\",\"value\":\"\"},{\"name\":\"registry_tags\",\"value\":[\"voicetech/uniproxy:${VERSION}\"]},{\"name\":\"registry_login\",\"value\":\"robot-voicetechbugs\"},{\"name\":\"vault_item_name\",\"value\":\"robot-voicetechbugs-registry-token\"},{\"name\":\"vault_item_owner\",\"value\":\"robot-voicetechbugs\"}]}" -H "Authorization: OAuth ${TOKEN}" -H "Expect:" https://sandbox.yandex-team.ru/api/v1.0/task/)
cat <<EOF
-----BEGIN SANDBOX RESPONSE-----
${TASK_ID_DATA}
-----END SANDBOX RESPONSE-----
EOF
TASK_ID=$(echo "${TASK_ID_DATA}" | python -c 'import json;import sys;print(json.load(sys.stdin).get("id"))')

echo "Task: $TASK_ID"

while true; do
    curl -fs -X PUT 'https://sandbox.yandex-team.ru/api/v1.0/batch/tasks/start' --data "[${TASK_ID}]" -H "Authorization: OAuth ${TOKEN}" > /dev/null
    CURL_EXIT_CODE=$?
    if [ $CURL_EXIT_CODE -eq 22 ]; then
        echo 'Curl returned 22 on task put, pass it'
        sleep 10
    elif [ $CURL_EXIT_CODE -eq 6 ]; then
        echo 'Curl returned 6 on task put, pass it'
        sleep 10
    elif [ $CURL_EXIT_CODE -ne 0 ]; then
        echo 'Curl exit code '$CURL_EXIT_CODE' on task put. Exiting.'
        exit 2
    else
        break
    fi
done

echo https://sandbox.yandex-team.ru/task/${TASK_ID}/view

set +e
while true; do
    RES=`curl -fs -H "Authorization: OAuth ${TOKEN}" "https://sandbox.yandex-team.ru/api/v1.0/task/$TASK_ID" | python -c 'import json; import sys; exec "try:\n  print(json.load(sys.stdin).get(\"status\"))\nexcept:\n  print(\"SANDBOX_API_FAIL\")"'`
    CURL_EXIT_CODE=$?
    if [ $CURL_EXIT_CODE -eq 22 ]; then
        echo 'Curl returned 22, pass it'
        sleep 10
    elif [ $CURL_EXIT_CODE -eq 6 ]; then
        echo 'Curl returned 6, pass it'
        sleep 10
    elif [ $CURL_EXIT_CODE -ne 0 ]; then
        echo 'Curl exit code '$CURL_EXIT_CODE
        exit 2
    elif echo 'FAILURE EXCEPTION NO_RES TIMEOUT' | grep -w $RES > /dev/null; then
        echo 'Failed'
        exit 1
    elif echo 'SUCCESS' | grep -w $RES > /dev/null; then
        echo 'Done'
        set -e
        HASH=`curl -fs -H "Authorization: OAuth ${TOKEN}" "https://sandbox.yandex-team.ru/api/v1.0/task/$TASK_ID" | python -c 'import json; import re; import requests; import sys; print(re.findall("digest: (sha256:.*) size", requests.get(list(filter(lambda x:x["name"]=="common.log", json.load(sys.stdin)["logs"]))[0]["url"].replace("common.log", "docker_push.out.txt")).text)[0])'`
        # export uniproxy docker cont. info to TC for use at next build steps (integration testing, etc)
        echo "##teamcity[setParameter name='env.UNIPROXY_DOCKER_HASH' value='${HASH}']"
        echo "##teamcity[setParameter name='env.UNIPROXY_DOCKER_VERSION' value='${VERSION}']"
        #echo "BRANCH=${BRANCH}"
        #if echo "${BRANCH:-master}" | grep pull >/dev/null; then
        #    echo 'Skip uploading this version to voice-ext.uniproxy.dev'
        #    exit 0
        #fi
        #echo 'Update qloud uniproxy dev'
        #export UNIPROXY_DOCKER_HASH=${HASH}
        #export UNIPROXY_DOCKER_VERSION=${VERSION}
        # QLOUD_TOKEN get from TC
        #python src/tests/qloud_cit.py --run update-dev

        # OLD qloud dev update:
        #rm -rf be-qloud-config
        #git clone ssh://bb.yandex-team.ru/voice/be-qloud-config.git
        #cd be-qloud-config && ./sync.sh ${TOKEN} voice-ext.uniproxy.dev
        #sed -i -e "s@\"repository.*@\"repository\": \"registry.yandex.net/voicetech/uniproxy:${VERSION}\",@" config/voice-ext.uniproxy.dev/components/be-uniproxy.json
        #sed -i -e "s@\"hash.*@\"hash\": \"${HASH}\",@" config/voice-ext.uniproxy.dev/components/be-uniproxy.json
        #sed -i -e "s@\"comment.*@\"comment\": \"Version ${VERSION}\",@" config/voice-ext.uniproxy.dev/config.json
        #./vdeploy.py -t ${TOKEN} -e voice-ext.uniproxy.dev
        #git add config/voice-ext.uniproxy.dev
        #git commit -m "Version ${VERSION} for uniproxy.dev"
        #git push
        exit 0
    else
        echo "Task status is $RES. Waiting."
        sleep 30
    fi
done
