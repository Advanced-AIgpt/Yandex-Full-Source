#!/bin/bash
set -e

# Setup agent name
echo "name=${QLOUD_APPLICATION}.${QLOUD_INSTANCE}" >> ${CONFIG_FILE}
touch /authorizationTokens.txt
echo authorizationToken=`grep "$QLOUD_INSTANCE" /authorizationTokens.txt | cut -f 2 -d '='` >> ${CONFIG_FILE}

# Setup known hosts
mkdir -p $(eval echo ~$USER/.ssh)
hosts=( "github.yandex-team.ru" "bitbucket.browser.yandex-team.ru" "bb.yandex-team.ru" )
for host in "${hosts[@]}"
do
    ssh-keyscan ${host} >> $(eval echo ~$USER/.ssh/known_hosts)
done

# Run agent
echo "QLOUD_MEMORY_LIMIT = $QLOUD_MEMORY_LIMIT"
memoryGib=$(expr $QLOUD_MEMORY_LIMIT / 1024 / 1024 / 1024 \* 800)
export _JAVA_OPTIONS="-Djava.net.preferIPv6Addresses=true -Djava.net.preferIPv6Stack=true -XX:MaxRAM=${memoryGib}m -Djava.io.tmpdir=/tmpfs"

LOG_DIR="${AGENT_HOME}/logs"
rm -f ${LOG_DIR}/*.pid

${AGENT_HOME}/bin/agent.sh start
while [ ! -f ${LOG_DIR}/teamcity-agent.log ];
do
   echo -n "."
   sleep 1
done

trap "${AGENT_HOME}/bin/agent.sh stop; sleep 10; exit 0;" SIGINT SIGTERM SIGHUP

touch anchor

tail -qF ${LOG_DIR}/teamcity-agent.log anchor &
wait
