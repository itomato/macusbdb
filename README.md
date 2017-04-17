# macusbdb
Jeroen/Spritesmods "macusbfb" LPC1343 firmware for Macintosh 9" CRT driver board
The code in these directories isn't the best I ever wrote, but at the moment
it works good enough for my own purposes. Use at your own risk!
Contents:

adb2usb/ : Code for the ADB to USB converter
adb2usb/adb2uart : Code for the uC that receives ADB
adb2usb/uart2usb : Code for the uC that sends the data over the USB bus
macusbfb : Firmware for the LPC1343 that a.o. controls the Macintosh CRT

A few notes about the shortcomings of the current code:
- macusbfb works just fine, but leaks memory when unloaded or the display
device is unplugged. As long as you plug in the device and load the module,
you should be fine though.
- The adb2usb device seems to drop adb replies every now and then, resulting
in e.g. a stuck key. I still need to figure out why. Until then, pressing 
the key a second time should make it work again

As always, if you fix or expand something, I'd love to get a patch.

Jeroen/Sprite_tm <jeroen@spritesmods.com>
