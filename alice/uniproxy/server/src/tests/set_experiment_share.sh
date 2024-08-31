#/usr/bash
# Example:
#   bash set_experiment_share.sh duplicate_asr_to_rtc_gpu 0.01

exp_id=$1
share=$2
qenv=$3

if [[ -z "$exp_id" || -z "$share" ]];then
    echo -e "Usage:\n\t $0 <uniproxy-experiment-id> <share>"
    exit 1
fi

if [[ -z "$qenv" || "$qenv" != "rtc" ]];then
    if [[ -z "$QLOUD_TOKEN" ]] ;then
        if [[ -f $HOME/.qloud-token ]]; then
            export QLOUD_TOKEN=`cat $HOME/.qloud-token`
        else
            echo "set env. var. QLOUD_TOKEN or keep token at HOME/.qloud-token file"
            exit 1
        fi
    fi

    echo "try set share=$share for exp_id=$exp_id"

    hosts=`python qloud_cit.py --env=stable --run=ls`
else
    hosts=`sky list Y@wsproxy-{man,sas,vla}`
fi

unset 'LC_ALL'

for host in $hosts; do
    echo -en "$host: "
    ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -q $host "curl -s 'localhost:8080/exp?id=$exp_id&share=$share'" &
    echo
done
