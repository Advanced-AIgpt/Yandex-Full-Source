#!/bin/bash

if [ "$USER_OAUTH_TOKEN" == "" ]; then
    echo 'USER_OAUTH_TOKEN not set'
    echo 'Go to https://oauth.yandex.ru/authorize?response_type=token&client_id=6bb9bdb1a3f24a6982bedf179f9850e4&force_confirm=yes and get it'
    exit 1
fi

# Get main route
USER_IP=$(python3 -c 'import socket; s = socket.socket(socket.AF_INET6); s.connect(("2a02:6b8:0:3400:0:1d6:0:3", 80)); print(s.getsockname()[0]); s.close()')

# Get tvm tickets
# 2000743 - uniproxy test
# 2001015 - yandex music (qa)
# WARNING: Do not use here main tvm ids 2000496 (uniproxy) and 2000090 (yandex music) for service ticket
export TVM_SERVICE_TICKET=$(ya tool tvmknife get_service_ticket sshkey --src 2000743 --dst 2001015)

# We must use here main uniproxy tvm
# Otherwise we will get ACCESS_DENIED by tvm ip filter
export TVM_USER_TICKET=$(echo $USER_OAUTH_TOKEN | ya tool tvmknife get_user_ticket oauth --tvm_id 2000496 -b 'blackbox-mimino.yandex.net' --user_ip $USER_IP)

#./music_match_client --chunk-size=20000 --chunk-duration=1000 --user-ip=$USER_IP --mime-format=audio/pcm-data --file=not_music.pcm
#./music_match_client --chunk-size=20000 --chunk-duration=1000 --user-ip=$USER_IP --mime-format=audio/pcm-data --file=output.pcm
#./music_match_client --chunk-size=2000 --chunk-duration=100 --user-ip=$USER_IP --mime-format=audio/pcm-data --file=output.pcm
#./music_match_client --chunk-size=2 --chunk-duration=100 --user-ip=$USER_IP
#./music_match_client --chunk-size=20000 --chunk-duration=100 --user-ip=$USER_IP
#./music_match_client --chunk-size=20000 --chunk-duration=100 --user-ip=$USER_IP --spotter-file=not_music.opus

#./music_match_client --chunk-size=20000 --chunk-duration=100 --user-ip=$USER_IP --port=20001 --path=/_streaming_no_block_outputs/music_match --apphost-graph=true
./music_match_client --chunk-size=20000 --chunk-duration=100 --user-ip=$USER_IP --spotter-file=not_music.opus --port=20001 --path=/_streaming_no_block_outputs/music_match --apphost-graph=true
