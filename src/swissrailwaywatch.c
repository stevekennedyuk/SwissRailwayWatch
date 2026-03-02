#include "swissrailwaywatch.h"

// ─── Globals ─────────────────────────────────────────────────────────────────
static Window   *s_window;
static Layer    *s_canvas_layer;
static struct tm s_last_time;

// ─── Helpers ─────────────────────────────────────────────────────────────────

// Return a point on the circle at angle (seconds-style, 0=12 o'clock, CW)
// angle_deg_x100: angle in centidegrees (36000 = full rotation)
static GPoint polar(GPoint center, int32_t angle_deg_x100, int32_t radius) {
  // Use int64 to avoid overflow: 33000 * 65536 = 2.16e9 which overflows int32
  int32_t angle = (int32_t)(((int64_t)angle_deg_x100 * TRIG_MAX_ANGLE) / 36000);
  return GPoint(
    center.x + (int32_t)(radius * sin_lookup(angle) / TRIG_MAX_RATIO),
    center.y - (int32_t)(radius * cos_lookup(angle) / TRIG_MAX_RATIO)
  );
}

// Draw a thick hand as a filled rectangle (polygon) from center outward
static void draw_hand(GContext *ctx, GPoint center, int32_t angle_deg_x100,
                      int len, int tail, int width) {
  int32_t angle = (angle_deg_x100 * TRIG_MAX_ANGLE) / 36000;
  int32_t s = sin_lookup(angle);
  int32_t c = cos_lookup(angle);
  int hw = width / 2;

  // tip and back points along the hand axis
  GPoint tip  = GPoint(center.x + (int32_t)(len  * s / TRIG_MAX_RATIO),
                       center.y - (int32_t)(len  * c / TRIG_MAX_RATIO));
  GPoint back = GPoint(center.x - (int32_t)(tail * s / TRIG_MAX_RATIO),
                       center.y + (int32_t)(tail * c / TRIG_MAX_RATIO));

  // perpendicular offset
  int32_t px = (int32_t)(hw * c / TRIG_MAX_RATIO);
  int32_t py = (int32_t)(hw * s / TRIG_MAX_RATIO);

  GPoint pts[4] = {
    GPoint(tip.x  + px,  tip.y  + py),
    GPoint(tip.x  - px,  tip.y  - py),
    GPoint(back.x - px,  back.y - py),
    GPoint(back.x + px,  back.y + py),
  };
  GPathInfo info = { .num_points = 4, .points = pts };
  GPath *path = gpath_create(&info);
  gpath_draw_filled(ctx, path);
  gpath_draw_outline(ctx, path);
  gpath_destroy(path);
}

