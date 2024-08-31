import vh


def get_yt_table(path):
    return vh.YTTable(path)


def get_yt_file(path):
    return vh.get_yt_file(path)


def get_table(table, cluster='hahn', creation_mode='CHECK_EXISTS', yt_token=vh.get_yt_token_secret()):
    return vh.op(id='6ef6b6f1-30c4-4115-b98c-1ca323b50ac0')(
        cluster=cluster,
        table=table,
        creationMode=creation_mode,
        yt_token=yt_token,
        yt_table_outputs=['outTable']
    )['outTable']


def get_file(file, cluster='hahn', creation_mode='CHECK_EXISTS', yt_token=vh.get_yt_token_secret()):
    return vh.op(id='ec4a8561-87ff-4095-afd5-3ca4f6226ea8')(
        cluster=cluster,
        _inputs={
            'path': file
        },
        creationMode=creation_mode,
        yt_token=yt_token,
    )['mr_file']


def get_directory(path, cluster='hahn', creation_mode='CHECK_EXISTS', yt_token=vh.get_yt_token_secret()):
    return vh.op(id='eec37746-6363-42c6-9aa9-2bfebedeca60', name='Get MR Directory')(
        cluster=cluster,
        _options={
            'path': path
        },
        creationMode=creation_mode,
        yt_token=yt_token,
    )['mr_directory']


def download_file(file, cluster='hahn', creation_mode='CHECK_EXISTS', yt_token=vh.get_yt_token_secret()):
    mr_file = get_file(file, cluster=cluster, creation_mode=creation_mode, yt_token=yt_token)
    return vh.op(id='531cb9d2-9234-440e-a16a-c7e06e704021')(
        file=mr_file,
        yt_token=yt_token,
    )['content']


def merge_tables(tables, merge_mode='AUTO', yt_token=vh.get_yt_token_secret()):
    return vh.op(id='66fdd3dc-23a2-11e7-ac34-3c970e24a776')(
        srcs=tables,
        merge_mode=merge_mode,
        yt_token=yt_token,
        yt_table_outputs=['merged']
    )['merged']


def mr_shuffle(table, mr_account='tmp', yt_token=vh.get_yt_token_secret()):
    return vh.op(id='052653a2-e98b-4765-b58a-f801b04fe4b3', name='MR Shuffle')(
        input_table=table,
        mr_account=mr_account,
        yt_token=yt_token,
        yt_table_outputs=['output_table']
    )['output_table']


def train_test_split_by_column(table, test_size, column, mr_account='tmp', yt_token=vh.get_yt_token_secret()):
    x = mr_shuffle(table, mr_account=mr_account, yt_token=yt_token)
    x = vh.op(id='cd0f24f8-6ad5-4d39-85ad-9d18ffc96ebd', name='MR table split by column value')(
        input=x,
        ratio=1. - test_size,
        seed=42,  # shuffle is not seeded, making seed option outside train_test_split is useless then
        column=column,
        yt_token=yt_token,
        mr_account=mr_account,
        yt_table_outputs=['output1', 'output2']
    )
    return x['output1'], x['output2']


def train_test_split(table, test_size, mr_account='tmp', yt_token=vh.get_yt_token_secret()):
    x = vh.op(id='052653a2-e98b-4765-b58a-f801b04fe4b3', name='MR Shuffle')(
        input_table=table,
        mr_account=mr_account,
        yt_token=yt_token,
        yt_table_outputs=['output_table']
    )['output_table']
    x = vh.op(id='aab6a69b-794b-428a-afd9-5df83c28233e', name='MR table split')(
        input=x,
        ratio=1. - test_size,
        seed=42,  # shuffle is not seeded, making seed option outside train_test_split is useless then
        yt_token=yt_token,
        mr_account=mr_account,
        yt_table_outputs=['output1', 'output2']
    )
    return x['output1'], x['output2']


