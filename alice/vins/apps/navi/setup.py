from setuptools import setup, find_packages

setup(
    name='navi_app',
    version='0.0.1',
    description='bot for processing commands from navigator',
    author='Sergey Gorelov',
    author_email='volerog@yandex-team.ru',
    packages=find_packages(),
    include_package_data=True,
    package_data={
        'navi_app': [
            'config/navi_ru/VinsProjectfile.json',
            'config/navi_ru/intents/*.json',
            'config/navi_ru/intents/*.nlu',
            'config/navi_ru/intents/*.nlg',
            'config/navi_ru/entities/*.json',
            'config/nlu_templates/*.txt',
            'config/Vinsfile.json'
        ],
    },
    entry_points={
        'console_scripts': [
            'navi_telegram=navi_app.app:run_bot',
        ],
    },
)
