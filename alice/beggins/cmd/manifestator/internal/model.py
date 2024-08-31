from typing import Optional

import attr


@attr.s(slots=True, frozen=True)
class DataEntry:
    text: str = attr.ib()
    target: int = attr.ib()
    source: Optional[str] = attr.ib(default=None)

    def as_dict(self):
        return attr.asdict(self)
