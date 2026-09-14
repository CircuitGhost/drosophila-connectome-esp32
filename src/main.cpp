#include <Arduino.h>
#include "Display_ST7789.h"

#define BOOT_BTN_PIN 9
#ifndef RGB_BUILTIN
#define RGB_BUILTIN 8
#endif

// WS2812 on Waveshare board requires (Green, Red, Blue) byte order in neopixelWrite
void set_led(uint8_t r, uint8_t g, uint8_t b) {
  neopixelWrite(RGB_BUILTIN, g, r, b);
}

// =========================================================================
// Graphics Engine (Double-Buffered Half-Screen Framebuffer: 172 x 160 x 2B)
// =========================================================================
#define FB_W 172
#define FB_H 160
uint16_t fb[FB_W * FB_H];

// Fast Color Swap for ST7789 (Big Endian)
inline uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) {
  uint16_t c = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
  return (c >> 8) | (c << 8);
}

// Colors
#define C_BLACK       rgb(0, 0, 0)
#define C_WHITE       rgb(255, 255, 255)
#define C_RED         rgb(255, 30, 30)
#define C_GREEN       rgb(40, 240, 50)
#define C_BLUE        rgb(40, 100, 255)
#define C_YELLOW      rgb(255, 230, 40)
#define C_MAGENTA     rgb(240, 50, 240)
#define C_CYAN        rgb(40, 240, 240)
#define C_ORANGE      rgb(255, 140, 0)
#define C_DARKGREY    rgb(35, 40, 50)
#define C_MIDGREY     rgb(70, 80, 100)
#define C_LIGHTGREY   rgb(160, 175, 195)
#define C_BG_TOP      rgb(15, 20, 30)
#define C_BG_BOT      rgb(5, 5, 8)

void fb_clear(uint16_t col) {
  for (int i = 0; i < FB_W * FB_H; i++) fb[i] = col;
}

inline void fb_pixel(int x, int y, uint16_t col) {
  if (x >= 0 && x < FB_W && y >= 0 && y < FB_H) {
    fb[y * FB_W + x] = col;
  }
}

void fb_fillRect(int x, int y, int w, int h, uint16_t col) {
  int x0 = max(0, x), x1 = min(FB_W, x + w);
  int y0 = max(0, y), y1 = min(FB_H, y + h);
  for (int cy = y0; cy < y1; cy++) {
    int row = cy * FB_W;
    for (int cx = x0; cx < x1; cx++) {
      fb[row + cx] = col;
    }
  }
}

void fb_drawRect(int x, int y, int w, int h, uint16_t col) {
  for (int cx = x; cx < x + w; cx++) { fb_pixel(cx, y, col); fb_pixel(cx, y + h - 1, col); }
  for (int cy = y; cy < y + h; cy++) { fb_pixel(x, cy, col); fb_pixel(x + w - 1, cy, col); }
}

void fb_drawLine(int x0, int y0, int x1, int y1, uint16_t col) {
  int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int err = dx + dy, e2;
  while (true) {
    fb_pixel(x0, y0, col);
    if (x0 == x1 && y0 == y1) break;
    e2 = 2 * err;
    if (e2 >= dy) { err += dy; x0 += sx; }
    if (e2 <= dx) { err += dx; y0 += sy; }
  }
}

void fb_drawCircle(int cx, int cy, int r, uint16_t col) {
  int f = 1 - r, ddF_x = 1, ddF_y = -2 * r, x = 0, y = r;
  fb_pixel(cx, cy + r, col); fb_pixel(cx, cy - r, col);
  fb_pixel(cx + r, cy, col); fb_pixel(cx - r, cy, col);
  while (x < y) {
    if (f >= 0) { y--; ddF_y += 2; f += ddF_y; }
    x++; ddF_x += 2; f += ddF_x;
    fb_pixel(cx + x, cy + y, col); fb_pixel(cx - x, cy + y, col);
    fb_pixel(cx + x, cy - y, col); fb_pixel(cx - x, cy - y, col);
    fb_pixel(cx + y, cy + x, col); fb_pixel(cx - y, cy + x, col);
    fb_pixel(cx + y, cy - x, col); fb_pixel(cx - y, cy - x, col);
  }
}

