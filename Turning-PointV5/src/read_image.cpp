#include <stdio.h>
#include "display/lvgl.h"
#include <stdlib.h>
#include <cerrno>

int openCounter = 0;

static lv_fs_res_t pcfs_open( void * file_p, const char * fn, lv_fs_mode_t mode)
{
    errno = 0;
    const char * flags = "";
    if(mode == LV_FS_MODE_WR) flags = "wb";
    else if(mode == LV_FS_MODE_RD) flags = "rb";
    else if(mode == (LV_FS_MODE_WR | LV_FS_MODE_RD)) flags = "a+";

    char buf[256];
    snprintf(buf, sizeof(buf), "/%s", fn);

    if (openCounter != 0)
        return LV_FS_RES_BUSY;

    FILE* f = fopen(buf, flags);

    if (f == NULL) {
        return LV_FS_RES_UNKNOWN;
    } else {
        openCounter++;
        // fseek(f, 0, SEEK_SET);
        *(FILE**)file_p = f;
    }

    return LV_FS_RES_OK;
}

static lv_fs_res_t pcfs_close(void * file_p)
{
    openCounter--;
    fclose(*(FILE**)file_p);
    return LV_FS_RES_OK;
}

static lv_fs_res_t pcfs_read( void * file_p, void * buf, uint32_t btr, uint32_t * br)
{
    FILE** fp =  (FILE* *)file_p;
    *br = fread(buf, 1, btr, *fp);
    return LV_FS_RES_OK;
}

static lv_fs_res_t pcfs_seek( void * file_p, uint32_t pos)
{
    FILE** fp = (FILE* *)file_p;
    fseek(*fp, pos, SEEK_SET);
    return LV_FS_RES_OK;
}

static lv_fs_res_t pcfs_tell( void * file_p, uint32_t * pos_p)
{
    FILE** fp =  (FILE* *)file_p;
    *pos_p = ftell(*fp);
    return LV_FS_RES_OK;
}


lv_obj_t * img_var = nullptr;

void lcd_gif_creation() {
    lv_fs_drv_t pcfs_drv;                         /*A driver descriptor*/
    memset(&pcfs_drv, 0, sizeof(lv_fs_drv_t));    /*Initialization*/

    pcfs_drv.file_size = sizeof(FILE*);       /*Set up fields...*/
    pcfs_drv.letter = 'S';
    pcfs_drv.open = pcfs_open;
    pcfs_drv.close = pcfs_close;
    pcfs_drv.read = pcfs_read;
    pcfs_drv.seek = pcfs_seek;
    pcfs_drv.tell = pcfs_tell;
    lv_fs_add_drv(&pcfs_drv);

    img_var = lv_img_create(lv_scr_act(), NULL);

    lv_img_set_src(img_var, "S:/usd/frame0.bin");
    lv_obj_set_pos(img_var, 0, 0);
}