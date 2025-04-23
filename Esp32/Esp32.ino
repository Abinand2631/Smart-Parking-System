#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h> // Use HTTPClient for simplicity here

// Replace with your network credentials
const char* ssid = "OnePlus Nord CE3 5G";
const char* password = "abi@2631";

// Replace with the URL of your backend server endpoint
// If running locally for testing: "http://YOUR_COMPUTER_IP:5000/upload_image"
// If deployed on Render/PythonAnywhere: "https://your-app-name.onrender.com/upload_image" (MUST use HTTPS for deployed apps)
// NOTE: Using HTTPS on ESP32 requires WiFiClientSecure and potentially certificate handling, which adds complexity.
//       Start with HTTP for local testing. For deployment, you MUST adapt this to use HTTPS.
const char* serverUrl = "http://192.168.106.252:5000/upload_image";

// --- CAMERA PIN CONFIGURATION ---
// AI-Thinker ESP32-CAM Pins - MOST COMMON MODEL
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

void setup() {
  Serial.begin(115200);
  Serial.println("\nESP32-CAM Booting...");

  // Camera Configuration
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG; // JPEG format is needed for web upload

  // Select resolution based on quality vs speed trade-off
  // Higher res = better LPR potential, but larger file & slower upload
  // config.frame_size = FRAMESIZE_UXGA; // (1600 x 1200) - High quality
  config.frame_size = FRAMESIZE_SVGA;  // (800 x 600) - Medium quality/size
  // config.frame_size = FRAMESIZE_VGA;   // (640 x 480) - Lower quality/size

  config.jpeg_quality = 12; // 0-63 lower number means higher quality
  config.fb_count = 1;      // Use 1 frame buffer. Increase if you face issues.

  // Initialize Camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
  Serial.println("Camera Initialized.");

  // Adjust camera settings (optional, experiment for better images)
  sensor_t * s = esp_camera_sensor_get();
  // s->set_brightness(s, 0);     // -2 to 2
  // s->set_contrast(s, 0);       // -2 to 2
  // s->set_saturation(s, 0);     // -2 to 2
  // s->set_special_effect(s, 0); // 0 to 6 (0 - No Effect)
  // s->set_whitebal(s, 1);       // 0 = disable , 1 = enable
  // s->set_awb_gain(s, 1);       // 0 = disable , 1 = enable
  // s->set_wb_mode(s, 0);        // 0 to 4 - if awb_gain enabled (0 - Auto, 1 - Sunny, 2 - Cloudy, 3 - Office, 4 - Home)
  // s->set_exposure_ctrl(s, 1);  // 0 = disable , 1 = enable
  // s->set_aec2(s, 0);           // 0 = disable , 1 = enable
  // s->set_ae_level(s, 0);       // -2 to 2
  // s->set_aec_value(s, 300);    // 0 to 1200
  // s->set_gain_ctrl(s, 1);      // 0 = disable , 1 = enable
  // s->set_agc_gain(s, 0);       // 0 to 30
  // s->set_gainceiling(s, (gainceiling_t)0);  // 0 to 6
  // s->set_bpc(s, 0);            // 0 = disable , 1 = enable
  // s->set_wpc(s, 1);            // 0 = disable , 1 = enable
  // s->set_raw_gma(s, 1);        // 0 = disable , 1 = enable
  // s->set_lenc(s, 1);           // 0 = disable , 1 = enable
  // s->set_hmirror(s, 0);        // 0 = disable , 1 = enable
  // s->set_vflip(s, 0);          // 0 = disable , 1 = enable
  // s->set_dcw(s, 1);            // 0 = disable , 1 = enable
  // s->set_colorbar(s, 0);       // 0 = disable , 1 = enable


  // Connect to Wi-Fi
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.println("Setup complete. Send any character via Serial to capture and upload image.");
}

void loop() {
  // Simple trigger: capture image when a character is received via Serial Monitor
  if (Serial.available() > 0) {
    Serial.read(); // Read the incoming byte
    Serial.println("Trigger received. Capturing photo...");
    captureAndSendPhoto();
  }
  delay(100); // Small delay to prevent busy-waiting
}

void captureAndSendPhoto() {
  camera_fb_t * fb = NULL;

  // Take Picture with Camera
  fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }
  Serial.printf("Picture taken! Size: %zu bytes\n", fb->len);

  // --- Send Picture using HTTP POST ---
  HTTPClient http;

  Serial.print("Connecting to server: ");
  Serial.println(serverUrl);

  // Use http.begin(serverUrl) for HTTP
  // For HTTPS you MUST use http.begin(client, serverUrl) where client is a WiFiClientSecure object
  // configured with certificates or fingerprints.
  http.begin(serverUrl); // Specify target URL - Assumes HTTP for now

  // Specify content-type header (important!)
  http.addHeader("Content-Type", "image/jpeg");

  // Send the request. fb->buf is the pointer to the image data, fb->len is the length.
  int httpResponseCode = http.POST(fb->buf, fb->len);

  Serial.print("HTTP Response code: ");
  Serial.println(httpResponseCode);

  // Get the response payload (if any)
  if (httpResponseCode > 0) {
    String payload = http.getString();
    Serial.println("Response payload:");
    Serial.println(payload);
  } else {
    Serial.printf("HTTP POST failed, error: %s\n", http.errorToString(httpResponseCode).c_str());
  }

  // Free resources
  http.end();
  esp_camera_fb_return(fb); // IMPORTANT: Release the frame buffer

  Serial.println("Image sent.");
}