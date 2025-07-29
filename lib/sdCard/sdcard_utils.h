#ifndef SDCARD_UTILS_H
#define SDCARD_UTILS_H

#include <string.h>
#include "ff.h"
#include "diskio.h"
#include "f_util.h"
#include "hw_config.h"
#include "my_debug.h"
#include "rtc.h"
#include "sd_card.h"

void sd_mount();
void sd_unmount();
void sd_format();
void sd_get_free_space();
void sd_list_files();
void sd_show_file(const char *filename);
void read_file(const char *filename);

#endif
