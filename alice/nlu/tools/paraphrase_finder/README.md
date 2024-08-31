# Paraphrase finder

Collects subsessions that contain paraphrases according to dssm or intersection similarity.

Example run:
```bash
ya m -r && ./paraphrase_finder --start-date 2020-05-01 --end-date 2020-05-30 --output-table //tmp/$USER/paraphrased_sessions
```

It parses sessions from `//home/voice/dialog/sessions` and returns paraphrases sessions with additional rows:
- session - session that starts and ends with similar phrases;
- similarity - similarity between those phrases;
- dialog_history - requests and replies from the session;
- full_session, full_dialog_history - full session from which this subsessions was extracted;
- is_end_of_session - whether the session and full session ends on the same turn (usually it means that user reformulated the request to achieve his goal - or just decided that Alice will not be able to fulfill it)
