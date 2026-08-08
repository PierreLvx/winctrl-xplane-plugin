Font Management System for WINCTRL FMC (MCDU or PFP 3N / PFP 4 / PFP 7)

The plugin reads this folder and loads fonts into the FMC in X-Plane.
Please also see https://rswilem.github.io/winctrl-font-editor/ for creating custom fonts.

## Installation

### Folder Location
Fonts live in the following directory:
`X-Plane root directory/Resources/plugins/winctrl/fonts/`
You have found this README.txt file there.

### Creating Custom Fonts
1. Visit https://rswilem.github.io/winctrl-font-editor/
2. Design or upload your custom font
3. Download the generated .xpwwf file
4. Place the .xpwwf file in the fonts folder specified above

### Selecting Fonts in X-Plane
1. Open X-Plane
2. Navigate to: Plugins > WINCTRL > FMC > Display font
3. Choose from the available fonts, or select "Managed by plugin" or "No custom font"
4. The selection will automatically persist across all aircraft

### Font Options
- **Managed by plugin (default)**: The plugin picks the included font that suits the
  currently selected aircraft
- **No custom font**: Reverts to the default font included with the WINCTRL FMC hardware.
  Please note, this is only on start-up. You will need to disconnect and reconnect the FMC
  to see changes if switching from a custom font.
- **Any font in this folder**: Both the included fonts below and your own .xpwwf files

## Included fonts

These ship with the plugin and are what "Managed by plugin" selects between.
Keep them in place: the plugin has no fonts compiled in, so a missing file
means that aircraft falls back to the WINCTRL Default font.

  winctrl.xpwwf       WINCTRL Default
  airbus.xpwwf        Airbus
  boeing-737.xpwwf    Boeing 737
  boeing-747.xpwwf    Boeing 747
  xcrafts.xpwwf       X-Crafts
  vga.xpwwf           VGA
  md11-cdu.xpwwf      MD-11
  ltn.xpwwf           LTN
  q4xp.xpwwf          Q4XP

## Behavior
- Font preferences are saved globally and apply to all aircraft
- Font changes take effect immediately, except when switching to "No custom font"
  which requires a device reconnect
- Any .xpwwf file you add here appears in the menu under its file name
