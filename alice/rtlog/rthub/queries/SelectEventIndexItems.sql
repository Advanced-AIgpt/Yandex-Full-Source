SELECT * FROM (
  SELECT EventIndexItem FROM Input
  WHERE EventIndexItem IS NOT NULL AND EventIndexItem.Key IS NOT NULL AND LENGTH(EventIndexItem.Key) > 0
) FLATTEN COLUMNS;
