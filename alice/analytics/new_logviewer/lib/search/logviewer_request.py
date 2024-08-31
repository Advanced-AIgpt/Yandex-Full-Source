import attr


@attr.s
class LogviewerRequest:
    begin: str = attr.ib()
    end: str = attr.ib()
    uuid: str = attr.ib()
    query: str = attr.ib()
    reply: str = attr.ib()
    intent: str = attr.ib()
    app: str = attr.ib()
    skill_id: str = attr.ib()
    generic_scenario: str = attr.ib()
    expboxes: str = attr.ib()
    search: str = attr.ib()
    mm_scenario = attr.ib()
    is_newbie = attr.ib(default=False)
