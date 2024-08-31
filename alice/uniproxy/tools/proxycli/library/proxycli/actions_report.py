import json

from .actions import *      # noqa
from .actions import ActionBase, action
from .context import Context

from colorama import Fore
from colorama import Style


@action('proxycli.report.aggregate')
class ReportAggregate(ActionBase):
    async def execute(self, context: Context):
        if not context['root'].reporting:
            return

        begins_at = context['root'].report.started_at
        for _name, _ctx in context['root'].iterprefix('app'):
            for name, ctx, depth in _ctx.itertags('report'):
                action = ctx.action
                description = ctx.description or ctx.message

                ok = (ctx.ok is None or ctx.ok) and ctx.duration

                if ctx.skipped:
                    status = f'{Fore.WHITE}[ SKIPPED   ]{Style.RESET_ALL}'
                elif ok:
                    status = f'{Fore.GREEN}[ OK        ]{Style.RESET_ALL}'
                elif ctx.warning:
                    status = f'{Fore.YELLOW}[      WARN ]{Style.RESET_ALL}'
                elif ctx.duration is None:
                    status = f'{Fore.RED}[   NOT RUN ]{Style.RESET_ALL}'
                else:
                    status = f'{Fore.RED}[    FAILED ]{Style.RESET_ALL}'

                indent = '\t' + '  ' * depth
                try:
                    msg = status
                    duration = ctx.get('duration', 0.0)
                    msg += ' %10.3fms' % (duration * 1000, )
                    if begins_at:
                        offset = (ctx.get('started_at', begins_at) - begins_at) * 1000
                        msg += ' %10.3fms' % (offset)
                    msg += indent
                    msg += action + ' '
                    if description:
                        msg += f'({description})'

                    print(msg)
                except Exception as ex:
                    context.log.exception(ex)
                    context.log.warning(json.dumps(ctx.to_dict(), indent=4))
