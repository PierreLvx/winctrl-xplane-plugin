# Contributing

Thanks for your interest in the WINCTRL Plugin. This is a small project that talks to physical hardware, so the rules below are stricter than for a normal library. Please read them before opening a pull request.

## What we are looking for

Welcome, and usually merged quickly:

- Fixes for a bug you actually saw on your own hardware, in the sim.
- Support for an aircraft that is not covered yet, or a new profile for a device you own.
- Corrections to datarefs, button indices, or display output that are wrong on hardware.

## What we do not accept

- **Refactors on their own.** Renames, extracting helpers, reordering, formatting, comment rewrites, and similar changes are declined unless they are part of a fix. They change no behaviour, and reviewing them still costs time and adds noise to the history.
- **Performance changes without a measurement.** State what you measured, how, and what the numbers were.
- **Changes to a device protocol that you could not test.** See below.

If you are not sure whether something falls in this category, open an issue first and ask. That costs you nothing and saves us both a review.

## The duplication in the protocol code is intentional

Each product writes its own frame header, byte offsets, and report layout. Those blocks look alike, but each one is a transcript of one specific device, reverse engineered from that device. WINCTRL can change the firmware of one panel without touching the others, so those blocks are expected to drift apart. Please do not send patches that merge them into one shared builder.

Logic that is genuinely the same rule for every device, for example segment encoding or string formatting, does live in shared helpers under `src/include/utils/`.

## Claims need a reference

If your pull request says the code does something, or does something inefficiently, point at the file and line that shows it. Most incorrect pull requests we get describe a mechanism that the code does not actually have. A file and line reference is the fastest way for both of us to find that out.

Two things in particular are already handled, and claims to the contrary are almost always wrong:

- Display datarefs are cached and only report a change when the value really changed, see `src/include/utils/dataref.cpp`. There is no need for a second layer of change detection on top of it.
- Reports are padded to the device report length on write. Do not add your own padding logic.

## Testing on hardware

Anything that touches a device protocol, a display, an LED, or a button mapping must be tested on the physical device. In the pull request, say which device, which aircraft, and which X-Plane version you tested with.

If you do not own the device, that is fine, say so. We would rather have a clear issue describing the problem than an untested patch.

## Style

Match the surrounding code. Keep comments short. Initialise every scalar, pointer, and handle member in the class definition, since uninitialised members are a recurring source of Windows only crashes.

## AI assistance

You may use AI tools. You are still responsible for the result, which means you have read every line, you understand why it works, and you have run it. Pull requests that were clearly generated without reading the surrounding code will be closed without a detailed review.
