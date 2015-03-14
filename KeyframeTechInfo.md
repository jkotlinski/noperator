# Introduction #

Keyframes will be marked in the byte stream with $94 (INST) followed by a 16-bit speed value. The speed value is fixed-point 4.12 and gives how many chars should be printed per frame.

# Editor view #

Following fields should be displayed when editing a keyframe:

  * Keys: Number of keys between current and next keyframe (or end)
  * Speed: chars/frame
  * Beat: Approximation of how many beats will pass between current and next keyframe

# Calculations #

<pre>
extremes:<br>
<br>
full screen = 40 * 25 = 1000 keys.<br>
draw in one bar: 4 beats = 4 * 24 = 96 ticks.<br>
for each tick, should print 10.41 keys.<br>
<br>
10 keys in 16 beats: 10 keys, 384 ticks.<br>
for each tick, should print 0.026 keys.<br>
<br>
good format for speed: 4.12 ? meaning, 16-bit speed is equal to (keys << 12) / ticks<br>
<br>
test<br>
<br>
1000 keys, 4 beats gives (1000 << 12) / (4 * 24) = 42666. 42666 / (1 << 12) gives 10.41650390625<br>
10 keys, 16 beats gives (10 << 12) / (16 * 24) = 106. 106 / (1 << 12) gives 0.02587890625</pre>

### Beat calc ###

<pre>
How to calculate beats from speed + keys?<br>
<br>
speed = (keys << 12) / ticks<br>
ticks = (keys << 12) / speed<br>
beats = (keys << 12) / (speed * 24)<br>
</pre>