import json
import nirvana.dc_context as dc
from library.python import resource
from datetime import datetime
import vh3
from alice.quality.collect_marks_graph.graph import CollectMarksContext, build_graph


def main():
    params = dc.context().get_parameters()
    config = json.loads(resource.find(params['config-file']))
    with vh3.Profile(context_class=CollectMarksContext,
                     cache_sync=int(datetime.now().timestamp()),
                     timestamp=datetime.now().strftime('%d.%m.%Y %H:%M:%S'),
                     data_part=datetime.now().strftime('%Y-%m-%d'),
                     mr_account=params['mr-account'],
                     prod_url=config['prod-url'],
                     hitman_labels=config['hitman-labels'],
                     yt_token=params['yt-token'],
                     yt_pool=params.get('yt-pool', ''),
                     yql_token=params['yql-token'],
                     marks_path=params['marks-path'],
                     requests_path=params['requests-path'],
                     eval_path=params['eval-path']).build(vh3.WorkflowInstance) as wi:
        build_graph(scenario_flags=config['scenarios'],
                    skip_existing=params['skip-existing'])
    print(json.dumps(wi.to_nirvana_dump(), indent=2))


if __name__ == '__main__':
    main()
