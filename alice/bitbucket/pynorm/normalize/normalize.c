#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util/config.h"
#include "util/configfile.h"
#include "util/util.h"
#include "util/log.h"
#include "util/wfst.h"
#include "util/ofst-symbol-table.h"
#include "util/fst-best-path.h"
#include "util/xalloc.h"
#include "util/mem_blob.h"

#include "normalize.h"

struct norm_data {
  int n_fsts;
  configfile_t *configfile;
  wfst_t **fsts;
  ofst_symbol_table_t *ost;
  bool report_intermediate;
};

norm_data_t *
norm_data_read(const char *path)
{
  enum {
    MAX_LINE_LEN = 128
  };
  norm_data_t *rd;

  rd = xcalloc(1, sizeof(norm_data_t));

  {
    char *flagpath = astrcat(path, PATH_SEP, "flags.txt", NULL);
    configfile_t *cf = configfile_read(flagpath);


    if (cf != NULL) {
      configfile_get_bool(cf, "report-intermediate", &rd->report_intermediate);
    }

    rd->configfile = cf;
    free(flagpath);
  }

  {
    char *sympath = astrcat(path, PATH_SEP, "symbols.sym", NULL);

    rd->ost = ofst_symbol_table_read(sympath);
    free(sympath);

    if (rd->ost == NULL) goto BAD;
  }

  {
    char *seqpath = astrcat(path, PATH_SEP, "sequence.txt", NULL);
    FILE *seqfp = fopen(seqpath, "r");
    char buf[MAX_LINE_LEN];

    if (seqfp == NULL) {
      log_warning("%s: Could not open %s", __FUNCTION__, seqpath);
      free(seqpath);
      goto BAD;
    }
    free(seqpath);

    while (fgets(buf, MAX_LINE_LEN, seqfp) != NULL) {
      if (buf[strlen(buf) - 1] != '\n') {
        log_warning("%s: sequence file line too long or no \\n at end of file: %s", __FUNCTION__, buf);
        fclose(seqfp);
        goto BAD;
      }
      buf[strlen(buf) - 1] = '\0';	/* remove the \n */

      if (buf[0] != '#') {
        char *fstname = astrcat(path, PATH_SEP, buf, ".fst", NULL);

        rd->n_fsts++;
        rd->fsts = xrealloc(rd->fsts, rd->n_fsts * sizeof(wfst_t*));
        rd->fsts[rd->n_fsts - 1] = wfst_read(fstname);

        free(fstname);

        if (rd->fsts[rd->n_fsts - 1] == NULL) {
          fclose(seqfp);
          goto BAD;
        }
      }
    }
    fclose(seqfp);
  }

  return rd;

BAD:
  norm_data_free(rd);
  return NULL;
}

norm_data_t *
norm_data_read_from_blobs(mem_blob_t flags, mem_blob_t symbols, fst_data_t *fst_data, size_t fst_data_size)
{
  enum {
    MAX_LINE_LEN = 128
  };
  norm_data_t *rd;

  rd = xcalloc(1, sizeof(norm_data_t));

  {
    configfile_t *cf = configfile_read_from_blob(flags);

    if (cf != NULL) {
      configfile_get_bool(cf, "report-intermediate", &rd->report_intermediate);
    }

    rd->configfile = cf;
  }

  {
    rd->ost = ofst_symbol_table_read_from_blob(symbols);

    if (rd->ost == NULL) goto BAD;
  }

  {
    size_t i = 0;
    rd->fsts = xmalloc(fst_data_size * sizeof(wfst_t*));
    rd->n_fsts = fst_data_size;
    while (i < fst_data_size) {
      rd->fsts[i] = wfst_read_from_blob(fst_data[i].fst_name, fst_data[i].blob);
      if (rd->fsts[i] == NULL) {
        goto BAD;
      }
      ++i;
    }
  }

  return rd;

BAD:
  norm_data_free(rd);
  return NULL;
}


void
norm_data_free(norm_data_t *rd)
{
  int i;

  if (rd == NULL) return;

  for (i = 0; i < rd->n_fsts; i++) {
    wfst_free(rd->fsts[i]);
  }
  free(rd->fsts);

  configfile_free(rd->configfile);
  ofst_symbol_table_free(rd->ost);
  free(rd);
}

/* Get model version.
   The returned value belongs to the config.
*/
char *
norm_model_version(norm_data_t *normalizer_data)
{
  char *res = "*unknown*";
  configfile_get_string(normalizer_data->configfile, "version", &res);
  return res;
}


char **
norm_data_get_normalizer_names(norm_data_t *normalizer_data)
{
    char **names = xcalloc(normalizer_data->n_fsts + 1, sizeof(char*));
    int i;
    for (i = 0; i < normalizer_data->n_fsts; ++i) {
        names[i] = wfst_fname(normalizer_data->fsts[i]);
    }
    return names;
}

wfst_t **
filter_fsts_by_name(wfst_t **fsts, int n_fsts, char **names_list, bool is_white_list)
{
  wfst_t **filtered_fsts = xcalloc(n_fsts + 1, sizeof(wfst_t*));
  int i, filtered_fsts_count = 0;
  for (i = 0; i < n_fsts; ++i) {
    if (names_list != NULL) {
      char **name = names_list;
      bool fst_in_list = false;
      while (*name) {
        if (strcmp(*name, wfst_fname(fsts[i])) == 0) {
          fst_in_list = true;
          break;
        }
        ++name;
      }
      if ((is_white_list && fst_in_list) || (!is_white_list && !fst_in_list)) {
        filtered_fsts[filtered_fsts_count++] = fsts[i];
      }
    } else {
      filtered_fsts[filtered_fsts_count++] = fsts[i];
    }
  }
  return filtered_fsts;
}

char *
norm_run_with_fstlist(norm_data_t *rd, char *str, char **fstlist, bool is_white_list)
{
  int *ilabels = NULL;
  int il_len;
  char *ostr;
  wfst_t **fsts;
  wfst_t **current_fst;

  ilabels = str_to_labels(rd->ost, str, &il_len);
  if (ilabels == NULL) return NULL;

  fsts = filter_fsts_by_name(rd->fsts, rd->n_fsts, fstlist, is_white_list);
  current_fst = fsts;

  while (*current_fst) {
    int *olabels = NULL;
    int ol_len;

    if (rd->report_intermediate) {
      char *str = labels_to_str(rd->ost, ilabels, il_len);
      log_info("%s before applying %s: \"%s\"",
                 __FUNCTION__, wfst_fname(*current_fst), str);
      free(str);
    }

    /** Should number of variants considered (band) be configurable? */
    olabels = fst_best_path_search(*current_fst, il_len, ilabels, /**/10000, &ol_len);
    if (olabels == NULL) {
      free(ilabels);
      free(fsts);
      return NULL;
    }

    free(ilabels);
    ilabels = olabels;
    il_len = ol_len;
    ++current_fst;
  }

  ostr = labels_to_str(rd->ost, ilabels, il_len);

  free(ilabels);
  free(fsts);

  return ostr;
}

char *
norm_run(norm_data_t *normalizer_data, char *string)
{
    return norm_run_with_fstlist(normalizer_data, string, NULL, true);
}

char *
norm_run_with_whitelist(norm_data_t *normalizer_data, char *string, char **whitelist)
{
    return norm_run_with_fstlist(normalizer_data, string, whitelist, true);
}

char *
norm_run_with_blacklist(norm_data_t *normalizer_data, char *string, char **blacklist)
{
    return norm_run_with_fstlist(normalizer_data, string, blacklist, false);
}
