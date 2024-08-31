from datetime import timedelta, date
from os.path import join
from pulsar import PulsarClient
from library.python.vault_client.instances import Production as VaultClient


pulsar_token = 'robot-voice-qa-pulsar_token'
pulsar_token_uuid = 'sec-01enn7se31qk4f4tj9m8nzby5z'

MODEL_DATASET_MAP = {
    "ue2e_quasar_accept": {
        "regular_prod": "ue2e_quasar/results",
        "regular_hamster": "ue2e_quasar/results_hamster"
    },
    "ue2e_quasar_fairytales_accept": {
        "regular_prod": "e2e_quasar_fairytale/results",
        "regular_hamster": "e2e_quasar_fairytale/results_hamster"
    },
    "ue2e_navi_auto_accept": {
        "regular_prod": "e2e_navi_auto/results",
        "regular_hamster": "e2e_navi_auto/results_hamster"
    },
    "ue2e_translate_accept": {
        "regular_prod": "ue2e_translate/results",
        "regular_hamster": "ue2e_translate/results_hamster"
    }
}


# taken from: https://a.yandex-team.ru/arc/trunk/arcadia/ads/quality/adv_machine/tsar/tools/format_transport/__main__.py?rev=r7631102#L305-311
def get_token_from_vault(secret_uuid):
    client = VaultClient(decode_files=True)
    secret = client.get_secret(secret_uuid)
    versions = secret['secret_versions']
    latest_version_uuid = max(versions, key=lambda x : x['created_at'])['version']
    latest_version = client.get_version(latest_version_uuid)
    return latest_version['value']['secret']


def daterange(start_date, end_date):
    for n in range(int((end_date - start_date).days)):
        yield start_date + timedelta(n)


def main():
    start_date = date(2020, 4, 2)
    end_date = date(2020, 4, 3)
    not_updated_instances = []
    path_prefix = "yt://hahn/home/alice/toloka/accept"
    client = PulsarClient(token=get_token_from_vault(pulsar_token_uuid))
    for single_date in daterange(start_date, end_date):
        str_date = single_date.strftime("%Y-%m-%d")
        print(str_date)

        instances = client.find(
            model_names=['regular_prod_%s' % str_date, 'regular_hamster_%s' % str_date],
            dataset_names=['ue2e_quasar_fairytales_accept']
        )
        print("instances:", instances)
        for instance in instances:
            model_output_url = instance.model_output_url
            username = instance.username
            print("current_url:", model_output_url)
            instance.model_output_url = join(path_prefix, MODEL_DATASET_MAP[instance.dataset.name]['_'.join(instance.model.name.split('_')[:2])], str_date)
            print("new url:", instance.model_output_url)
            if username == "robot-voice-qa":
                try:
                    client.update(instance)
                except Exception:
                    not_updated_instances.append(instance)
            else:
                not_updated_instances.append(instance)

    print("could not update:", not_updated_instances)


if __name__ == '__main__':
    main()
