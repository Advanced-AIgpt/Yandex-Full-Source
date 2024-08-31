#!/bin/bash

input_dir=$1
output_dir=$2

python tf_upgrade.py --intree $input_dir --outtree $output_dir

for filename in $output_dir/*.py; do
	sed -i -e 's/nn.rnn_cell/contrib.rnn/g' $filename
done