def mr_move(table, path, force=True, yt_token=vh.get_yt_token_secret()):
    return vh.op(id='83f0cf88-63d9-11e6-a050-3c970e24a776', name='MR Move Table')(
        src=table,
        dst_path=path,
        force=force,
        yt_token=yt_token,
        yt_table_outputs=['moved']
    )['moved']


def mr_copy(table, path, force=True, yt_token=vh.get_yt_token_secret()):
    return vh.op(id='23762895-cf87-11e6-9372-6480993f8e34', name='MR Copy Table')(
        src=table,
        dst_path=path,
        force=force,
        yt_token=yt_token,
        yt_table_outputs=['copy']
    )['copy']


def mr_merge(srcs, merge_mode='AUTO', append=False, mr_account='tmp', yt_token=vh.get_yt_token_secret()):
    return vh.op(id='66fdd3dc-23a2-11e7-ac34-3c970e24a776', name='MR Merge')(
        srcs=srcs,
        merge_mode=merge_mode,
        append=append,
        mr_account=mr_account,
        yt_token=yt_token,
        yt_table_outputs=['merged']
    )['merged']


def mr_mkdir(path, cluster='hahn', yt_token=vh.get_yt_token_secret()):
    return vh.op(id='33d81b2c-1d18-11e7-904c-3c970e24a776', name='MR Create Directory')(
        src_cluster=cluster,
        src_path=path,
        yt_token=yt_token
    )['created']


def to_tsv(table, columns, missing_value_mode='FAIL', max_disk=8096, yt_token=vh.get_yt_token_secret()):
    return vh.op(id='d23ec268-5a94-42f9-bd6b-5062dccbd3f5', name='MR Read TSV')(
        table=table,
        columns=columns,
        missing_value_mode=missing_value_mode,
        max_disk=max_disk,
        yt_token=yt_token,
    )['tsv']


def mr_upload(
        file, dst, write_mode='OVERWRITE',
        dst_cluster='hahn',  # TODO fix as vh.get_yt_proxy() does not work
        yt_token=vh.get_yt_token_secret()
):
    return vh.op(id='fe5be04f-5e6b-494a-8673-961535b036a3', name='MR Upload File')(
        content=file,
        dst_path=dst,
        write_mode=write_mode,
        mr_default_cluster=dst_cluster,
        yt_token=yt_token,
    )['new_file']


def mr_sort(srcs, sort_by, mr_account='tmp', yt_token=vh.get_yt_token_secret()):
    return vh.op(id='51061ea1-c630-4fa1-a2e5-53afd15db896', name='MR Sort')(
        srcs=srcs,
        sort_by=sort_by,
        mr_account=mr_account,
        yt_token=yt_token,
        yt_table_outputs=['sorted']
    )['sorted']


def mr_left_join(main_table, other_table, join_by, mr_account='tmp', yt_token=vh.get_yt_token_secret()):
    main_table = mr_sort(main_table, sort_by=join_by, mr_account=mr_account, yt_token=yt_token)
    other_table = mr_sort(other_table, sort_by=join_by, mr_account=mr_account, yt_token=yt_token)

    return vh.op(id='7816c3b4-b5d7-445b-8a15-62dd70c9d662', name='MR Left Join First')(
        main_table=main_table,
        tables=other_table,
        join_by=join_by,
        yt_token=yt_token,
        mr_account=mr_account,
        max_ram=10000,
        yt_table_outputs=['joined_table']
    )['joined_table']


