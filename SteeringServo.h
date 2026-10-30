#pragma once

#include <Arduino.h>

/**
 * SteeringServo — real steering-servo driver for the 2WD1M chassis (R-20).
 *
 * The steering servo hangs off SERVO_PIN (D27). Older firmware mapped the
 * SERVO token onto differential motor skid-steer, which never steered this
 * chassis (it has a dedicated steering servo). This module drives the servo
 * directly with LEDC (timer 1 / channel 2 — the L298N motor driver owns
 * timer 0 / channels 0-1) at the standard 50 Hz servo refresh.
 *
 * Angle semantics are the fleet protocol's: 0..180, center 90, larger angle
 * = LEFT. A lower-level writePulse is exposed in case a conversion servo
 * needs a custom pulse span via Config.h.
 */
class SteeringServo {
public:
  // Attach the LEDC timer/channel + pin and drive the servo to center (90).
  // Safe to call more than once (subsequent calls re-assert center).
  bool begin();

  // Command an absolute angle 0..180 (clamped). Center = 90.
  // Servo entry points (see SteeringServo.cpp):
  //  - writeAngle(): the CENTER path — keeps the R-20b centre snap (|a−90|≤2 ⇒ 90).
  //  - driveAngle(): the DRIVE path — no snap, redundant-write guard kept. FIN-49:
  //    the snap swallowed the trim-shifted command (trim ±1..2 → 90∓1..2), so trim
  //    never moved the horn; stick-steering (±5 steps) was unaffected. Drive
  //    commands must reach the servo verbatim; the hunting defence lives in the
  //    redundant-write guard (a repeated angle never restarts the pulse train).
  void writeAngle(int angle);
  void driveAngle(int angle);

  // Snap back to center (trim/limit changes, failsafe, mode entry).
  void center() { writeAngle(90); }

  // Last commanded angle (cached, not measured).
  int angle() const { return angle_; }

private:
  int angle_ = 90;   // commanded angle 0..180 (center 90)
  bool attached_ = false;

  // Shared LEDC duty update for writeAngle()/driveAngle() (no clamping/snap
  // logic here — callers clamp; the redundant-write guard lives in them).
  void pulse(int angle);
};
