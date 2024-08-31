from datetime import datetime, timedelta

from_date = '2020-02-16'
in_file_name = 'data/data_wt_date.csv'
out_file_name = 'data/data.csv'


def save_to_file(seqs, file_name):
    with open(file_name, "w") as file:
        start_from_date = datetime.strptime(from_date, '%Y-%m-%d').date()
        days = 0
        file.write("date,text\n")
        for x in seqs:
            file.write('"' + str(start_from_date + timedelta(days=days)) + '","' + x.strip() + '"\n')
            days = days + 1


def convert():
    with open(in_file_name, "r") as in_file:
        seqs = [line for line in in_file]

    save_to_file(seqs, out_file_name)


if __name__ == '__main__':
    convert()
