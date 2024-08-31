#!/bin/bash
DIR='/home/alipov/datasets/vins'
echo 'Filtering dataset'
( cat $DIR/librusec_raw | ./filter_rus.py | ./preprocess_librusec.sh > $DIR/librusec_filtered ) || exit 1

echo 'Building dict'
( ./build_dict.py --text-file $DIR/librusec_filtered --dict-file $DIR/librusec.dict --dict-size 150000 --lower --separate-punctuation > $DIR/librusec.freq ) || exit 1

echo 'Shuffling dataset'
( shuf $DIR/librusec_filtered --random-source=<(./random-bytes.sh) > $DIR/librusec_filtered_shuf ) || exit 1

echo 'Splitting train and validation'
tail -n200000 $DIR/librusec_filtered_shuf > $DIR/librusec_filtered_shuf.big_val
tail -n20000 $DIR/librusec_filtered_shuf.big_val > $DIR/librusec_filtered_shuf.val
head -n-200000 $DIR/librusec_filtered_shuf > $DIR/librusec_filtered_shuf.train

echo 'Building train'
./build_dialog_dataset.py --text-file $DIR/librusec_filtered_shuf.train --output-file $DIR/librusec_cx.train --max-context-tokens 300 --max-context-turns 300 --dialog-per-line --lower --separate-punctuation || exit 1
echo 'Building validation'
./build_dialog_dataset.py --text-file $DIR/librusec_filtered_shuf.val --output-file $DIR/librusec_cx.val --max-context-tokens 300 --max-context-turns 300 --dialog-per-line --lower --separate-punctuation || exit 1

echo 'Shuffling train'
shuf $DIR/librusec_cx.train -o $DIR/librusec_cx.train --random-source=<(./random-bytes.sh) || exit 1
