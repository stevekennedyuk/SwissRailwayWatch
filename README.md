# Swiss Railway Watch Face for Pebble

A clean, Mondaine-inspired analog watchface featuring:
- **White** dial background
- **Black** hour and minute hands (wide, bold — Mondaine proportions)
- **Red** second hand with a distinctive red circle counterweight
- Black tick marks (thick for hours, thin for minutes)
- Black outer ring
- **Day/date window** at 3 o'clock — a small bordered box showing the day abbreviation (e.g. MON) above and the date number below, separated by a divider line

## Files

```
SwissRailwayWatch/
├── appinfo.json                 # App manifest
├── wscript                      # Build script
└── src/
    └── swissrailway.c           # Main watchface source
    └── swissrailway.h           # Main watchface headers
└── Resources/
    └── nettek_logo.png          # Logo image
    └── WatchFace.png            # Watchface image
    └── WatchFace144x168.png     # Watchface image 144x168 pixels
    └── WatchFace180x180.png     # Watchface image 180x180 pixels
    └── WatchFace200x228.png     # Watchface image 200x228 pixels
    └── WatchFace260x260.png     # Watchface image 260x260 pixels

```

## Building

### Option 1 — CloudPebble (Easiest)
1. Go to https://cloudpebble.repebble.com
2. Create a new project → **Pebble C SDK**
3. Delete the default source file and paste in `src/mondaine.c`
4. Update project settings to match `appinfo.json`
5. Click **Run** to build and install

### Option 2 — Local SDK
1. Install the Pebble SDK: https://developer.repebble.com/sdk/
2. From the project root:
   ```
   pebble build
   pebble install --emulator basalt   # or aplite/chalk
   ```

## Design Notes

The Mondaine SBB (Swiss Federal Railways) clock is iconic for its:
- Minimalist white dial
- Bold, rectangular black hands
- Sweeping red second hand with a round counterweight disc
- Simple baton hour markers

This watchface faithfully replicates those proportions within the Pebble display.