// ─── Draw callback ────────────────────────────────────────────────────────────
static void canvas_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GPoint center = GPoint(CENTER_X, CENTER_Y);

  // ── Background: black bezel ───────────────────────────────────────────────
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  // ── Dial: white filled circle ──────────────────────────────────────────────
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_circle(ctx, center, RADIUS);

  // ── Outer black ring ───────────────────────────────────────────────────────
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_stroke_width(ctx, 3);
  graphics_draw_circle(ctx, center, RADIUS);

  // ── Tick marks ─────────────────────────────────────────────────────────────
  // Minute ticks (thin, skip hour positions)
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_stroke_width(ctx, TICK_MINOR_W);
  for (int i = 0; i < 60; i++) {
    if (i % 5 == 0) continue;
    int32_t ang = i * 600;
    GPoint outer = polar(center, ang, RADIUS);
    GPoint inner = polar(center, ang, RADIUS - TICK_MINOR_LEN);
    graphics_draw_line(ctx, outer, inner);
  }

  // Hour indices (12, all equal size)
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_stroke_width(ctx, TICK_MAJOR_W);
  for (int i = 0; i < 12; i++) {
    int32_t ang = i * 3000;  // 360*100 / 12 = 3000
    GPoint outer = polar(center, ang, RADIUS);
    GPoint inner = polar(center, ang, RADIUS - TICK_MAJOR_LEN);
    graphics_draw_line(ctx, outer, inner);
  }

  // ── Day / Date box (near 3 o'clock) ──────────────────────────────────────
  // Right edge sits just inside the 3-o'clock tick inner end.
  // Inner end of a major tick is at radius (RADIUS - 2 - TICK_MAJOR_LEN) from center.
  int box_right = CENTER_X + (RADIUS - 2 - TICK_MAJOR_LEN) - 3;
  int box_left  = box_right - DATE_BOX_W;
  int box_top   = CENTER_Y - DATE_BOX_H / 2;

  // White fill + black border
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, GRect(box_left, box_top, DATE_BOX_W, DATE_BOX_H), 0, GCornerNone);
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_rect(ctx, GRect(box_left, box_top, DATE_BOX_W, DATE_BOX_H));

  // Divider between day and date halves
  int divider_y = box_top + DATE_BOX_H / 2;
  graphics_draw_line(ctx, GPoint(box_left, divider_y), GPoint(box_left + DATE_BOX_W - 1, divider_y));

  // Day abbreviation and date number
  static const char *s_days[] = {"SUN","MON","TUE","WED","THU","FRI","SAT"};
  const char *day_str = s_days[s_last_time.tm_wday];
  char date_str[3];
  snprintf(date_str, sizeof(date_str), "%d", s_last_time.tm_mday);

  // FONT_KEY_GOTHIC_14_BOLD renders ~14px tall but has ~2px internal top padding.
  // Subtract that offset so glyphs sit visually centred in each half-box.
  // Each half is DATE_BOX_H/2 = 14px tall; font content is ~11px; nudge down 1px.
  GFont font_small = fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD);
  // Gothic 14 Bold has significant internal descender space; the visible glyph
  // sits in roughly the top 10px of the 14px em-square. Pull the rect up so
  // the rendered text lands in the vertical centre of each 14px half-box.
  int text_y_off = -3;

  GRect day_rect  = GRect(box_left + 1, box_top     + text_y_off, DATE_BOX_W - 2, DATE_BOX_H / 2);
  GRect date_rect = GRect(box_left + 1, divider_y   + text_y_off, DATE_BOX_W - 2, DATE_BOX_H / 2);

  graphics_context_set_text_color(ctx, GColorBlack);
  graphics_draw_text(ctx, day_str,  font_small, day_rect,
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
  graphics_draw_text(ctx, date_str, font_small, date_rect,
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);

  int hour   = s_last_time.tm_hour % 12;
  int minute = s_last_time.tm_min;
  int second = s_last_time.tm_sec;

  // angle in centidegrees
  int32_t hour_angle = (hour * 3000) + (minute * 50);   // 360*100/12=3000, +smooth
  int32_t min_angle  = minute * 600;                     // 360*100/60=600
  int32_t sec_angle  = second * 600;

  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_stroke_width(ctx, 1);
  draw_hand(ctx, center, hour_angle, HOUR_LEN, HOUR_TAIL, HOUR_WIDTH);

  // ── Minute hand (black) ────────────────────────────────────────────────────
  draw_hand(ctx, center, min_angle, MIN_LEN, MIN_TAIL, MIN_WIDTH);

  // ── Second hand (red, thin line + circle at tip) ─────────────────────────
  graphics_context_set_stroke_color(ctx, GColorRed);
  graphics_context_set_stroke_width(ctx, 1);
  GPoint sec_tip = polar(center, sec_angle, RADIUS - TICK_MAJOR_LEN - 2);
  graphics_draw_line(ctx, center, sec_tip);

  // Solid red circle at the outer tip
  graphics_context_set_fill_color(ctx, GColorRed);
  graphics_fill_circle(ctx, sec_tip, SEC_CIRCLE_R);

  // ── Center dot ────────────────────────────────────────────────────────────
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_circle(ctx, center, 4);
  graphics_context_set_fill_color(ctx, GColorRed);
  graphics_fill_circle(ctx, center, 2);
}

// ─── Tick handler ─────────────────────────────────────────────────────────────
static void handle_tick(struct tm *tick_time, TimeUnits units_changed) {
  s_last_time = *tick_time;
  layer_mark_dirty(s_canvas_layer);
}

// ─── Window lifecycle ─────────────────────────────────────────────────────────
static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(root);

  s_canvas_layer = layer_create(bounds);
  layer_set_update_proc(s_canvas_layer, canvas_update_proc);
  layer_add_child(root, s_canvas_layer);

  // Get the current time immediately
  time_t now = time(NULL);
  s_last_time = *localtime(&now);
}

static void window_unload(Window *window) {
  layer_destroy(s_canvas_layer);
}

// ─── App lifecycle ────────────────────────────────────────────────────────────
static void init(void) {
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers){
    .load   = window_load,
    .unload = window_unload,
  });
  window_set_background_color(s_window, GColorWhite);
  window_stack_push(s_window, true);

  // Subscribe to second-level ticks for the second hand
  tick_timer_service_subscribe(SECOND_UNIT, handle_tick);
}

static void deinit(void) {
  tick_timer_service_unsubscribe();
  window_destroy(s_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
