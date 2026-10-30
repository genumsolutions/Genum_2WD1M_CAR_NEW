// 2WD1M.cpp
// Signed SPD + SERVO drive handler for the remote's 2WD1M mode (R-20, v1.0.8).
// One drive motor (L298N) + a REAL steering servo on D27: the servo angle is
// commanded directly (no differential skid-steer), the throttle is a fixed
// setting (no proportional ramp from stick deflection).

#include "2WD1M.h"
#include "ModeManager.h"
#include "Config.h"
#include <string.h>
#include <stdlib.h>

void TwoWD1M::reset() {
  speed_ = 0;
  servo_ = 90;
  lastTripMs_ = 0;
}

void TwoWD1M::setTrim(int16_t t) {
  if (t < -45) t = -45;
  if (t > 45) t = 45;
  trim_ = t;
}

void TwoWD1M::setSteerLimit(int deg) {
  if (deg < STEER_LIMIT_MIN) deg = STEER_LIMIT_MIN;
  if (deg > STEER_LIMIT_MAX) deg = STEER_LIMIT_MAX;
  steerLimit_ = deg;
}

void TwoWD1M::resetTrip() {
  tripAccum_ = 0;
  tripSeconds_ = 0;
  maxSteer_ = 0;
  lastTripMs_ = 0;
}

// R-19: 1 Hz trip tick — accumulate |speed| and track max steer deviation.
// Called from handleCommand (command stream implies an active link, matching
// how the failsafe clock is refreshed).
static void tickTrip(int16_t speed, int servo, uint32_t &tripAccum,
                     uint32_t &tripSeconds, int &maxSteer, uint32_t &lastMs) {
  if (speed == 0) { lastMs = 0; return; }
  uint32_t now = millis();
  if (lastMs == 0) { lastMs = now; return; }
  while (now - lastMs >= 1000) {
    lastMs += 1000;
    tripAccum += (uint32_t)abs(speed);
    ++tripSeconds;
  }
  int dev = abs(servo - 90);
  if (dev > maxSteer) maxSteer = dev;
}

bool TwoWD1M::handleCommand(const char* cmdBuf, MotorControl* motor, uint16_t &speed, ModeManager* modeMgr) {
  if (!cmdBuf || cmdBuf[0] == '\0') return false;

  // Fixed-throttle speed setting: SPD-255 .. SPD255 (0 = stop). The magnitude
  // IS the PWM speed — no proportional mapping from joystick deflection.
  if (strncasecmp(cmdBuf, "SPD", 3) == 0) {
    int val = atoi(cmdBuf + 3);
    if (val < -255) val = -255;
    if (val > 255) val = 255;
    speed_ = static_cast<int16_t>(val);
    if (speed_ != 0) {
      speed = static_cast<uint16_t>(abs(speed_));  // expose magnitude for OLED
    }
    tickTrip(speed_, servo_, tripAccum_, tripSeconds_, maxSteer_, lastTripMs_);
    applyDrive(motor, modeMgr);
    return true;
  }

  // Real steering servo: SERVO0 .. SERVO180 (center 90), clamped to ±steerLimit.
  if (strncasecmp(cmdBuf, "SERVO", 5) == 0) {
    int val = atoi(cmdBuf + 5);
    if (val < 0) val = 0;
    if (val > 180) val = 180;
    servo_ = val;
    tickTrip(speed_, servo_, tripAccum_, tripSeconds_, maxSteer_, lastTripMs_);
    applyDrive(motor, modeMgr);
    return true;
  }

  // Explicit STOP (R-20): zero the throttle and center the servo. The app's
  // D-pad center / stop pads send 'S'; previously this fell through every
  // branch and the car answered "Unknown mode: S" + a NACK.
  if (cmdBuf[1] == '\0' && (cmdBuf[0] == 'S' || cmdBuf[0] == 's')) {
    speed_ = 0;
    servo_ = 90;
    tickTrip(0, 90, tripAccum_, tripSeconds_, maxSteer_, lastTripMs_);
    applyDrive(motor, modeMgr);
    return true;
  }

  // R-19 (FIN-45): TRIM / TRIPRESET are handled by ModeManager::handleCommand
  // (persist + STATE echo live there); this handler only consumes drive tokens.

  return false;
}

void TwoWD1M::applyDrive(MotorControl* motor, ModeManager* modeMgr) const {
  if (!motor || !modeMgr) return;

  // Steering: clamp the commanded angle to ±steerLimit around 90, apply the
  // trim offset (trim + = car pulls LEFT, so the center moves toward smaller
  // servo values — R-19 semantics kept), then drive the REAL servo (D27).
  int deviation = servo_ - 90;
  if (deviation > steerLimit_) deviation = steerLimit_;
  if (deviation < -steerLimit_) deviation = -steerLimit_;
  modeMgr->commandSteering(90 + deviation - trim_);

  // Throttle: fixed setting. The magnitude is the PWM speed (already clamped
  // into MIN..MAX by the sender); zero (or a 'stop') means motor stop.
  if (speed_ == 0) {
    motor->drive(Action::STOP, 0);
    modeMgr->setStatusText("Stopped");
    return;
  }

  uint16_t mag = static_cast<uint16_t>(abs(speed_));
  if (mag < static_cast<uint16_t>(MIN_SPEED)) mag = MIN_SPEED;
  if (mag > static_cast<uint16_t>(MAX_SPEED)) mag = MAX_SPEED;

  if (speed_ > 0) {
    motor->drive(Action::FORWARD, mag);
    modeMgr->setStatusText("Forward");
  } else {
    motor->drive(Action::BACKWARD, mag);
    modeMgr->setStatusText("Backward");
  }
}
