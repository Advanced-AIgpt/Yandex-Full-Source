from personal_assistant.testing_framework import load_testcase


def test_functional_tests_are_well_formed():
    placeholders = set()
    for _ in load_testcase('functional_data', placeholders):
        # if this test fails and it's unclear which yaml is broken, please read the debug log
        pass


def test_integration_tests_are_well_formed():
    placeholders = set()
    for _ in load_testcase('integration_data', placeholders):
        # if this test fails and it's unclear which yaml is broken, please read the debug log
        pass
