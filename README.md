# Dreidel Controller
This is a Arduino sketch for controlling a specially designed PCB for displaying a dreidel on my roof.
When a button is pushed, the dreidel is "spun" by cycling through the four symbols and randomly landing
on one.

When not being played, the display cycles through each symbol in a fading action. This is done via
a triac using zero-cross over detection. Please note that you may have to fiddle with the timing
to account for any frequency drift from your electric supplier. Please see the dreidel PCB respository
for the KiCad files. You could also hack this to run on a set of LEDs instead.
