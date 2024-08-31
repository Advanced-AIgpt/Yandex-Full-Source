from library.python.vault_client.instances import Production as VaultClient
import argparse
import json


if __name__ == '__main__':
    # parse arguments
    parser = argparse.ArgumentParser(description='I will give you the secrets!')
    parser.add_argument('--token', dest='token', required=True)
    parser.add_argument('--key', dest='key', required=True)
    args = parser.parse_args()

    # get secrets
    kwargs = {'authorization': 'OAuth ' + args.token}
    client = VaultClient(**kwargs)
    secrets = client.get_version(args.key)['value']

    print(json.dumps(secrets))