def build_arcadia_project(
        arcadia_url='arcadia:/arc/trunk/arcadia',
        arcadia_revision=None,
        build_type='release',
        targets='',
        arts='',
        arts_source=None,
        result_single_file=False,
        definition_flags=None,
        sandbox_oauth_token='',
        arcadia_patch=None,
        owner=None,
        use_aapi_fuse=True,
        use_arc_instead_of_aapi=False,
        aapi_fallback=False,
        kill_timeout=None,
        sandbox_requirements_disk=None,
        sandbox_requirements_ram=None,
        checkout=False,
        clear_build=True,
        strip_binaries=False,
        lto=False,
        thinlto=False,
        musl=False,
        use_system_python=False,
        target_platform_flags=None,
        javac_options=None,
        ya_yt_proxy=None,
        ya_yt_dir=None,
        ya_yt_token_vault_owner=None,
        ya_yt_token_vault_name=None,
        result_rt=None,
        timestamp=None
):
    return vh.op(id='95ddb9b8-8777-4946-a015-1737e4219317', name='Build Arcadia Project')(
        arcadia_url=arcadia_url,
        arcadia_revision=arcadia_revision,
        build_type=build_type,
        targets=targets,
        arts=arts,
        arts_source=arts_source,
        result_single_file=result_single_file,
        definition_flags=definition_flags,
        sandbox_oauth_token=sandbox_oauth_token,
        arcadia_patch=arcadia_patch,
        owner=owner,
        use_aapi_fuse=use_aapi_fuse,
        use_arc_instead_of_aapi=use_arc_instead_of_aapi,
        aapi_fallback=aapi_fallback,
        kill_timeout=kill_timeout,
        sandbox_requirements_disk=sandbox_requirements_disk,
        sandbox_requirements_ram=sandbox_requirements_ram,
        checkout=checkout,
        clear_build=clear_build,
        strip_binaries=strip_binaries,
        lto=lto,
        thinlto=thinlto,
        musl=musl,
        use_system_python=use_system_python,
        target_platform_flags=target_platform_flags,
        javac_options=javac_options,
        ya_yt_proxy=ya_yt_proxy,
        ya_yt_dir=ya_yt_dir,
        ya_yt_token_vault_owner=ya_yt_token_vault_owner,
        ya_yt_token_vault_name=ya_yt_token_vault_name,
        result_rt=result_rt,
        timestamp=timestamp
    )


def mr_map_exec_with_options(input_table, executable=None, mr_account='tmp', yt_token=vh.get_yt_token_secret(),
                             yt_pool=vh.get_yt_pool(), ttl=360, max_ram=100, max_disk=1024, options=[]):
    if isinstance(executable, dict):
        executable = build_arcadia_project(**executable)

    return vh.op(id='6e6d438f-90f4-4a8d-b156-6ee7ef82ec0b', name='MR Map exec with options')(
        executable=executable,
        input_table=input_table,
        yt_token=yt_token,
        mr_account=mr_account,
        yt_pool=yt_pool,
        ttl=ttl,
        max_ram=max_ram,
        max_disk=max_disk,
        options=options,
        yt_table_outputs=['output_table']
    )['output_table']


def yt_grep(input, grep_expr, start_expr=None, preprocess_expr=None,
            mr_account='tmp', yt_token=vh.get_yt_token_secret(),
            yt_pool=vh.get_yt_pool(), ttl=360, max_ram=100):
    return vh.op(id='6614be1a-2d33-4d6e-970f-074fd5777cae', name='YT Grep')(
        input=input,
        grep_expr=grep_expr,
        start_expr=start_expr,
        preprocess_expr=preprocess_expr,
        mr_account=mr_account,
        yt_token=yt_token,
        yt_pool=yt_pool,
        ttl=ttl,
        max_ram=max_ram,
        yt_table_outputs=['output']
    )['output']


def yt_reduce(input, reduce_by, reduce_expr, start_expr=None, prereduce_expr=None, postreduce_expr=None,
              mr_account='tmp', yt_token=vh.get_yt_token_secret(),
              yt_pool=vh.get_yt_pool(), ttl=360, max_ram=100):
    return vh.op(id='9930d9a9-001e-4aa6-9d5c-84211ce758a1', name='YT Reduce')(
        input=input,
        reduce_by=reduce_by,
        reduce_expr=reduce_expr,
        start_expr=start_expr,
        prereduce_expr=prereduce_expr,
        postreduce_expr=postreduce_expr,
        mr_account=mr_account,
        yt_token=yt_token,
        yt_pool=yt_pool,
        ttl=ttl,
        max_ram=max_ram,
        yt_table_outputs=['output']
    )['output']


