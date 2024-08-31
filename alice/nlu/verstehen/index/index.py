import abc


class Index:
    """
    Abstract class of Index which can search with a query returning not more than
    `n_samples` results in descending order of their score.
    """

    def search(self, query, n_samples=None):
        """
        Searching in the index by a given query.

        Arguments:
            query: query of some format that can be passed to self.preprocessing function.
            n_samples: max number of returned entries by the search.

        Returns:
            results of the search in the index containing not more than `n_samples` of entries.
        """
        preprocessed_query = self.preprocessing(query)
        return self.search_preprocessed(preprocessed_query, n_samples=n_samples)

    @abc.abstractmethod
    def preprocessing(self, query):
        """
        Preprocessing the query so that it can be passed to self.search_preprocessed function.

        Arguments:
            query: query of some format.

        Returns:
            preprocessed query that depends on the implementation of the index.
        """
        raise NotImplementedError()

    @abc.abstractmethod
    def search_preprocessed(self, preprocessed_query, n_samples=None):
        """
        Searching in the index by a given preprocessed query.

        Arguments:
            preprocessed_query: query of some format that is defined by self.preprocessing function.
            n_samples: max number of returned entries by the search.

        Returns:
            results of the search in the index containing not more than `n_samples` of entries.
        """
        raise NotImplementedError()

    @abc.abstractmethod
    def estimate(self, query):
        """
        Getting scores ordered for the whole the index by a given query.
        This is needed to get scores for certain samples in Granet part (Needed samples are filtered before sending response.)
        TODO: optimize this - evaluate only necessary inputs.

        Arguments:
            query: query of some format that can be passed to self.preprocessing function.

        Returns:
            scores used by the index.
        """
        preprocessed_query = self.preprocessing(query)
        return self.estimate_preprocessed(preprocessed_query)

    @abc.abstractmethod
    def estimate_preprocessed(self, preprocessed_query):
        raise NotImplementedError()

    @staticmethod
    def from_config(index_config, texts, payload=None, indexes_map=None):
        """
        Creating index from config with possible index reusing

        Arguments:
            index_config: config of current index
            texts: texts on which index will be built
            payload: optional data about texts
            index_map: optional dictionary from index name to index instance

        Returns:
            instance of index
        """
        raise NotImplementedError()

    @staticmethod
    def from_config_by_skill_id(index_config, texts, skill_to_idxs_map, payload=None, indexes_map=None):
        """
        Creating index from config with possible index reusing

        Arguments:
            index_config: config of current index
            texts: texts on which index will be built
            payload: optional data about texts
            index_map: optional dictionary from index name to index instance

        Returns:
            instance of index
        """
        raise NotImplementedError()
