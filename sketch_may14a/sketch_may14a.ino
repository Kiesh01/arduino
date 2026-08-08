#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

const char* ssid = "Network";
const char* wifiPassword = "admin2021";

String email = "kierugitau0@outlook.com";
String omadaPassword = "Latech76.";

String fullCookie =
"UID_SSO_SID=ce073dbe1e5a45e989212833cab8bebf; "
"SESSION=ce073dbe1e5a45e989212833cab8bebf; "
"TPEC_SID=e1-iam-8f455ea1ecf94d808cea7394240bdfc1; "
"csrfToken=d46b863240ff4386b53d1354413df51c; "
"TPOMADA_SESSIONID=e1-iam-8f455ea1ecf94d808cea7394240bdfc1";

String csrfToken =
"d46b863240ff4386b53d1354413df51c";


void authorizeClient(String macAddress) {

    WiFiClientSecure client;
    client.setInsecure();

    // =========================
    // STEP 1: AUTHORIZE OMADA FIRST
    // =========================
    HTTPClient http2;

    String authURL =
    "https://euw1-api-omada-essential-controller.tplinkcloud.com/ffff168c91945034e0271db876fe1071/api/v2/sites/68c91946034e0271db876fe3/cmd/clients/"
    + macAddress +
    "/auth";

    http2.begin(client, authURL);

    http2.addHeader("Cookie", fullCookie);
    http2.addHeader("csrf-token", csrfToken);

    int httpCode = http2.POST("");

    Serial.print("Authorization Response Code: ");
    Serial.println(httpCode);

    String response = http2.getString();

    Serial.println("Authorization Response:");
    Serial.println(response);

    http2.end();

    // =========================
    // STEP 2: CHECK SUCCESS
    // =========================
    if (httpCode == 200 && response.indexOf("\"errorCode\":0") >= 0) {

        Serial.println("Omada SUCCESS → updating backend...");

        // =========================
        // STEP 3: UPDATE BACKEND
        // =========================
        HTTPClient http1;

        http1.begin(client, "https://surfsimple-telecommunications-92c91eb5b761.herokuapp.com/mark_authorized");
        http1.addHeader("Content-Type", "application/json");

        String payload = "{\"mac\":\"" + macAddress + "\"}";

        int backendCode = http1.POST(payload);

        Serial.print("Backend Response Code: ");
        Serial.println(backendCode);

        String backendResponse = http1.getString();
        Serial.println("Backend Response:");
        Serial.println(backendResponse);

        http1.end();

    } else {

        Serial.println("Omada FAILED → NOT updating backend");
    }
}
void deauthorizeClient(String macAddress) {

    WiFiClientSecure client;
    client.setInsecure();

    // =========================
    // STEP 1: DEAUTHORIZE OMADA FIRST
    // =========================
    HTTPClient http;

    String unauthURL =
    "https://euw1-api-omada-essential-controller.tplinkcloud.com/ffff168c91945034e0271db876fe1071/api/v2/sites/68c91946034e0271db876fe3/cmd/clients/"
    + macAddress +
    "/unauth";

    http.begin(client, unauthURL);

    http.addHeader("Cookie", fullCookie);
    http.addHeader("csrf-token", csrfToken);

    int httpCode = http.POST("");

    Serial.print("Deauthorize Response Code: ");
    Serial.println(httpCode);

    String omadaResponse = http.getString();

    Serial.println("Deauthorize Response:");
    Serial.println(omadaResponse);

    http.end();

    // =========================
    // STEP 2: CHECK SUCCESS
    // =========================
    if (httpCode == 200 && omadaResponse.indexOf("\"errorCode\":0") >= 0) {

        Serial.println("Omada DEAUTH SUCCESS → updating backend...");

        // =========================
        // STEP 3: UPDATE BACKEND
        // =========================
        HTTPClient http2;

        http2.begin(client, "https://surfsimple-telecommunications-92c91eb5b761.herokuapp.com/mark_deauthorized");
        http2.addHeader("Content-Type", "application/json");

        String payload = "{\"mac\":\"" + macAddress + "\"}";

        int backendCode = http2.POST(payload);

        Serial.print("Backend Update Code: ");
        Serial.println(backendCode);

        String backendResponse = http2.getString();
        Serial.println("Backend Response:");
        Serial.println(backendResponse);

        http2.end();

    } else {

        Serial.println("Omada DEAUTH FAILED → NOT updating backend");
    }
}

