#!/bin/bash
set -xeo pipefail

echo "Start proxy"

../main.py &
proxy_pid=$!
trap 'pkill -9 -g `ps -q $proxy_pid -o pgid | tail -n 1`' 0 SIGINT SIGTERM
sleep 3

curl --data-binary "@data/n3.wav" -X POST http://localhost:8887/recog_secret?topic=general\&pretty=3

curl --data-binary "@data/n3.wav" -X POST http://localhost:8887/recog_secret

curl --data-binary "@data/n3.wav" -X POST http://localhost:8887/recog_secret?topic=dialog-general\&pretty=1

curl --data-binary "@data/n3.wav" -X POST http://localhost:8887/recog_secret?topic=desktopgeneral\&pretty=2

curl --data-binary "@data/n3.wav" -X POST http://localhost:8887/recog_secret?topic=desktopgeneral\&pretty=3

curl --data-binary "@data/n3.wav" -X POST http://localhost:8887/recog_secret?topic=general\&session_id=1

curl --data-binary "@data/address.opus" -X POST http://localhost:8887/recog_secret?topic=dialog-general\&message_id=1\&format="audio/opus"\&pretty=2\&chunk_size=2000

test `curl "http://localhost:8887/tts_generate?text=123&voice=jane&format=audio/ogg%3Bcodecs=flac" | wc --b | cut -f 1 -d " "` -gt 0
test `curl "http://localhost:8887/tts_generate?text=123&voice=jane&format=audio/opus" | wc --b | cut -f 1 -d " "` -gt 0
test `curl "http://localhost:8887/tts_generate?text=123&voice=shitova.us&format=pcm" | wc --b | cut -f 1 -d " "` -gt 0

