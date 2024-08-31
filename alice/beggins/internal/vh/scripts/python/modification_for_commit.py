import tarfile
import textwrap
import os.path


class ArchiveModifier:
    """
    Class for archive modification
    """

    def modify(self, archive: str, context=None):
        """
        1. Extracts the specified archive to the storage
        2. Modifies the storage
        3. Archives the storage to the specified archive
        """
        storage = self.__generate_storage_name()
        self.extract(archive, storage)

        for file_modifier in [
            DataIncModifier(open(os.path.join(storage, 'models/alice_binary_intent_classifier/data_dev.inc'), 'r+')),
            FramesPbTxtModifier(open(os.path.join(storage, 'dev/frames.pb.txt'), 'r+')),
        ]:
            file_modifier.modify(context)
            file_modifier.file.close()

        self.archive(storage, archive)

    @staticmethod
    def extract(archive: str, storage: str):
        """
        Extracts the specified archive to the storage
        """
        with tarfile.open(archive) as tar:
            tar.extractall(storage)

    @staticmethod
    def archive(storage: str, archive: str):
        """
        Archives the storage to the specified archive
        """
        with tarfile.open(archive, 'w:gz') as tar:
            for unit in os.listdir(storage):
                tar.add(os.path.join(storage, unit), unit)

    @staticmethod
    def __generate_storage_name():
        result = 'storage'
        while os.path.exists(result):
            result = result + '1'
        return result


class AbstractFileModifier:
    """
    Abstract class for file modification
    """

    def __init__(self, file):
        self.file = file
        self.content = None

    def modify(self, context):
        """
        Modifies file using `context`
        """
        self.file.seek(0)
        self.content = self.file.read()

        data = self.generate_data(context)
        for modification_method in self.get_modification_methods():
            modification_method(data)

        self.file.seek(0)
        self.file.write(self.content)
        self.file.truncate()
        self.file.seek(0)

    def get_modification_methods(self):
        """
        Returns list of modification method
        They will be applied sequentially
        """
        raise NotImplementedError()

    def generate_data(self, context):
        """
        Generate data for this modifier using collective context
        """
        raise NotImplementedError()


class DataIncModifier(AbstractFileModifier):
    def get_modification_methods(self):
        return [self.__add_from_sandbox_part, self.__add_set_part]

    def generate_data(self, context):
        model_name = context['model_name']
        classifier_name = ''.join([part.title() for part in model_name.split('_')])
        resource_id = context['resource_id']
        embedder = context['embedder']
        process_id = context['process_id']

        begemot_classifier_name = f'Alice{embedder}{classifier_name}__{process_id}'

        from_sandbox = textwrap.dedent(f'''
            FROM_SANDBOX(
                {resource_id}
                RENAME
                {model_name}/model.cbm
                OUT_NOAUTO
                ${{PREFIX}}{embedder.lower()}_models/{begemot_classifier_name}/model.cbm
            )
            ''')

        model_path = textwrap.dedent(f'${{PREFIX}}{embedder.lower()}_models/{begemot_classifier_name}/model.cbm')

        return {'from_sandbox': from_sandbox, 'model_path': model_path}

    def __add_from_sandbox_part(self, data):
        from_sandbox = data['from_sandbox'].strip()
        entities = self.content.split('\n\n')
        entities.insert(-1, from_sandbox)
        self.content = '\n\n'.join(entities)

    def __add_set_part(self, data):
        entities = self.content.split('\n\n')

        model_path = data['model_path']
        set_entity = entities[-1]
        set_lines = set_entity.split('\n')
        set_lines.insert(-2, f'    {model_path}')
        new_set_entity = '\n'.join(set_lines)

        entities[-1] = new_set_entity
        self.content = '\n\n'.join(entities)


class FramesPbTxtModifier(AbstractFileModifier):
    def get_modification_methods(self):
        return [self.__add_new_frame]

    def generate_data(self, context):
        model_name = context['model_name']
        classifier_name = ''.join([part.title() for part in model_name.split('_')])
        threshold = context['threshold']
        embedder = context['embedder']
        process_id = context['process_id']

        begemot_classifier_name = f'Alice{embedder}{classifier_name}__{process_id}'
        frame_name = f'alice.beggins_{model_name}__{process_id}'
        experiment = f'bg_beggins_{begemot_classifier_name}'

        frame_info = textwrap.dedent(f'''
            {{
              Name: "{frame_name}"
              Rules: [{{
                Experiments: ["{experiment}"]
                Classifier: {{
                  Source: "AliceBinaryIntentClassifier"
                  Intent: "{begemot_classifier_name}"
                  Threshold: {threshold}
                  Confidence: 1
                }}
                Tagger: {{
                  Source: "Always"
                }}
              }}]
            }}
            ''')

        def shift(text: str):
            return '  ' + '  '.join(text.strip().splitlines(True))

        return {'frame_info': shift(frame_info)}

    def __add_new_frame(self, data):
        frame_info = data['frame_info'].split('\n')

        lines = self.content.split('\n')
        new_content = lines[:-3] + ['  },'] + frame_info + [']']
        self.content = '\n'.join(new_content) + '\n'


def commit_draft(archive, model_meta_info):
    archive_modifier = ArchiveModifier()
    archive_modifier.modify(archive, model_meta_info)

    return archive
