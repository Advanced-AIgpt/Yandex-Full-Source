import collections
import json
import nirvana.job_context as nv


def main():
    data = {}
    for input_file in nv.context().get_inputs().get_list('infiles'):
        with open(input_file) as f:
            data = {'slices': f.read()}

    with open(nv.context().get_outputs().get('out_json'), 'w') as f:
        json.dump(data, f)


main()
