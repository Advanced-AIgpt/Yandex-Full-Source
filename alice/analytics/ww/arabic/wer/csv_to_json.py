import re
import json


data = []
with open('compared_texts_full.csv') as f:
    for idx, line in enumerate(f):
        if idx == 0:
            # skip header
            continue

        line = line.strip().lower()
        line = line.replace('‏', ' ')

        parts = line.split(',')
        if 'unclear' in line or 'speech' in line \
                or not parts[3] or parts[3] == ' ' or len(parts[3]) < 3 \
                or not parts[2] or parts[2] == ' ' or len(parts[2]) < 3:
            print(f'error in item: "{line}"')
            continue

        # удаляем английский в скобках
        parts[2] = re.sub(r'([(][a-zA-Z ]{3,}[)])', '', parts[2])
        parts[3] = re.sub(r'([(][a-zA-Z ]{3,}[)])', '', parts[3])

        # удаляем просто английский
        parts[2] = re.sub(r'([a-zA-Z ]{3,})', '', parts[2])
        parts[3] = re.sub(r'([a-zA-Z ]{3,})', '', parts[3])

        # удаляем лишние кавычки
        parts[2] = parts[2].replace('"', '')
        parts[3] = parts[3].replace('"', '')

        if re.match('.*[a-zA-Z].*', parts[2]):
            print(f'english in text "{parts[2]}"')

        if re.match('.*[a-zA-Z].*', parts[3]):
            print(f'english in text "{parts[3]}"')

        data.append({
            'text_old': parts[3],
            'text_new': parts[2],
            'id': parts[1],
        })

with open('in1.json', 'w') as f:
    json.dump(data, f, indent=4, ensure_ascii=False)
