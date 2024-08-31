import os
import json
import vh3
import argparse
from alice.quality.collect_marks_graph.graph import CollectMarksContext, build_graph


def main():
    argument_parser = argparse.ArgumentParser()
    argument_parser.add_argument(
        '--config', required=True, help='Config for graph options')
    argument_parser.add_argument(
        '--profile', required=False, help='User profile, containing workflow, tokens and paths')
    args = argument_parser.parse_args()
    config = json.load(open(args.config))
    profile = json.load(open(args.profile or os.path.expanduser('~/.collect_marks_graph/profile.json')))

    with vh3.Profile(CollectMarksContext,
                     workflow=vh3.Workflow(id=profile['workflow']['id']),
                     cache_sync=profile['cache-sync'],
                     timestamp=profile['timestamp'],
                     data_part=profile['data-part'],
                     mr_account=profile['mr-account'],
                     prod_url=config['prod-url'],
                     hitman_labels=config['hitman-labels'],
                     yt_token=profile['yt-token'],
                     yql_token=profile['yql-token'],
                     marks_path=profile['marks-path'],
                     requests_path=profile['requests-path'],
                     eval_path=profile['eval-path'],
                     yt_pool=profile.get('yt-pool', '')
                     ).build(vh3.WorkflowInstance) as wi:
        build_graph(scenario_flags=config['scenarios'],
                    skip_existing=profile['skip-existing'])
    wi.run(start=False)


if __name__ == '__main__':
    main()
