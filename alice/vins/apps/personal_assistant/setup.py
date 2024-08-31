from setuptools import setup, find_packages

setup(
    name='vins-personal-assistant',
    version='0.0.1',
    description='Personal assistant app',
    author='Vins developers',
    author_email='vins@yandex-team.ru',
    include_package_data=True,
    packages=find_packages(),
    entry_points={
        'console_scripts': [
            'pa_telegram=personal_assistant.pa_bot:run_bot',
        ],
    },
)
