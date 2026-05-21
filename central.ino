#include <WiFi.h>
#include <WiFiUdp.h>

const char* SSID     = "Vodafone-4516EF_2.4G";
const char* PASSWORD = "eCkmCpt8vjsBKwdc";
const int   UDP_PORT = 4210;
unsigned long last_prediction = 0;
const unsigned long PREDICTION_COOLDOWN_MS = 100;

WiFiUDP udp;
char packetBuffer[256];

// Latest readings
float est_ax, est_ay, est_az, est_gx, est_gy, est_gz;
float lom_ax, lom_ay, lom_az, lom_gx, lom_gy, lom_gz;
bool est_ready = false, lom_ready = false;

int simulateModel() {
  // TODO: replace this with actual model prediction
  // Inputs available: est_ax, est_ay, est_az, est_gx, est_gy, est_gz
  //                   lom_ax, lom_ay, lom_az, lom_gx, lom_gy, lom_gz
  // No reason to pass arguments to function  
  // since they are declared globally
  return random(1, 7); // 1 to 6 inclusive
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\nConnected: " + WiFi.localIP().toString());
  udp.begin(UDP_PORT);
}

void loop() {
  int packetSize = udp.parsePacket();
  if (packetSize) {
    int len = udp.read(packetBuffer, 255);
    packetBuffer[len] = '\0';

    // Parse: node,ax,ay,az,gx,gy,gz,t_ms
    char* parts[8];
    int i = 0;
    char* token = strtok(packetBuffer, ",");
    while (token && i < 8) {
      parts[i++] = token;
      token = strtok(NULL, ",");
    }

    if (i >= 7) {
      String node = String(parts[0]);
      // atribute to correct node type
      float ax = atof(parts[1]), ay = atof(parts[2]), az = atof(parts[3]);
      float gx = atof(parts[4]), gy = atof(parts[5]), gz = atof(parts[6]);

      if (node == "esterno") {
        est_ax=ax; est_ay=ay; est_az=az;
        est_gx=gx; est_gy=gy; est_gz=gz;
        est_ready = true;
      }else if (node == "lombar") {
        lom_ax=ax; lom_ay=ay; lom_az=az;
        lom_gx=gx; lom_gy=gy; lom_gz=gz;
        lom_ready = true;
      }

      // When both readings are available, run inference
      if (est_ready && lom_ready && (millis() - last_prediction) >= PREDICTION_COOLDOWN_MS) {
        // TODO: model inference goes here
        int prediction = simulateModel();
        Serial.printf("EST: %.1f %.1f %.1f | LOM: %.1f %.1f %.1f --------------- ",
          est_ax, est_ay, est_az, lom_ax, lom_ay, lom_az);
        Serial.printf("Prediction: %d\n", prediction);
        est_ready = false; lom_ready = false;
        last_prediction = millis();  
      }
    } 
  }
}