void fb_fillCircle(int cx, int cy, int r, uint16_t col) {
  for (int y = -r; y <= r; y++) {
    for (int x = -r; x <= r; x++) {
      if (x * x + y * y <= r * r) {
        fb_pixel(cx + x, cy + y, col);
      }
    }
  }
}

// Built-in 5x7 Font for Ultra-Crisp Telemetry
const uint8_t font5x7[] PROGMEM = {
  0x00,0x00,0x00,0x00,0x00, // Space
  0x00,0x00,0x5F,0x00,0x00, // !
  0x00,0x07,0x00,0x07,0x00, // "
  0x14,0x7F,0x14,0x7F,0x14, // #
  0x24,0x2A,0x7F,0x2A,0x12, // $
  0x23,0x13,0x08,0x64,0x62, // %
  0x36,0x49,0x55,0x22,0x50, // &
  0x00,0x05,0x03,0x00,0x00, // '
  0x00,0x1C,0x22,0x41,0x00, // (
  0x00,0x41,0x22,0x1C,0x00, // )
  0x14,0x08,0x3E,0x08,0x14, // *
  0x08,0x08,0x3E,0x08,0x08, // +
  0x00,0x50,0x30,0x00,0x00, // ,
  0x08,0x08,0x08,0x08,0x08, // -
  0x00,0x60,0x60,0x00,0x00, // .
  0x20,0x10,0x08,0x04,0x02, // /
  0x3E,0x51,0x49,0x45,0x3E, // 0
  0x00,0x42,0x7F,0x40,0x00, // 1
  0x42,0x61,0x51,0x49,0x46, // 2
  0x21,0x41,0x45,0x4B,0x31, // 3
  0x18,0x14,0x12,0x7F,0x10, // 4
  0x27,0x45,0x45,0x45,0x39, // 5
  0x3C,0x4A,0x49,0x49,0x30, // 6
  0x01,0x71,0x09,0x05,0x03, // 7
  0x36,0x49,0x49,0x49,0x36, // 8
  0x06,0x49,0x49,0x29,0x1E, // 9
  0x00,0x36,0x36,0x00,0x00, // :
  0x00,0x56,0x36,0x00,0x00, // ;
  0x08,0x14,0x22,0x41,0x00, // <
  0x14,0x14,0x14,0x14,0x14, // =
  0x00,0x41,0x22,0x14,0x08, // >
  0x02,0x01,0x51,0x09,0x06, // ?
  0x32,0x49,0x79,0x41,0x3E, // @
  0x7E,0x11,0x11,0x11,0x7E, // A
  0x7F,0x49,0x49,0x49,0x36, // B
  0x3E,0x41,0x41,0x41,0x22, // C
  0x7F,0x41,0x41,0x22,0x1C, // D
  0x7F,0x49,0x49,0x49,0x41, // E
  0x7F,0x09,0x09,0x09,0x01, // F
  0x3E,0x41,0x49,0x49,0x7A, // G
  0x7F,0x08,0x08,0x08,0x7F, // H
  0x00,0x41,0x7F,0x41,0x00, // I
  0x20,0x40,0x41,0x3F,0x01, // J
  0x7F,0x08,0x14,0x22,0x41, // K
  0x7F,0x40,0x40,0x40,0x40, // L
  0x7F,0x02,0x0C,0x02,0x7F, // M
  0x7F,0x04,0x08,0x10,0x7F, // N
  0x3E,0x41,0x41,0x41,0x3E, // O
  0x7F,0x09,0x09,0x09,0x06, // P
  0x3E,0x41,0x51,0x21,0x5E, // Q
  0x7F,0x09,0x19,0x29,0x46, // R
  0x46,0x49,0x49,0x49,0x31, // S
  0x01,0x01,0x7F,0x01,0x01, // T
  0x3F,0x40,0x40,0x40,0x3F, // U
  0x1F,0x20,0x40,0x20,0x1F, // V
  0x7F,0x20,0x18,0x20,0x7F, // W
  0x63,0x14,0x08,0x14,0x63, // X
  0x07,0x08,0x70,0x08,0x07, // Y
  0x61,0x51,0x49,0x45,0x43  // Z
};

