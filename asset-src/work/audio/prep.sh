#!/bin/sh
# Re-creates every runtime sound from the untouched originals (D-030).
# Each line is the complete list of modifications for that asset; the same
# text is recorded in assets/ASSET_MANIFEST.md.
#
# Run from the project folder after building the tool:
#   cmake --build --preset release --target audio_prep
#   sh asset-src/work/audio/prep.sh
set -e
T=build/windows-msvc/Release/audio_prep.exe
O=asset-src/originals/audio
E=assets/audio/effects
A=assets/audio/ambience
W=asset-src/work/audio/audition

mkdir -p "$E" "$A" "$W"

# SFX_SHOT — one discharge of a Daisy Powerline 880 air rifle; ends before the
# handling noise at 1.44 s. Leading silence trimmed (R-9).
$T cut $O/fs750571_FOSSarts_daisy_powerline_880_fired_6.hq.mp3 $E/shot.wav \
   --start 0.86 --end 1.40 --trim-lead -36 --fade-in 1 --fade-out 60 --peak -1 --mono

# SFX_DUCK_HIT — pellet on sheet metal: a struck metal ping, whole recording.
$T cut $O/fs217459_JarredGibb_metal_ping_1.hq.mp3 $E/duck_hit.wav \
   --start 0.0 --end 0.418 --trim-lead -50 --fade-in 1 --fade-out 40 --peak -1 --mono

# SFX_DUCK_FLIP — a hinged metal flap dropping shut: the target slamming flat.
$T cut $O/fs50736_RutgerMuller_mailbox_metal_flap.hq.mp3 $E/duck_flip.wav \
   --start 0.0 --end 0.200 --trim-lead -40 --fade-in 1 --fade-out 25 --peak -1 --mono

# SFX_DUCK_RESET — dull rattle of old hinges: the target swinging back up.
$T cut $O/fs124710_alienistcog_broken_hinge_rattle.hq.mp3 $E/duck_reset.wav \
   --start 2.78 --end 3.75 --trim-lead -40 --fade-in 2 --fade-out 120 --peak -1 --mono

# SFX_MISS — a single close knock on wood (first knock, isolated).
$T cut $O/fs573835_TRP_door_knocks_wood_close.hq.mp3 $E/miss_thud.wav \
   --start 0.05 --end 0.36 --trim-lead -40 --fade-in 1 --fade-out 60 --peak -1 --mono

# SFX_CHAIN_LOOP — gear mechanism rattle, 8 s seamless loop.
$T loop $O/fs452238_kyles_gears_mechanism_metal_rattle5.hq.mp3 $E/chain_loop.wav \
   --start 20.0 --length 8.0 --xfade 0.5 --peak -3 --mono

# SFX_BELL_START — one strike of a boxing bell with its natural ring.
$T cut $O/fs571096_TRP_boxing_bell.hq.mp3 $E/bell_start.wav \
   --start 1.66 --end 4.60 --trim-lead -40 --fade-in 1 --fade-out 400 --peak -1 --mono

# SFX_TICK — one tick of an 18th-century clock.
$T cut $O/fs42139_fauxpress_old_scots_clock_ticking.hq.mp3 $E/tick.wav \
   --start 5.49 --end 5.80 --trim-lead -45 --fade-in 1 --fade-out 80 --peak -1 --mono

# SFX_END — the boxing bell's longest strike (the game rings it twice).
$T cut $O/fs571096_TRP_boxing_bell.hq.mp3 $E/end_bell.wav \
   --start 229.30 --end 234.40 --trim-lead -40 --fade-in 1 --fade-out 600 --peak -1 --mono

# SFX_UI_CLICK — one wooden click.
$T cut $O/fs565027_Anzbot_wooden_clicks.hq.mp3 $E/ui_click.wav \
   --start 2.70 --end 2.90 --trim-lead -40 --fade-in 1 --fade-out 40 --peak -1 --mono

# SFX_SCORE_TICK — one clack of an old adding-machine mechanism (score board).
$T cut $O/fs637735_kyles_old_adding_machine_mechanism.hq.mp3 $E/score_tick.wav \
   --start 30.42 --end 30.72 --trim-lead -40 --fade-in 1 --fade-out 60 --peak -1 --mono

# SFX_RESULTS — a toy cash-register bell: the round's score is rung up.
$T cut $O/fs453763_kyles_bell_toy_cash_register_ding.hq.mp3 $E/results_ding.wav \
   --start 0.84 --end 2.10 --trim-lead -40 --fade-in 1 --fade-out 300 --peak -1 --mono

# AMB_MIDWAY — fairground crowd and machinery, 90 s seamless stereo loop from
# the calmest stretch (peaks no higher than -1.9 dBFS in 5 s windows).
$T loop $O/fs635159_HECKFRICKER_fairground_ambience.hq.mp3 $A/midway_loop.ogg \
   --start 125.0 --length 90.0 --xfade 3.0 --peak -3

# MUS_CALLIOPE — NOT exported to assets/. The recordist names only the third
# of five songs ("Over the Waves", public domain); the free preview stops at
# 396 s and the song boundaries cannot be confirmed without listening.
# Audition cuts for the owner (Asset Guide §3.6):
$T cut $O/fs870752_CHallSmith_large_band_organ_at_carousel.hq.mp3 $W/band_organ_song_a_222-265s.ogg \
   --start 222.5 --end 264.5 --fade-in 500 --fade-out 1500
$T cut $O/fs870752_CHallSmith_large_band_organ_at_carousel.hq.mp3 $W/band_organ_song_b_270-396s.ogg \
   --start 270.0 --end 395.9 --fade-in 500 --fade-out 1500
