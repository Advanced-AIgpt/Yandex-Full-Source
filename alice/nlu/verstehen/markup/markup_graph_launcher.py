import vh


def vh_setup_main_thread():
    """
    vh needs to be set up in main thread in order to be used in other threads during requests
    """
    vh.setup_main_thread()


def process_markup_request(request):
    """
    Depending on the request and its parameters we will use different functions
    to launch all graphs. We assume that all these functions return URL of the
    Nirvana graph.
    """

    scenario = request['markup-scenario']
    if scenario == 'binary_classification':
        nirvana_url = launch_binary_classification_task_on_toloka_via_nirvana(
            input_data=request['input-data'],
            nirvana_token=request['nirvana-token'],
            toloka_token=request['toloka-secret'],
            yt_token=request['yt-secret'],
            ssh_key=request['ssh-secret'],
            yt_destination=request['binary-classification-yt-destination'],
            is_sandbox=request['is-sandbox'],
            project_name=request['binary-classification-toloka-project-name'],
            project_instructions=request['binary-classification-project-instructions'],
            task_question=request['binary-classification-task-question'],
            trainingset_size=request['binary-classification-advanced-trainingset-size'],
            training_passing_skill_value=request['binary-classification-advanced-training-passing-skill-value'],
            pool_overlap=request['binary-classification-advanced-overlap'],
            project_markup=request['binary-classification-advanced-custom-markup'],
            project_styles=request['binary-classification-advanced-custom-styles'],
            project_script=request['binary-classification-advanced-custom-script'],
            pool_task_time_length=request['binary-classification-advanced-task-time-length']
        )
    elif scenario == 'intent_classification':
        nirvana_url = launch_intent_markup(
            input_data=request['input-data'],
            nirvana_token=request['nirvana-token'],
            toloka_secret=request['toloka-secret'],
            yql_token=request['yql-secret'],
            yt_token=request['yt-secret'],
            yt_destination=request['intent-classification-yt-destination'],
            prj_key=request['intent-classification-prj-key']
        )
    elif scenario == 'tagging':
        nirvana_url = launch_tagging_task_on_toloka_via_nirvana(
            input_data=request['input-data'],
            nirvana_token=request['nirvana-token'],
            toloka_token=request['toloka-secret'],
            yt_token=request['yt-secret'],
            ssh_key=request['ssh-secret'],
            yt_destination=request['tagging-yt-destination'],
            is_sandbox=request['is-sandbox'],
            task_categories=request['tagging-task-categories'],
            goldenset=request['tagging-goldenset'],
            project_name=request['tagging-toloka-project-name'],
            project_instructions=request['tagging-project-instructions'],
            trainingset_size=request['tagging-advanced-trainingset-size'],
            training_passing_skill_value=request['tagging-advanced-training-passing-skill-value'],
            pool_overlap=request['tagging-advanced-overlap'],
            project_markup=request['tagging-advanced-custom-markup'],
            pool_task_time_length=request['tagging-advanced-task-time-length']
        )
    elif scenario == 'binary_classification_and_tagging':
        nirvana_url = launch_binary_classificatation_and_tagging_task_on_toloka_via_nirvana(
            input_data=request['input-data'],
            nirvana_token=request['nirvana-token'],
            toloka_token=request['toloka-secret'],
            yt_token=request['yt-secret'],
            ssh_key=request['ssh-secret'],
            yt_destination_classification=request['binary-classification-yt-destination'],
            is_sandbox=request['is-sandbox'],
            project_name_classification=request['binary-classification-toloka-project-name'],
            project_instructions_classification=request['binary-classification-project-instructions'],
            task_question=request['binary-classification-task-question'],
            trainingset_size_classification=request['binary-classification-advanced-trainingset-size'],
            training_passing_skill_value_classification=request[
                'binary-classification-advanced-training-passing-skill-value'],
            pool_overlap_classification=request['binary-classification-advanced-overlap'],
            project_markup=request['binary-classification-advanced-custom-markup'],
            project_styles=request['binary-classification-advanced-custom-styles'],
            project_script=request['binary-classification-advanced-custom-script'],
            pool_task_time_length_classification=request['binary-classification-advanced-task-time-length'],

            yt_destination_tagging=request['tagging-yt-destination'],
            task_categories=request['tagging-task-categories'],
            goldenset=request['tagging-goldenset'],
            project_name_tagging=request['tagging-toloka-project-name'],
            project_instructions_tagging=request['tagging-project-instructions'],
            trainingset_size_tagging=request['tagging-advanced-trainingset-size'],
            training_passing_skill_value_tagging=request['tagging-advanced-training-passing-skill-value'],
            pool_overlap_tagging=request['tagging-advanced-overlap'],
            project_markup_tagging=request['tagging-advanced-custom-markup'],
            pool_task_time_length_tagging=request['tagging-advanced-task-time-length']
        )
    else:
        raise ValueError('Cannot find markup scenario: `{}`'.format(scenario))
    return nirvana_url


