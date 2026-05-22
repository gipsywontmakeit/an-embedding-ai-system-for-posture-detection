#include <Wire.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <MPU6050.h> 

// --- CONFIG ---
const char* SSID     = "Vodafone-4516EF_2.4G";
const char* PASSWORD = "eCkmCpt8vjsBKwdc";
const char* PC_IP    = "192.168.1.199";  // IP do teu PC na rede
const int   UDP_PORT = 4210;
const char* NODE_ID  = "lombar";       

// --- CONFIG ---
// const char* SSID     = "Iphone do joel";
// const char* PASSWORD = "carlossinner";
// const char* PC_IP    = "172.20.10.6";  // IP do teu PC na rede
// const int   UDP_PORT = 4210;
// const char* NODE_ID  = "esterno";  

WiFiUDP udp;
MPU6050 mpu;

void setup() {
  Serial.begin(115200);
  Wire.begin();

  // WiFi
  WiFi.begin(SSID, PASSWORD);
  Serial.print("Connecting WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected: " + WiFi.localIP().toString());

  // MPU
  mpu.initialize();
  if (!mpu.testConnection()) {
    Serial.println("MPU6050 connection FAILED");
    while (1);
  }
  Serial.println("MPU OK");

  udp.begin(UDP_PORT);
}

void loop() {
  int16_t ax, ay, az, gx, gy, gz;
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  // Formato: NODE,ax,ay,az,gx,gy,gz,timestamp
  char packet[128];
  snprintf(packet, sizeof(packet), "%s,%d,%d,%d,%d,%d,%d,%lu",
           NODE_ID, ax, ay, az, gx, gy, gz, millis());

  udp.beginPacket(PC_IP, UDP_PORT);
  udp.print(packet);
  udp.endPacket();

  delay(20);  // 50Hz
}