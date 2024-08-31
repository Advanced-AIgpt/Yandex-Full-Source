import sys
import json
import codecs

if __name__ == "__main__":
    input_file = sys.argv[1]
    out = []
    with codecs.open(input_file, encoding='utf-8') as f:
        for i, line in enumerate(f):
            out.append({
                'dialog': [line.strip()],
                'key': str(i)
            })
    with codecs.open(input_file + '.out', encoding='utf-8', mode='w') as fout:
        json.dump(out, fout, ensure_ascii=False, indent=4)
