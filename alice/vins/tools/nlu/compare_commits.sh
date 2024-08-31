#!/bin/bash
set -x

SCRIPT_DIR=`dirname $0`

# copy scripts to keep them intact across commits
cp $SCRIPT_DIR/process_pa_nlu_on_validation_sets.sh .
cp $SCRIPT_DIR/compare_base.sh .

git checkout $1
sh process_pa_nlu_on_validation_sets.sh
mkdir a
mv *.pkl a

git checkout $2
sh process_pa_nlu_on_validation_sets.sh
mkdir b
mv *.pkl b

# produce files with metrics using the same evaluation algorithm
sh compare_base.sh non_existing_dir a
# finally, the comparison - under b's renaming scheme
sh compare_base.sh a b
