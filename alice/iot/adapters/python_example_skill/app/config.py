from multiprocessing import cpu_count

max_requests = 1000
timeout = 300
workers = 1  # cpu_count() * 2 + 1
logging = '/app/logging.ini'