def mr_upload_file_operation():
    return vh.op(name='MR Upload File', id='fe5be04f-5e6b-494a-8673-961535b036a3')


def launch_binary_classification_task_on_toloka_via_nirvana(
        input_data,
        nirvana_token,
        toloka_token,
        yt_token,
        ssh_key,
        yt_destination,
        is_sandbox,
        project_name,
        project_instructions,
        task_question,
        trainingset_size,
        training_passing_skill_value,
        pool_overlap,
        project_markup,
        project_styles,
        project_script,
        pool_task_time_length
):
    with vh.Graph() as g:
        # Preparing data
        data_input = vh.data_from_str(input_data)

        # Launching classification operation
        classification_operation = vh.op(name='Toloka Verstehen Text Binary Classification', owner='artemkorenev')
        json_output = classification_operation(
            data_input=data_input,
            is_sandbox=is_sandbox,
            toloka_token=toloka_token,
            ssh_key=ssh_key,
            task_question=task_question,
            trainingset_size=int(trainingset_size),
            training_passing_skill_value=int(training_passing_skill_value),
            pool_overlap=int(pool_overlap),
            pool_task_time_length=float(pool_task_time_length),
            project_name=project_name,
            project_instructions=project_instructions,
            project_markup=project_markup,
            project_script=project_script,
            project_styles=project_styles,
        )

        # Uploading the result
        mr_upload_file_operation()(
            content=json_output,
            yt_token=yt_token,
            mr_default_cluster='hahn',
            dst_path=yt_destination,
            write_mode='OVERWRITE'
        )

    # Running graph
    info = vh.run(g, oauth_token=nirvana_token).get_workflow_info()
    return 'https://nirvana.yandex-team.ru/flow/{}'.format(info.workflow_id)


def launch_intent_markup(
        input_data,
        nirvana_token,
        toloka_secret,
        yql_token,
        yt_token,
        yt_destination,
        prj_key
):
    with vh.Graph() as g:
        # Preparing data
        data_input = vh.data_from_str(input_data)

        # Launching classification operation
        markup_operation = vh.op(name='Toloka Verstehen Text Intent Classification', owner='artemkorenev')
        mr_table_output = markup_operation(
            data_input=data_input,
            toloka_token=toloka_secret,
            yql_token=yql_token,
            yt_token=yt_token,
            prj_key=prj_key,
        )

        # Uploading the result
        mr_move_table_operation = vh.op(name='MR Move Table', id='83f0cf88-63d9-11e6-a050-3c970e24a776')
        mr_move_table_operation(src=mr_table_output, moved=yt_destination, yt_token=yt_token)

    # Running graph
    info = vh.run(g, oauth_token=nirvana_token).get_workflow_info()
    return 'https://nirvana.yandex-team.ru/flow/{}'.format(info.workflow_id)


def launch_tagging_task_on_toloka_via_nirvana(
        input_data,
        nirvana_token,
        toloka_token,
        yt_token,
        ssh_key,
        yt_destination,
        is_sandbox,
        task_categories,
        goldenset,
        project_name,
        project_instructions,
        trainingset_size,
        training_passing_skill_value,
        pool_overlap,
        project_markup,
        pool_task_time_length
):
    with vh.Graph() as g:
        # Preparing data
        data_input = vh.data_from_str(input_data)

        if goldenset.isdigit():
            # Launching tagging operation
            tagging_operation = vh.op(name='Toloka Verstehen Text Tagging', owner='artemkorenev')
            json_output = tagging_operation(
                data_input=data_input,
                is_sandbox=is_sandbox,
                toloka_token=toloka_token,
                ssh_key=ssh_key,
                task_categories=task_categories,
                goldenset_size=int(goldenset),
                trainingset_size=int(trainingset_size),
                training_passing_skill_value=int(training_passing_skill_value),
                pool_overlap=int(pool_overlap),
                pool_task_time_length=float(pool_task_time_length),
                project_name=project_name,
                project_instructions=project_instructions,
                project_markup=project_markup
            )
        else:
            # Preparing goldenset data
            goldenset_input = vh.data_from_str(goldenset)

            # Launching tagging operation
            tagging_operation = vh.op(name='Toloka Verstehen Text Tagging with Honeypots', owner='artemkorenev')
            json_output = tagging_operation(
                data_input=data_input,
                goldenset_input=goldenset_input,
                is_sandbox=is_sandbox,
                toloka_token=toloka_token,
                ssh_key=ssh_key,
                task_categories=task_categories,
                trainingset_size=int(trainingset_size),
                training_passing_skill_value=int(training_passing_skill_value),
                pool_overlap=int(pool_overlap),
                pool_task_time_length=float(pool_task_time_length),
                project_name=project_name,
                project_instructions=project_instructions,
                project_markup=project_markup
            )

        # Uploading the result
        mr_upload_file_operation()(
            content=json_output,
            yt_token=yt_token,
            mr_default_cluster='hahn',
            dst_path=yt_destination,
            write_mode='OVERWRITE'
        )

    # Running graph
    info = vh.run(g, oauth_token=nirvana_token).get_workflow_info()
    return 'https://nirvana.yandex-team.ru/flow/{}'.format(info.workflow_id)


