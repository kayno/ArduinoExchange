# ArduinoExchange

Based on the "Decadic Dial Decoder and Switching Controller" by Beau Walker from bjshed.better-than.tv. Emulatesa telephone exchange, allowing old pulse dialing phones to call each other.

Original project at [http://bjshed.better-than.tv/projects/electronics/rotary_arduino_exchange/](http://bjshed.better-than.tv/projects/electronics/rotary_arduino_exchange/)

This sketch extends the original project by allowing more than 2 phones (yet to be tested!). It also adds busyand ringback tones. 

The ringer generator code is also different. It supports a PCR-SIN03V12F20-C ring generator (found on eBay) which generates the 90VACring voltage. This device is switched on and off via an "inhibit" signal (which is just 5V) from the Arduino.

This sketch uses a [Microview](http://microview.io/) to display infomation about the exchange's status.