def svn_export_single_file(arcadia_path, return_type='text', revision=None, path_prefix='', only_info=False):
    return vh.op(id='940e9ba8-11a0-4daa-ae23-8bf71f11e04c', name='SVN: Export single file (Deterministic)')(
        _options={
            'arcadia_path': arcadia_path
        },
        revision=revision,
        path_prefix=path_prefix,
        only_info=only_info
    )[return_type]


def dialog_preprocess(table, twitter_specificity_columns, normalize_nlg_columns):  # todo valhalize it
    executable = vh.op(id='313a903b-7d26-4696-86bb-a6df0e8d7044')(
        arcadia_revision=5094231,
        targets='alice/boltalka/tools/dssm_preprocessing/preprocessing',
        arts='alice/boltalka/tools/dssm_preprocessing/preprocessing/preprocessing',
        sandbox_oauth_token='artemkorenev_sandbox_token'  # TODO
    )['ARCADIA_PROJECT']
    table = vh.op(id='8044fb09-08b5-472f-86dc-fe20b62eea0d')(
        yt_token=vh.get_yt_token_secret(),
        mr_account='tmp',
        max_ram=1000,
        options='--skip-rt --skip-url --skip-hashtag --skip-new-post --strip-mention --replace-html-entities --skip-at',
        columns=twitter_specificity_columns,
        binary=executable,
        src=table,
        yt_table_outputs=['dst']
    )['dst']
    return vh.op(id='8fc6f528-e3ec-4522-b2c8-7730f117f63b')(
        dialogs=table,
        yt_token=vh.get_yt_token_secret(),
        mr_account='tmp',
        columns=' '.join(normalize_nlg_columns),
        yt_table_outputs=['output']
    )['output']


def yt_list(directory, absolute_path=False, mr_account='tmp', yt_token=vh.get_yt_token_secret()):
    return vh.op(id='31032446-7b13-41b2-aeb2-ed9b7d2a5fa1', name='YT list')(
        directory=directory,
        absolute_path=absolute_path,
        mr_account=mr_account,
        yt_token=yt_token
    )['list']


def mr_cat(tables_tsv, mr_default_cluster='hahn', mr_account='tmp', yt_token=vh.get_yt_token_secret()):
    return vh.op(id='235f134e-74d0-4295-b7d9-a383d80d9c22', name='MR Cat (tables list)')(
        tables=tables_tsv,
        mr_default_cluster=mr_default_cluster,
        mr_account=mr_account,
        yt_token=yt_token
    )['table']


def get_merged_table_from_directory(directory, cluster='hahn', mr_account='tmp', yt_token=vh.get_yt_token_secret()):
    directory = get_directory(directory, cluster=cluster, yt_token=yt_token)
    tables_tsv = yt_list(directory, absolute_path=True, mr_account=mr_account, yt_token=yt_token)
    return mr_cat(tables_tsv, mr_default_cluster=cluster, mr_account=mr_account, yt_token=yt_token)


def sh_wget(url, oauth, max_disk=50000, timestamp=None):
    return vh.op(id='3f8a4b71-861c-4abf-a34d-8ba6b50ea882', name='sh wget (one file)')(
        _options={
            'url': url
        },
        oauth=oauth,
        out_type='binary',
        max_disk=max_disk,
        ts=timestamp
    )['binary']


def rewrite_respect(input, source_key, target_key, target_gender, arcadia_revision=7582828,
                    yt_token=vh.get_yt_token_secret(), mr_account='tmp',
                    memory_limit=16384, cpu_limit=8, job_count=1000):
    return vh.op(id='101ac089-86d1-4268-a08c-f36c89a91d75', name='Rewrite Respect')(
        input=input,
        source_key=source_key,
        target_key=target_key,
        target_gender=target_gender,
        arcadia_revision=arcadia_revision,
        yt_token=yt_token,
        mr_account=mr_account,
        memory_limit=memory_limit,
        cpu_limit=cpu_limit,
        job_count=job_count,
        yt_table_outputs=['output']
    )['output']
