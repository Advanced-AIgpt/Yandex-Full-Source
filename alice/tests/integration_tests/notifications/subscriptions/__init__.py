AliceDigest = 1
TvShowsReleases = 2
MusicReleases = 3

all_subscriptions = frozenset(v for k, v in globals().items() if not k.startswith('_'))
