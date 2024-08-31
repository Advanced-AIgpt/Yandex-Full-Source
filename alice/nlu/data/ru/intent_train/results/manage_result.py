import logging
import click
import pandas as pd


class MetricsCounter:
    def __init__(self, y_true, y_pred, weights=1.):
        self.tp = 0.
        self.fp = 0.
        self.fn = 0.

        assert y_true.shape == y_pred.shape
        self.tp += ((y_pred == 1) * (y_true == 1) * weights).sum()
        self.fp += ((y_pred == 1) * (y_true == 0) * weights).sum()
        self.fn += ((y_pred == 0) * (y_true == 1) * weights).sum()

    @property
    def precision(self):
        return self.tp / max(1, self.tp + self.fp)

    @property
    def recall(self):
        return self.tp / max(1, self.tp + self.fn)

    @property
    def f1(self):
        return 2 * self.precision * self.recall / max(1, self.precision + self.recall)


@click.command()
@click.option(
    '--input',
    type=click.Path(exists=True),
    required=True,
    help="Path to file with tsv results.")
@click.option(
    '--output',
    type=click.Path(),
    required=True,
    help="Path where to save report.")
@click.option(
    '--pool',
    type=click.Path(),
    help="Path to file with pool of samples (to extract names of intents).")
@click.option(
    '--additional', 
    '-a',
    multiple=True,
    help="Path to file with pool of samples (to extract names of intents).")
def main(input, output, pool, additional):
    logging.basicConfig(level=logging.INFO, format=u'\u001b[35;1m[%(asctime)s]\u001b[36m [%(pathname)s:%(lineno)d] [%(levelname)s]\u001b[0m\n%(message)s\n')

    dataframe = pd.read_csv(input, sep="\t")

    dataframe['predict'] = dataframe['predicted'] >= 0.5

    with open(output, 'w') as output_file:
        output_file.write('Dataset of size {} with {} positives and {} negatives\n\n'.format(
            len(dataframe),
            len(dataframe[dataframe['real'] == 1]),
            len(dataframe[dataframe['real'] == 0])
            ))

        if pool is not None:
            pool_df = pd.read_csv(pool, sep="\t")[['text', *additional]].drop_duplicates(subset='text')
            dataframe = dataframe.merge(pool_df, left_on='text', right_on='text', how='left')

        counter = MetricsCounter(dataframe['real'].to_numpy(), dataframe['predict'].to_numpy())

        output_file.write('precision: %3.2f | recall: %3.2f |  f1: %3.2f\n\n' % (
            counter.precision, counter.recall, counter.f1
        ))

        false_positives = dataframe[(dataframe['real'] == 0) & (dataframe['predict'] == 1)]

        output_file.write('False positives (sample, predicted probability and additional info {}):\n'.format(
            additional))

        for _, row in false_positives.iterrows():
            if pool is None:
                output_file.write('\t{}\t{}\n'.format(row['text'], row['predicted']))
            else:
                output_file.write('\t{}\t{}\t'.format(row['text'], row['predicted']))
                for addon in additional:
                    output_file.write('{}\t'.format(row[addon]))
                output_file.write('\n')
        
        false_negatives = dataframe[(dataframe['real'] == 1) & (dataframe['predict'] == 0)]

        output_file.write('\nFalse negatives (sample, predicted probability and additional info {}):\n'.format(
            additional))

        for _, row in false_negatives.iterrows():
            if pool is None:
                output_file.write('\t{}\t{}\n'.format(row['text'], row['predicted']))
            else:
                output_file.write('\t{}\t{}\t'.format(row['text'], row['predicted']))
                for addon in additional:
                    output_file.write('{}\t'.format(row[addon]))
                output_file.write('\n')
