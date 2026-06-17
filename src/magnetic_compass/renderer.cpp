#include "renderer.h"

#include <lvgl.h>

void Renderer_Init(void) {
  lv_obj_t *label = lv_label_create(lv_scr_act());
  lv_label_set_text(label, "1.47 Display Test");
  lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
}
