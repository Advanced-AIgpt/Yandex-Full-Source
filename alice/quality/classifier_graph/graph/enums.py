import vh3
import typing


ClientTypeEnum = vh3.Enum[
    typing.Literal[
        "ECT_SMART_SPEAKER",
        "ECT_SMART_TV",
        "ECT_NAVIGATION",
        "ECT_TOUCH",
        "ECT_DESKTOP",
        "ECT_ELARI_WATCH",
    ]
]


FormulaResourceOptionsEnum = vh3.Enum[
    typing.Literal[
        "prod",
        "test",
        "skip"
    ]
]
