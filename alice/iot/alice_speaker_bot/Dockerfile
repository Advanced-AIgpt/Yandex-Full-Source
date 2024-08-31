FROM python:3

RUN pip install python-telegram-bot requests psycopg2 emoji
RUN pip install "ptbcontrib[postgres_persistence] @ git+https://github.com/python-telegram-bot/ptbcontrib.git"

COPY *.* /app/
WORKDIR app

CMD ["python3", "app.py"]
