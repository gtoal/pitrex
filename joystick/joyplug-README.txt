SDL2 Hotplug Example
--------------------

This is simple, example code which shows how to handle SDL2 Joystick
hotplugging.

Joysticks have a Device Index, Instance ID, and Player Index, which are all
managed separately. The key to making working SDL2 hotplugging code is
understanding how to contend with all three.
