#include "ui_pages.h"

/* External variable declarations */
extern device_status_t robot_status;
extern robot_status_t robot_current_status;

/* Global variables */
static lv_obj_t *mode_buttons[3] = {NULL, NULL, NULL};
static lv_obj_t *status_labels[3] = {NULL, NULL, NULL};

/* Button click event handler function */
static void button_click_event(lv_event_t *e)
{
    lv_obj_t *target = lv_event_get_target(e);
    int i;
    
    // Iterate through all buttons, set clicked button to ON, others to OFF
    for (i = 0; i < 3; i++) {
        if (mode_buttons[i] == target) {
            // Set clicked button to ON
            lv_label_set_text(status_labels[i], "ON");
            lv_obj_set_style_text_color(status_labels[i], lv_color_hex(0xFCE616), 0);
            
            // Update robot status
            switch (i) {
                case 0: // Clean up
                    robot_current_status = ROBOT_STATUS_CLEANING;
                    break;
                case 1: // Charging
                    robot_current_status = ROBOT_STATUS_CHARGING;
                    break;
                case 2: // Automatic
                    robot_current_status = ROBOT_STATUS_CLEANING; // Automatic mode starts cleaning by default
                    break;
            }
        } else {
            // Set other buttons to OFF
            lv_label_set_text(status_labels[i], "OFF");
            lv_obj_set_style_text_color(status_labels[i], lv_color_hex(0x888888), 0);
        }
    }
    
    // Update UI status
    update_robot_ui();
}

/* Robot device card click event handler function */
static void robot_card_click_event(lv_event_t *e)
{
    // Get device index
    int device_index = (int)lv_event_get_user_data(e);
    
    // Toggle device status
    bool new_state = !robot_devices[device_index].is_running;
    robot_devices[device_index].is_running = new_state;
    
    // Update robot status summary
    update_robot_summary();
}

/* Initialize robot control page */
void init_robot_page(lv_obj_t *parent)
{
    // 1. Basic page style: white background, disable scrolling
    lv_obj_set_style_bg_color(parent, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_pad_all(parent, 0, 0);
    lv_obj_clear_flag(parent, LV_OBJ_FLAG_SCROLLABLE);

    // 2. Create main container
    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_size(cont, 780, 360); 
    lv_obj_align(cont, LV_ALIGN_TOP_MID, 0, 15);
    lv_obj_set_style_bg_opa(cont, 0, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_pad_all(cont, 5, 0);
    
    // Set Flex layout
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    // Set spacing between cards
    lv_obj_set_style_pad_row(cont, 15, 0);
    lv_obj_set_style_pad_column(cont, 10, 0);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

    // 3. Define common card style
    static lv_style_t style_card;
    if(style_card.prop_cnt == 0)
    {
        lv_style_init(&style_card);
        lv_style_set_border_color(&style_card, lv_color_hex(0x00A0E9));
        lv_style_set_border_width(&style_card, 4);
        lv_style_set_radius(&style_card, 25);
        lv_style_set_bg_color(&style_card, lv_color_hex(0xFFFFFF));
        lv_style_set_shadow_width(&style_card, 10);
        lv_style_set_shadow_opa(&style_card, 20);
        lv_style_set_shadow_ofs_y(&style_card, 5);
    }

    // 4. Loop to create robot device cards
    int i;
    for (i = 0; i < robot_device_count; i++) {
        // Create card body
        lv_obj_t *card = lv_obj_create(cont);
        lv_obj_set_size(card, 340, 155);
        lv_obj_add_style(card, &style_card, 0);
        lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

        // Status display (top left)
        lv_obj_t *status_label = lv_label_create(card);
        const char *status_str;
        lv_color_t color;
        
        if (robot_devices[i].is_running) {
            status_str = "Clearing";
            color = lv_color_hex(0x1396CE);
        } else {
            status_str = "Standby";
            color = lv_color_hex(0xCCCCCC);
        }
        
        lv_label_set_text(status_label, status_str);
        lv_obj_align(status_label, LV_ALIGN_TOP_LEFT, 0, 0);
        lv_obj_set_style_text_color(status_label, color, 0);
        lv_obj_set_style_text_font(status_label, &lv_font_montserrat_20, 0);

        // Device name (bottom center)
        lv_obj_t *name_label = lv_label_create(card);
        lv_label_set_text(name_label, robot_devices[i].name);
        lv_obj_align(name_label, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_obj_set_style_text_font(name_label, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_color(name_label, lv_color_hex(0x1D1D1F), 0);

        // Add click event to card
        lv_obj_add_event_cb(card, robot_card_click_event, LV_EVENT_CLICKED, (void *)i);

        // Store UI object handles in array for subsequent gesture logic to update status
        robot_devices[i].card = card;
        robot_devices[i].status_label = status_label;
    }

    // Create battery status display
    lv_obj_t *battery_label = lv_label_create(parent);
    lv_label_set_text(battery_label, "Battery: 100%");
    lv_obj_align(battery_label, LV_ALIGN_BOTTOM_MID, 0, -50);
    lv_obj_set_style_text_font(battery_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(battery_label, lv_color_hex(0x1D1D1F), 0);

    // Create top right warning icon
    // lv_obj_t *alert_icon = lv_label_create(parent);
    // lv_label_set_text(alert_icon, LV_SYMBOL_WARNING);
    // lv_obj_align(alert_icon, LV_ALIGN_TOP_RIGHT, -20, 20);
    // lv_obj_set_style_text_color(alert_icon, lv_color_hex(0xA15B9A), 0);
    // lv_obj_set_style_text_font(alert_icon, &lv_font_montserrat_32, 0);
}

/* Initialize robot settings sub-page (overlay) */
void init_robot_settings(lv_obj_t *parent)
{
    // No longer create prompt screen, directly create an empty sub-page
    robot_sub_page = lv_obj_create(parent);
    if (robot_sub_page) {
        lv_obj_set_size(robot_sub_page, lv_pct(100), lv_pct(100));
        lv_obj_align(robot_sub_page, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_bg_opa(robot_sub_page, 0, 0);
        lv_obj_set_style_border_width(robot_sub_page, 0, 0);
        
        /* Default hide this overlay */
        lv_obj_add_flag(robot_sub_page, LV_OBJ_FLAG_HIDDEN);
    }
}
