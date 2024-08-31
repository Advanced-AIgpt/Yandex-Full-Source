import argparse
import json

from vins_core.nlu.samples_extractor import SamplesExtractor
from vins_core.nlu.sample_processors.normalize import NormalizeSampleProcessor
from vins_core.nlu.sample_processors.wizard import WizardSampleProcessor


def read_input(input_path):
    with open(input_path) as input:
        data = json.load(input)
        return [request for request in data]


def write_result(result, output_path):
    with open(output_path, 'w') as output:
        json.dump(result, output)


def main():
    parser = argparse.ArgumentParser(description='Wizard mock data prepare tool')
    parser.add_argument('--source')
    parser.add_argument('--output')

    args = parser.parse_args()

    input = read_input(args.source)
    pipeline = [NormalizeSampleProcessor('normalizer_ru'), WizardSampleProcessor()]
    extractor = SamplesExtractor(pipeline=pipeline)
    result = {}
    for input_request in input:
        for sample in extractor(input_request):
            result[input_request] = sample.annotations['wizard'].to_dict() if 'wizard' in sample.annotations else None

    write_result(result, args.output)


if __name__ == "__main__":
    main()
