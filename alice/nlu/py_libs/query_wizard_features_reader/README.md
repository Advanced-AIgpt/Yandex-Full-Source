# Python wrapper for query wizard features reader

## Building for python 2

1. `ya make -r -DUSE_ARCADIA_PYTHON=no -DOS_SDK=local`. This command will produce `query_wizard_features_reader.so` which you can import from python.
2. Optionally, add ``export PYTHONPATH=`pwd`":$PYTHONPATH"`` to be able to do `from query_wizard_features_reader import QueryWizardFeaturesReader` everywhere.
3. Copy compiled SO to query_wizard_features_reader folder: `cp *.so query_wizard_features_reader/`.
4. If not yet, obtain pypi keys: <https://wiki.yandex-team.ru/pypi/>.
5. Increase package version in `setup.py`.
6. Upload package to pypi: `python setup.py sdist upload -r yandex`.