def launch_binary_classificatation_and_tagging_task_on_toloka_via_nirvana(
        input_data,
        nirvana_token,
        toloka_token,
        yt_token,
        ssh_key,
        # classification part
        yt_destination_classification,
        is_sandbox,
        project_name_classification,
        project_instructions_classification,
        task_question,
        trainingset_size_classification,
        training_passing_skill_value_classification,
        pool_overlap_classification,
        project_markup,
        project_styles,
        project_script,
        pool_task_time_length_classification,
        yt_destination_tagging,
        # tagging part
        task_categories,
        goldenset,
        project_name_tagging,
        project_instructions_tagging,
        trainingset_size_tagging,
        training_passing_skill_value_tagging,
        pool_overlap_tagging,
        project_markup_tagging,
        pool_task_time_length_tagging
):
    with vh.Graph() as g:
        # Preparing data
        data_input = vh.data_from_str(input_data)

        # Launching classification operation
        classification_operation = vh.op(name='Toloka Verstehen Text Binary Classification', owner='artemkorenev')
        classification_json_output = classification_operation(
            data_input=data_input,
            is_sandbox=is_sandbox,
            toloka_token=toloka_token,
            ssh_key=ssh_key,
            task_question=task_question,
            trainingset_size=int(trainingset_size_classification),
            training_passing_skill_value=int(training_passing_skill_value_classification),
            pool_overlap=int(pool_overlap_classification),
            pool_task_time_length=float(pool_task_time_length_classification),
            project_name=project_name_classification,
            project_instructions=project_instructions_classification,
            project_markup=project_markup,
            project_script=project_script,
            project_styles=project_styles,
        )

        # Uploading the result
        upload_result = mr_upload_file_operation()(
            content=classification_json_output,
            yt_token=yt_token,
            mr_default_cluster='hahn',
            dst_path=yt_destination_classification,
            write_mode='OVERWRITE'
        )

        # Launching filtering operation
        filtering_operation = vh.op(name='Filter positives and prepare for tagging', owner='artemkorenev')
        filtered_data = filtering_operation(
            data=classification_json_output, _after=[upload_result]
        )

        if goldenset.isdigit():
            tagging_operation = vh.op(name='Toloka Verstehen Text Tagging', owner='artemkorenev')
            tagging_json_output = tagging_operation(
                data_input=filtered_data,
                is_sandbox=is_sandbox,
                toloka_token=toloka_token,
                ssh_key=ssh_key,
                task_categories=task_categories,
                goldenset_size=int(goldenset),
                trainingset_size=int(trainingset_size_tagging),
                training_passing_skill_value=int(training_passing_skill_value_tagging),
                pool_overlap=int(pool_overlap_tagging),
                pool_task_time_length=float(pool_task_time_length_tagging),
                project_name=project_name_tagging,
                project_instructions=project_instructions_tagging,
                project_markup=project_markup_tagging
            )
        else:
            # Preparing goldenset data
            goldenset_input = vh.data_from_str(goldenset)

            # Launching tagging operation
            tagging_operation = vh.op(name='Toloka Verstehen Text Tagging with Honeypots', owner='artemkorenev')
            tagging_json_output = tagging_operation(
                data_input=filtered_data,
                goldenset_input=goldenset_input,
                is_sandbox=is_sandbox,
                toloka_token=toloka_token,
                ssh_key=ssh_key,
                task_categories=task_categories,
                trainingset_size=int(trainingset_size_tagging),
                training_passing_skill_value=int(training_passing_skill_value_tagging),
                pool_overlap=int(pool_overlap_tagging),
                pool_task_time_length=float(pool_task_time_length_tagging),
                project_name=project_name_tagging,
                project_instructions=project_instructions_tagging,
                project_markup=project_markup_tagging
            )

        # Uploading the result
        mr_upload_file_operation()(
            content=tagging_json_output,
            yt_token=yt_token,
            mr_default_cluster='hahn',
            dst_path=yt_destination_tagging,
            write_mode='OVERWRITE'
        )

    # Running graph
    info = vh.run(g, oauth_token=nirvana_token).get_workflow_info()
    return 'https://nirvana.yandex-team.ru/flow/{}'.format(info.workflow_id)
