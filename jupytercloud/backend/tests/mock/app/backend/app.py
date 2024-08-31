from jupytercloud.backend.app import JC
from jupyterhub.auth import DummyAuthenticator


class DummyWebpackAssets:
    def generate_handlers(self):
        return []


class MockJC(JC):
    authenticator_class = DummyAuthenticator
    webpack_assets = DummyWebpackAssets()


main = MockJC.launch_instance
