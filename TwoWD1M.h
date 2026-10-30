#pragma once

#include <Arduino.h>
#include "MotorControl.h"

// Forward declare ModeManager
class ModeManager;

/**
 * 2WD1M remote drive mode (R-20, v1.0.8).
 *
 * This chassis has ONE drive motor (L298N — differential is impossible) plus a
 * REAL steering servo on D27 (SteeringServo). Drive model:
 *
 *   SPD<±255>   throttle SETTING — the magnitude clamps into MIN_SPEED..MAX_
 *               speed and IS the PWM speed. No proportional mapping from
 *               joystick deflection: hold the stick, the car holds one speed.
 *   SERVO<0..180>  absolute steering angle (center 90) — commands the real
 *               servo, clamped to ±steerLimit, then the trim offset applies.
 *   S           explicit STOP (throttle 0, servo centers). The app's D-pad
 *               center and stop pads send this.
 *
 * The handler owns no hardware: applyDrive() routes the angle through
 * ModeManager's SteeringServo and the throttle through the MotorControl.
 * Status text is routed through ModeManager::setStatusText() so the OLED
 * never flickers between the drive status and the manager's own status text.
 */
class TwoWD1M {
public:
  TwoWD1M() = default;
  ~TwoWD1M() = default;

  bool handleCommand(const char* cmdBuf, MotorControl* motor, uint16_t &speed, ModeManager* modeMgr);

  // Clear joystick state (called when entering the mode).
  void reset();

  // Current servo angle 0..180 (center 90) for the OLED steer display.
  int getServo() const { return servo_; }

  // R-19 (FIN-45): steering trim (−45..+45, effective center = 90 − trim).
  void setTrim(int16_t t);
  int16_t getTrim() const { return trim_; }

  // R-20: steering servo travel limit — max |servo − 90| the drive model may
  // command (10..90, car-persisted; the STEER token sets it).
  void setSteerLimit(int deg);
  int getSteerLimit() const { return steerLimit_; }

  // R-19: trip metrics since last reset — distance proxy (|speed|·s) and the
  // largest |steer deviation from 90| seen while moving.
  uint32_t getTripAccum() const { return tripAccum_; }
  uint32_t getTripSeconds() const { return tripSeconds_; }
  int getMaxSteer() const { return maxSteer_; }
  void resetTrip();

  // Re-apply current speed/steer/trim to the servo + motors (public so
  // ModeManager can push a fresh trim/limit without a synthetic command).
  void applyDrive(MotorControl* motor, ModeManager* modeMgr) const;

private:

  int16_t speed_ = 0;   // signed -255..255 (0 = stop): the throttle SETTING
  int servo_ = 90;      // 0..180, center 90
  int16_t trim_ = 0;    // R-19: −45..+45 steering offset
  int steerLimit_ = 90; // R-20: max |servo − 90| (STEER token / car-persisted)
  uint32_t tripAccum_ = 0;   // Σ|speed| per moving second
  uint32_t tripSeconds_ = 0; // moving-seconds counter
  int maxSteer_ = 0;         // max |servo_ − 90| seen while moving
  uint32_t lastTripMs_ = 0;  // last accumulator tick
};
