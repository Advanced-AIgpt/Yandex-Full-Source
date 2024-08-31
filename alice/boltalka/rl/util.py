import os
import re
import string

DATA_DIR = os.environ.get('INPUT_PATH', '/mnt/storage/nzinov/rl')
STATE_DIR = os.environ.get('SNAPSHOT_PATH', '/mnt/storage/nzinov/rl/experiments')
LOG_DIR = os.environ.get('LOGS_PATH', '/mnt/storage/nzinov/rl/runs')

PUNCT_REGEX = re.compile('([' + string.punctuation + r'\\])')
def normalize(s):
    s = s.lower()
    s = re.sub(PUNCT_REGEX, r' \1 ', s).strip()
    s = re.sub(r'\s+', ' ', s, flags=re.U).strip()
    return s