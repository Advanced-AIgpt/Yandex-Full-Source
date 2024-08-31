## Turkish microintents checher

Collects matches from logs for the given phrases. Run:

```bash
ya m -r --checkout && ./handcrafted_phrases_checker -i <path to phrases you wanna add> -o <path to output table> [--threshold <threshold>]
```

The input file should contain phrases, one per line.

The output table has following format:
- `utterance_text` - the text of the request from the logs;
- `count` - number of times the utterance_text appeared in the logs;
- `score` - the similarity between the text and the most similar phrase from your file.

You can adjust the threshold based on your observations:
- Find a threshold that wouldn't allow bad phrases;
- Add good phrases with scores under the threshold to the index;
- Check the threshold again.
