import yt.wrapper as yt

def mapper(row):
    session = row['session'].strip().split('\t')
    for idx in range(1, len(session), 2):
        context = '\t'.join(session[max(idx - 3, 0):idx])
        reply = session[idx]
        terminal = int(idx == len(session) - 1)
        target = None
        if not terminal:
            idx += 2
            target = '\t'.join(session[idx - 3:idx])
        yield dict(context=context, reply=reply, terminal=terminal, target=target)

if __name__ == '__main__':
    yt.config.set_proxy('hahn')
    yt.run_map(mapper, '//home/voice/nzinov/gc_sessions', '//home/voice/nzinov/rl_dataset')
