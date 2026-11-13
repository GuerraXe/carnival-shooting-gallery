#!/usr/bin/perl
# Writes assets/licenses/<ID>.md for every sound taken from Freesound (D-030).
# Data below was copied from each sound's own Freesound page on 2026-11-11
# (title, uploader, licence label, description); SHA-256 values are of the
# files in asset-src/originals/audio/ exactly as downloaded.
# Run from the project folder:  perl asset-src/work/audio/evidence.pl
use strict;
use warnings;
use utf8;

my $date = '2026-11-11';
my @rows = (
 # [ID, fsid, uploader, title, original file, sha256, runtime path, "description (verbatim)", modifications, notes]
 ['SFX_SHOT', 750571, 'FOSSarts', 'Daisy Powerline 880 fired 6',
  'fs750571_FOSSarts_daisy_powerline_880_fired_6.hq.mp3', '260240c0e5760332c0ff6d93e0a692dba20b8cb26949d6fd884d621867386602',
  'audio/effects/shot.wav', 'Raw and unedited Equipment: Zoom H1 essential',
  'segment 0.884-1.400 s (one discharge, before handling noise at 1.44 s); leading silence trimmed to 2 ms (R-9); 1 ms fade-in, 60 ms fade-out; downmixed to mono; peak-normalised to -1 dBFS; 16-bit WAV 48 kHz',
  'A pump-up pneumatic BB/pellet air rifle (brief A-13: air rifle, not a firearm). The maker name appears only in the title, never in the sound.'],
 ['SFX_DUCK_HIT', 217459, 'JarredGibb', 'Metal Ping 1.wav',
  'fs217459_JarredGibb_metal_ping_1.hq.mp3', '4ad8d3b057a813497f618dc015adb734e68624173c0557eb7f8f8d5a71f60797',
  'audio/effects/duck_hit.wav', 'A metal ping',
  'whole recording from 0.054 s (leading silence trimmed); 1 ms fade-in, 40 ms fade-out; mono; peak -1 dBFS; 16-bit WAV 48 kHz',
  'Page tags include "foley", "struck", "design". The description does not name the struck object or the equipment: owner to confirm by ear that it reads as a pellet on sheet metal.'],
 ['SFX_DUCK_FLIP', 50736, 'RutgerMuller', 'Mailbox Metal Flap.wav',
  'fs50736_RutgerMuller_mailbox_metal_flap.hq.mp3', '761395b794468938ad085316c0df36a82640501498922569363ec1c17c40bf87',
  'audio/effects/duck_flip.wav', 'Opening a metal mailbox flap in a door. Lifting it and dropping it. Recorded with Edirol R-1 & Sennheiser e185S.',
  'whole recording (0.200 s); 1 ms fade-in, 25 ms fade-out; mono; peak -1 dBFS; 16-bit WAV 44.1 kHz',
  'A hinged metal plate dropping shut: the same mechanism as a gallery target slamming flat.'],
 ['SFX_DUCK_RESET', 124710, 'alienistcog', 'broken-hinge-rattle.aif',
  'fs124710_alienistcog_broken_hinge_rattle.hq.mp3', '950b0b89c73411569780d34af1a450e020c3775267569498782a4cfe96f133e3',
  'audio/effects/duck_reset.wav', 'Dull squeaking rattle made by fiddling with a pile of old broken hinges.',
  'segment 2.780-3.750 s; 2 ms fade-in, 120 ms fade-out; mono; peak -1 dBFS; 16-bit WAV 44.1 kHz',
  'Segment chosen by transient analysis (two hinge clatters, then silence), not by ear.'],
 ['SFX_MISS', 573835, 'TRP', 'Door knocks wood close 77mel 191026.wav',
  'fs573835_TRP_door_knocks_wood_close.hq.mp3', '194a061b61f24d61bc16f41a02f4b3230c66192f6723ac1fa68380a6de45b7ce',
  'audio/effects/miss_thud.wav', 'Various door knocks close up wood door. Neumann KMR82i.',
  'segment 0.075-0.360 s (the first, isolated knock); 1 ms fade-in, 60 ms fade-out; mono; peak -1 dBFS; 16-bit WAV 48 kHz',
  'A knuckle knock on a wooden door standing in for a pellet into the wooden backboard; played at half level with pitch variation.'],
 ['SFX_CHAIN_LOOP', 452238, 'kyles', 'gears Mitchel animation table mechanism turn metal rattle5.flac',
  'fs452238_kyles_gears_mechanism_metal_rattle5.hq.mp3', '0ed88c225fba9191dbca136528933624979666f61d4ce1a24023b407664bccff',
  'audio/effects/chain_loop.wav', 'gears Mitchel animation table mechanism turn metal rattle5.flac',
  'segment 20.0-28.0 s made seamless with a 0.5 s equal-power crossfade into 28.0-28.5 s; mono; peak -3 dBFS; 16-bit WAV 48 kHz',
  'Recording equipment is stated on the uploader\'s other pages of the same series (Sony PCM-D50 / Zoom H2), not on this one.'],
 ['SFX_BELL_START', 571096, 'TRP', '180820 Boxing bell, various rings dings pulses close distant, Soulpepper TORONTO.flac',
  'fs571096_TRP_boxing_bell.hq.mp3', 'e2f56ab6d771c00ccacb965d2ca29466d13025b3f2f5b4ce3b1d14234c42358b',
  'audio/effects/bell_start.wav', 'Various rings and dings from boxing bell.',
  'segment 1.684-4.600 s (one strike and its ring); 1 ms fade-in, 400 ms fade-out; mono; peak -1 dBFS; 16-bit WAV 48 kHz',
  'Same original as SFX_END (different strike).'],
 ['SFX_TICK', 42139, 'fauxpress', 'Old Scots Clock - Ticking.wav',
  'fs42139_fauxpress_old_scots_clock_ticking.hq.mp3', '889622967f81b08c91c53b5b336d8cd5c6f7609693d0889d8156fa12f7033919',
  'audio/effects/tick.wav', '18th Century Scottish clock ticking. This is such a sweet, unusual clock. Recorded with a Schoeps stereo XY pair to Fostex PD-6.',
  'segment 5.490-5.800 s (one tick); 1 ms fade-in, 80 ms fade-out; mono; peak -1 dBFS; 16-bit WAV 48 kHz', ''],
 ['SFX_END', 571096, 'TRP', '180820 Boxing bell, various rings dings pulses close distant, Soulpepper TORONTO.flac',
  'fs571096_TRP_boxing_bell.hq.mp3', 'e2f56ab6d771c00ccacb965d2ca29466d13025b3f2f5b4ce3b1d14234c42358b',
  'audio/effects/end_bell.wav', 'Various rings and dings from boxing bell.',
  'segment 229.300-234.400 s (the longest single strike); 1 ms fade-in, 600 ms fade-out; mono; peak -1 dBFS; 16-bit WAV 48 kHz. The game plays it twice, 0.4 s apart.',
  'Same original as SFX_BELL_START (different strike).'],
 ['SFX_UI_CLICK', 565027, 'Anzbot', 'Wooden clicks.wav',
  'fs565027_Anzbot_wooden_clicks.hq.mp3', '00820a3855330c0e81290f23593e59be4235e2ef9eef895c4091bfdcd659d16d',
  'audio/effects/ui_click.wav', 'Not even sure how I made these noises...! Lovely wooden clicks though, could be used for variety of things',
  'segment 2.715-2.900 s (one click); 1 ms fade-in, 40 ms fade-out; mono; peak -1 dBFS; 16-bit WAV 44.1 kHz',
  'The uploader states they made the sounds; tags: click, instrument, percussion, wood.'],
 ['SFX_SCORE_TICK', 637735, 'kyles', 'old adding machine mechanism like toy cash register drawer.flac',
  'fs637735_kyles_old_adding_machine_mechanism.hq.mp3', '6bc4b03538bdad57031a216c4b1791ad121104f098b8940a0a5b1fed74edb15d',
  'audio/effects/score_tick.wav', 'old adding machine mechanism like toy cash register drawer recorded with Sony PCM-D50, Tascam DAP1 DAT with AT835 stereo mic, or Zoom H2',
  'segment 30.420-30.720 s (one clack); 1 ms fade-in, 60 ms fade-out; mono; peak -1 dBFS; 16-bit WAV 48 kHz', ''],
 ['SFX_RESULTS', 453763, 'kyles', 'bell toy cash register ding.flac',
  'fs453763_kyles_bell_toy_cash_register_ding.hq.mp3', 'e22c0138cebad95f04ad5dcd92592e14c7c4ce92cc05d258f98c21608294337c',
  'audio/effects/results_ding.wav', 'bell toy cash register ding.flac',
  'segment 0.863-2.100 s (the first ding); 1 ms fade-in, 300 ms fade-out; mono; peak -1 dBFS; 16-bit WAV 48 kHz', ''],
 ['AMB_MIDWAY', 635159, 'HECKFRICKER', 'Fairground Ambience',
  'fs635159_HECKFRICKER_fairground_ambience.hq.mp3', '8ed87898d5baf866c092824d7d028559ce762ba2c3bc6ed8912462cec7de745a',
  'audio/ambience/midway_loop.ogg', 'The whirring of machines, crowd walla, carnie announcements, and screaming at a fairground that was passing through my town. Recorded on TASCAM DR-05X, limited to remove peaks and normalized in audacity.',
  'segment 125.0-215.0 s made seamless with a 3.0 s equal-power crossfade into 215.0-218.0 s; stereo; peak -3 dBFS; Ogg Vorbis 44.1 kHz',
  'OWNER LISTENING CHECK REQUIRED: the description mentions carnie announcements. The segment was chosen as the calmest stretch by level analysis; whether any speech in it is intelligible, or any music recognisable (spec: neither), has not been heard.'],
);

