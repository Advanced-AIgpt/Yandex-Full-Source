import click
import pandas as pd


@click.group()
def main():
    pass


@main.command()
@click.argument('input_file', type=click.Path())
@click.argument('output_file', type=click.Path())
@click.argument('lines', default=100)
@click.option('--seed', default=42)
@click.option('--no-header', is_flag=True)
def sample(input_file, output_file, lines, seed, no_header):
    header = None if no_header else 'infer'
    data = pd.read_csv(input_file, sep='\t', encoding='utf-8', header=header)
    data.sample(lines, random_state=seed).to_csv(output_file, index=None, encoding='utf-8', header=header, sep='\t')


if __name__ == '__main__':
    main(obj={})
