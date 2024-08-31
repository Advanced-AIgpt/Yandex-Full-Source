# Begemot binary classifiers test

Collects binary intent classifier scores from Begemot.

Run:
```bash
ya m -r --yt-store
./collect_begemot_responses \
    -i <input_table_path> \
    -o <output_table_path> \
    --text-column <e.g., text> \
    --prob-column <e.g., class_prob> \
    --intent <your intent>
    [--begemot-url <http://hamzard.yandex.net:8891/wizard by default>]
```
