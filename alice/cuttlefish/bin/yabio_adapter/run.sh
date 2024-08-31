if [[ -f eventlog ]]; then
    rm -f eventlog
fi
#gdb --args
ulimit -c 1000000000
./yabio_adapter -c yabio_adapter_prod.json
