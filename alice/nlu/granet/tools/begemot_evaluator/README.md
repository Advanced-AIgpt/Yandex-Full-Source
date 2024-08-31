Evaluates granet's intent prediction quality.

To collect results from begemot:  
```./begemot_requester collect_granet_responses --begemot-url <begemot url> --output-path result.txt [--row-count 20000] [--use-nonsense-tagger]```  

To calculate some metrics and obtain false negative and false positive samples:  
```./begemot_requester calculate_metrics --input-path result.txt --intent-renames intent_renames.json```  

`intent_renames.json` contains mapping from toloka intents to granet intents, only intents from this file are used in evaluation.