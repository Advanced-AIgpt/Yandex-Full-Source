import grequests
from gevent.pool import Pool
from alice.boltalka.extsearch.query_basesearch.lib.main import QueryBasesearch


class QueryBasesearch(QueryBasesearch):
    def process_iterator(self, iterator, use_entity):
        def send(r):
            return r.send()

        def generate_requests(iterator):
            for query in iterator:
                entity = None
                if use_entity:
                    entity, query = query
                params = self.get_params_with_entity(query, entity)
                yield grequests.get(self.url, session=self.session, params=params)

        pool = Pool(self.pool_size)
        for response in pool.imap(send, generate_requests(iterator)):
            if response.response is None:
                raise response.exception
            if not response.response.ok:
                raise Exception('Server respondend with status {}'.format(response.response.status_code))
            yield self.parse_response(response.response.content)