void fb_drawChar(int x, int y, char c, uint16_t col) {
  if (c >= 'a' && c <= 'z') c -= 32;
  if (c < 32 || c > 'Z') return;
  int idx = (c - 32) * 5;
  for (int i = 0; i < 5; i++) {
    uint8_t line = pgm_read_byte(&font5x7[idx + i]);
    for (int j = 0; j < 7; j++) {
      if (line & (1 << j)) {
        fb_pixel(x + i, y + j, col);
      }
    }
  }
}

void fb_print(int x, int y, const char* str, uint16_t col) {
  int cx = x;
  while (*str) {
    fb_drawChar(cx, y, *str, col);
    cx += 6;
    str++;
  }
}

// =========================================================================
// Fruit Fly Central Complex (CX) Connectome Model
// =========================================================================
const int N = 16;
const float TWO_PI_F = 6.2831853f;

// Connectome State
float EPG[N] = {0};            // 16 E-PG Compass Wedges
float W_rec[N][N];             // Recurrent excitation/inhibition
float ER[N] = {0};             // 16 Visual Ring Neurons
float W_plastic[N][N];         // Plastic synapses: ER -> E-PG

// Learning parameters
const float ETA = 0.012f;       // Learning rate
const float REC_EXC = 1.3f;    // Local excitation
const float REC_INH = 0.35f;   // Global inhibition
const float BETA_SHIFT = 0.55f;// Yaw shift gain
float dW_meter = 0.0f;

// Fan-Shaped Body (FB) Goal Vector & Dopamine
float FB_goal_angle = 0.0f;
float dopamine_PPL1 = 0.0f;
float dopamine_PAM = 0.0f;

// 2D Corridor Arena & Fly Physics
float fly_x = 86.0f;           // Fly position [28 to 144]
float fly_y = 115.0f;
float fly_heading = 0.0f;
float fly_speed = 1.4f;
float fly_turn = 0.0f;
float scroll_z = 0.0f;
float last_steer = 0.0f;

// Predator Attack State
bool predator_active = false;
float predator_x = 86.0f;      // Shadow location
float predator_y = 60.0f;
float shadow_radius = 0.0f;
int predator_timer = 0;
unsigned long next_predator_time = 0;

// Button state (BOOT = GPIO9)
bool btn_prev = HIGH;
unsigned long btn_time = 0;
bool btn_held_down = false;

// Behavioral Phase & Statistics
int sim_phase = 1;
unsigned long start_ms = 0;
int wall_bumps = 0;
int flies_eaten_count = 0;

// Reset all intelligence & neural state from scratch
void reset_intelligence() {
  for (int i = 0; i < N; i++) {
    for (int j = 0; j < N; j++) {
      float d = (float)(i - j) * (TWO_PI_F / (float)N);
      W_rec[i][j] = (REC_EXC * cosf(d)) - REC_INH;
      W_plastic[i][j] = 0.35f + 0.05f * ((float)rand() / (float)RAND_MAX);
    }
    float a = (float)i * (TWO_PI_F / (float)N);
    EPG[i] = max(0.0f, cosf(a));
    ER[i] = 0.0f;
  }
  fly_x = 86.0f;
  fly_y = 115.0f;
  fly_heading = 0.0f;
  fly_turn = 0.0f;
  FB_goal_angle = 0.0f;
  dopamine_PPL1 = 0.0f;
  dopamine_PAM = 0.0f;
  wall_bumps = 0;
  sim_phase = 1;
  start_ms = millis();
  predator_active = false;
  shadow_radius = 0.0f;
}

