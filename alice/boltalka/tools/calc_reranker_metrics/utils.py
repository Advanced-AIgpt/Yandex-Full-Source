def make_report(dcts):
    lines = []
    for i, dct in enumerate(dcts):
        names = []
        values = []
        for name in dct:
            if dct[name] is None:
                value = '-'
            elif name == 'model':
                value = dct[name]
            elif name == 'logloss':
                assert 'logloss_iter' in dct
                value = '{:.4f} ({})'.format(dct[name], dct['logloss_iter'])
            elif name == 'logloss_iter':
                continue
            elif name.endswith('_ndcg'):
                value = '{:.5f}'.format(dct[name])
            else:
                value = '{:.3f}'.format(dct[name])
            names.append(name)
            values.append(value)
        if i == 0:
            lines.append('|| ' + ' | '.join(names) + ' ||')
        lines.append('|| ' + ' | '.join(values) + ' ||')
    return '\n'.join(lines)
