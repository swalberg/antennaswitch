# Remote antenna switch

This is a remote controller for one of those DIY antenna switch kits. It listens on Wifi for commands and controls the relays. They take a +12V signal to one of 4 relays with a common ground to pick which port is active, and ground the rest:

![example antenna switch](images/s-l500.jpg)


The firmware for this allows both manual control, and to listen to N1MM+/RUMLogNG broadcasts to pick the band based on configured port/band combinations.

![screenshots](images/screenshot.png)

When the device boots without a config it'll start a new AP for you to configure it.
