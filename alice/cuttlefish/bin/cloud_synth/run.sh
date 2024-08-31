rm -f eventlog rtlog

export CLOUD_SYNTH_TOKEN="Api-Key $(ya yav get version ver-01fqxpd20tjz1a99t5mp2nrbjh --only-value api_key)"

./cloud_synth -c cloud_synth.json
