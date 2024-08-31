rm -f eventlog rtlog
ulimit -c 100000000
#./asr_adapter -c asr_adapter_prod_yaldi_rwr.json
./asr_adapter -c asr_adapter_prod_yaldi.json
#./asr_adapter -c asr_adapter_prod_asr2.json
#./asr_adapter -c asr_adapter_local_asr2.json
#./asr_adapter -c asr_adapter_local_yaldi.json
#./asr_adapter -c asr_adapter_fake.json
