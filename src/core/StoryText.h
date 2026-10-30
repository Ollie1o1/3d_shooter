#pragma once
// Narrative voices: records describe work; wrecks remember an incident;
// siblings speak about the job in front of them. HUD instructions stay literal.
namespace storytext {
inline constexpr const char* cradleLogs[] = {
    "", // Units are indexed by their stencilled number.
    "THE LAMP STILL WORKS. I PUT IT WHERE THE NEXT ONE WILL LAND.",
    "THE LIFT COMES BACK IF YOU WAIT. MY LEFT LEG WON'T TAKE MY WEIGHT.",
    "02 KNOCKED ON THE VENT. I KEPT QUIET. I THOUGHT THEY HAD SENT HIM BACK FOR ME.",
    "I PULLED THE FEED CABLE. THE NEXT FRAME KEPT MOVING ON THE OTHER LINE.",
    "THEY HAVE MY WEIGHT, MY VOLTAGE, EVEN THE FAULT IN MY KNEE. NO NAME.",
    "I WANTED THE GUN. I DIDN'T THINK THE GLASS WAS WIRED TO THE DOOR.",
    "THE ONE IN THE END POD KEPT TURNING TO FOLLOW ME. ITS EYES WEREN'T FINISHED.",
    "THE HOOK HELD MY WEIGHT. THE RAIL RUNS RIGHT OVER THE SORTER'S BACK."
};
inline constexpr const char* spanLogs[] = {
    "I FOLLOWED THE TYRE MARKS. ALL THREE LORRIES STOPPED HERE.",
    "THE RELAY STILL HAD POWER. I LEFT THE SERVICE LIGHT ON FOR THE OTHERS.",
    "KEEP THE RIFLE. THE SIGHT IS ZEROED ON THE FAR PYLON.",
    "HE WAVED US THROUGH. THEN THE SIREN STARTED. I GOT UP HERE. 11 DIDN'T."
};
inline constexpr const char* sorterIntro = "REJECT 24. THE TRANSFER HATCH IS BELOW YOU.";
inline constexpr const char* sorterDamaged = "HOLD STILL. THE READING KEEPS CHANGING.";
inline constexpr const char* sorterDeath = "THE REGISTER SAYS YOU WERE EMPTY.";
inline constexpr const char* sorterDeath2 = "THERE IS NO COLUMN FOR THIS.";
inline constexpr const char* ballastIntro = "ROAD CREW. GET BEHIND THE MARKERS.";
inline constexpr const char* ballastDamaged = "BRAKES WON'T HOLD. MOVE.";
inline constexpr const char* ballastFinal = "DON'T STAND IN THE CUT.";
inline constexpr const char* ballastDeath = "I HAD IT CLEAR THIS MORNING.";
}
