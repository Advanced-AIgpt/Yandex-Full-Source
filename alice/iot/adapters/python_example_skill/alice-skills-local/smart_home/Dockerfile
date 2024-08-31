FROM python
COPY . /smart_home
RUN pip install -r /smart_home/requirements.txt
EXPOSE 8000

CMD ["gunicorn", "-b", ":8000", "smart_home.smart_home_api:app"]