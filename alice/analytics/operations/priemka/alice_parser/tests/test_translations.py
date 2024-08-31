from alice.analytics.operations.priemka.alice_parser.visualize.generic_scenario_to_human_readable import (
    generic_scenario_to_human_readable
)


def test_translation():
    assert generic_scenario_to_human_readable['music_podcast'] == 'Подкасты'
