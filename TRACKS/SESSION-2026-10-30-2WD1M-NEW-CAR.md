# SESSION — 2026-10-30 — 2WD1M New Car (Genum_2WD1M_CAR_NEW)

## Recovery point
- Tag: `recovery-2wd1m-new-2026-10-30` -> `bdebb40` (local, do not delete).
- Base initial commit: `ce0c9b7` (origin/main at clone time).
- HEAD: `bdebb40` "Fix compile: move setDriveTrim out of header".

## Commits on top of initial (unpushed — push pending, see below)
1. `9fcd73b` Fix CI: correct sketch filename in page-js gate; disable deltas report on fresh repo
2. `2584df9` Fix compile: correct include case/name in TwoWD1M.cpp (2WD1M.h -> TwoWD1M.h)
3. `bdebb40` Fix compile: move setDriveTrim out of header (g_2wd1mHandler not in scope)

## What this project is
Clone of `Genum_4WD4M_CAR` (v2 fleet reference) re-purposed as a 2WD1M car:
- Mode registry flipped: `2WD1M` = LIVE (signed SPD + SERVO), `4WD4M` = CS (legacy alias kept).
- Boot default `Mode::M2WD1M`; `BT`/`BLUETOOTH` -> M2WD1M alias retained.
- 2WD1M drive via `g_2wd1mHandler` (trim, steer limit, 1200 ms failsafe holding steering).
- Upgraded OLED from 4WD4M adopted: link context, transport toggles, tick() rotation, page-buffer driver.
- Config identity: FW_NAME/BT_SPP_NAME `2WD1M Car`, WIFI_AP_NAME `2WD1MCar_WiFi`, WIFI_AP_IP `192.168.245.1`.

## Current state
- Working tree clean. Both CI gates green:
  - `node tools/check-webpage-js.mjs` -> PASS (exit 0)
  - `node tools/check-firmware-contract.mjs` -> 0 gate failures
- Arduino compile not run locally (arduino-cli not installed); CI compile job is the gate.
- Push to origin is PENDING: network/auth unavailable in this session. origin/main still at `ce0c9b7`. Upstream tracking now set.
- CI run "Cancelled after 4m" on the last push attempt — likely superseded/cancelled run or failed push; re-run via GitHub Actions.

## Pending work (see guide/SESSION note for full list)
- Docs/comments still say "4WD4M" in storyboard.md, Readme.md, Config.h banners, ModeManager.h, CATALOG.md, CHANGELOG.md, tools scripts.
- App + website do not yet recognize `2WD1M Car` / `2WD1MCar_WiFi` (device registry, dial.ts).

## Restart recipe
- If a change breaks the build: `git stash && git checkout recovery-2wd1m-new-2026-10-30`, verify gates, then re-apply.
- Push later: `git push -u origin main` (network required). Re-run CI: GitHub Actions -> Re-run all jobs.
- Re-run gates after edits: page-js + firmware-contract + Arduino compile (huge_app).
- Never rename `2WD1M Car` / `2WD1MCar_WiFi` without a pairing-impact review (device-registry rule #1).
