import vh
import json
import hashlib

#from alice.boltalka.valhalla.lib.toloka.select_replies_from_tops import select_replies_from_tops

boltalka_nature_evaluation_op = vh.op(id="58411b81-ef25-4147-b5fd-e2f0b6fef98a", allow_deprecated=False).partial(yt_table_outputs=['table'])
boltalka_properties_evaluation_op = vh.op(id="3f9ffb8a-0e37-44e6-9773-f910a9d51814", allow_deprecated=False).partial(yt_table_outputs=['table'])
make_toloka_input_op = vh.op(id="c0cde181-40a3-4c65-9822-71675919d530", allow_deprecated=False)

@vh.lazy.hardware_params(vh.HardwareParams(max_ram=10000, max_disk=10000))
@vh.lazy(
    inp=vh.mkinput(vh.JSONFile),
    output=vh.mkoutput(vh.JSONFile)
)
def add_toloka_keys(inp, output):
    js = []
    inp = json.load(open(inp))
    for el in inp:
        context = ["salt20200608", el["reply"].encode('utf-8')]
        for i in range(9):
            k = "context_{}".format(i)
            if k not in el:
                break
            context.append(el[k].encode('utf-8'))
        el["key"] = hashlib.md5("\t".join(reversed(context))).hexdigest()
        el["source"] = "model"
        js.append(el)
    with open(output, "w") as f:
        json.dump(js, f)
    return output


def make_toloka_input(tsv, context_len=9, unique=False, top_size=None, sample_top_size=None, salt='0'):
    return make_toloka_input_op(
        _inputs={
            'src': tsv,
        },
        _options={
            'yt-token': vh.get_yt_token_secret(),
            'mr-account': vh.get_mr_account(),
            'context-len': context_len,
            'unique-top': unique,
            'top-size': top_size,
            'sample-top-size': sample_top_size,
            'salt': salt,
        },
    )['dst_json']


def boltalka_nature_evaluation(json, priority, output_path, after, mr_account=None):
    return boltalka_nature_evaluation_op(
        _inputs={
            'tasks': json,
        },
        _options={
            'yt_token': vh.get_yt_token_secret(),
            'mr-account': vh.get_mr_account() if not mr_account else mr_account,
            'priority': priority,
            'output_table_path': output_path,
            'abc_id': 2738,
            'reuse': True,
            'confidence_level': 0
        },
        _after=after,
    )


def boltalka_properties_evaluation(json, priority, output_path, after, mr_account=None):
    return boltalka_properties_evaluation_op(
        _inputs={
            'tasks': json,
        },
        _options={
            'yt_token': vh.get_yt_token_secret(),
            'mr-account': vh.get_mr_account() if not mr_account else mr_account,
            'priority': priority,
            'output_table_path': output_path,
            'abc_id': 2738,
        },
        _after=after,
    )

def boltalka_interest_evaluation(input_table, output_table, after):
    return vh.op(id="b1638308-6e84-4933-9da7-0d38946ce3b8")(
        _inputs={
            'input_table': input_table,
        },
        _options={
            'yt-token': vh.get_yt_token_secret(),
            'encryptedOauthToken': 'liksna_token',
            'dst-table': output_table
        },
        _after=after,
    )