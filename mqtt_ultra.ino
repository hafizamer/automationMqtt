#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Servo.h>

// Replace with your network credentials
const char* ssid = "";
const char* password = "";

// Replace with your MQTT broker details
const char* mqtt_server = "";
const int mqtt_port = 8883;
const char* mqtt_topic = "esp8266/led";
const char* mqtt_user = "";
const char* mqtt_password = "";

// Replace with your ThingSpeak details
const char* thingSpeakApiKey = "";
const unsigned long thingSpeakChannelID = ;

// Ultrasonic sensor pins
const int trigPin = D6; // GPIO2
const int echoPin = D5; // GPIO0

// LED pins
const int led1Pin = D1; // GPIO5
const int led2Pin = D2; // GPIO4
const int led3Pin = D3; // GPIO0
const int led4Pin = D4; // GPIO2

// Servo pin
const int servoPin = D7;

// Global variable to keep track of LED and servo control state
bool controlEnabled = true;

WiFiClientSecure espClient; // Use WiFiClientSecure for SSL/TLS
PubSubClient client(espClient);
Servo servo;  // Create servo object to control a servo

// Variables for ultrasonic sensor
long duration;
int distance;

void setup() {
  Serial.begin(115200);

  // Set LED pins as outputs
  pinMode(led1Pin, OUTPUT);
  pinMode(led2Pin, OUTPUT);
  pinMode(led3Pin, OUTPUT);
  pinMode(led4Pin, OUTPUT);

  // Attach the servo to the servo pin
  servo.attach(servoPin);

  // Set ultrasonic sensor pins
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to Wi-Fi");

  espClient.setInsecure(); // Disable SSL certificate verification
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  reconnect();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Clears the trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  // Sets the trigPin on HIGH state for 10 microseconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);

  // Calculating the distance
  distance = duration * 0.034 / 2;

  // Print distance to Serial Monitor
  Serial.print("Distance: ");
  Serial.println(distance);

  // Read light level
  int light = analogRead(A0); // Directly use the raw value for LDR

  // Print light level to Serial Monitor
  Serial.print("Darkness level: ");
  Serial.println(light);

  // Control LEDs and servo based on distance and light if control is enabled
  if (controlEnabled) {
    // Control LEDs based on distance
    if (distance > 10) { // Adjust the threshold as needed
      digitalWrite(led1Pin, HIGH);
      digitalWrite(led2Pin, LOW);
    } else {
      digitalWrite(led1Pin, LOW);
      digitalWrite(led2Pin, HIGH);
      //servo.detach();
    }

    // Control LEDs based on light level
    if (light < 800) { // Adjust the threshold as needed
      digitalWrite(led3Pin, HIGH);
      digitalWrite(led4Pin, LOW);
    } else {
      digitalWrite(led3Pin, LOW);
      digitalWrite(led4Pin, HIGH);
      //servo.detach();
    }

    // Control the servo
    if (distance > 10 && light < 800) {
      for (int pos = 0; pos <= 180; pos += 1) { // rotate from 0 degrees to 180 degrees
        servo.write(pos);                   // tell servo to go to position in variable 'pos'
        delay(10);                          // waits 10ms for the servo to reach the position
      }

      for (int pos = 180; pos >= 0; pos -= 1) { // rotate from 180 degrees to 0 degrees
        servo.write(pos);                   // tell servo to go to position in variable 'pos'
        delay(10);                          // waits 10ms for the servo to reach the position
      }
    }
  } else {
    // Turn off all LEDs and stop the servo if control is disabled
    digitalWrite(led1Pin, LOW);
    digitalWrite(led2Pin, LOW);
    digitalWrite(led3Pin, LOW);
    digitalWrite(led4Pin, LOW);
    servo.detach();
  }

  // Publish sensor data to ThingSpeak
  String payload = "field1=" + String(distance) + "&field2=" + String(light);
  WiFiClient thingSpeakClient;
  if (thingSpeakClient.connect("api.thingspeak.com", 80)) {
    thingSpeakClient.print(String("GET /update?api_key=") + thingSpeakApiKey + "&" + payload + " HTTP/1.1\r\n" +
                           "Host: api.thingspeak.com\r\n" +
                           "Connection: close\r\n\r\n");
    thingSpeakClient.stop();
  }

  delay(1000); // Adjust delay as needed
}

void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.print("Message arrived: ");
  Serial.println(message);

  // Update control state based on MQTT message
  if (message == "OFF") {
    controlEnabled = false;
  } else if (message == "ON") {
    controlEnabled = true;
    servo.attach(servoPin); // Reattach the servo when control is enabled
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP8266Client", mqtt_user, mqtt_password)) {
      Serial.println("connected");
      client.subscribe(mqtt_topic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}