// Visual Ring Neurons (ER)
void update_visual_sensors() {
  float dist_left = (fly_x - 24.0f) / 60.0f;
  float dist_right = (148.0f - fly_x) / 60.0f;

  for (int k = 0; k < N; k++) {
    float eye_angle = fly_heading + ((float)k * (TWO_PI_F / (float)N));
    while (eye_angle > PI)  eye_angle -= TWO_PI_F;
    while (eye_angle < -PI) eye_angle += TWO_PI_F;

    float energy = 0.12f;
    // Left eye (wall stripes)
    if (eye_angle < -0.4f && eye_angle > -2.7f) {
      float stripe = 0.5f + 0.5f * sinf(scroll_z * 0.2f + eye_angle * 2.0f);
      energy += (0.75f / (dist_left + 0.25f)) * stripe;
    }
    // Right eye (wall stripes)
    else if (eye_angle > 0.4f && eye_angle < 2.7f) {
      float stripe = 0.5f + 0.5f * sinf(scroll_z * 0.2f - eye_angle * 2.0f);
      energy += (0.75f / (dist_right + 0.25f)) * stripe;
    }
    // Front open corridor
    else {
      energy += 0.4f * max(0.0f, cosf(eye_angle));
    }

    // Looming threat detected visually across front visual sectors!
    if (predator_active && shadow_radius > 6.0f && abs(eye_angle) < 1.4f) {
      energy += (shadow_radius / 35.0f) * 2.0f;
    }

    ER[k] = (ER[k] * 0.3f) + (energy * 0.7f);
  }
}

// Compass Attractor & Synaptic Plasticity
void update_compass_and_learning() {
  float EPG_next[N];
  float total_act = 0.0f;
  float total_dw = 0.0f;

  for (int i = 0; i < N; i++) {
    float rec = 0.0f;
    for (int j = 0; j < N; j++) rec += W_rec[i][j] * EPG[j];

    int l_idx = (i + 1) % N, r_idx = (i + N - 1) % N;
    float shift = 0.0f;
    if (fly_turn > 0.001f) shift = BETA_SHIFT * fly_turn * EPG[r_idx] * 6.0f;
    else if (fly_turn < -0.001f) shift = BETA_SHIFT * (-fly_turn) * EPG[l_idx] * 6.0f;

    float vis = 0.0f;
    for (int k = 0; k < N; k++) {
      vis -= W_plastic[k][i] * ER[k];
      // Anti-Hebbian depression
      float dw = ETA * ER[k] * EPG[i];
      W_plastic[k][i] = constrain(W_plastic[k][i] + dw, 0.05f, 1.6f);
      total_dw += abs(dw);
    }

    float raw = EPG[i] * 0.35f + rec * 0.35f + shift * 0.8f + (vis + 1.0f) * 0.3f;
    EPG_next[i] = max(0.0f, raw);
    total_act += EPG_next[i];
  }

  if (total_act > 0.001f) {
    for (int i = 0; i < N; i++) EPG[i] = (EPG_next[i] / total_act) * 2.2f;
  }
  dW_meter = total_dw;
}

// Steering computation from FB comparator
float compute_steering() {
  float s_sum = 0.0f, c_sum = 0.0f;
  for (int i = 0; i < N; i++) {
    float a = (float)i * (TWO_PI_F / (float)N);
    s_sum += EPG[i] * sinf(a);
    c_sum += EPG[i] * cosf(a);
  }
  float compass_angle = atan2f(s_sum, c_sum);
  float err = compass_angle - FB_goal_angle;
  while (err > PI)  err -= TWO_PI_F;
  while (err < -PI) err += TWO_PI_F;

  float steer = -sinf(err) * 0.09f;
  float l_vis = ER[12] + ER[13] + ER[14];
  float r_vis = ER[2] + ER[3] + ER[4];
  steer += (r_vis - l_vis) * 0.045f;
  return steer;
}

