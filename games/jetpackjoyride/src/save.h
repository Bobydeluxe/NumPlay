/* What's kept: the records, the next run's prizes and a run left with Home, in jetpackjoyride.sav, and a copy
 * of it in a Python script, jetpackjoyride_saves.py (installing apps from the NumWorks website keeps only
 * scripts, so the save comes back from it). */
#ifndef JJ_SAVE_H
#define JJ_SAVE_H
#include "jj.h"

typedef struct {
  uint32_t best, coins, runs;
  uint8_t guide_seen, head_start, double_coins, has_run;
} jj_profile;

#define JJ_SAVE_NAME "jetpackjoyride.sav"
#define JJ_COPY_NAME "jetpackjoyride_saves.py"
#define JJ_MAGIC 0x31524A4Au   /* "JJR1" */
#define JJ_VERSION 1

typedef struct {
  uint32_t magic;
  uint16_t version, size;
  jj_profile p;
  jj_run run;
  uint32_t sum;                /* FNV-1a of everything before it */
} jj_file;

uint32_t jj_sum(const void *data, uint32_t n);
bool jj_file_ok(const jj_file *f);
void save_load(jj_profile *p, jj_run *r);   /* the save, its copy, or a fresh start */
bool save_write(const jj_profile *p, const jj_run *r);
#endif
