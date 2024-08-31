#./asr_client --chunk-size=2000 --chunk-duration=100 --debug=1
#./asr_client --chunk-size=2000 --chunk-duration=100 --debug=1 --spotter-file=spotter.opus --spotter-phrase=алиса --spotter-back=2000 --request-front=0 --request-id=test-reqid --mime-format="audio/ogg;codecs=opus"
#./asr_client --chunk-size=20000 --chunk-duration=10 --debug=1 --request-id=test-reqid --mime-format="audio/ogg;codecs=opus"
#./asr_client --file=privet.opus --chunk-size=1000 --chunk-duration=100 --debug=1 --request-id=test-reqid --mime-format="audio/opus" --debug=0 --apphost-graph=false
#./asr_client --chunk-size=400 --chunk-duration=100 --file=privet.opus --request-id=test-reqid --mime-format="audio/opus" --normalization=true --debug=1 --path=/_streaming_no_block_outputs/asr_local --apphost-graph=true --topic=quasar
./asr_client --chunk-size=400 --chunk-duration=100 --file=privet.opus --request-id=test-reqid --mime-format="audio/opus" --normalization=true --debug=1 --path=/_streaming_no_block_outputs/asr --apphost-graph=true --topic=quasar
#./asr_client --port=10111 --chunk-size=2000 --chunk-duration=100 --spotter-file=spotter.opus --spotter-phrase=алиса --request-id=test-reqid --mime-format="audio/opus" --normalization=true --debug=1 --path=/_streaming_no_block_outputs/asr --apphost-graph=false
#./asr_client --chunk-size=2000 --chunk-duration=100 --spotter-file=spotter.opus --spotter-phrase=алиса --request-id=test-reqid --mime-format="audio/opus" --normalization=true --debug=1 --path=/_streaming_no_block_outputs/asr --apphost-graph=false
#./asr_client --file=trash.raw_0 --mime-format="audio/x-pcm;bit=16;rate=16000" --chunk-size=20000 --chunk-duration=10 --debug=1 --request-id=test-reqid
#./asr_client --chunk-size=200 --chunk-duration=1000 --debug=1 --spotter-file=spotter.opus --spotter-phrase=алиса --request-id=test-reqid --mime-format="audio/ogg;codecs=opus"
#./asr_client --chunk-size=2000 --chunk-duration=100 --debug=1 --spotter-file=spotter.opus --spotter-phrase=алиса --spotter-back=2000 --request-front=0 --request-id=test-reqid --mime-format="audio/opus"
#./asr_client --chunk-size=2000 --chunk-duration=100 --debug=1 --spotter-file=spotter.opus --spotter-phrase=вася --spotter-back=2000 --request-front=0 --request-id=test-reqid
#./asr_client --file=alice_break.opus --chunk-size=2000 --chunk-duration=100 --debug=1 --spotter-file=alice_break.opus --spotter-phrase=алиса --spotter-back=850 --request-front=1200
#./asr_client --file=alice_break.opus --chunk-size=2000 --debug=1 --spotter-file=alice_break.opus --spotter-phrase="алиса" --spotter-back=850 --request-front=1200
#./asr_client --chunk-size=2000 --chunk-duration=100 --debug=1 --spotter-file=alice_break.opus --spotter-phrase=хватит --spotter-back=2000 --request-front=0
#./asr_client --chunk-size=2000 --chunk-duration=100 --debug=1 --spotter-file=alice_break.opus --spotter-phrase=алиса --spotter-back=2000 --request-front=0
