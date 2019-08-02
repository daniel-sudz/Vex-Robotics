#include "lcd.h"
#include "main.h"
#include "aton.h"
#include <string.h>
#include <string>
#include "display/lvgl.h"

using namespace pros::c;




LCD::LCD()
{
    CreateControls();
}

lv_res_t LCD::click_action(lv_obj_t * btn) 
{
    uint8_t id = lv_obj_get_free_num(btn);
    auto& value = GetMain().lcd.m_buttons[id].value;
    value = !value;

    lv_obj_t * label = lv_obj_get_child(btn, NULL);
    
    if (value)
        lv_label_set_text(label, GetMain().lcd.m_buttons[id].label);
    else
        lv_label_set_text(label, GetMain().lcd.m_buttons[id].label2);

    ReportStatus("Click: %d:  %s = %d\n)", id, GetMain().lcd.m_buttons[id].label, value);

    GetMain().vision.SetFlipX(GetMain().lcd.AtonBlueRight);
    return LV_RES_OK;
}

lv_obj_t* LCD::CreateButton(uint8_t id, const char* label, lv_obj_t* container, lv_obj_t* prevElement, bool toggled)
{
    //show fluxion logo on main screen  (acesses image from c array in src folder, causes file size ballooning as the old image converter does not compress and stores individual pixels)
    //  commented for test LV_IMG_DECLARE(oldfluxcompat); //searches files for image array of this variable
    //  commented for test lv_obj_t * img1 = lv_img_create(lv_scr_act(), NULL); //creates image object
    //  commented for test lv_img_set_src(img1, &oldfluxcompat); //binds image object to image variable
    //  commented for test lv_obj_align(img1, NULL, LV_ALIGN_CENTER, 0, -20); //sets image object alignment 

    //show fluxion logo on main screen  (acesses image from binary file on sd card, allows compression while supporting older lvgl lib)
    lv_obj_t * img1 = lv_img_create(lv_scr_act(), NULL); //creates image object
    lv_img_set_src(img1, "fluxlogo_bin.bin"); //binds image object to external bin file on sd card
    lv_obj_set_pos(img1, 0, 0);
    //lv_obj_align(img1, NULL, LV_ALIGN_CENTER, 0, -20); //sets image object alignment 


    // ReportStatus("lcd: %d %d %s\n, id, toggled, label");
    lv_obj_t * btn = lv_btn_create(container, NULL);
    lv_btn_set_toggle(btn, true);
    
    if (prevElement)
        lv_obj_align(btn, prevElement, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    else
        lv_obj_set_pos(btn, 40, 40);

    lv_cont_set_fit(btn, true, true); // enable auto-resize
    // lv_obj_set_size(btn, 100, 50);
    lv_btn_set_toggle(btn, true); // it's a toggle-button
    
    if (toggled)
        lv_btn_set_state(btn, LV_BTN_STATE_TGL_REL);

    lv_obj_set_free_num(btn, id);

    lv_btn_set_action(btn, LV_BTN_ACTION_CLICK, click_action); 

    /*Add text*/
    lv_obj_t * labelEl = lv_label_create(btn, NULL);
    lv_label_set_text(labelEl, label);    

    return btn;
}

void LCD::CreateControls()
{
    m_textobj = lv_label_create(lv_scr_act(), NULL);
    lv_obj_align(m_textobj, NULL, LV_ALIGN_IN_BOTTOM_LEFT, 100, -30);
    lv_label_set_text(m_textobj, "");

    m_battery = lv_label_create(lv_scr_act(), NULL);
    lv_obj_align(m_battery, m_textobj, LV_ALIGN_OUT_TOP_LEFT, 0, -10);

    // auto container = lv_scr_act();
    auto container  = lv_cont_create(lv_scr_act(), NULL);

    lv_obj_t* last = nullptr;

    for (int i = 0; i < CountOf(m_buttons); i++){
        last = CreateButton(
            i,
            m_buttons[i].value ? m_buttons[i].label : m_buttons[i].label2, 
            container,
            last,
            m_buttons[i].value);
    }

    lv_cont_set_fit(container, true, true);
    lv_obj_align(container, NULL, LV_ALIGN_IN_TOP_MID, 0, 10);
}

void LCD::Update()
{
    if ((m_count % 50) == 0)
    {
        sprintf(m_batteryBuffer, "Battery %.0f %%", battery_get_capacity());
        lv_label_set_text(m_battery, m_batteryBuffer);
    }
    m_count++;
}

void LCD::PrintMessage(const char *message)
{
    lv_label_set_text(m_textobj, message);
}