void fetchExpiredClients() {

    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;

    String apiURL =
    "https://surfsimple-telecommunications-92c91eb5b761.herokuapp.com/expired_clients";

    http.begin(client, apiURL);

    int httpCode = http.GET();

    Serial.print("Expired Clients Response Code: ");
    Serial.println(httpCode);

    String response = http.getString();

    Serial.println(response);

    DynamicJsonDocument doc(4096);

    deserializeJson(doc, response);

    JsonArray clients =
    doc.as<JsonArray>();

    for (JsonObject clientObj : clients) {

        String mac =
        clientObj["mac"].as<String>();

        Serial.print("Deauthorizing: ");
        Serial.println(mac);

        deauthorizeClient(mac);

        delay(2000);
    }

    http.end();
}
void fetchPendingClients() {

    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;

    String apiURL =
    "https://surfsimple-telecommunications-92c91eb5b761.herokuapp.com/pending_clients";

    http.begin(client, apiURL);

    int httpCode = http.GET();

    Serial.print("Pending Clients Response: ");
    Serial.println(httpCode);

    String response = http.getString();

    Serial.println(response);

    DynamicJsonDocument doc(4096);

    deserializeJson(doc, response);

    JsonArray clients =
    doc.as<JsonArray>();

    for (JsonObject clientObj : clients) {

        String mac =
        clientObj["mac"].as<String>();

        Serial.print("Authorizing: ");
        Serial.println(mac);

        authorizeClient(mac);

        delay(2000);
    }

    http.end();
}
void setup() {

    Serial.begin(115200);

    WiFi.begin(ssid, wifiPassword);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("\nWiFi Connected");

    loginToOmada();

    getCsrfToken();

    authorizeClient("");

 
}

void loop() {
    fetchPendingClients();
    fetchExpiredClients();

    delay(10000);
}

void loginToOmada() {

    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;

    String loginURL =
    "https://api-id.tplinkcloud.com/api/v1/login";

    http.begin(client, loginURL);

    http.addHeader("Content-Type", "application/json");

    const char* headerKeys[] = {
        "Set-Cookie",
        "set-cookie"
    };

    http.collectHeaders(headerKeys, 2);

    String payload = "{";
    payload += "\"email\":\"" + email + "\",";
    payload += "\"password\":\"" + omadaPassword + "\",";
    payload += "\"privatePolicyChecked\":false";
    payload += "}";

    int httpCode = http.POST(payload);

    Serial.print("Login Response Code: ");
    Serial.println(httpCode);

    String response = http.getString();

    Serial.println("\nLogin Response:");
    Serial.println(response);

    Serial.println("\nAll Headers:");

    for (int i = 0; i < http.headers(); i++) {

        Serial.println(
            http.headerName(i) +
            ": " +
            http.header(i)
        );
    }

  

    http.end();
}

void getCsrfToken() {

    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;

    String statusURL =
    "https://euw1-api-omada-essential-controller.tplinkcloud.com/ffff168c91945034e0271db876fe1071/api/v2/current/login-status?needToken=true";

    http.begin(client, statusURL);

    http.addHeader("Cookie", fullCookie);
    http.addHeader("csrf-token",csrfToken);

    int httpCode = http.GET();

    Serial.print("CSRF Response Code: ");
    Serial.println(httpCode);

    String response = http.getString();

    Serial.println("\nLogin Status Response:");
    Serial.println(response);

    DynamicJsonDocument doc(4096);

    deserializeJson(doc, response);

    csrfToken =
    doc["result"]["csrfToken"].as<String>();

    Serial.println("\nCSRF Token:");
    Serial.println(csrfToken);

    http.end();
}