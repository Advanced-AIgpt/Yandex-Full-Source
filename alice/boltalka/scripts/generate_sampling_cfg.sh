#!/bin/bash
DIR=$1
mkdir -p models
cp $DIR/models/backup.net models/
for cfg in $(find $DIR -name "*.cfg" -printf "%f\n"); do
    cp $DIR/$cfg init_$cfg
done
sed -i -re 's/([^ ]+\.cfg)/init_\1/' init_config.cfg
EDIR=$(echo -n "$DIR" | sed -re 's/\//\\\//g')
NL="\n"
sed -i -re "s/<\/Layer>/    InitFromModel $EDIR\/config.cfg, models\/backup.net$NL<\/Layer>/" *.cfg
echo "Do not forget to delete the delay layer from init_decoder.cfg!"