// Compute Real-time Connectome Learning & Intelligence Score [0% to 99%]
float compute_intelligence_score() {
  // 1. Compass bump sharpness (Contrast between peak node and baseline)
  float max_v = 0.0f, min_v = 999.0f, sum_v = 0.0f;
  for (int i = 0; i < N; i++) {
    if (EPG[i] > max_v) max_v = EPG[i];
    if (EPG[i] < min_v) min_v = EPG[i];
    sum_v += EPG[i];
  }
  float bump_contrast = (max_v - min_v) / ((sum_v / (float)N) + 0.01f);
  float bump_score = constrain((bump_contrast - 0.4f) / 1.8f, 0.0f, 1.0f);

  // 2. Synaptic Plasticity Differentiation (weights diverged from initial state)
  float weight_var = 0.0f;
  for (int k = 0; k < N; k++) {
    for (int i = 0; i < N; i++) {
      float diff = W_plastic[k][i] - 0.35f;
      weight_var += diff * diff;
    }
  }
  float syn_score = constrain(weight_var / 12.0f, 0.0f, 1.0f);

  // 3. Navigation Stability (Time in center vs wall collisions)
  unsigned long elapsed = (millis() - start_ms) / 1000;
  float stability_score = constrain((float)elapsed / 40.0f, 0.0f, 1.0f) * max(0.2f, 1.0f - ((float)wall_bumps * 0.12f));

  // Composite Learned Intelligence Score
  float total_iq = (bump_score * 0.45f + syn_score * 0.35f + stability_score * 0.20f) * 100.0f;
  return constrain(total_iq, 0.0f, 99.0f);
}

float learned_iq = 0.0f;

// Death / Eaten Animation & Full Brain Reset
void trigger_eaten_animation() {
  flies_eaten_count++;
  Serial.println("[DEATH] Fly Caught by Predator! Intelligence Resetting to 0%...");

  for (int f = 0; f < 6; f++) {
    set_led(255, 0, 0); // Flash RED
    fb_clear(C_RED);
    fb_print(42, 50, "FLY EATEN!", C_WHITE);
    fb_print(28, 70, "BRAIN REBOOTING...", C_YELLOW);
    LCD_addWindow(0, 0, FB_W - 1, FB_H - 1, fb);
    LCD_addWindow(0, 160, FB_W - 1, 319, fb);
    delay(120);

    set_led(0, 0, 0);
    fb_clear(C_BLACK);
    LCD_addWindow(0, 0, FB_W - 1, FB_H - 1, fb);
    LCD_addWindow(0, 160, FB_W - 1, 319, fb);
    delay(100);
  }

  // Complete wipe of synaptic learning & intelligence
  reset_intelligence();
}

