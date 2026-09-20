# GUI Resources

The Ugly Reverb GUI uses a hybrid VSTGUI approach.

Source assets:
- `125A_Logo_Master_FINAL.svg` — authoritative 125A branding master
- `fender_amp1.png` — 101-frame, 125x125-per-frame knob master used only as build input

Generated at configure time:
- transparent wear/patina overlay
- transparent glass overlay
- 1x/2x multi-resolution knob strips for the logical GUI knob sizes

The generated resources are packaged into the VST3. The 125x125 master strip remains a source/build asset and is not required at runtime.
