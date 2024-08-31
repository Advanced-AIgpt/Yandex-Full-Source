#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

export PG_DATABASENAME=paskillsdb_priemka
export PG_USER=dialogovo
export PG_PASSWORD=$(ya vault get version ver-01etb46avkmg9m1qxsfqt30fzv --only-value dialogovo_password)
export XIVA_TOKEN=$(ya vault get version ver-01dsjgvp7v2a0jx7nb1eny3ez7 --only-value XIVA_TOKEN)
export PG_MULTI_HOST=vla-6p6gg0zklgfpliia.db.yandex.net:6432,man-d84liusfqlj7tn0c.db.yandex.net:6432,sas-4mvide6opucq9bhg.db.yandex.net:6432
export APP_METRICA_ENCRYPTION_SECRET=
export YDB_ENDPOINT=ydb-ru-prestable.yandex.net:2135
export YDB_DATABASE=/ru-prestable/alice/development/dialogovo-state-db
export ADDITIONAL_OPTIONS="-Dlog4j.configurationFile=$DIR/config/dev/log4j2.xml"

export ALICE4_BUSINESS_PG_READ_WRITE_DATABASE_URL=$(ya vault get version ver-01g1nmw2f572nn2n7enpvan6k9 --only-value jdbcReadWriteUrl)
export ALICE4_BUSINESS_PG_READ_ONLY_DATABASE_URL=$(ya vault get version ver-01g1nmw2f572nn2n7enpvan6k9 --only-value jdbcReadOnlyUrl)
export ALICE4_BUSINESS_PG_USER=$(ya vault get version ver-01ekwb2wb4xj6ykgtmwkyqhkmd --only-value username)
export ALICE4_BUSINESS_PG_PASSWORD=$(ya vault get version ver-01ekwb2wb4xj6ykgtmwkyqhkmd --only-value password)

$DIR/../../kronstadt/shard_runner/start_shard.sh --tvm-mode=tvmapi --output=$DIR/dialogovo.log --scenario-path="$DIR" $@
