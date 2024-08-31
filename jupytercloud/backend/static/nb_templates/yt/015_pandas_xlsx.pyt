# ---
# jupyter:
#   jupytext:
#     text_representation:
#       extension: .py
#       format_name: percent
#       format_version: '1.3'
#       jupytext_version: 1.4.1
#   kernelspec:
#     display_name: Arcadia Python 3 Default
#     language: python
#     name: arcadia_default_py3
#   nb_template:
#     name_en: "YT to Excel"
#     description_en: >-
#       Скачать данные из YT в формате Microsoft Excel.
#
#       Для простоты использования рекомендуется открывать этот шаблон в Jupyter Notebook.
# ---

# %% [markdown]
Выполните этот ноутбук, в результате внизу будет ссылка на скачивание .xlsx-файла.

Перед самым первым запуском не забудьте получить [YT токен](https://yt.yandex-team.ru/docs/gettingstarted.html#auth).

# %%
from tqdm import tqdm_notebook as tqdm
from IPython.display import FileLink, Markdown
import xlsxwriter
import yt.wrapper as yt
from itertools import islice
from warnings import warn


path = "{{ path }}"
filename = path.lstrip("/").replace("/", "-") + ".xlsx"

yt.config["proxy"]["url"] = "{{ cluster }}"
MAX_ROWS = 2**20 - 1
MAX_COLUMNS = 2**15


keys = set()
row_count = yt.row_count(path)

if row_count > MAX_ROWS:
    warn(f"""\n\nWarning! Excel files cannot have more than {MAX_ROWS} rows.
Your table has {row_count} rows.
Rest will be truncated.""")
    row_count = MAX_ROWS

for i, row in tqdm(
    islice(enumerate(yt.read_table(path)), row_count),
    total=row_count,
    desc="Calculating columns"
):
    keys.update(row.keys())

keys = sorted(list(keys))

if len(keys) > MAX_COLUMNS:
    warn(f"""\n\nWarning! Excel files cannot have more than {MAX_COLUMNS} columns.
Your table has {len(keys)} columns.
Rest will be truncated.""")
    keys = keys[:MAX_COLUMNS]


workbook = xlsxwriter.Workbook(filename, {'constant_memory': True})
workbook.use_zip64()
worksheet = workbook.add_worksheet()

for i, row in tqdm(
    islice(enumerate(yt.read_table(path)), row_count),
    total=row_count,
    desc="Downloading data"
):
    title_format = workbook.add_format({"bold": True})
    worksheet.write_row(0, 0, keys, title_format)

    for j, column in enumerate(keys):
        if column not in row:
            continue
        if not (isinstance(row[column], (int, float, str)) or row[column] is None):
            row[column] = str(row[column])
        worksheet.write(i+1, j, row[column])

workbook.close()
display(Markdown("# Download link:"), FileLink(filename))
