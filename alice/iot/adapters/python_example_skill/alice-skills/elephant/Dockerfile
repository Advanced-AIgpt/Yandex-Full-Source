FROM python
COPY . /elephant
RUN pip install -r /elephant/requirements.txt
EXPOSE 8001

CMD ["gunicorn", "-b", ":8001", "elephant.elephant:app"]