// =========================================================================
// Simulation Step
// =========================================================================
void step_simulation() {
  learned_iq = compute_intelligence_score();

  // Predator is ONLY active when manually summoned by user holding BOOT button
  if (predator_active) {
    predator_timer++;
    shadow_radius += 1.35f; // Expanding shadow descending

    // Evasion agility is DIRECTLY tied to learned intelligence!
    // An untrained fly (low IQ) has sluggish escape velocity and weak steering
    // A trained fly (high IQ) has instantaneous reflex and high-speed evasive banking!
    float flee_direction = (fly_x >= predator_x) ? 1.0f : -1.0f;
    if (abs(fly_x - predator_x) < 8.0f) {
      flee_direction = (fly_x < 86.0f) ? -1.0f : 1.0f;
    }

    // Scale turn rate and speed based on IQ
    float agility = constrain(learned_iq / 100.0f, 0.12f, 1.0f);
    fly_turn = flee_direction * (0.07f + (agility * 0.28f));
    fly_speed = 1.1f + (agility * 1.9f);

    // Strike down!
    if (shadow_radius >= 38.0f) {
      float dist_to_shadow = abs(fly_x - predator_x);
      // If fly did not learn well enough to clear the shadow -> EATEN!
      if (dist_to_shadow < 24.0f) {
        trigger_eaten_animation();
        return;
      } else {
        // Fly successfully learned to evade!
        Serial.println("[SURVIVED] High IQ Fly Evaded Predator!");
        dopamine_PAM = 1.0f; // Big reward for survival!
        predator_active = false;
        shadow_radius = 0.0f;
      }
    }
  } else {
    // Normal Navigation Loop
    float s = compute_steering();
    last_steer = s;
    fly_turn = (fly_turn * 0.5f) + (s * 0.5f);
    fly_speed = 1.35f;
  }

  // Physical motion
  fly_heading += fly_turn;
  while (fly_heading > PI)  fly_heading -= TWO_PI_F;
  while (fly_heading < -PI) fly_heading += TWO_PI_F;

  fly_x += sinf(fly_heading) * fly_speed + (fly_turn * 2.2f);
  scroll_z += cosf(fly_heading) * fly_speed * 1.5f;

  // Wall collisions
  const float L_WALL = 30.0f, R_WALL = 142.0f;
  if (fly_x <= L_WALL) {
    fly_x = L_WALL + 1.0f;
    fly_heading = abs(fly_heading) + 0.35f;
    dopamine_PPL1 = 1.0f; // Punishment
    wall_bumps++;
    FB_goal_angle += 0.45f;
  } else if (fly_x >= R_WALL) {
    fly_x = R_WALL - 1.0f;
    fly_heading = -abs(fly_heading) - 0.35f;
    dopamine_PPL1 = 1.0f; // Punishment
    wall_bumps++;
    FB_goal_angle -= 0.45f;
  } else {
    dopamine_PAM = min(1.0f, dopamine_PAM + 0.05f);
    dopamine_PPL1 = max(0.0f, dopamine_PPL1 - 0.08f);
  }
  FB_goal_angle *= 0.985f;

  // Update Connectome
  update_visual_sensors();
  update_compass_and_learning();

  // Phase Progression
  unsigned long sec = (millis() - start_ms) / 1000;
  if (predator_active)                  sim_phase = 4; // GFN THREAT
  else if (sec > 35 && wall_bumps > 2)  sim_phase = 3; // CENTERING
  else if (sec > 10)                    sim_phase = 2; // LOCKED
  else                                  sim_phase = 1; // NAIVE

  // Onboard RGB LED feedback
  if (predator_active || dopamine_PPL1 > 0.3f) {
    set_led(180, 0, 0); // Red Flash: Wall Bump / Threat
  } else if (dopamine_PAM > 0.6f && abs(fly_x - 86.0f) < 22.0f) {
    set_led(0, 150, 0); // Green Pulse: Safe Centered Cruising
  } else {
    set_led(0, 20, 90); // Dim Blue: Steady Cruise
  }
}

