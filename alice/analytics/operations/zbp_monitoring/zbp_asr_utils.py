from json import loads

def make_header(new_date, new_nirvana_link, old_date, old_nirvana_link):
    head = '''\n<#<html><head><style type='text/css'>TABLE {border-collapse: collapse;border: 1px solid;}TD, TH { padding: 4px; border: 1px solid;} </style> </head> <body> <table border='1' border-style='solid'>'''
    header = '''<tr><td colspan=2></td><td colspan=4><p><a href='{old_nirvana_link}'>{old_date}</a></p></td><td colspan=4><p><a href='{new_nirvana_link}'>{new_date}</a></p></td></tr><tr><td>Устройство</td><td>Референс</td><td>ASR</td><td>wer</td><td>werp</td><td>Корректно?</td><td>ASR</td><td>wer</td><td>werp</td><td>Корректно?</td></tr>'''.format(old_nirvana_link=old_nirvana_link.decode("utf-8"), new_nirvana_link=new_nirvana_link.decode("utf-8"), old_date=old_date.decode("utf-8"), new_date=new_date.decode("utf-8"))
    return head + header

def pretty_print(new_summary, old_summary):
    new_hashes = set(new_summary.keys())
    old_hashes = set(old_summary.keys())
    all_hashes = new_hashes.union(old_hashes)
    is_good = {True: bytes("да", 'utf-8'), False: bytes("нет", 'utf-8')}
    row = '''<tr><td>{app}</td><td>{text}</td><td><p><a href='{old_setrace}'>{old_asr}</a></p></td><td>{old_wer}</td><td>{old_werp}</td><td align='center'>{old_quality}</td><td><p><a href='{new_setrace}'>{new_asr}</a></p></td><td>{new_wer}</td><td>{new_werp}</td><td align='center'>{new_quality}</td></tr>'''
    body = ""
    # "app": 0, "asr_is_good": 1, "asr_text": 2, "audio": 3, "setrace_url": 4, "text": 5, "wer": 6, "werp": 7
    decode_double = lambda x: bytes("N/A", 'utf-8') if x is None else bytes(str(round(x, 2)), "utf-8")
    for key in all_hashes:
        data = {}
        new_exists, old_exists = key in new_hashes, key in old_hashes
        data["text"] = new_summary[key][5] if new_exists else old_summary[key][5]
        data["app"] = new_summary[key][0] if new_exists else old_summary[key][0]
        
        fields, indices = ["asr", "setrace", "quality", "wer", "werp"], [2, 4, 1, 6, 7]
        decoders = [lambda x: x, lambda x: x, lambda x: is_good[x], decode_double, decode_double]
        for i in range(5):
            for prefix, summary, exists in zip(["new", "old"], [new_summary, old_summary], [new_exists, old_exists]):
                if exists:
                    data[prefix + "_" + fields[i]] = decoders[i](summary[key][indices[i]])
                else:
                    data[prefix + "_" + fields[i]] = b"" if fields[i] != "asr" else bytes("<нет примера в корзине>", 'utf-8')
        data = {key:data[key].decode("utf-8") if data[key] is not None else "N/A" for key in data.keys()}
        body += row.format(**data)

    return body

def make_text(change, good_part, sample_count):
    """
       formats text for comment
    """
    if change == b"open":
        result = "Привет!\nАвтоматическая проверка показала, что этот тикет вновь воспроизвелся в {part:.1%} из {count} примеров в корзине. Переоткрываю тикет.".format(part=1 - good_part, count=sample_count)
    elif change == b"close":
        result = "Привет!\nАвтоматическая проверка показала, что из {count} примеров в корзине текст был распознан правильно в {part:.2%} случаев. Закрываю тикет.".format(part=good_part, count=sample_count)
    elif change == b"not_enough_samples":
        result = "Привет!\nАвтоматическая проверка не нашла в корзине достаточно записей для этого тикета. Найдено примеров: {count}, нужно ещё {missing}. Пожалуйста, воспроизведите проблему на тексте из описания нужное число раз, затем призовите ((https://abc.yandex-team.ru/services/aliceanalytics/duty/?role=1775 дежурного))".format(missing=5 - sample_count, count=sample_count)
    else:
        raise ValueError("Unknown change: " + str(change))
    return result

def extract_hyp_from_asr_responses(asr_responses):
    recognition = asr_responses[-1]['directive']['payload']['recognition']
    words = recognition[0]['words'] if recognition else []
    return ' '.join([word['value'] for word in words]).lower()

def get_asr(VinsResponse, asr_responses, ignore=False):
    if VinsResponse:
        vins_loaded = loads(VinsResponse)
        if not ignore or (ignore and 'original_utterance' in vins_loaded['directive']['payload']['megamind_analytics_info']):
            return vins_loaded['directive']['payload']['megamind_analytics_info']['original_utterance']
        elif ignore and asr_responses:
            return extract_hyp_from_asr_responses(json.loads(asr_responses))
    return ''