for my $r (@rows) {
    my ($id, $fs, $user, $title, $file, $sha, $rt, $desc, $mods, $notes) = @$r;
    my $preview_dir = int($fs / 1000);
    open my $out, '>:encoding(UTF-8)', "assets/licenses/$id.md" or die "$id: $!";
    print $out <<"EOT";
# $id — licence evidence

| Field | Value |
|---|---|
| Asset ID | $id |
| Runtime path | assets/$rt |
| Source URL: | https://freesound.org/people/$user/sounds/$fs/ |
| Title on the page | $title |
| Creator (as credited) | $user (Freesound uploader; the description states or implies own recording, see below) |
| License: | Creative Commons 0 (CC0 1.0 Universal Public Domain Dedication) — https://creativecommons.org/publicdomain/zero/1.0/ ; full text: [CC0-1.0.txt](CC0-1.0.txt) |
| Verbatim licence statement | "License: Creative Commons 0" (licence label on the sound page, retrieved $date) |
| Attribution required | No (CC0). Credited anyway in the manifest's credits section |
| Retrieved | $date |
| File obtained | Freesound's public high-quality MP3 preview (https://cdn.freesound.org/previews/$preview_dir/...-hq.mp3). The uploader's original lossless file needs a Freesound login and was **not** downloaded; replace the original with it for the final release if possible |
| Original path | asset-src/originals/audio/$file |
| Original SHA-256: | $sha |
| Archived snapshot | not made (no web.archive.org request was sent) |
| Modifications | $mods. Command: asset-src/work/audio/prep.sh |
| Synthesis / AI | None. A real recording; edits are cut, fade, mono downmix, normalise and loop crossfade only |
| Subject clearance | No speech, music, brand audio or identifiable person expected in this cut (not yet confirmed by ear) |
| Review (Guide §6) | Pending: owner listening review. Status IN_PROGRESS until then |

## Description on the source page (verbatim)

> $desc

EOT
    print $out "## Notes\n\n$notes\n" if $notes ne '';
    close $out;
    print "wrote assets/licenses/$id.md\n";
}
