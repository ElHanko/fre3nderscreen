#include "button_container.h"
#include "config.h"
#include "spdlog/spdlog.h"

ButtonContainer::ButtonContainer(lv_obj_t *parent,
				 const void *btn_img,
				 const char *text,
				 lv_event_cb_t cb,
				 void* user_data,
				 const std::string &prompt,
				 const std::function<void()> &pcb)
  : btn_cont(lv_obj_create(parent))
  , btn(lv_imagebutton_create(btn_cont))
  , label(lv_label_create(btn_cont))
  , prompt_text(prompt)
  , prompt_callback(pcb)
{
  lv_obj_set_style_pad_all(btn_cont, 0, 0);
  auto width_scale = (double)lv_display_get_physical_horizontal_resolution(NULL) / 800.0;
  lv_obj_set_size(btn_cont, 150 * width_scale, LV_SIZE_CONTENT);

  lv_obj_set_scrollable(btn_cont, false);
  // lv_obj_add_style(btn_cont, &style, LV_PART_MAIN);
  lv_imagebutton_set_src(btn, LV_IMAGEBUTTON_STATE_RELEASED, NULL, btn_img, NULL);
  lv_obj_set_width(btn, LV_SIZE_CONTENT);
  // lv_obj_align(btn, LV_ALIGN_CENTER, 0, 0);
  lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, 0);

  lv_obj_set_event_bubble(btn, true);
  // lv_obj_set_event_bubble(btn_cont, true);

  Config *conf = Config::get_instance();
  auto estop = conf->get_json("/prompt_emergency_stop");
  auto prompt_estop = estop.is_null() ? false : estop.template get<bool>();

  if (cb != NULL && !pcb) {
    lv_obj_add_event_cb(btn_cont, &ButtonContainer::_handle_callback, LV_EVENT_PRESSED, this);
    lv_obj_add_event_cb(btn_cont, &ButtonContainer::_handle_callback, LV_EVENT_RELEASED, this);
    
    lv_obj_add_event_cb(btn_cont, cb, LV_EVENT_CLICKED, user_data);
  }
  else if (cb != NULL && pcb) {
    if (prompt_estop) {
      lv_obj_add_event_cb(btn_cont, [](lv_event_t *e) {
	lv_event_code_t code = lv_event_get_code(e);
	if (code == LV_EVENT_CLICKED) {
	  ((ButtonContainer*)lv_event_get_user_data(e))->handle_prompt();
	}
      }, LV_EVENT_CLICKED, this);
    } else {
      lv_obj_add_event_cb(btn_cont, &ButtonContainer::_handle_callback, LV_EVENT_PRESSED, this);
      lv_obj_add_event_cb(btn_cont, &ButtonContainer::_handle_callback, LV_EVENT_RELEASED, this);
    
      lv_obj_add_event_cb(btn_cont, cb, LV_EVENT_CLICKED, user_data);
    }
  }

  lv_label_set_text(label, text);
  lv_obj_set_width(label, LV_PCT(100));
  lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_color(label, lv_palette_darken(LV_PALETTE_GREY, 1), LV_STATE_DISABLED);

  lv_obj_align_to(label, btn, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
  // lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, 5);
  // lv_obj_set_style_border_width(btn_cont, 2, 0);
}

ButtonContainer::~ButtonContainer() {
}

lv_obj_t *ButtonContainer::get_container() {
  return btn_cont;
}

void ButtonContainer::disable() {
  lv_obj_set_disabled(btn, true);
  lv_obj_set_disabled(btn_cont, true);
  lv_obj_set_disabled(label, true);
  
}

void ButtonContainer::enable() {
  lv_obj_set_disabled(btn, false);
  lv_obj_set_disabled(btn_cont, false);
  lv_obj_set_disabled(label, false);
}

void ButtonContainer::hide() {
  lv_obj_set_hidden(btn_cont, true);
}

void ButtonContainer::handle_callback(lv_event_t *e) {
  const lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_PRESSED) {
    lv_imagebutton_set_state(btn, LV_IMAGEBUTTON_STATE_PRESSED);
  } else if (code == LV_EVENT_RELEASED) {
    lv_imagebutton_set_state(btn, LV_IMAGEBUTTON_STATE_RELEASED);
  }
}

void ButtonContainer::handle_prompt() {
  lv_obj_t *mbox = lv_msgbox_create(nullptr);
  lv_obj_t *msg = lv_msgbox_add_text(mbox, prompt_text.c_str());
  lv_obj_set_style_text_align(msg, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_width(msg, lv_pct(100));

  lv_obj_t *confirm = lv_msgbox_add_footer_button(mbox, "Confirm");
  lv_obj_t *cancel = lv_msgbox_add_footer_button(mbox, "Cancel");
  lv_obj_t *footer = lv_msgbox_get_footer(mbox);
  auto hscale = (double)lv_display_get_physical_vertical_resolution(nullptr) / 480.0;
  lv_obj_set_size(footer, lv_pct(90), 50 * hscale);
  lv_obj_set_size(mbox, lv_pct(50), lv_pct(35));

  lv_obj_add_event_cb(confirm, [](lv_event_t *e) {
    lv_obj_t *box = lv_obj_get_parent(lv_obj_get_parent(lv_event_get_target_obj(e)));
    static_cast<ButtonContainer *>(lv_event_get_user_data(e))->run_callback();
    lv_msgbox_close(box);
  }, LV_EVENT_CLICKED, this);
  lv_obj_add_event_cb(cancel, [](lv_event_t *e) {
    lv_msgbox_close(static_cast<lv_obj_t *>(lv_event_get_user_data(e)));
  }, LV_EVENT_CLICKED, mbox);
  lv_obj_center(mbox);
}

void ButtonContainer::run_callback() {
  prompt_callback();
}
