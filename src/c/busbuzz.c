#include <pebble.h>

#define KEY_OP 0

#define KEY_RES_STOPS_COUNT 1
#define KEY_RES_BUSES_BUS_COUNT 2
#define KEY_RES_BUSES_STOP_INDEX 3

#define KEY_REQ_BUS_FOR_STOP_STOP_INDEX 1

#define KEY_REQ_STOP_AT_INDEX_INDEX 4

#define WATCH_OP_REQ_STOPS_COUNT 0
#define WATCH_OP_REQ_STOPS 1
#define WATCH_OP_REQ_BUS_FOR_STOP_COUNT 2
#define WATCH_OP_REQ_BUS_FOR_STOP_BUSES 3

#define PHONE_OP_RES_STOPS_COUNT 1
#define PHONE_OP_RES_STOPS 2
#define PHONE_OP_RES_BUSES_BUS_COUNT 3
#define PHONE_OP_RES_BUSES 4

static Window *window;
static Window *window_buses;

static SimpleMenuLayer *menu_layer;

static SimpleMenuLayer *buses_menu_layer;

static SimpleMenuItem items[] = {
  { "Loading", NULL, NULL, NULL },
  { "Loading", NULL, NULL, NULL },
  { "Loading", NULL, NULL, NULL },
  { "Loading", NULL, NULL, NULL },
  { "Loading", NULL, NULL, NULL }
};
static SimpleMenuSection sections[] = {
  { NULL, items, 5 }
};

static SimpleMenuItem bus_items[10] = {
  { "Loading", NULL, NULL, NULL },
  { "Loading", NULL, NULL, NULL },
  { "Loading", NULL, NULL, NULL },
  { "Loading", NULL, NULL, NULL },
  { "Loading", NULL, NULL, NULL },
  { "Loading", NULL, NULL, NULL },
  { "Loading", NULL, NULL, NULL },
  { "Loading", NULL, NULL, NULL },
  { "Loading", NULL, NULL, NULL },
  { "Loading", NULL, NULL, NULL }
};
static SimpleMenuSection bus_sections[] = {
  { NULL, bus_items, 10 }
};

static void bb_send_req_buses(int stopIndex) {
  DictionaryIterator *iter;
  int op = WATCH_OP_REQ_BUS_FOR_STOP_BUSES;
  if (app_message_outbox_begin(&iter) == APP_MSG_OK) {
    dict_write_int(iter, KEY_OP, &op, sizeof(int), true);
    dict_write_int(iter, KEY_REQ_BUS_FOR_STOP_STOP_INDEX, &stopIndex, sizeof(int), true);
    app_message_outbox_send();
    APP_LOG(APP_LOG_LEVEL_INFO, "Send request for buses at stop index  %d", stopIndex);
  } else {
    APP_LOG(APP_LOG_LEVEL_WARNING, "Outbox not ready");
  }
}
static void bb_send_req_buses_count(int stopIndex) {
  DictionaryIterator *iter;
  int op = WATCH_OP_REQ_BUS_FOR_STOP_COUNT;
  if (app_message_outbox_begin(&iter) == APP_MSG_OK) {
    dict_write_int(iter, KEY_OP, &op, sizeof(int), true);
    dict_write_int(iter, KEY_REQ_BUS_FOR_STOP_STOP_INDEX, &stopIndex, sizeof(int), true);
    app_message_outbox_send();
    APP_LOG(APP_LOG_LEVEL_INFO, "Sent request for bus count at index %d", stopIndex);
  } else {
    APP_LOG(APP_LOG_LEVEL_WARNING, "Outbox not ready");
  }
}

static void bb_click_stop(int index, void *context) {
  bb_send_req_buses_count(index);
}

static void bb_send_req_stops() {
  DictionaryIterator *iter;
  int op = WATCH_OP_REQ_STOPS;
  if (app_message_outbox_begin(&iter) == APP_MSG_OK) {
    dict_write_int(iter, KEY_OP, &op, sizeof(int), true);
    app_message_outbox_send();
    APP_LOG(APP_LOG_LEVEL_INFO, "Sent request for stops");
  } else {
    APP_LOG(APP_LOG_LEVEL_WARNING, "Outbox not ready");
  }

}

