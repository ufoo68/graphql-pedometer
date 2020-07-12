#include <PubSubClient.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <M5StickC.h>
#include <math.h>

const char *ssid = "SSID";
const char *password = "PASS";

const char *mqtt_broker = "mqtt.eclipse.org";
const char *topic = "test/1/env";

WiFiClient espClient;
PubSubClient client(espClient);

const size_t capacity = JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(3);
DynamicJsonDocument doc(capacity);
JsonObject data = doc.createNestedObject("data");

float accX = 0;
float accY = 0;
float accZ = 0;

const int numOfSample = 50;
float sample[numOfSample];
float threshold = 0;
int countSample = 0;
float range = 50.0;

unsigned long countStep = 0;

float temp, humid = 0.0;
char output[128];
/*
 * getLocalTimeNTP: Get Epoch Time Stamp from NTP server
*/
unsigned long getLocalTimeNTP()
{
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time");
    return 0;
  }
  time(&now);
  return now;
}

float getDynamicThreshold(float *s)
{
  float maxVal = s[0];
  float minVal = s[0];
  for (int i = 1; i < sizeof(s); i++)
  {
    maxVal = max(maxVal, s[i]);
    minVal = min(minVal, s[i]);
  }
  return (maxVal + minVal) / 2.0;
}

float getFilterdAccelData()
{
  static float y[2] = {0};
  M5.MPU6886.getAccelData(&accX, &accY, &accZ);
  y[1] = 0.8 * y[0] + 0.2 * (abs(accX) + abs(accY) + abs(accZ)) * 1000.0;
  y[0] = y[1];
  return y[1];
}

void calcSteps()
{
  countSample++;
  sample[countSample] = getFilterdAccelData();
  if (abs(sample[countSample] - sample[countSample - 1]) < range)
  {
    sample[countSample] = sample[countSample - 1];
    countSample--;
  }
  if (sample[countSample] < threshold && sample[countSample - 1] > threshold)
  {
    countStep++;
  }
  if (countSample == numOfSample)
  {
    threshold = getDynamicThreshold(&sample[0]);
    countSample = 0;
    sample[countSample] = getFilterdAccelData();
  }
}

void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("arduinoClient"))
    {
      Serial.println("connected");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup()
{
  Serial.begin(115200);

  M5.begin();
  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setCursor(0, 0);
  M5.MPU6886.Init();
  sample[countSample] = getFilterdAccelData();

  WiFi.begin(ssid, password);
  configTime(9 * 3600L, 0, "ntp.nict.jp", "time.google.com", "ntp.jst.mfeed.ad.jp");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi Connected");
  Serial.println("IP Address: ");
  Serial.println(WiFi.localIP());
  delay(1000);
  client.setServer(mqtt_broker, 1883);
}

void loop()
{
  if (!client.connected())
  {
    reconnect();
  }
  client.loop();

  // Creating JSON Format
  data["time"].set(getLocalTimeNTP());
  data["steps"].set(countStep);

  // Output Format
  serializeJson(doc, output);

  Serial.println(output); // Data Format
  client.publish(topic, output);

  calcSteps();

  if (digitalRead(M5_BUTTON_RST) == LOW)
  {
    countStep = 0;
    M5.Lcd.fillScreen(BLACK);
    while (digitalRead(M5_BUTTON_RST) == LOW);
  }

  M5.Lcd.setCursor(0, 30);
  M5.Lcd.println(countStep);
  delay(100);
}