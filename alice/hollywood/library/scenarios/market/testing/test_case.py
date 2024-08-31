from alice.hollywood.library.python.testing.run_request_generator.run_request_generator import (
    make_generator_params,
    make_runner_params,
    create_run_request_generator_fun,
)
from alice.hollywood.library.python.testing.integration.conftest import create_test_function


class TestCase:
    def __init__(
            self,
            tests_data,
            scenario_name,
            scenario_handle,
            data_path,
            app_presets,
            experiments=None,
            fixtures=None,
            oauth_token_fixture=None
            ):
        self.__tests_data = tests_data
        self.__app_presets = app_presets
        self.__scenario_name = scenario_name
        self.__scenario_handle = scenario_handle
        self.__data_path = data_path
        self.__experiments = experiments or []
        self.__fixtures = fixtures or []
        self.__oauth_token_fixture = oauth_token_fixture

    def __test_gen_params(self):
        return make_generator_params(self.__tests_data, self.__app_presets)

    def __test_run_params(self):
        return make_runner_params(self.__tests_data, self.__app_presets)

    def create_generator_function(self):
        return create_run_request_generator_fun(
            self.__scenario_name,
            self.__scenario_handle,
            self.__test_gen_params(),
            self.__data_path,
            self.__tests_data,
            self.__experiments,
            usefixtures=self.__fixtures,
            use_oauth_token_fixture=self.__oauth_token_fixture,
        )

    def create_test_function(self):
        return create_test_function(
            self.__data_path,
            self.__test_run_params(),
            scenario_handle=self.__scenario_handle,
            usefixtures=self.__fixtures,
        )
