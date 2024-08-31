# Neural network models for VINS using Arcadia tensorflow

## In order to upload new version to pypi:

1. In this directory run `ya make . -r -DUSE_ARCADIA_PYTHON=no -DOS_SDK=local -DPYTHON_CONFIG=python2-config`. This creates .so file.
2. Run `cp *.so vins_models_tf/`.
3. Increase package version in `setup.py`.
4. Run `python setup.py sdist` to create a .tar.gz file with package in `./dist/` directory.
5. You need to `pip install twine` if not yet.
6. Obtain pypi username/password if not yet. (<https://wiki.yandex-team.ru/pypi/> will help)
6. Upload package to pypi with `python -m twine upload -u <username> -p <password> --repository-url https://pypi.yandex-team.ru/simple dist/<archive with package>`.
