#include <lvgl.h>
#include <math.h>
#include <esp_heap_caps.h>   // heap_caps_malloc/free

class HornetADI_BallOnly {
public:
  HornetADI_BallOnly(lv_obj_t* parent, int ball_size_px)
  : size(ball_size_px)
  {
    canvas = lv_canvas_create(parent);
    lv_obj_set_size(canvas, size, size);
    lv_obj_center(canvas);

    // Allocate RGB565 canvas buffer
    size_t buf_size = LV_CANVAS_BUF_SIZE_TRUE_COLOR(size, size);
    buf = (lv_color_t*)heap_caps_malloc(buf_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if(!buf) buf = (lv_color_t*)heap_caps_malloc(buf_size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    ok = (buf != nullptr);

    if(ok) {
      lv_canvas_set_buffer(canvas, buf, size, size, LV_IMG_CF_TRUE_COLOR);
      lv_canvas_fill_bg(canvas, lv_color_black(), LV_OPA_COVER);
    }

    // Ensure canvas is opaque
    lv_obj_set_style_img_opa(canvas, LV_OPA_COVER, 0);
    lv_obj_set_style_opa(canvas, LV_OPA_COVER, 0);

    // Descriptors
    lv_draw_line_dsc_init(&line_dsc);
    line_dsc.opa = LV_OPA_COVER;

    lv_draw_label_dsc_init(&label_dsc);
    label_dsc.opa = LV_OPA_COVER;
    label_dsc.align = LV_TEXT_ALIGN_CENTER;

    // Default colors (Hornet-style BW ball)
    sky         = lv_color_white();
    ground      = lv_color_black();
    sym_sky     = lv_color_black();  // symbology on white sky
    sym_ground  = lv_color_white();  // symbology on black ground

    // Geometry
    cx = size / 2;
    cy = size / 2;
    clip_r = (size / 2) - 1;   // 300 -> 149

    // Ladder tuning
    px_per_deg = 5.0f;
    max_pitch_draw = 40;

    build_circle_spans();
  }

  ~HornetADI_BallOnly() {
    if(buf) heap_caps_free(buf);
  }

  // Access
  lv_obj_t* obj() { return canvas; }
  bool is_ok() const { return ok; }

  // Config
  void set_background_colors(lv_color_t sky_c, lv_color_t ground_c) {
    sky = sky_c;
    ground = ground_c;
  }
  void set_sym_contrast_colors(lv_color_t on_sky, lv_color_t on_ground) {
    sym_sky = on_sky;
    sym_ground = on_ground;
  }
  void set_px_per_deg(float v) { px_per_deg = v; }
  void set_max_pitch_draw(int deg) { max_pitch_draw = deg; }

  int get_pitch_px(float pitch_deg) const { return (int)lrintf(pitch_deg * px_per_deg); }
  int get_clip_radius() const { return clip_r; }

  // pitch_deg: +nose up
  // roll_deg : +right wing down
  void update(float pitch_deg, float roll_deg)
  {
    if(!ok) return;

    float rr = roll_deg * (float)M_PI / 180.0f;
    int32_t c = (int32_t)(cosf(rr) * 65536.0f);
    int32_t s = (int32_t)(sinf(rr) * 65536.0f);

    int pitch_px = get_pitch_px(pitch_deg);

    // 1) Sky/ground fill (robust)
    fill_sky_ground_signed_distance(c, s, pitch_px);

    // 2) Horizon line (two-tone)
    draw_horizon_two_tone(c, s, pitch_px);

    // 3) Micro ladder around horizon (0.5/1/2/2.5)
    draw_micro_ladder(c, s, pitch_px);

    // 4) Standard ladder (5° increments)
    draw_ladder(c, s, pitch_px);

    // 5) Special labels at ±1.5, ±3.0
    draw_special_pitch_labels(c, s, pitch_px);

    lv_obj_invalidate(canvas);
  }

private:
  // LVGL
  lv_obj_t* canvas = nullptr;
  lv_color_t* buf = nullptr;
  bool ok = false;

  // Geometry
  int size;
  int cx, cy;
  int clip_r;

  // Background
  lv_color_t sky, ground;

  // Dual sym
  lv_color_t sym_sky;
  lv_color_t sym_ground;

  // Ladder
  float px_per_deg;
  int max_pitch_draw;

  // Circle spans
  int16_t x_left[320];
  int16_t x_right[320];

  // Draw descriptors
  lv_draw_line_dsc_t  line_dsc;
  lv_draw_label_dsc_t label_dsc;

private:
  // ------------------------------------------------------------
  // Precompute circle spans
  // ------------------------------------------------------------
  void build_circle_spans()
  {
    for(int y = 0; y < size; y++) {
      int dy = y - cy;
      int dx2 = clip_r*clip_r - dy*dy;

      if(dx2 <= 0) {
        x_left[y]  = cx;
        x_right[y] = cx - 1;
      } else {
        int dx = (int)lrintf(sqrtf((float)dx2));   // round, don't truncate
        x_left[y]  = cx - dx;
        x_right[y] = cx + dx;
      }
    }
  }

  // ------------------------------------------------------------
  // Rotate local (x,y) to screen coords and add center
  // ------------------------------------------------------------
  inline void rot_xy(int x, int y, int32_t c, int32_t s, lv_point_t &out)
  {
    int32_t xr = ((x * c - y * s) >> 16);
    int32_t yr = ((x * s + y * c) >> 16);
    out.x = cx + (int)xr;
    out.y = cy + (int)yr;
  }

  // ------------------------------------------------------------
  // Clip a line segment to the circle boundary
  // ------------------------------------------------------------
  bool clip_line_to_circle(int &x1, int &y1, int &x2, int &y2)
  {
    float X1 = x1 - cx, Y1 = y1 - cy;
    float X2 = x2 - cx, Y2 = y2 - cy;
    float dx = X2 - X1, dy = Y2 - Y1;
    float rr = (float)clip_r * (float)clip_r;

    bool in1 = (X1*X1 + Y1*Y1) <= rr;
    bool in2 = (X2*X2 + Y2*Y2) <= rr;
    if(in1 && in2) return true;

    float a = dx*dx + dy*dy;
    float b = 2.0f*(X1*dx + Y1*dy);
    float cc = X1*X1 + Y1*Y1 - rr;
    float disc = b*b - 4.0f*a*cc;
    if(disc < 0) return false;

    float sd = sqrtf(disc);
    float t1 = (-b - sd) / (2.0f*a);
    float t2 = (-b + sd) / (2.0f*a);

    float tmin = fminf(t1, t2);
    float tmax = fmaxf(t1, t2);

    if(tmax < 0 || tmin > 1) return false;

    if(!in1) {
      float t = (tmin >= 0) ? tmin : tmax;
      X1 += dx*t; Y1 += dy*t;
    }
    if(!in2) {
      float t = (tmax <= 1) ? tmax : tmin;
      X2 += dx*t; Y2 += dy*t;
    }

    x1 = (int)lrintf(X1 + cx);
    y1 = (int)lrintf(Y1 + cy);
    x2 = (int)lrintf(X2 + cx);
    y2 = (int)lrintf(Y2 + cy);
    return true;
  }

  // ------------------------------------------------------------
  // Draw a clipped line with specified color
  // ------------------------------------------------------------
  void draw_line_clipped(lv_point_t p1, lv_point_t p2, int width, lv_color_t col)
  {
    int x1 = p1.x, y1 = p1.y, x2 = p2.x, y2 = p2.y;
    if(!clip_line_to_circle(x1, y1, x2, y2)) return;

    line_dsc.width = width;
    line_dsc.color = col;

    lv_point_t pts[2] = {
      {(lv_coord_t)x1, (lv_coord_t)y1},
      {(lv_coord_t)x2, (lv_coord_t)y2}
    };
    lv_canvas_draw_line(canvas, pts, 2, &line_dsc);
  }

  // ============================================================
  // Fill sky/ground using signed distance method (robust, no gaps)
  // ============================================================
  void fill_sky_ground_signed_distance(int32_t c, int32_t s, int pitch_px)
  {
    lv_canvas_fill_bg(canvas, ground, LV_OPA_COVER);

    // Horizon point
    lv_point_t h0;
    rot_xy(0, pitch_px, c, s, h0);

    // Normal vector N = (s, -c) points to sky side
    float nx = (float)s / 65536.0f;
    float ny = (float)(-c) / 65536.0f;

    const float epsilon = 1e-6f;
    if(fabsf(nx) < epsilon && fabsf(ny) < epsilon) return;

    for(int y = 0; y < size; y++) {
      int xl = x_left[y];
      int xr = x_right[y];
      if(xl > xr) continue;

      float dy = (float)(y - h0.y);

      float sideL = (float)(xl - h0.x) * nx + dy * ny;
      float sideR = (float)(xr - h0.x) * nx + dy * ny;

      // both sky
      if(sideL > 0 && sideR > 0) {
        line_dsc.width = 1;
        line_dsc.color = sky;
        lv_point_t pts[2] = { {(lv_coord_t)xl,(lv_coord_t)y}, {(lv_coord_t)xr,(lv_coord_t)y} };
        lv_canvas_draw_line(canvas, pts, 2, &line_dsc);
        continue;
      }

      // both ground
      if(sideL <= 0 && sideR <= 0) continue;

      // crossing
      float denom = (sideR - sideL);
      if(fabsf(denom) < epsilon) continue;

      float t = (0.0f - sideL) / denom;
      int x_cross = xl + (int)lrintf(t * (float)(xr - xl));

      int sky_l, sky_r;
      if(sideL > 0) {
        sky_l = xl;
        sky_r = x_cross;
      } else {
        sky_l = x_cross;
        sky_r = xr;
      }

      if(sky_l < xl) sky_l = xl;
      if(sky_r > xr) sky_r = xr;

      if(sky_l <= sky_r) {
        line_dsc.width = 1;
        line_dsc.color = sky;
        lv_point_t pts[2] = { {(lv_coord_t)sky_l,(lv_coord_t)y}, {(lv_coord_t)sky_r,(lv_coord_t)y} };
        lv_canvas_draw_line(canvas, pts, 2, &line_dsc);
      }
    }
  }

  // ============================================================
  // Horizon line: two-tone for contrast
  // ============================================================
  void draw_horizon_two_tone(int32_t c, int32_t s, int pitch_px)
  {
    const int half_len = (int)(size * 0.45f);
    const int gap = 26;

    auto draw_h = [&](int width, lv_color_t col) {
      lv_point_t p1, p2;

      rot_xy(-half_len, pitch_px, c, s, p1);
      rot_xy(-gap,      pitch_px, c, s, p2);
      draw_line_clipped(p1, p2, width, col);

      rot_xy(+gap,      pitch_px, c, s, p1);
      rot_xy(+half_len, pitch_px, c, s, p2);
      draw_line_clipped(p1, p2, width, col);
    };

    draw_h(6, sym_ground); // white base
    draw_h(3, sym_sky);    // black top
  }

  // ============================================================
  // Micro ladder: 0.5/1.0/2.0/2.5 degrees (single lines)
  // ============================================================
  void draw_micro_ladder(int32_t c, int32_t s, int pitch_px)
  {
    // Skip 1.5 and 3.0 here because special label functions draw them.
    const float degs[] = { 0.5f, 1.0f, 2.0f, 2.5f };
    const int count = sizeof(degs) / sizeof(degs[0]);

    for(int i = 0; i < count; i++) {
      float d = degs[i];
      draw_micro_line(+d, c, s, pitch_px);
      draw_micro_line(-d, c, s, pitch_px);
    }
  }

  void draw_micro_line(float deg, int32_t c, int32_t s, int pitch_px)
  {
    int y = pitch_px - (int)lrintf(deg * px_per_deg);
    if(y < -size || y > size) return;

    lv_color_t col = (y < pitch_px) ? sym_sky : sym_ground;

    bool is_int = fabsf(deg - roundf(deg)) < 0.01f;
    int half_len = (int)(size * (is_int ? 0.18f : 0.12f));
    int width = 2;

    lv_point_t p1, p2;
    rot_xy(-half_len, y, c, s, p1);
    rot_xy(+half_len, y, c, s, p2);

    draw_line_clipped(p1, p2, width, col);
  }

  // ============================================================
  // Standard ladder: 5° increments + hooks
  // ============================================================
  void draw_ladder(int32_t c, int32_t s, int pitch_px)
  {
    const int gap5  = 30;
    const int gap10 = 22;

    for(int deg = 5; deg <= max_pitch_draw; deg += 5) {
      draw_pitch_line(+deg, c, s, pitch_px, gap5, gap10);
      draw_pitch_line(-deg, c, s, pitch_px, gap5, gap10);
    }
  }

  void draw_pitch_line(int deg, int32_t c, int32_t s, int pitch_px, int /*gap5*/, int /*gap10*/)
  {
    int y = pitch_px - (int)lrintf(deg * px_per_deg);
    if(y < -size || y > size) return;

    lv_color_t col = (y < pitch_px) ? sym_sky : sym_ground;

    bool is10 = (deg % 10 == 0);
    bool is20 = (deg % 20 == 0);

    int half_len = is10 ? (int)(size * 0.30f) : (int)(size * 0.22f);
    int width    = is20 ? 3 : 2;

    lv_point_t p1, p2;
    rot_xy(-half_len, y, c, s, p1);
    rot_xy(+half_len, y, c, s, p2);
    draw_line_clipped(p1, p2, width, col);

    // Hooks on 5° lines (not on 10°)
    if(!is10) {
      int tick = 10;

      lv_point_t h1, h2;

      // left hook: at the left end of the line
      rot_xy(-half_len, y, c, s, h1);
      rot_xy(-half_len, y + tick, c, s, h2);
      draw_line_clipped(h1, h2, 2, col);

      // right hook: at the right end of the line
      rot_xy(+half_len, y, c, s, h1);
      rot_xy(+half_len, y + tick, c, s, h2);
      draw_line_clipped(h1, h2, 2, col);
    }
  }

  // ============================================================
  // Special labels: ±1.5 CLIMB/DIVE, ±3.0 3--0
  // ============================================================
  void draw_special_pitch_labels(int32_t c, int32_t s, int pitch_px)
  {
    draw_split_label_at_pitch(+1.5f, "CLI", "MB", c, s, pitch_px);
    draw_split_label_at_pitch(-1.5f, "DI",  "VE", c, s, pitch_px);

    draw_split_label_at_pitch(+3.0f, "3",   "0",  c, s, pitch_px);
    draw_split_label_at_pitch(-3.0f, "3",   "0",  c, s, pitch_px);
  }

  void draw_split_label_at_pitch(float deg,
                               const char* leftTxt,
                               const char* rightTxt,
                               int32_t c, int32_t s,
                               int pitch_px)
  {
    int y = pitch_px - (int)lrintf(deg * px_per_deg);
    if(y < -size || y > size) return;

    lv_color_t col = (y < pitch_px) ? sym_sky : sym_ground;

    bool is3 = (fabsf(deg) >= 2.9f);
    int half_len = (int)(size * (is3 ? 0.22f : 0.18f));
    int width    = 2;

    // 1) Draw ONE continuous pitch line
    lv_point_t p1, p2;
    rot_xy(-half_len, y, c, s, p1);
    rot_xy(+half_len, y, c, s, p2);
    draw_line_clipped(p1, p2, width, col);

    // 2) Draw split text ON TOP of the line, so it visually "breaks" it
    const int text_y_offset = -6;

    // place text near center, left and right
    int split_dx = is3 ? 20 : 30;

    lv_point_t tL, tR;
    rot_xy(-split_dx, y + text_y_offset, c, s, tL);
    rot_xy(+split_dx, y + text_y_offset, c, s, tR);

    label_dsc.color = col;

    lv_canvas_draw_text(canvas, tL.x - 22, tL.y - 10, 44, &label_dsc, leftTxt);
    lv_canvas_draw_text(canvas, tR.x - 22, tR.y - 10, 44, &label_dsc, rightTxt);
  }
};