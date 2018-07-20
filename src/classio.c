// Standard includes
#include "pebble.h"


// App-specific data
Window *window; // All apps must have at least one window
TextLayer *time_layer; 
TextLayer *minute_layer;
TextLayer *battery_layer;
TextLayer *connection_layer;

static GBitmap *time_format_image;
static BitmapLayer *time_format_layer;

static void handle_battery(BatteryChargeState charge_state) {
  static char battery_text[] = "100";

  if (charge_state.is_charging) {
    snprintf(battery_text, sizeof(battery_text), "00");
  } else {
    snprintf(battery_text, sizeof(battery_text), "%d", charge_state.charge_percent/10);
  }
  text_layer_set_text(battery_layer, battery_text);
}

static void set_container_image(GBitmap **bmp_image, BitmapLayer *bmp_layer, const int resource_id, GPoint origin) {
  GBitmap *old_image = *bmp_image;

  *bmp_image = gbitmap_create_with_resource(resource_id);
  GRect frame = (GRect) {
    .origin = origin,
    .size = (*bmp_image)->bounds.size
  };
  bitmap_layer_set_bitmap(bmp_layer, *bmp_image);
  layer_set_frame(bitmap_layer_get_layer(bmp_layer), frame);

  if (old_image != NULL) {
    gbitmap_destroy(old_image);
  }
}

static void update_display(struct tm *current_time) {
  static char time_text[] = "88"; // Needs to be static because it's used by the system later.
  static char hr_text[] = "88";
  strftime(time_text, sizeof(time_text), "%M", current_time);
  strftime(hr_text, sizeof(hr_text), "%I", current_time);
  text_layer_set_text(minute_layer, time_text);
  text_layer_set_text(time_layer, hr_text);
  handle_battery(battery_state_service_peek());
  if (current_time->tm_hour >= 12) {
      set_container_image(&time_format_image, time_format_layer, RESOURCE_ID_IMAGE_PM_ICON, GPoint(114, 70+30));
    } else {
      set_container_image(&time_format_image, time_format_layer, RESOURCE_ID_IMAGE_AM_ICON, GPoint(114, 70+30));
    }

}

// Called once per second
static void handle_minute_tick(struct tm* tick_time, TimeUnits units_changed) {
  update_display(tick_time);
}

static void handle_hour_tick(struct tm* tick_time, TimeUnits units_changed) {
  static char time_text[] = "00"; // Needs to be static because it's used by the system later.

  strftime(time_text, sizeof(time_text), "%I", tick_time);
  text_layer_set_text(time_layer, time_text);

  handle_battery(battery_state_service_peek());
}

static void handle_bluetooth(bool connected) {
  text_layer_set_text(connection_layer, connected ? "8" : " ");
}

// Handle the start-up of the app
static void do_init(void) {

  // Create our app's base window
  window = window_create();
  window_stack_push(window, true);
  window_set_background_color(window, GColorBlack);

  Layer *root_layer = window_get_root_layer(window);
  GRect frame = layer_get_frame(root_layer);

  // Init the text layer used to show the time
  time_layer = text_layer_create(GRect(0, 0+30, frame.size.w-30 /* width */, 44/* height */));
  text_layer_set_text_color(time_layer, GColorWhite);
  text_layer_set_background_color(time_layer, GColorClear);
  text_layer_set_font(time_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_PORSCHE_44)));
  text_layer_set_text_alignment(time_layer, GTextAlignmentRight);

  minute_layer = text_layer_create(GRect(0, 44+30, frame.size.w-30 /* width */, 44/* height */));
  text_layer_set_text_color(minute_layer, GColorWhite);
  text_layer_set_background_color(minute_layer, GColorClear);
  text_layer_set_font(minute_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_PORSCHE_44)));
  text_layer_set_text_alignment(minute_layer, GTextAlignmentRight);

  connection_layer = text_layer_create(GRect(2, 10, /* width */ 70, 18 /* height */));
  text_layer_set_text_color(connection_layer, GColorWhite);
  text_layer_set_background_color(connection_layer, GColorClear);
  text_layer_set_font(connection_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_PORSCHE_18)));
  text_layer_set_text_alignment(connection_layer, GTextAlignmentLeft);
  handle_bluetooth(bluetooth_connection_service_peek());

  battery_layer = text_layer_create(GRect(72, 10, /* width */ 72, 18 /* height */));
  text_layer_set_text_color(battery_layer, GColorWhite);
  text_layer_set_background_color(battery_layer, GColorClear);
  text_layer_set_font(battery_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_PORSCHE_18)));
  text_layer_set_text_alignment(battery_layer, GTextAlignmentRight);
  text_layer_set_text(battery_layer, "100");

  time_format_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_AM_ICON);
    GRect frames = (GRect) {
      .origin = { .x = 114, .y = 70+30 },
      .size = time_format_image->bounds.size
    };
    //Layer *window_layer = window_get_root_layer(window);
    time_format_layer = bitmap_layer_create(frames);
    bitmap_layer_set_bitmap(time_format_layer, time_format_image);
    layer_add_child(root_layer, bitmap_layer_get_layer(time_format_layer));

  // Ensures time is displayed immediately (will break if NULL tick event accessed).
  // (This is why it's a good idea to have a separate routine to do the update itself.)
  time_t now = time(NULL);
  struct tm *current_time = localtime(&now);
  handle_minute_tick(current_time, MINUTE_UNIT);
  //handle_hour_tick(current_time, HOUR_UNIT);

  tick_timer_service_subscribe(MINUTE_UNIT, &handle_minute_tick);
  battery_state_service_subscribe(&handle_battery);
  bluetooth_connection_service_subscribe(&handle_bluetooth);

  layer_add_child(root_layer, text_layer_get_layer(time_layer));
  layer_add_child(root_layer, text_layer_get_layer(minute_layer));
  //layer_add_child(root_layer, text_layer_get_layer(format_layer));
  layer_add_child(root_layer, text_layer_get_layer(connection_layer));
  layer_add_child(root_layer, text_layer_get_layer(battery_layer));
}

static void do_deinit(void) {
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();
  text_layer_destroy(time_layer);
  //text_layer_destroy(format_layer);
  text_layer_destroy(connection_layer);
  text_layer_destroy(battery_layer);
  text_layer_destroy(minute_layer);

  layer_remove_from_parent(bitmap_layer_get_layer(time_format_layer));
  bitmap_layer_destroy(time_format_layer);
  gbitmap_destroy(time_format_image);

  window_destroy(window);
}

// The main event/run loop for our app
int main(void) {
  do_init();
  app_event_loop();
  do_deinit();
}
