# -*- coding: utf-8 -*-

import click
import pandas as pd
import json

from itertools import izip
from tqdm import tqdm
from dataset import VinsDataset


@click.command()
@click.option('--data-path', type=click.Path(exists=True))
@click.option('--out-path', type=click.Path())
def main(data_path, out_path):
    dataset = VinsDataset.restore(data_path)
    data = []
    for ind, (intent, text, info) in tqdm(
        enumerate(izip(dataset._intents, dataset._preprocessed_texts, dataset._additional_infos)),
        total=len(dataset)
    ):
        data.append((text, intent, json.dumps(info.raw_factors_data)))

    pd.DataFrame(data, columns=['text', 'intent', 'raw_factors']).to_csv(out_path, sep='\t', encoding='utf-8')


if __name__ == '__main__':
    main()
