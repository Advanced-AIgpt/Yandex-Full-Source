from alice.boltalka.telegram_bot.lib.rpc_source import RpcSource


if __name__ == '__main__':
    source = RpcSource()
    for i in range(50):
        print(source.get_candidates({
            'addr': 'http://localhost:5555',
            'source_type': 'TfnnModelSource',
            'model_name': 'after_pretrain_md_all_empty_ctx',
            'entity_id': 258687
        }, ['какие впечатления о фильме?']))
