#include <pebble.h>
#include <stdio.h>

#define TAP_NOT_DATA false
#define SQUARE(x) x*x

#define MIN_THRESHOLD  5000000
#define MAX_THRESHOLD 15000000
#define XY_THRESHOLD 100000



int32_t logs[500];
unsigned count;

unsigned timer;
bool clapped;
int max_mag;
bool waiting_for_pos;
bool flat;

static Window *s_main_window;
static TextLayer *s_output_layer;

static void send(uint8_t cmd) {
  Tuplet val = TupletInteger(0, cmd);

  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);

  if (iter == NULL)
    return;

  dict_write_tuplet(iter, &val);
  dict_write_end(iter);

  app_message_outbox_send();
}

static int get_metric(AccelData data) {
//   return SQUARE(data.x) + SQUARE(data.y) + SQUARE(data.z);
  return data.z;
}

static void data_handler(AccelData *data, uint32_t num_samples) {
  // Long lived buffer
  static char s_buffer[128];

  int magnitude = get_metric(data[0]);
  int mag_x = data[0].x;
  int mag_y = data[0].y;
  logs[count] = magnitude;
  count++;

  // Compose string of all data
  snprintf(s_buffer, sizeof(s_buffer),
    "%d",
    magnitude
  );

  //Show the data
  text_layer_set_text(s_output_layer, s_buffer);
}

static void clap_detector(AccelData *data, uint32_t num_samples) {
  // Long lived buffer
  static char s_buffer[128];

  int magnitude = get_metric(data[0]);
  int mag_x = data[0].x;
  int mag_y = data[0].y;

  bool reset = false;

  if (timer != 0)
    timer++;

  if (timer > 25) {
    timer = 0;
    waiting_for_pos = false;
    reset = true;
  }

  if (SQUARE(mag_x) < XY_THRESHOLD && SQUARE(mag_y) < XY_THRESHOLD) {
    flat = true;
  }
  else {
    flat = false;
  }

//   if (magnitude > MIN_THRESHOLD) {
  if (!flat) {
    if (magnitude > 0 && waiting_for_pos) {
      waiting_for_pos = false;
    }
    if (magnitude < -1000) {
      max_mag = magnitude;
      if (timer > 1 && !waiting_for_pos) {
        clapped = !clapped;
        timer = 0;
        send(clapped);
      }
      else{
        timer = 1;
        waiting_for_pos = true;
      }
    }
  }

  (void)reset;
/*  if (reset)
    snprintf(s_buffer, sizeof(s_buffer), "RESET\ncnt: %d", timer);*/
  if (clapped)
    snprintf(s_buffer, sizeof(s_buffer), "ON\nmag: %d\ncnt: %d", max_mag, timer);
  else if (!clapped)
    snprintf(s_buffer, sizeof(s_buffer), "OFF\nmag: %d\ncnt: %d", max_mag, timer);

  //Show the data
  text_layer_set_text(s_output_layer, s_buffer);
}

static void tap_handler(AccelAxisType axis, int32_t direction) {
  switch (axis) {
  case ACCEL_AXIS_X:
    if (direction > 0) {
      text_layer_set_text(s_output_layer, "X axis positive.");
    } else {
      text_layer_set_text(s_output_layer, "X axis negative.");
    }
    break;
  case ACCEL_AXIS_Y:
    if (direction > 0) {
      text_layer_set_text(s_output_layer, "Y axis positive.");
    } else {
      text_layer_set_text(s_output_layer, "Y axis negative.");
    }
    break;
  case ACCEL_AXIS_Z:
    if (direction > 0) {
      text_layer_set_text(s_output_layer, "Z axis positive.");
    } else {
      text_layer_set_text(s_output_layer, "Z axis negative.");
    }
    break;
  }
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Message received!");
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect window_bounds = layer_get_bounds(window_layer);

  // Create output TextLayer
  s_output_layer = text_layer_create(GRect(5, 0, window_bounds.size.w - 10, window_bounds.size.h));
  text_layer_set_font(s_output_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  text_layer_set_text(s_output_layer, "No data yet.");
  text_layer_set_overflow_mode(s_output_layer, GTextOverflowModeWordWrap);
  layer_add_child(window_layer, text_layer_get_layer(s_output_layer));
}

static void main_window_unload(Window *window) {
  // Destroy output TextLayer
  text_layer_destroy(s_output_layer);
}

static void init() {
  // Create main Window
  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  window_stack_push(s_main_window, true);
  count = 0;
  timer = 0;
  clapped = false;
  waiting_for_pos = false;
  flat = true;

  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);

  app_message_open(10, 10);

  // Use tap service? If not, use data service
  if (TAP_NOT_DATA) {
    // Subscribe to the accelerometer tap service
    accel_tap_service_subscribe(tap_handler);
  } else {
    // Subscribe to the accelerometer data service
    int num_samples = 1;
    accel_data_service_subscribe(num_samples, clap_detector);

    // Choose update rate
    accel_service_set_sampling_rate(ACCEL_SAMPLING_25HZ);
  }
}

static void deinit() {
  // Destroy main Window
  window_destroy(s_main_window);

  for (unsigned i = 0; i < count; i++)
    APP_LOG(APP_LOG_LEVEL_INFO, "%ld\n", logs[i]);

  if (TAP_NOT_DATA) {
    accel_tap_service_unsubscribe();
  } else {
    accel_data_service_unsubscribe();
  }
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}