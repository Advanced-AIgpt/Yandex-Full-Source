import os
import re
import string

DATA_DIR = os.environ.get('DATA_DIR', '')
STATE_DIR = os.environ.get('STATE_DIR', '')
LOG_DIR = os.environ.get('LOG_DIR', '')

PUNCT_REGEX = re.compile('([' + string.punctuation + r'\\])')
def normalize(s):
    s = s.lower()
    s = re.sub(PUNCT_REGEX, r' \1 ', s).strip()
    s = re.sub(r'\s+', ' ', s, flags=re.U).strip()
    return s