from flask_wtf import FlaskForm
from wtforms import (
    StringField,
    SelectField,
    SubmitField,
    BooleanField,
)
from wtforms.validators import ValidationError

import re
from datetime import (
    date,
    datetime,
    timedelta
)

DATETIME_FORMAT = "%Y-%m-%d %H:%M"
DATE_FORMAT = "%Y-%m-%d"

SUPPORTED_DAYS_NUMBER = 28


def get_default_timestamp(of_begin: bool) -> str:
    time = datetime.min.time() if of_begin else datetime.max.time()
    day = date.today() - timedelta(days=1)
    return datetime.combine(day, time).strftime(DATETIME_FORMAT)


def escape_string(string: str) -> str:
    escape_chars = {
        "\b": "\\b",
        "\f": "\\f",
        "\r": "\\r",
        "\n": "\\n",
        "\t": "\\t",
        "\0": "\\0",
        "\\": "\\\\",
        "'": "\\'"
    }
    return string.translate(str.maketrans(escape_chars))


def timestamp_validation(timestamp: str):
    try:
        day = datetime.strptime(timestamp, DATETIME_FORMAT).date()
    except ValueError:
        raise ValidationError("time must be in <YYYY-MM-DD hh:mm> format")
    today = date.today()
    if not (today - timedelta(days=SUPPORTED_DAYS_NUMBER) <= day < today):
        raise ValidationError(f"time must be one of the last {SUPPORTED_DAYS_NUMBER} days")


class SearchForm(FlaskForm):

    # ------- FIELDS -------

    begin = StringField("From")
    end = StringField("To")
    uuid = StringField("Uuid", default=str())
    query = StringField("Query", default=str(), filters=[escape_string, str.lower])
    reply = StringField("Reply", default=str(), filters=[escape_string])
    intent = StringField("Intent", default=str(), filters=[escape_string])
    generic_scenario = StringField("Generic Scenario", default=str(), filters=[escape_string])
    expboxes = StringField("Expboxes", default=str(), filters=[escape_string])
    app = SelectField("App")
    skill_id = SelectField("Skill")
    mm_scenario = StringField("Megamind Scenario", default=str(), filters=[escape_string])
    search = SubmitField("Search")
    is_newbie = BooleanField("Is newbie", default=False)

    # ------- INIT -------

    def __init__(self, apps: list[str], skills: list[str], *args, **kwargs):
        super().__init__(*args, **kwargs)
        if not self.begin.data:
            self.begin.data = get_default_timestamp(of_begin=True)
        if not self.end.data:
            self.end.data = get_default_timestamp(of_begin=False)
        self.app.choices = apps
        self.skill_id.choices = skills

    # ------- FIELD VALIDATION -------

    @staticmethod
    def validate_begin(form, field):
        timestamp_validation(field.data)

    @staticmethod
    def validate_end(form, field):
        timestamp_validation(field.data)

    @staticmethod
    def validate_uuid(form, field):
        if field.data:
            result = re.match(r"^uu/([0-9a-f]{32})$", field.data)
            if not result:
                raise ValidationError("uuid is a hex string of length 32 with 'uu/' prefix")
