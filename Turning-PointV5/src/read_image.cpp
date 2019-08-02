#include <stdio.h>
#include "display/lvgl.h"
#include <stdlib.h>
#include <cerrno>
#include "main.h"

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


//lv_obj_t * globe_object = nullptr;
//lv_img_t * globe_frames[43];

int current_frame = 0;
void lcd_gif_creation() {
    /*
    lv_fs_drv_t pcfs_drv;                         
    memset(&pcfs_drv, 0, sizeof(lv_fs_drv_t));  

    pcfs_drv.file_size = sizeof(FILE*);      
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
    /**/

     LV_IMG_DECLARE(frame0_globe);
     LV_IMG_DECLARE(frame1_globe);
     LV_IMG_DECLARE(frame2_globe);
     LV_IMG_DECLARE(frame3_globe);
     LV_IMG_DECLARE(frame4_globe);
     LV_IMG_DECLARE(frame5_globe);
     LV_IMG_DECLARE(frame6_globe);
     LV_IMG_DECLARE(frame7_globe);
     LV_IMG_DECLARE(frame8_globe);
     LV_IMG_DECLARE(frame9_globe);
     LV_IMG_DECLARE(frame10_globe);
     LV_IMG_DECLARE(frame11_globe);
     LV_IMG_DECLARE(frame12_globe);
     LV_IMG_DECLARE(frame13_globe);
     LV_IMG_DECLARE(frame14_globe);
     LV_IMG_DECLARE(frame15_globe);
     LV_IMG_DECLARE(frame16_globe);
     LV_IMG_DECLARE(frame17_globe);
     LV_IMG_DECLARE(frame18_globe);
     LV_IMG_DECLARE(frame19_globe);
     LV_IMG_DECLARE(frame20_globe);
     LV_IMG_DECLARE(frame21_globe);
     LV_IMG_DECLARE(frame22_globe);
     LV_IMG_DECLARE(frame23_globe);
     LV_IMG_DECLARE(frame24_globe);
     LV_IMG_DECLARE(frame25_globe);
     LV_IMG_DECLARE(frame26_globe);
     LV_IMG_DECLARE(frame27_globe);
     LV_IMG_DECLARE(frame28_globe);
     LV_IMG_DECLARE(frame29_globe);
     LV_IMG_DECLARE(frame30_globe);
     LV_IMG_DECLARE(frame31_globe);
     LV_IMG_DECLARE(frame32_globe);
     LV_IMG_DECLARE(frame33_globe);
     LV_IMG_DECLARE(frame34_globe);
     LV_IMG_DECLARE(frame35_globe);
     LV_IMG_DECLARE(frame36_globe);
     LV_IMG_DECLARE(frame37_globe);
     LV_IMG_DECLARE(frame38_globe);
     LV_IMG_DECLARE(frame39_globe);
     LV_IMG_DECLARE(frame40_globe);
     LV_IMG_DECLARE(frame41_globe);
     LV_IMG_DECLARE(frame42_globe);
     LV_IMG_DECLARE(frame43_globe);


     
    const lv_img_t * globe_frames[] = {&frame0_globe, &frame1_globe, &frame2_globe, &frame3_globe, &frame4_globe, &frame5_globe, &frame6_globe, &frame7_globe, &frame8_globe, &frame9_globe, &frame10_globe, &frame11_globe, &frame12_globe, &frame13_globe, &frame14_globe, &frame15_globe, &frame16_globe, &frame17_globe, &frame18_globe, &frame19_globe, &frame20_globe, &frame21_globe, &frame22_globe, &frame23_globe, &frame24_globe, &frame25_globe, &frame26_globe, &frame27_globe, &frame28_globe, &frame29_globe, &frame30_globe, &frame31_globe, &frame32_globe, &frame33_globe, &frame34_globe, &frame35_globe, &frame36_globe, &frame37_globe, &frame38_globe, &frame39_globe, &frame40_globe, &frame41_globe, &frame42_globe, &frame43_globe};

    lv_obj_t * globe_object = lv_img_create(lv_scr_act(), NULL);
    lv_obj_align(globe_object, NULL, LV_ALIGN_CENTER, 0, -20);


    
    lv_img_set_src(globe_object, &frame43_globe);
  //  current_frame++; 

 //   pros::c::delay(1000);
    
    


    
}