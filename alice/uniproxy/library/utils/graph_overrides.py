class GraphOverrides:

    CGI = 'unigraphrwr'

    def __init__(self, unigraph_args):
        self.override_apply = None
        self.override_run = None
        for arg in unigraph_args:
            if arg.startswith('UNIMMAPPLY:'):
                self.override_apply = arg[11:]
            elif arg.startswith('UNIMMRUN:'):
                self.override_run = arg[9:]