static void bb_bus_details_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  buses_menu_layer = simple_menu_layer_create(GRect(0, 0, bounds.size.w, bounds.size.h), window, bus_sections, 1, NULL);
  layer_add_child(window_layer, simple_menu_layer_get_layer(buses_menu_layer));
}

static void bb_bus_details_unload(Window *window) {
  simple_menu_layer_destroy(buses_menu_layer);
}

static void bb_show_bus_details() {
  window_buses = window_create();
  window_set_window_handlers(window_buses, (WindowHandlers) {
    .load = bb_bus_details_load,
    .unload = bb_bus_details_unload
  });
  window_stack_push(window_buses, true);
}

static void bb_receive_data(DictionaryIterator *iter, void *context) {
  Tuple *op = dict_find(iter, KEY_OP);
  if (op) {

    if (op->value->int32 == PHONE_OP_RES_STOPS_COUNT) {
        Tuple *count = dict_find(iter, KEY_RES_STOPS_COUNT);
        
        sections[0].num_items = count->value->int32;
        bb_send_req_stops();

        APP_LOG(APP_LOG_LEVEL_INFO, "Recieved stops count %d", *(count->value));
    } else if (op->value->int32 == PHONE_OP_RES_STOPS) {
       for (int i = 0; i < (int) sections[0].num_items; i++) {
          Tuple *id = dict_find(iter, (i * 2) + 1);
          Tuple *name = dict_find(iter, (i * 2) + 2);
          SimpleMenuItem item = { name->value->cstring, id->value->cstring, NULL, bb_click_stop };
          items[i] = item;
       }
       layer_mark_dirty(window_get_root_layer(window));
    } else if (op->value->int32 == PHONE_OP_RES_BUSES_BUS_COUNT) {
      Tuple *count = dict_find(iter, KEY_RES_BUSES_BUS_COUNT);
      Tuple *index = dict_find(iter, KEY_RES_BUSES_STOP_INDEX);

      bus_sections[0].num_items = count->value->int32;
      bb_send_req_buses(index->value->int32);
    } else if (op->value->int32 == PHONE_OP_RES_BUSES) {
      for (int i = 0; i < (int) bus_sections[0].num_items; i++) {
        Tuple *headsign = dict_find(iter, (i * 2) + 1);
        Tuple *relativeTime = dict_find(iter, (i * 2) + 2);
        SimpleMenuItem item = { headsign->value->cstring, relativeTime->value->cstring, NULL, NULL };
        bus_items[i] = item;

        bb_show_bus_details();
      }
    }

  } else {
    APP_LOG(APP_LOG_LEVEL_WARNING, "Received null message");
  }
}

static void bb_send_init_message() {
  DictionaryIterator *iter;
  int op = WATCH_OP_REQ_STOPS_COUNT;
  if (app_message_outbox_begin(&iter) == APP_MSG_OK) {
    dict_write_int(iter, KEY_OP, &op, sizeof(int), true);
    app_message_outbox_send();
    APP_LOG(APP_LOG_LEVEL_INFO, "Sent init request");
  } else {
    APP_LOG(APP_LOG_LEVEL_WARNING, "Outbox not ready");
  }
}


static void bb_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  menu_layer = simple_menu_layer_create(GRect(0, 0, bounds.size.w, bounds.size.h), window, sections, 1, NULL);
  layer_add_child(window_layer, simple_menu_layer_get_layer(menu_layer));

  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
  app_message_register_inbox_received(bb_receive_data);

  bb_send_init_message();
}


static void bb_window_unload(Window *window) {
  simple_menu_layer_destroy(menu_layer);
}

static void bb_init(void) {
  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = bb_window_load,
    .unload = bb_window_unload,
  });
  window_stack_push(window, true);
}

static void bb_deinit(void) {
  window_destroy(window);
  window_destroy(window_buses);
}

int main(void) {
  bb_init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

  app_event_loop();
  bb_deinit();
}
