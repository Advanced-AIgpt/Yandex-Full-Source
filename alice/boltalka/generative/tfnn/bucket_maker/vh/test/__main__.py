import vh

from alice.boltalka.generative.tfnn.bucket_maker.vh import generate_bucket_op


def main():
    contexts_table = vh.YTTable('//home/voice/artemkorenev/buckets/bucket_toloka_test_t_0.80_topk_10')
    model_archive = vh.op(name='Get sandbox resource', id='20004369-27de-11e6-a29e-0025909427cc')(resource_id='1320505815')
    output_table = vh.YTTable('//home/voice/artemkorenev/TEST_VALHALLA')

    generate_bucket_op(contexts_table, model_archive, output_table)

    vh.run(arcadia_root='/home/artemkorenev/code/arcadia', yt_token_secret='artemkorenev_yt_token', yt_proxy='hahn')


if __name__ == '__main__':
    main()
