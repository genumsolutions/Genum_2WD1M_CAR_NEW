// =====================================================================
// Genum_2WD1M_CAR_NEW — multimode 2WD1M car firmware (ESP32 + L298N + servo)
// File: ParamsStore.cpp — boot-state persistence (last mode + speed, T-13)
// Property of GENUM SOLUTIONS PVT. LTD.
// =====================================================================
#include "ParamsStore.h"
#include <Preferences.h>
#include "Config.h"

static const char *PREF_NS = "botcfg";
static const char *KEY_MODE = "mode";
static const char *KEY_SPD  = "spd";
static const char *KEY_TRIM = "trim";
static const char *KEY_STEER_LIMIT = "steerlimit";

bool loadBootState(uint8_t &mode, uint16_t &speed) {
  Preferences prefs;
  if (!prefs.begin(PREF_NS, true)) return false;
  mode = prefs.getUChar(KEY_MODE, 0xFF);
  speed = prefs.getUShort(KEY_SPD, 0xFFFF);
  prefs.end();
  if (mode == 0xFF || speed == 0xFFFF) return false;
  return true;
}

bool loadDriveTrim(int16_t &out) {
  Preferences prefs;
  if (!prefs.begin(PREF_NS, true)) return false;
  out = prefs.getShort(KEY_TRIM, 0x7FFF);
  prefs.end();
  if (out == 0x7FFF) return false;
  return true;
}
bool saveDriveTrim(int16_t v) {
  Preferences prefs;
  if (!prefs.begin(PREF_NS, false)) return false;
  prefs.putShort(KEY_TRIM, v);
  prefs.end();
  return true;
}
bool loadSteerLimit(int &out) {
  Preferences prefs;
  if (!prefs.begin(PREF_NS, true)) return false;
  int v = prefs.getShort(KEY_STEER_LIMIT, 0x7FFF);
  prefs.end();
  if (v == 0x7FFF) return false;
  out = v;
  if (out < STEER_LIMIT_MIN) out = STEER_LIMIT_MIN;
  if (out > STEER_LIMIT_MAX) out = STEER_LIMIT_MAX;
  return true;
}
bool saveSteerLimit(int v) {
  Preferences prefs;
  if (!prefs.begin(PREF_NS, false)) return false;
  prefs.putShort(KEY_STEER_LIMIT, (int16_t)v);
  prefs.end();
  return true;
}
bool saveBootState(uint8_t mode, uint16_t speed) {
  Preferences prefs;
  if (!prefs.begin(PREF_NS, false)) return false;
  prefs.putUChar(KEY_MODE, mode);
  prefs.putUShort(KEY_SPD, speed);
  prefs.end();
  return true;
}