from setuptools import setup, find_packages

requires = [
    'requests>=2.18.4',
]

setup(
    name='jupytercloud',
    version='0.1.1',
    author='JupyterCloud Team',
    author_email='jupyter@yandex-team.ru',
    url='https://a.yandex-team.ru/arc/trunk/arcadia/jupytercloud/library',
    description='util functions for jupytercloud',
    packages=find_packages(),
    install_requires=requires,
)
