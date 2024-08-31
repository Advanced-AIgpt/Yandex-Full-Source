def collect_match_stats(v):
    d = {x['dataset']: x for x in v}

    def stat(d, count_key):
        positive = d['matched'][count_key]
        total = d['total'][count_key]
        ratio = positive / max(1, total)
        return {
            'ratio': ratio,
            'positive': positive,
            'total': total,
        }

    return {
        'weighted': stat(d, 'count_weighted'),
        'unique': stat(d, 'count_unique'),
    }
