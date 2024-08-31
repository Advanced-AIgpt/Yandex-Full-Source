#!/bin/bash

cd ../../core/vins_core/test/nlu_eval_reports/$1/$2

echo 
echo '###################################'
echo '# true_intent != predicted_intent #'
echo '###################################'
echo

awk -F',' '{if ($3!=$5) print $1"|"$2"|"$3"|"$5"("$6")|"$4"|"$7" "$8","$9","$10}' test_fold_*.tsv  | sort -u

echo
echo '##################################################################'
echo '# true_intent == predicted_intent, true_slots != predicted_slots #'
echo '##################################################################'
echo 

awk -F',' '{if ($3==$5 && $4!=$7) print $1"|"$2"|"$3"|"$5"("$6")|"$4"|"$7" "$8","$9","$10}' test_fold_*.tsv  | sort -u

