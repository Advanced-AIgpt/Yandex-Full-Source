Run your granet on custom requests.
1. Run 'prepare_data.py' to get positive and negative samples. Input parameters are --intent intent_name and --table-path table_path. Default table is table with all nlu requests. Table must have column 'text' with requests and column 'mock' with fetched entities for requests. Example of run:
```
./prepare_data --intent personal_assistant.scenarios.bluetooth_on
```
2. Run 'test.sh' to test your grammar on requests. Scripts takes only one parameter that is intent name. After that he will save results in 'result/true_negative.tsv' and 'result/false_positive.tsv' and output result in the console. If you want to show the result again you could use result/output_result.py. It takes two agrument '--true-negative' and '--false-positive' table pathes. Example of run:
```
./test.sh personal_assistant.scenarios.bluetooth_on
```
