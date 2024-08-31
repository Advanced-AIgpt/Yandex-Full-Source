import attr


@attr.s(frozen=True)
class Span(object):
    pos = attr.ib()
    length = attr.ib()

    @classmethod
    def deserialize(cls, text):
        pos, length = map(int, text.split(','))
        return cls(
            pos=pos,
            length=length
        )

    def serialize(self):
        return '{0},{1}'.format(self.pos, self.length)


@attr.s(frozen=True)
class TolokaTag(object):
    text = attr.ib()
    label = attr.ib()
    span = attr.ib()

    @classmethod
    def deserialize(cls, text):
        text, label, serialized_span = text.split('~')
        return cls(
            text=text,
            label=label,
            span=Span.deserialize(serialized_span)
        )

    def serialize(self):
        return '~'.join([self.text, self.label, self.span.serialize()])


@attr.s(frozen=True)
class TolokaMarkup(object):
    text = attr.ib()
    tags = attr.ib()

    @classmethod
    def deserialize(cls, line):
        text, serialized_markup = line.split('\t')
        serialized_tags = serialized_markup.split('|')
        tags = map(TolokaTag.deserialize, serialized_tags)
        return cls(
            text=text,
            tags=tags
        )

    def serialize(self):
        return self.text + '\t' + self.serialize_tags()

    def serialize_tags(self):
        return '|'.join([tag.serialize() for tag in self.tags])
