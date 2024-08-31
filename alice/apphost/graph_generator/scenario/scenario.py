import alice.library.python.utils as utils
import alice.megamind.library.config.protos.config_pb2 as config_proto
import alice.megamind.library.config.scenario_protos.config_pb2 as scenario_config
import alice.megamind.protos.common.data_source_type_pb2 as ds_type
from alice.tests.library.uniclient import ProtoWrapper
from cached_property import cached_property
from google.protobuf.json_format import MessageToDict


EDataSourceType = ds_type.EDataSourceType


class Scenario(ProtoWrapper):
    proto_cls = scenario_config.TScenarioConfig

    def __init__(self, config, timeouts):
        super().__init__(config)
        self._timeouts = config_proto.TConfig.TScenarios.TConfig()
        self._timeouts.CopyFrom(timeouts.Scenarios.DefaultConfig)
        scenario_timeouts = timeouts.Scenarios.Configs.get(self.Name)
        if scenario_timeouts:
            self._timeouts.MergeFrom(scenario_timeouts)
        self._stages = []

    @property
    def is_pure(self):
        return self.Handlers.RequestType == scenario_config.ERequestType.AppHostPure

    @property
    def is_pure_or_transferring(self):
        return self.is_pure or self.Handlers.IsTransferringToAppHostPure

    @cached_property
    def graph_prefix(self):
        return self.Handlers.GraphsPrefix or utils.to_snake_case(self.Name)

    @classmethod
    def is_web_search_datasource_name(cls, name):
        return name.startswith('WEB_SEARCH_')

    @classmethod
    def is_web_search_datasource(cls, ds):
        return cls.is_web_search_datasource_name(EDataSourceType.Name(ds.Type))

    @cached_property
    def has_web_search_datasource(self):
        has = False
        for ds in self.DataSources:
            has |= self.is_web_search_datasource(ds)
        return has

    def _filter_datasource(self, filter_func):
        return [MessageToDict(_) for _ in self.DataSources if filter_func(_)]

    @cached_property
    def datasources(self):
        return self._filter_datasource(lambda _: not self.is_web_search_datasource(_))

    @cached_property
    def web_search_datasources(self):
        return self._filter_datasource(self.is_web_search_datasource)

    @property
    def timeouts(self):
        return self._timeouts

    @property
    def stages(self):
        return self._stages

    def all_conditional_datasources(self):
        return [EDataSourceType.Name(datasource.DataSourceType) for datasource in self._timeouts.ConditionalDataSources]

    @property
    def conditional_datasources(self):
        return [ds_name for ds_name in self.all_conditional_datasources() if not self.is_web_search_datasource_name(ds_name)]

    @property
    def conditional_websearch_datasources(self):
        return [ds_name for ds_name in self.all_conditional_datasources() if self.is_web_search_datasource_name(ds_name)]
