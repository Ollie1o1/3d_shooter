#pragma once
// =============================================================================
// Weapons.h — the four guns: base stats, upgrade maths, ammo/reload state.
// No OpenGL, so the tests can check the numbers (a scoped Longshot one-shots
// everything except the boss, upgrades always make a gun better, …).
//
//   1 REVOLVER  8-round hitscan sidearm
//   2 SHOTGUN   2-shell pump, 10 pellets
//   3 KAR98     bolt-action rifle. RMB: iron sights (small zoom). Headshots
//               do 3x, a one-shot kill on anything but the armored heavies.
//   4 LONGSHOT  heavy .50 bolt sniper. RMB: full scope with a reticle. One
//               shot kills any regular enemy (not the armored Juggernaut:
//               parry that) and punches through three of
//               them. Fire just as the scope settles for a QUICKSCOPE bonus.
//
// Upgrades (bought with points from levelling up, see Progression.h):
//   DAMAGE    +20% per tier            FIRE RATE  -15% time between shots
//   MAGAZINE  more rounds, -12% reload MOD        one special perk per gun
// =============================================================================
#include <array>
#include <algorithm>

enum class WeaponId { REVOLVER, SHOTGUN, KAR, LONGSHOT, COUNT };
static constexpr int WEAPON_COUNT = (int)WeaponId::COUNT;

enum class UpgradeStat { DAMAGE, RATE, MAG, MOD, COUNT };
static constexpr int UPGRADE_STATS = (int)UpgradeStat::COUNT;
static constexpr int MAX_TIER = 3;   // DAMAGE / RATE / MAG tiers; MOD is on/off

struct WeaponDef {
    const char* name;
    const char* modName;
    const char* modDesc;
    float damage;        // per bullet / pellet
    int   pellets;
    float spreadHip;     // max offset of a shot from the aim line (tangent units)
    float spreadAds;     // same, fully aimed
    float cooldown;      // seconds between shots (the bolt cycle on the rifles)
    int   magSize;
    int   magPerTier;    // extra rounds per MAG tier
    float reloadTime;
    float range;
    float headMult;      // headshot multiplier (humanoids' heads)
    bool  canAim;        // RMB aims down sights / scope
    bool  scope;         // full scope overlay (vs. iron sights)
    float aimFov;        // FOV multiplier when fully aimed
    float aimTime;       // seconds from hip to fully aimed
    int   pierce;        // extra enemies a shot passes through
    float recoil;        // camera kick, degrees
    float aimMoveMult;   // movement speed while aimed
};

inline const WeaponDef& weaponDef(WeaponId w) {
    static const WeaponDef D[] = {
        {"REVOLVER", "PIERCING ROUNDS", "SHOTS PASS THROUGH 2 MORE ENEMIES",
         35.f, 1, 0.f, 0.f, 0.15f, 8, 2, 1.2f, 90.f, 1.5f, false, false, 1.f, 0.f, 0, 0.7f, 1.f},
        {"SHOTGUN", "DRAGON BREATH", "+5 PELLETS, TIGHTER SPREAD",
         9.f, 10, 0.18f, 0.18f, 0.55f, 2, 1, 1.4f, 60.f, 1.f, false, false, 1.f, 0.f, 0, 2.2f, 1.f},
        {"KAR98", "HEADHUNTER", "HEADSHOT KILLS REFUND THE ROUND AND SKIP THE BOLT",
         120.f, 1, 0.045f, 0.f, 0.85f, 5, 2, 2.0f, 200.f, 3.0f, true, false, 0.62f, 0.16f, 0, 2.6f, 0.9f},
        {"LONGSHOT", "EXPLOSIVE TIPS", "SHOTS BURST ON IMPACT FOR AREA DAMAGE",
         360.f, 1, 0.10f, 0.f, 1.25f, 4, 1, 2.8f, 300.f, 1.5f, true, true, 0.26f, 0.22f, 3, 4.0f, 0.8f},
    };
    return D[(int)w];
}

struct WeaponUpgrades {
    int  tier[3] = {0, 0, 0};   // DAMAGE, RATE, MAG
    bool mod = false;
};

// Effective stats after upgrades
inline float weaponDamage(WeaponId w, const WeaponUpgrades& u) {
    return weaponDef(w).damage * (1.f + 0.2f * u.tier[(int)UpgradeStat::DAMAGE]);
}
inline float weaponCooldown(WeaponId w, const WeaponUpgrades& u) {
    return weaponDef(w).cooldown * (1.f - 0.15f * u.tier[(int)UpgradeStat::RATE]);
}
inline int weaponMag(WeaponId w, const WeaponUpgrades& u) {
    return weaponDef(w).magSize + weaponDef(w).magPerTier * u.tier[(int)UpgradeStat::MAG];
}
inline float weaponReload(WeaponId w, const WeaponUpgrades& u) {
    return weaponDef(w).reloadTime * (1.f - 0.12f * u.tier[(int)UpgradeStat::MAG]);
}
inline int weaponPellets(WeaponId w, const WeaponUpgrades& u) {
    return weaponDef(w).pellets + ((w == WeaponId::SHOTGUN && u.mod) ? 5 : 0);
}
inline float weaponSpread(WeaponId w, const WeaponUpgrades& u, float aim) {
    const WeaponDef& d = weaponDef(w);
    float s = d.spreadHip + (d.spreadAds - d.spreadHip) * aim;
    if (w == WeaponId::SHOTGUN && u.mod) s *= 0.75f;
    return s;
}
inline int weaponPierce(WeaponId w, const WeaponUpgrades& u) {
    return weaponDef(w).pierce + ((w == WeaponId::REVOLVER && u.mod) ? 2 : 0);
}

// Ammo, cooldown and reload for one gun.
struct WeaponState {
    int   ammo = 0;
    float cooldown = 0.f;      // time until the next shot is allowed
    bool  reloading = false;
    float reloadTimer = 0.f;
    float reloadTotal = 1.f;

    bool ready() const { return cooldown <= 0.f && !reloading && ammo > 0; }

    void startReload(float t) { reloading = true; reloadTimer = reloadTotal = t; }

    // Returns true the tick a reload finishes
    bool tick(float dt, int mag) {
        cooldown = std::max(0.f, cooldown - dt);
        if (reloading) {
            reloadTimer -= dt;
            if (reloadTimer <= 0.f) { reloading = false; reloadTimer = 0.f; ammo = mag; return true; }
        }
        return false;
    }
    float reloadProgress() const { return reloading ? 1.f - reloadTimer / reloadTotal : 1.f; }
};
