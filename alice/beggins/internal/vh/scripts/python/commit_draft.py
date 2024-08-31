def commit_draft(v, w, x):
    import textwrap

    model_name = v['model_name']
    classifier_name = ''.join([part.title() for part in model_name.split('_')])
    threshold = v['threshold']
    resource_id = v['resource_id']
    embedder = v['embedder']

    data_manifest = '\n'.join(w)

    toloka_manifest = []
    for option_name, option_value in x.items():
        toloka_manifest.append(f'{option_name}: |')
        toloka_manifest.append(textwrap.indent(option_value, '  '))
        toloka_manifest.append('')
    toloka_manifest = '\n'.join(toloka_manifest)

    new_line_for_dedent = """
        """

    def prepare_for_dedent(text):
        return text.replace('\n', new_line_for_dedent)

    frames_pb_txt = textwrap.dedent(f"""
        #################################################
        # Add to alice/nlu/data/ru/config/frames.pb.txt #
        #################################################

        {{
            Name: "<your frame name>"
            Rules: [{{
                Experiments: ["bg_beggins_{model_name}"]
                Classifier: {{
                    Source: "AliceBinaryIntentClassifier"
                    Intent: "Alice{embedder}{classifier_name}"
                    Threshold: {threshold}
                    Confidence: 1
                }}
                Tagger: {{
                    Source: "Always"
                }}
            }}]
        }}

        #################################################
        """)

    model_data_middle = textwrap.dedent(f"""
        ###########################################################################
        # Add to alice/nlu/data/ru/models/alice_binary_intent_classifier/data.inc #
        ###########################################################################

        FROM_SANDBOX(
            {resource_id}
            RENAME
            {model_name}/model.cbm
            OUT_NOAUTO
            ${{PREFIX}}{embedder.lower()}_models/Alice{embedder}{classifier_name}/model.cbm
        )

        ###########################################################################

        """)

    model_data_bottom = textwrap.dedent(f"""
        ###########################################################################
        # Add to alice/nlu/data/ru/models/alice_binary_intent_classifier/data.inc #
        # ~~~~~~~~~~~~~~~~~~~~~~~ to the bottom of file ~~~~~~~~~~~~~~~~~~~~~~~~~~#
        ###########################################################################

        ${{PREFIX}}{embedder.lower()}_models/Alice{embedder}{classifier_name}/model.cbm

        ###########################################################################

        """)

    manifest_yaml = textwrap.dedent(f"""
        ###########################################################################
        # Add to alice/beggins/data/classification/{model_name}/manifest.yaml #
        ###########################################################################

        {prepare_for_dedent(data_manifest)}
        {prepare_for_dedent(toloka_manifest)}

        ###########################################################################

        """)

    return [
        frames_pb_txt,
        model_data_middle,
        model_data_bottom,
        manifest_yaml,
    ]
