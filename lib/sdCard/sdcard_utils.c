#include "sdcard_utils.h"

static sd_card_t *sd_get_by_name(const char *const name) {
    for (size_t i = 0; i < sd_get_num(); ++i)
        if (0 == strcmp(sd_get_by_num(i)->pcName, name))
            return sd_get_by_num(i);
    DBG_PRINTF("%s: unknown name %s\n", __func__, name);
    return NULL;
}

static FATFS *sd_get_fs_by_name(const char *name) {
    for (size_t i = 0; i < sd_get_num(); ++i)
        if (0 == strcmp(sd_get_by_num(i)->pcName, name))
            return &sd_get_by_num(i)->fatfs;
    DBG_PRINTF("%s: unknown name %s\n", __func__, name);
    return NULL;
}

void sd_mount() {
    const char *arg1 = sd_get_by_num(0)->pcName;
    FATFS *p_fs = sd_get_fs_by_name(arg1);
    if (!p_fs) return;
    FRESULT fr = f_mount(p_fs, arg1, 1);
    if (FR_OK != fr) {
        printf("f_mount error: %s (%d)\n", FRESULT_str(fr), fr);
        return;
    }
    sd_card_t *pSD = sd_get_by_name(arg1);
    myASSERT(pSD);
    pSD->mounted = true;
    printf("Cartão SD ( %s ) montado com sucesso.\n", pSD->pcName);
}

void sd_unmount() {
    const char *arg1 = sd_get_by_num(0)->pcName;
    FATFS *p_fs = sd_get_fs_by_name(arg1);
    if (!p_fs) return;
    FRESULT fr = f_unmount(arg1);
    if (FR_OK != fr) {
        printf("f_unmount error: %s (%d)\n", FRESULT_str(fr), fr);
        return;
    }
    sd_card_t *pSD = sd_get_by_name(arg1);
    myASSERT(pSD);
    pSD->mounted = false;
    pSD->m_Status |= STA_NOINIT;
    printf("Cartão SD ( %s ) desmontado.\n", pSD->pcName);
}

void sd_format() {
    const char *arg1 = sd_get_by_num(0)->pcName;
    FATFS *p_fs = sd_get_fs_by_name(arg1);
    if (!p_fs) return;
    FRESULT fr = f_mkfs(arg1, 0, 0, FF_MAX_SS * 2);
    if (FR_OK != fr) {
        printf("f_mkfs error: %s (%d)\n", FRESULT_str(fr), fr);
    }
}

void sd_get_free_space() {
    const char *arg1 = sd_get_by_num(0)->pcName;
    DWORD fre_clust, fre_sect, tot_sect;
    FATFS *p_fs = sd_get_fs_by_name(arg1);
    if (!p_fs) return;
    FRESULT fr = f_getfree(arg1, &fre_clust, &p_fs);
    if (FR_OK != fr) {
        printf("f_getfree error: %s (%d)\n", FRESULT_str(fr), fr);
        return;
    }
    tot_sect = (p_fs->n_fatent - 2) * p_fs->csize;
    fre_sect = fre_clust * p_fs->csize;
    printf("%10lu KiB total drive space.\n%10lu KiB available.\n", tot_sect / 2, fre_sect / 2);
}

void sd_list_files() {
    char cwdbuf[FF_LFN_BUF] = {0};
    char const *p_dir;
    FRESULT fr;

    fr = f_getcwd(cwdbuf, sizeof cwdbuf);
    if (FR_OK != fr) {
        printf("f_getcwd error: %s (%d)\n", FRESULT_str(fr), fr);
        return;
    }
    p_dir = cwdbuf;
    printf("Arquivos no diretório: %s\n", p_dir);
    DIR dj;
    FILINFO fno;
    fr = f_findfirst(&dj, &fno, p_dir, "*");
    while (fr == FR_OK && fno.fname[0]) {
        printf("%s [%s] [size=%llu]\n", fno.fname, (fno.fattrib & AM_DIR) ? "dir" : "file", fno.fsize);
        fr = f_findnext(&dj, &fno);
    }
    f_closedir(&dj);
}

void sd_show_file(const char *filename) {
    FIL file;
    FRESULT res = f_open(&file, filename, FA_READ);
    if (res != FR_OK) {
        printf("[ERRO] Não foi possível abrir o arquivo %s\n", filename);
        return;
    }
    char buffer[128];
    UINT br;
    while (f_read(&file, buffer, sizeof(buffer) - 1, &br) == FR_OK && br > 0) {
        buffer[br] = '\0';
        printf("%s", buffer);
    }
    f_close(&file);
}

void read_file(const char *filename) {
    sd_show_file(filename);
}
