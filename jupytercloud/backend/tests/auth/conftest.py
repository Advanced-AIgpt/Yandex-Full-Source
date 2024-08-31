import pytest

from jupytercloud.backend.tests.mock.spec import JupyterHubSpec


# function scope due to links generated for each test function
# but beware, jupyterhub - one and only one for each ya.make
@pytest.fixture(scope='function')
def jupyterhub(links):
    spec = JupyterHubSpec.load()

    links.set('JupyterHub stdout', spec.stdout_log)
    links.set('JupyterHub stderr', spec.stderr_log)

    return spec
