# ESP8266 to Salesforce Sketch for Trailhead IoT Project #
 
Uses an ESP866 to connect to salesforce, login via OAuth user/password flow and create platform event records
This is an example based on the trailhead project "Build an IoT Integration with Electric Imp"(https://trailhead.salesforce.com/projects/workshop-electric-imp)
The ESP8266 replaces the Electric Imp with minimal change to the instructions for the badge

You will need an ESP8266 (I am using a NodeMCU 1.0, pin names may vary with other boards), a DHT11 (or DHT22) temp/humidity sensor, and a switch/5k resistor (or a light sensitive switch/resistor)

## Changes required for Badge Steps##
 
* Step 2 - Set Up the Electric Imp Hardware
Ignore this step, connect your DHT11 (or DHT22) sensor to +3.3v, GND and D0 (data) of your ESP8266
Connect a switch between D1 and +3.3v and a 5k resistory from D1 to GND
 
* Step 4 - Create a Salesforce Connected Appplication
Rather than the URL https://agent.electricimp.com/???? you can use http://localhost/_auth in Step 5, section 2
 
* Step 10 - Build and Run the Electric Imp Application
Ignore this, simply flash this code to your ESP8266
 
* Step 11 - Place Your impExplorer Kit in a Refrigerator 
Place the ESP8266 in your refrigerator, you will need a way to trigger the switch, alternatively you could use a light sensor (as per the electric imp) however this would need additional code to support it. If you are using an NodeMCU or similar you can easily power it from a USB mobile phone battery.
 
You can simply use the code in loop() to generate random data to test with if you don't wish to do this
 
## Setup required ##
 
1. You will need to set the SSID (Network name) and Password variables to match your WiFi network
 
2. You will need to generate certificate fingerprints for both your login url (e.g login.salesforce.com) and your instance url (e.g eu1.salesforce.com) and insert them below, you can do this by running this command on macOS/*nix 

`echo | openssl s_client -connect <your instance url>:443 | openssl x509 -fingerprint -noout`
 
3. You will need to change the username, password, token, client secret and client key to match your org's configuration
