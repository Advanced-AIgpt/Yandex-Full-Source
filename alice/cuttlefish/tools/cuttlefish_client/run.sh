# NOTE: cuttlefish server run example:
#     ~/arc/arcadia/alice/cuttlefish/bin/cuttlefish$ ./cuttlefish --port 4000
#./cuttlefish_client --method=asr.recognize --chunk-size=1000 --chunk-duration=100 --spotter-file=spotter.opus --file=privet.opus --debug=1
# RUN via apphost
./cuttlefish_client --method=asr.recognize --chunk-size=1000 --chunk-duration=500 --spotter-file=spotter.opus --file=privet.opus --debug=1 --port=40001 --path=/_streaming_no_block_outputs/asr_recognize
#./cuttlefish_client --method=asr.response --debug=1
#./cuttlefish_client --method=asr.response --error="test error" --debug=1
#./cuttlefish_client --method=tts.generate --debug=1
