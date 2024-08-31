from nirvana_make import NirvanaOperation

@NirvanaOperation.operations
def operations():
    op = NirvanaOperation(path='alice/boltalka/tools/build_mx_pool')
    op.yt('yt://hahn/home/voice/dialog/operations')
    op.meta(name='Build NLG Mx Pool',
            description='',
            version='4.{{revision}}',
            deterministic=True)
    op.processor('mr', 'run')
    op.tags([])
    op.permissions([])
    op.input(name='input',
             description='',
             type=['mrTable'],
             lower=1,
             upper=1)
    op.output(name='output',
              description='',
              type='mrTable',
              optional=False)
    op.output(name='output_pairs',
              description='',
              type='mrTable',
              optional=False)
    op.param(name='job-command',
             type='string',
             required=True,
             default='./build_mx_pool --src ${mr_input["input"]} --dst ${mr_output["output"]}'
                ' --priority ${param["priority"]} --mode ${param["mode"]} --dst-pairs ${mr_output["output_pairs"]}'
                ' [#if param["is-test"]] --is-test [/#if] --train-fraction ${param["train-fraction"]}'
                ' --normalize-by-num-pairs ${param["normalize-by-num-pairs"]}'
                ' --priority-weights ${param["priority-weights"]} [#if param["use-priority-weights-for-pair-count"]]'
                ' --use-priority-weights-for-pair-count [/#if]')
    op.param(name='job-binary-url',
             type='resource',
             default='FILE(alice/boltalka/tools/build_mx_pool/build_mx_pool)')
    op.param(name='job-environments',
             type='multiple_strings',
             required=False,
             default=None)
    op.param(name='job-variables',
             description=["Job's environment variables, in the form `VARIABLE=value`.",
                          'Roughly equivalent to using `VAR1=value1 .. VARn=valueN <your command line>` syntax in a Bourne-like shell.'],
             type='multiple_strings',
             required=False,
             default=None)
    op.param(name='job-volume',
             type='multiple_resources',
             required=False,
             default=None)
    op.param(name='yt-token',
             type='secret',
             required=True,
             default=None,
             label='YT Token:',
             tooltip=['ID of Nirvana Secret with YT access token (https://nda.ya.ru/3RSzVU).',
                      'Guide to Nirvana Secrets: https://nda.ya.ru/3RSzWZ'])
    op.param(name='mr-account',
             type='string',
             required=True,
             default=None,
             label='MR Account:',
             tooltip=['MR Account Name.',
                      'By default, output tables and directories will be created in some subdirectory of home/<MR Account>/<workflow owner>/nirvana'])
    op.param(name='yt-pool',
             description='Pool used by [YT operation scheduler](https://nda.ya.ru/3Rk4af). Leave this blank to use default pool.',
             type='string',
             required=False,
             default=None,
             label='YT Pool:',
             tooltip=['Pool used by YT scheduler. Leave blank to use default pool.',
                      'This option has no effect on YaMR.'])
    op.param(name='ttl',
             description="Job's maximum execution time, in minutes",
             type='integer',
             required=True,
             default=360,
             label='Job TTL, min.:',
             tooltip=None)
    op.param(name='max-ram',
             description="Job's maximum memory usage, in megabytes",
             type='integer',
             required=True,
             default=100,
             label='Max. RAM, Mb:',
             tooltip=None)
    op.param(name='cpu-guarantee',
             description='% of CPU used by the job. (1 logical core is 100%)',
             type='integer',
             required=True,
             default=1)
    op.param(name='max-disk',
             type='integer',
             required=False,
             default=1024)
    op.param(name='priority',
             description='',
             type='string',
             required=True,
             default='result',
             label='priority',
             tooltip='comma separated list of reply attributes to compare, e.g. "result,male,rude,you"')
    op.param(name='mode',
             description='',
             type='enum',
             required=True,
             default='pairwise',
             label='mode',
             tooltip=None,
             enum=[('pairwise', 'pairwise')])
    op.param(name='train-fraction',
             description='',
             type='number',
             required=False,
             default=1.0,
             label='train-fraction',
             tooltip=None)
    op.param(name='is-test',
             description='',
             type='boolean',
             required=False,
             default=False,
             label='is-test',
             tooltip=None)
    op.param(name='normalize-by-num-pairs',
             description='',
             type='enum',
             required=False,
             default='none',
             label='normalize-by-num-pairs',
             tooltip=None,
             enum=[('none', 'none'),
                   ('count', 'count'),
                   ('sqrt', 'sqrt')])
    op.param(name='priority-weights',
             description='',
             type='string',
             required=False,
             default='equal',
             label='priority-weights',
             tooltip='Either "equal" or comma separated array of weights')
    op.param(name='use-priority-weights-for-pair-count',
             description='',
             type='boolean',
             required=False,
             default=False,
             label='use-priority-weights-for-pair-count',
             tooltip=None)
    yield op
