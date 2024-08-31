def read_maps(maps):
    for it in maps:
        lst = it.get("list")
        if lst is None:
            yield it
        else:
            base = it.copy()
            del base["list"]
            for sub in read_maps(lst):
                yield {**base, **sub}


def split_lang(lang):
    tk = lang.split('-', 1)
    if len(tk) == 1:
        return tk[0], tk[0].upper()
    return tk[0], tk[1]


class PhraseMap:  # phrase -> topic
    def __init__(self):
        self.maps = {}
        self.default = None

    def add(self, phrase, dst_topic):
        if phrase == "default":
            self.default = dst_topic
        else:
            self.maps[phrase] = dst_topic

    def get(self, phrase):
        for k, v in self.maps.items():
            if k in phrase:
                return v
        return self.default


class TopicMap:  # topic -> PhraseMap
    # Topic wildcards are not sorted, so any matched widcard may be used!

    @staticmethod
    def __match(key, topic):
        if key[0] == key[-1] == "*":
            return key[1:-1] in topic
        elif key[-1] == "*":
            return topic.startswith(key[:-1])
        else:  # key[0] == "*"
            return topic.endswith(key[1:])

    def __init__(self):
        self.maps = {}
        self.maps_wc = {}
        self.default = None

    def add(self, src_topic, phrase, dst_topic):
        if src_topic == "*":
            if self.default is None:
                self.default = PhraseMap()
            mapped = self.default
        else:
            maps = self.maps_wc if ("*" in src_topic) else self.maps
            mapped = maps.setdefault(src_topic, PhraseMap())
        mapped.add(phrase, dst_topic)

    def get(self, topic, phrase):
        mapped = self.maps.get(topic)
        if mapped is not None:
            return mapped.get(phrase) or topic
        for key, mapped in self.maps_wc.items():
            if self.__match(key, topic):
                return mapped.get(phrase) or topic
        if self.default is not None:
            return self.default.get(phrase) or topic
        return topic


class LangMap:  # (l1, l2) -> TopicMap
    def __init__(self):
        self.maps = {}

    def add(self, lang, src_topic, phrase, dst_topic):
        key = split_lang(lang)
        mapped = self.maps.setdefault(key, TopicMap())
        mapped.add(src_topic, phrase, dst_topic)

    def get(self, lang, topic, phrase):
        l1, l2 = split_lang(lang)
        mapped = self.maps.get((l1, l2))
        if mapped is not None:
            return mapped.get(topic, phrase)
        mapped = self.maps.get((l1, "*"))
        if mapped is not None:
            return mapped.get(topic, phrase)
        mapped = self.maps.get(("*", "*"))
        if mapped is not None:
            return mapped.get(topic, phrase)
        return topic


class SpotterMaps(LangMap):
    def __init__(self, maps):
        super().__init__()
        for entry in read_maps(maps):
            self.add(
                lang=entry["lang"],
                src_topic=entry["from"],
                phrase=entry.get("phrase", "default"),
                dst_topic=entry["to"]
            )
