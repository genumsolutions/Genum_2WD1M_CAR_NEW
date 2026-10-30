// SteeringServo.cpp — R-20: real steering servo on D27 for the 2WD1M car.
#include "SteeringServo.h"
#include "Config.h"
#include "driver/ledc.h"

// The L298NControl motor driver owns LEDC timer 0 / channels 0-1 (LEDC_HIGH_
// SPEED_MODE, 4 kHz / 10-bit for the drive PWM). The servo gets its own timer
// + channel so the two never fight: 50 Hz / 16-bit, timer 1, channel 2.
static const ledc_mode_t SERVO_LEDC_MODE = LEDC_HIGH_SPEED_MODE;
static const ledc_timer_t SERVO_LEDC_TIMER = LEDC_TIMER_1;
static const ledc_channel_t SERVO_LEDC_CHANNEL = LEDC_CHANNEL_2;
static const uint32_t SERVO_PWM_FREQ_HZ = 50;   // standard servo refresh
static const uint8_t SERVO_PWM_RESOLUTION = 16; // high resolution → smooth pulse math

// Pulse span (µs) — the standard 500..2500 window; a conversion servo with a
// different span can be tuned via Config.h without touching the logic here.
#ifndef SERVO_PULSE_MIN_US
#define SERVO_PULSE_MIN_US 500
#endif
#ifndef SERVO_PULSE_MAX_US
#define SERVO_PULSE_MAX_US 2500
#endif

bool SteeringServo::begin() {
  if (attached_) {
    center();          // re-assert center on a second call
    return true;
  }

  pinMode(SERVO_PIN, OUTPUT);
  digitalWrite(SERVO_PIN, LOW);

  ledc_timer_config_t timer = {};
  timer.speed_mode = SERVO_LEDC_MODE;
  timer.timer_num = SERVO_LEDC_TIMER;
  timer.duty_resolution = static_cast<ledc_timer_bit_t>(SERVO_PWM_RESOLUTION);
  timer.freq_hz = SERVO_PWM_FREQ_HZ;
  timer.clk_cfg = LEDC_AUTO_CLK;
  if (ledc_timer_config(&timer) != ESP_OK) {
    Serial.println("[SERVO] LEDC timer config failed");
    return false;
  }

  ledc_channel_config_t ch = {};
  ch.speed_mode = SERVO_LEDC_MODE;
  ch.channel = SERVO_LEDC_CHANNEL;
  ch.timer_sel = SERVO_LEDC_TIMER;
  ch.intr_type = LEDC_INTR_DISABLE;
  ch.gpio_num = SERVO_PIN;
  ch.duty = 0;
  ch.hpoint = 0;
  if (ledc_channel_config(&ch) != ESP_OK) {
    Serial.println("[SERVO] LEDC channel config failed");
    return false;
  }

  attached_ = true;
  center();  // power up centered (90) — car must never boot mid-turn
  Serial.println("[SERVO] Steering servo ready on D27 (LEDC 50Hz, 500-2500us)");
  return true;
}

void SteeringServo::writeAngle(int angle) {
  // R-20b (owner: "servo is running on its own like in a pulse"): the servo
  // used to be rewritten on EVERY drive token with a re-derived duty, so a
  // 1-bit duty ripple or a ±1° command wobble made the horn hunt/pulse around
  // its position even with the sticks centred. Two defences:
  //   1. CENTER SNAP — anything within SERVO_CENTER_SNAP_DEG of 90 IS 90, so
  //      ADC jitter near centre can never command a small angle.
  //   2. REDUNDANT-WRITE GUARD — a write that lands on the already-commanded
  //      angle is a no-op (no LEDC churn, no pulse-train restart).
  if (angle < SERVO_ANGLE_MIN) angle = SERVO_ANGLE_MIN;
  if (angle > SERVO_ANGLE_MAX) angle = SERVO_ANGLE_MAX;
  if (abs(angle - 90) <= SERVO_CENTER_SNAP_DEG) angle = 90;
  if (attached_ && angle == angle_) return;  // already there — do not touch the LEDC
  angle_ = angle;
  if (!attached_) return;
  pulse(angle);
}

// FIN-49 (owner gate 2026-09-20: "trim is not properly increasing and
// decreasing"): the drive path must NOT centre-snap. Trim is applied as a
// shift of the centre (90 − trim, R-19 semantics), so a trim of ±1..2 commands
// 89/91/88/92 — exactly the band the snap collapsed to 90. STEER steps are ±5
// and survived, which is why ONLY trim appeared dead on both controllers.
// The anti-hunt defence on this path is the redundant-write guard alone (a
// repeated angle is a no-op — no pulse-train restart), which was the actual
// root cause of the R-20b hunting. Non-invasive: writeAngle()/center()
// behaviour is unchanged; only the live drive/trim command path switches to
// driveAngle().
void SteeringServo::driveAngle(int angle) {
  if (angle < SERVO_ANGLE_MIN) angle = SERVO_ANGLE_MIN;
  if (angle > SERVO_ANGLE_MAX) angle = SERVO_ANGLE_MAX;
  if (attached_ && angle == angle_) return;  // redundant-write guard kept
  angle_ = angle;
  if (!attached_) return;
  pulse(angle);
}

void SteeringServo::pulse(int angle) {
  const uint32_t periodUs = 1000000u / SERVO_PWM_FREQ_HZ;             // 20000 µs
  const uint32_t spanUs = SERVO_PULSE_MAX_US - SERVO_PULSE_MIN_US;    // 2000 µs
  // 0° → MIN pulse, 180° → MAX pulse (center 90 → 1500 µs).
  const uint32_t pulseUs = SERVO_PULSE_MIN_US +
      (uint32_t)((spanUs * (uint32_t)angle) / 180u);

  const uint32_t maxDuty = (1u << SERVO_PWM_RESOLUTION) - 1u;
  uint32_t duty = (uint32_t)(((uint64_t)pulseUs * maxDuty) / periodUs);
  if (duty > maxDuty) duty = maxDuty;

  ledc_set_duty(SERVO_LEDC_MODE, SERVO_LEDC_CHANNEL, duty);
  ledc_update_duty(SERVO_LEDC_MODE, SERVO_LEDC_CHANNEL);
}
