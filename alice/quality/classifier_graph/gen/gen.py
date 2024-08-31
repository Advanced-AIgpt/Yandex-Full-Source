import argparse
import json
import nirvana.dc_context as dc
from library.python import resource
from datetime import datetime
import vh3
from alice.quality.classifier_graph.graph import ClassifierGraphContext, build_graph


def main():
    # Default parameters are used for dc launches
    argument_parser = argparse.ArgumentParser()
    argument_parser.add_argument(
        '--config', required=False, help='Config, containing dynamic options for graph generation')
    argument_parser.add_argument(
        '--profile', required=False, help='User profile, containing individual settings for this graph')
    argument_parser.add_argument(
        '--create', required=False, action='store_const', const=True, help='Create graph instance')
    argument_parser.add_argument(
        '--workflow', required=False, help='Target workflow id')
    args = argument_parser.parse_args()
    if args.profile:
        params = json.load(open(args.profile))
        entities = params['entites']
    else:
        params = dc.context().get_parameters()
        entities = dc.context().get_entities()

    if args.config:
        config = json.load(open(args.config))
    else:
        config = json.loads(resource.find(params['config_file']))

    workflow_id = args.workflow
    assert args.create is None or workflow_id is not None
    learn_features = entities[params['learn_features']].get_data()
    learn_marks_requests = entities[learn_features['marks-requests']].get_data()
    basket_features = entities[params['basket_features']].get_data()
    basket_marks_requests = entities[basket_features['marks-requests']].get_data()

    if params['use_filtered_features_for_basket']:
        basket_features = basket_features['features-filtered-path']
    else:
        basket_features = basket_features['features-path']

    dataset_name = entities[learn_features['marks-requests']].get_name().replace(' ', '_')

    config_overrides_list = ['force_zero_factors_pre', 'force_zero_factors_post']
    for option in config_overrides_list:
        if params.get(option) is None:
            params[option] = config[option]

    optional_args = {}
    optional_args_list = ['build_target_patch', 'build_target_additional_flags', 'eval_features']
    for option in optional_args_list:
        if params.get(option) is not None:
            optional_args[option] = params[option]
    with vh3.Profile(ClassifierGraphContext,
                     workflow=vh3.Workflow(id=workflow_id),
                     timestamp_for_pulsar=str(int(datetime.now().timestamp())),
                     timestamp_for_training=datetime.now().strftime('%Y.%m.%d %H:%M:%S'),
                     timestamp_for_data_preparation=str(int(datetime.now().timestamp())),
                     arcadia_revision=params['arcadia_revision'],
                     build_target_revision=config['build_target_revision'],
                     basket_eval_folder=basket_marks_requests['eval-path'],
                     basket_eval_date=basket_marks_requests['last-eval-date'],
                     test_data_requests=basket_marks_requests['requests-path'],
                     test_data_marks=basket_marks_requests['marks-path'],
                     train_data_marks=learn_marks_requests['marks-path'],
                     learn_features=learn_features['features-path'],
                     test_features=basket_features,
                     slices=learn_features['slices-path'],
                     name=params['name'],
                     tags=params['tags'],
                     force_zero_factors_pre=params['force_zero_factors_pre'],
                     force_zero_factors_post=params['force_zero_factors_post'],
                     model_name=config['model_name'],
                     dataset_name=dataset_name,
                     iterations=params['iterations'],
                     mr_account=params['mr_account'],
                     mr_output_ttl=params['mr_output_ttl'],
                     yt_token=params['yt_token'],
                     yt_cluster='hahn',
                     yql_token=params['yql_token'],
                     sandbox_token=params['sandbox_token'],
                     pulsar_token=params['pulsar_token'],
                     client_type=config['client_type'],
                     create_resource=params['create_resource'],
                     **optional_args).build(vh3.WorkflowInstance) as wi:
        build_graph(scenarios=config['scenarios'],
                    eval_features=params.get('eval_features'),
                    scenarios_confident=config['scenarios_confident'],
                    scenarios_recall_precision=config['scenarios_recall_precision'],
                    build_target_options=config['build_target_options'],
                    train_full_factors_postclassifier=params['train_full_factors_postclassifier'])
    if args.create:
        wi.run(start=False)
    else:
        print(json.dumps(wi.to_nirvana_dump(), indent=2))


if __name__ == '__main__':
    main()
