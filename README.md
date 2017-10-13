vitalk
======

Communication with Vitosolar 200 (VScotHO1, 20CB) and custom DTR via Optolink. This is also an english port of this software.
This version has been translated in english and also support raw get, raw set and quit (`rg`, `rs`, `q` commands).

This application works for `Ox20CB` (`VScotHO1`) Viessmann device heater based on P300 protocol.

It also integrates control for a custom made Differential Temperature Regulator that replaces the original (static) Viessmann module. The custom DTR module can be controlled dynamically in function of data read from the main interface and other custom needs.

## links :
* https://github.com/mqu/viessmann-mqtt : MQTT gateway in Ruby with Android and Cayenne dashboard.
* https://github.com/klauweg/vitalk : original vitalk project : thanks to klauweg
* http://openv.wikispaces.com/ : Viessmann heater general forum (in german).
* https://gist.github.com/mqu/9519e39ccc474f111ffb : 
  * my first ruby script with P300 protocol implementation : there is some bugs and is not really usable ; 
  * this repository describe P300 protocols
* wireless link with ESP8266 : https://github.com/rene-mt/vitotronic-interface-8266