// =========================================================================
// Framebuffer Rendering
// =========================================================================
void render_top_half() {
  fb_clear(C_BG_TOP);

  int sh = 16;
  int off = (int)scroll_z % (sh * 2);

  // Left Wall (Yellow / Blue)
  for (int y = -sh * 2; y < FB_H + sh; y += sh) {
    int cy = y + off;
    uint16_t c = (((cy / sh) % 2) == 0) ? C_YELLOW : C_BLUE;
    fb_fillRect(0, cy, 24, sh, c);
    fb_drawRect(0, cy, 24, sh, C_BLACK);
  }

  // Right Wall (Magenta / Green)
  for (int y = -sh * 2; y < FB_H + sh; y += sh) {
    int cy = y + off;
    uint16_t c = (((cy / sh) % 2) == 0) ? C_MAGENTA : C_GREEN;
    fb_fillRect(148, cy, 24, sh, c);
    fb_drawRect(148, cy, 24, sh, C_BLACK);
  }

  // Center track dashed line
  for (int y = 0; y < FB_H; y += 20) {
    int cy = (y + off) % FB_H;
    fb_drawLine(86, cy, 86, cy + 8, C_DARKGREY);
  }

  // Draw Looming Predator Shadow
  if (predator_active && shadow_radius > 1.0f) {
    fb_fillCircle((int)predator_x, (int)predator_y, (int)shadow_radius, C_RED);
    fb_drawCircle((int)predator_x, (int)predator_y, (int)shadow_radius + 2, C_WHITE);
    fb_print(28, 8, "! PREDATOR LOOMING !", C_WHITE);
  }

  // Draw 2D Fly Avatar
  int fx = (int)fly_x, fy = (int)fly_y;
  float h = fly_heading;
  int nx = fx + (int)(sinf(h) * 9.0f), ny = fy - (int)(cosf(h) * 9.0f);
  int ltx = fx - (int)(sinf(h + 2.4f) * 7.0f), lty = fy + (int)(cosf(h + 2.4f) * 7.0f);
  int rtx = fx - (int)(sinf(h - 2.4f) * 7.0f), rty = fy + (int)(cosf(h - 2.4f) * 7.0f);

  fb_drawLine(nx, ny, ltx, lty, C_ORANGE);
  fb_drawLine(nx, ny, rtx, rty, C_ORANGE);
  fb_drawLine(ltx, lty, rtx, rty, C_ORANGE);
  fb_fillCircle(fx, fy, 3, C_WHITE);

  // Flapping Wings
  int wf = (int)(sinf(millis() * 0.08f) * 5.0f);
  fb_drawLine(fx, fy, fx - 9, fy - 2 + wf, C_CYAN);
  fb_drawLine(fx, fy, fx + 9, fy - 2 + wf, C_CYAN);

  // Top Header Banner
  fb_fillRect(0, 0, FB_W, 12, C_BLACK);
  fb_print(24, 3, "2D OPTIC ARENA", C_WHITE);

  // Shock / Collision banner
  if (dopamine_PPL1 > 0.3f && !predator_active) {
    fb_fillRect(24, 144, 124, 14, C_RED);
    fb_print(34, 147, "WALL COLLISION!", C_WHITE);
  }

  // Flush to screen (0, 0)
  LCD_addWindow(0, 0, FB_W - 1, FB_H - 1, fb);
}

