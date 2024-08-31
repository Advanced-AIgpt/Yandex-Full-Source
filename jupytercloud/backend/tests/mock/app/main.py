import sys

import jupytercloud.backend.tests.mock.app

from jupytercloud.backend.launcher import JCLauncher


class MockJCLauncher(JCLauncher):
    service_parent = jupytercloud.backend.tests.mock.app
    service_prefix = service_parent.__name__ + '.'

    def add_misc_commands(self):
        pass


def main():
    sys.exit(MockJCLauncher().launch())


if __name__ == '__main__':
    main()
