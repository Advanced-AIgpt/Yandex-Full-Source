import pandas as pd
import sys
import click

@click.command()
@click.option('--true-negative', required=True, help='Table with true negative samples.')
@click.option('--false-positive', required=True, help='Table with false positive samples.')
def main(true_negative, false_positive):
    tn = pd.read_csv(true_negative, sep='\t')
    print('Expected positive:')
    for _, line in tn.iterrows():
        print('\t\t' + line['text'])

    fp = pd.read_csv(false_positive, sep='\t')
    fp.sort_values('intent')
    cur_intent = ''
    print('Expected negative:')
    for _, line in fp.iterrows():
        if line['intent'] != cur_intent:
            cur_intent = line['intent']
            print('\tIntent {}:'.format(cur_intent))
        print('\t\t' + line['text'])

if __name__ == '__main__':
    print(os.getcwd())
    main()
