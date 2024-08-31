import pytest
from freezegun import freeze_time

from wtforms.validators import ValidationError

from alice.analytics.new_logviewer.lib.app.forms import (
    get_default_timestamp,
    escape_string,
    timestamp_validation,
)
from alice.analytics.new_logviewer.lib.app.logviewer_config import (
    LogviewerConfig,
    ConfigError,
)


@freeze_time("2001-06-09 12:00:00")
def test_get_default_timestamp():
    actual_begin = get_default_timestamp(of_begin=True)
    actual_end = get_default_timestamp(of_begin=False)
    expected_begin = "2001-06-08 00:00"
    expected_end = "2001-06-08 23:59"
    assert actual_begin == expected_begin
    assert actual_end == expected_end


def test_escape_string():
    string = "' DROP TABLE"
    expected = "\\' DROP TABLE"
    actual = escape_string(string)
    assert actual == expected


@freeze_time("2001-06-09 12:00:00")
def test_timestamp_validation():
    first = "2001-06-08 12:00"
    second = "2001-06-09 12:00"
    third = "2001 06 08 12:00"

    timestamp_validation(first)
    with pytest.raises(ValidationError):
        timestamp_validation(second)
    with pytest.raises(ValidationError):
        timestamp_validation(third)


def test_validate_storage_directory():
    good = "//home/alice-dev/andreyshspb/wonderlogs_extraction"
    bad = "/home/alice-dev/andreyshspb/wonderlogs_extraction"

    LogviewerConfig.validate_storage_directory(good)
    with pytest.raises(ConfigError):
        LogviewerConfig.validate_storage_directory(bad)
