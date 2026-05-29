#include <WiFi.h>
#include <WiFiUdp.h>

// ===================== TFLITE MICRO (ESP32 BUILT-IN) =====================
// Uses libespressif__esp-tflite-micro.a bundled with the ESP32 core.
// Do NOT install the Chirale_TensorFlowLite library — it duplicates these
// symbols and causes "multiple definition" link errors.
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"

// ===================== MODEL VERSION =====================
// Uncomment ONE of the two lines below:
// #include "model.h"            // v1 — original architecture, 3 classes
#include "model_finetuned.h" // v2 — BN + L2 + Dropout 0.5, 3 classes

// ===================== CONFIG WIFI =====================
const char* SSID     = "iPhone";
const char* PASSWORD = "4lv4r0.WIFI.2003";
const int   UDP_PORT = 4210;

WiFiUDP udp;
char packetBuffer[256];

// ===================== CONFIG MODELO =====================
const int N_STEPS    = 100; // model waits for 100 sequential samples before predicting
const int N_FEATURES = 12;
const int N_OUTPUTS  = 3;

// quantization values from deploy_params
const float INPUT_SCALE  = 0.09173505753278732f; // quantization parameters (int8) in
const int   INPUT_ZP     = -3;
const float OUTPUT_SCALE = 0.00390625f;// quantization parameters (int8) out
const int   OUTPUT_ZP    = -128;

// standard scaler params trained on the notebook for normalization
const float MEAN[12] = {
  15101.760613f, -214.947396f, -1801.342407f,  86.836280f, -271.902451f,  335.995077f,
   1836.087527f, 13667.807440f, 1767.886214f, -134.170678f,  184.851204f,  -67.514223f
};
const float STD[12] = {
   3128.989625f, 1005.776162f, 6055.113310f, 1030.515671f, 471.947376f, 456.084918f,
    998.206247f, 3968.598815f, 5628.089728f,  284.687593f, 481.395959f, 306.142571f
};

const char* LABELS[3] = { "bad_back", "bad_front", "good" };

// ===================== TFLITE =====================

// pointers for the tflite model loaded of model.h and for the interpreter
const tflite::Model* model = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;

// tensor pointers
TfLiteTensor* input = nullptr;
TfLiteTensor* output = nullptr;

constexpr int kArenaSize = 40 * 1024;
uint8_t tensor_arena[kArenaSize];

// ===================== ESTADO =====================
float window[N_STEPS][N_FEATURES];
int   win_count = 0; // inference starts when win_count >= 100

float est[6], lom[6];
bool  est_ready = false, lom_ready = false;

// ===================== SETUP =====================
void setup() {
  Serial.begin(115200);

  WiFi.begin(SSID, PASSWORD);
  Serial.print("Connecting WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected: " + WiFi.localIP().toString());

  udp.begin(UDP_PORT);

  // swap this line when switching model:
  //model = tflite::GetModel(model_tflite);           // v1
  model = tflite::GetModel(model_finetuned_tflite); // v2

  if (model->version() != TFLITE_SCHEMA_VERSION) {
    Serial.println("Model schema mismatch");
    while (1);
  }

  static tflite::MicroMutableOpResolver<10> resolver;
  resolver.AddConv2D();
  resolver.AddMean();
  resolver.AddFullyConnected();
  resolver.AddSoftmax();
  resolver.AddReshape();
  resolver.AddQuantize();
  resolver.AddExpandDims();
  resolver.AddMul();
  resolver.AddAdd();
  resolver.AddDequantize();

  // initialize inference and prepare in/out pointers to runInference()
  static tflite::MicroInterpreter static_interp(
    model, resolver, tensor_arena, kArenaSize);
  interpreter = &static_interp;

  if (interpreter->AllocateTensors() != kTfLiteOk) {
    Serial.println("AllocateTensors FAILED");
    while (1);
  }

  input  = interpreter->input(0);
  output = interpreter->output(0);
  Serial.println("Model ready");
}

// ===================== INFERÊNCIA =====================
void runInference() {
  int idx = 0;
  for (int t = 0; t < N_STEPS; t++)
    for (int f = 0; f < N_FEATURES; f++) {
      float norm = (window[t][f] - MEAN[f]) / STD[f];
      int q = (int)round(norm / INPUT_SCALE) + INPUT_ZP; // quant
      if (q < -128) q = -128;
      if (q >  127) q =  127;
      input->data.int8[idx++] = (int8_t)q;
    }

  if (interpreter->Invoke() != kTfLiteOk) { //run the model
    Serial.println("Invoke FAILED");
    return;
  }

  int   best  = 0;
  float bestp = -1;
  for (int i = 0; i < N_OUTPUTS; i++) {
    float p = (output->data.int8[i] - OUTPUT_ZP) * OUTPUT_SCALE;
    if (p > bestp) { bestp = p; best = i; }
  }
  Serial.printf("Posture: %s (%.2f)\n", LABELS[best], bestp); // class + prob
}

// ===================== LOOP =====================
void loop() {
  int packetSize = udp.parsePacket(); // check if an udp packet has arrived
  if (!packetSize) return;

  int len = udp.read(packetBuffer, 255);
  if (len < 0) len = 0;
  packetBuffer[len] = '\0';
  // Serial.printf("RX from %s: %s\n", udp.remoteIP().toString().c_str(), packetBuffer); // DEBUG

  char* parts[8];
  int i = 0;
  char* token = strtok(packetBuffer, ",");
  while (token && i < 8) {
    parts[i++] = token;
    token = strtok(NULL, ",");
  }
  if (i < 7) return;

  String node = String(parts[0]);
  float v[6];
  for (int k = 0; k < 6; k++) v[k] = atof(parts[k + 1]);

  if (node == "esterno")     { memcpy(est, v, sizeof(v)); est_ready = true; }
  else if (node == "lombar") { memcpy(lom, v, sizeof(v)); lom_ready = true; }

  if (est_ready && lom_ready) {
    float row[12] = {
      lom[0], lom[1], lom[2], lom[3], lom[4], lom[5],
      est[0], est[1], est[2], est[3], est[4], est[5]
    };

    for (int t = 0; t < N_STEPS - 1; t++)
      memcpy(window[t], window[t + 1], sizeof(float) * N_FEATURES);
    memcpy(window[N_STEPS - 1], row, sizeof(row));

    if (win_count < N_STEPS) win_count++;

    est_ready = false;
    lom_ready = false;

    if (win_count >= N_STEPS) runInference();
  }
}
