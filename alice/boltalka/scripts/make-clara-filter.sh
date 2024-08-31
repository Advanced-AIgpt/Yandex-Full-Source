cat clara.questions | sed -re 's/^/\\B/;s/$/\\B/' | tr '\n' '|' > clara.filter