void render_bottom_half() {
  fb_clear(C_BG_BOT);
  fb_drawLine(0, 0, FB_W - 1, 0, C_MIDGREY);

  int cx = 86, cy = 76;
  int r_out = 48, r_in = 32;

  fb_drawCircle(cx, cy, r_out, C_DARKGREY);
  fb_drawCircle(cx, cy, r_in, C_MIDGREY);
  fb_drawCircle(cx, cy, 14, C_DARKGREY);

  // 1. Visual ER Ring (16 outer nodes)
  for (int k = 0; k < N; k++) {
    float a = (float)k * (TWO_PI_F / (float)N) - (PI / 2.0f);
    int nx = cx + (int)(cosf(a) * (float)r_out);
    int ny = cy + (int)(sinf(a) * (float)r_out);
    uint8_t br = constrain((int)(ER[k] * 180.0f), 30, 255);
    fb_fillCircle(nx, ny, 3, rgb(0, br, br / 2));
  }

  // 2. E-PG Compass Bump (16 inner nodes)
  float max_v = -1.0f, max_a = 0.0f;
  for (int i = 0; i < N; i++) {
    float a = (float)i * (TWO_PI_F / (float)N) - (PI / 2.0f);
    int nx = cx + (int)(cosf(a) * (float)r_in);
    int ny = cy + (int)(sinf(a) * (float)r_in);
    float v = EPG[i];
    if (v > max_v) { max_v = v; max_a = a; }

    int rad = constrain(2 + (int)(v * 3.5f), 2, 6);
    uint8_t r = constrain((int)(v * 240.0f), 20, 255);
    uint8_t g = constrain((int)(v * 160.0f), 10, 200);
    fb_fillCircle(nx, ny, rad, rgb(r, g, 30));
  }

  // 3. Goal Vector (Green)
  float ga = FB_goal_angle - (PI / 2.0f);
  int gx = cx + (int)(cosf(ga) * 26.0f), gy = cy + (int)(sinf(ga) * 26.0f);
  fb_drawLine(cx, cy, gx, gy, C_GREEN);
  fb_fillCircle(gx, gy, 2, C_GREEN);

  // 4. Heading Vector (Orange)
  int hx = cx + (int)(cosf(max_a) * 20.0f), hy = cy + (int)(sinf(max_a) * 20.0f);
  fb_drawLine(cx, cy, hx, hy, C_ORANGE);

  // Labels & Telemetry
  fb_print(22, 4, "CENTRAL COMPLEX", C_LIGHTGREY);

  const char* p_str = "P1: NAIVE";
  if (sim_phase == 2) p_str = "P2: LOCKED";
  if (sim_phase == 3) p_str = "P3: CENTERING";
  if (sim_phase == 4) p_str = "P4: GFN ESCAPE";
  fb_print(22, 16, p_str, C_ORANGE);

  char buf[32];
  snprintf(buf, sizeof(buf), "IQ: %d%%", (int)learned_iq);
  fb_print(4, 148, buf, C_GREEN);

  snprintf(buf, sizeof(buf), "DW:%0.2F", dW_meter);
  fb_print(64, 148, buf, C_CYAN);

  snprintf(buf, sizeof(buf), "S:%+0.1F", last_steer);
  fb_print(120, 148, buf, C_LIGHTGREY);

  // Flush to screen (0, 160)
  LCD_addWindow(0, 160, FB_W - 1, 319, fb);
}

// =========================================================================
// Setup & Main Loop
// =========================================================================
void setup() {
  Serial.begin(115200);
  pinMode(BOOT_BTN_PIN, INPUT_PULLUP);

  // Initialize Native Waveshare LCD Driver
  LCD_Init();
  LCD_Clear(C_BLACK);

  // Start with clean slate
  reset_intelligence();

  set_led(0, 100, 100);
  Serial.println("Fruit Fly Connectome Initialized with Evasion & Reset Dynamics!");
}

void loop() {
  bool btn = digitalRead(BOOT_BTN_PIN);
  unsigned long now = millis();

  if (btn_prev == HIGH && btn == LOW) {
    btn_time = now;
    btn_held_down = false;
  } else if (btn == LOW && !btn_held_down) {
    // Hold BOOT button for half a second -> Spawn a Predator Attack directly on the fly!
    if (now - btn_time >= 350) {
      btn_held_down = true;
      if (!predator_active) {
        predator_active = true;
        predator_x = fly_x; // Target directly at fly's lane!
        predator_y = 45.0f;
        shadow_radius = 4.0f;
        predator_timer = 0;
        Serial.println("[GFN] Manual Predator Attack Spawned via BOOT Hold!");
      }
    }
  } else if (btn_prev == LOW && btn == HIGH) {
    if (!btn_held_down) {
      // Quick Tap -> Shock / Negative Reinforcement
      dopamine_PPL1 = 1.0f;
      wall_bumps++;
      FB_goal_angle += 0.5f;
      Serial.println("[DAN] Shock Administered via BOOT Tap!");
    }
  }
  btn_prev = btn;

  step_simulation();

  render_top_half();
  render_bottom_half();

  delay(25);
}
