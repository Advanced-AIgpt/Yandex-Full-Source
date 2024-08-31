import argparse
import json

from ml_data_reader.iterators import round_robin, gather
from tfnn.ops import mpi
from tfnn.task.cls.tools.score import load_model_and_problem, configure_parser, create_data_iter, score


def main():
    parser = configure_parser(argparse.ArgumentParser())
    parser.add_argument('--out-file', required=True)
    args = parser.parse_args()
    mpi.set_provider(args.mpi_provider)

    # Score
    with mpi.Session(gpu_group=mpi.local_rank()) as sess:
        model, problem = load_model_and_problem(args)
        data = create_data_iter(args, problem) if mpi.is_master() else None

        results = []
        i = 0
        for scores in gather(score(model, sess, round_robin(data))):
            for s in scores:
                results.append({
                    'id': i,
                    'scores': s
                })
                print('id: {}, scores: {}'.format(i, str(s)))
                i += 1
        with open(args.out_file, 'w') as f:
            json.dump(results, f)


if __name__ == '__main__':
    main()
