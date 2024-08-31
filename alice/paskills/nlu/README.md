# NER/NLU service for external skills


## Installing dependencies

```
virtualenv -p python2 venv
source venv/bin/activate
pip install -r requirements.txt
```

Install optional development requirements
```
pip install -r requirements-dev.txt
```


## Running the app

Add `nlu_service` and lemmer to `PYTHONPATH` 
```
export PYTHONPATH=$(pwd)/src:$(pwd)/binary/lemmer/osx
```

Run web application
```
source venv/bin/activate
python cli.py --line-logging serve
```

## Testing

```
pytest
```

## Releasing

1. Install dev dependencies
2. Checkout to `master` branch
3. Run `bumpversion <patch|minor|major>`
4. Run `git push --tags`
5. Release will be automatically built and tagged in Drone CI at https://drone.yandex-team.ru/paskills/nlu
6. Deploy docker image to QLOUD
