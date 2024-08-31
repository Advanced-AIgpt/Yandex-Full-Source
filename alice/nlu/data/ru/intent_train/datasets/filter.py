import logging
import click
import numpy as np
import json
import pandas as pd

@click.command()
@click.option(
    '--input',
    type=click.Path(exists=True),
    required=True,
    help="Path to input tsv file with intent column.")
@click.option(
    '--pattern',
    required=True,
    help="Substring in column, which we need to filter.")
@click.option(
    '--column-name',
    required=True,
    help="Column we need to filter in.")
@click.option(
    '--output',
    type=click.Path(),
    required=True,
    help="Path to file where to save lines of initial tsv file, with have subtring in column.")
def main(input, pattern, column_name, output):
    logging.basicConfig(level=logging.INFO, format=u'\u001b[35;1m[%(asctime)s]\u001b[36m [%(pathname)s:%(lineno)d] [%(levelname)s]\u001b[0m\n%(message)s\n')

    dataframe = pd.read_csv(input, sep="\t")

    try:
        dataframe.dropna(subset=[column_name], inplace=True)
        dataframe[column_name] = dataframe[column_name].astype(str)
        chosen = dataframe.loc[dataframe[column_name] == pattern]
        chosen.to_csv(output, sep='\t', index=False)
        print("chose {}/{}".format(len(chosen), len(dataframe)))
    except:
        logging.exception('target column in input dataset has some error. ')
        raise
