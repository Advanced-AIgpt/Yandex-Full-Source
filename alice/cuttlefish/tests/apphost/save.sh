#/bin/bash

THIS_DIR=$(dirname $(readlink -f $0))

ya make -ttt $THIS_DIR --test-param preserve-dir=$THIS_DIR/horizon-data --checkout

echo "+-----------------------------------------------------+"
echo "| If you have added new backends, don't forget to run |"
echo "| 'svn add' or 'arc add' on them in horizon data.     |"
echo "+-----------------------------------------------------+"
