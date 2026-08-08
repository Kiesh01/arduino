#include <WiFi.h>
#include <WebSocketsServer.h>

WebSocketsServer webSocket(81);

const char* ssid = "Network";
const char* password = "admin2021";

// ---------------- Playlist ----------------

const char* playlist[] = {

   "https://www.facebook.com/share/r/1FsX9yDf1p"
   

};

const int playlistSize = sizeof(playlist) / sizeof(playlist[0]);

int currentVideo = 0;

// ---------------- Timer ----------------

// Change this to whatever you want
unsigned long videoDuration = 60000UL;   // 1 minute

unsigned long playStartTime = 0;

bool waitingForNext = false;

uint8_t currentClient = 0;

// ----------------------------------------------------

void sendOpenNew(uint8_t client)
{
    String command =
        "{\"command\":\"NEXT\",\"url\":\"";

    command += playlist[currentVideo];

    command += "\"}";

    Serial.println();
    Serial.println("Opening next video...");
    Serial.println(command);

    webSocket.sendTXT(client, command);
}

// ----------------------------------------------------

void webSocketEvent(uint8_t num,
                    WStype_t type,
                    uint8_t *payload,
                    size_t length)
{
    switch (type)
    {

        case WStype_CONNECTED:
        {
            Serial.printf("[%u] Connected\n", num);
            break;
        }

        case WStype_DISCONNECTED:
        {
            Serial.printf("[%u] Disconnected\n", num);
            break;
        }

        case WStype_TEXT:
        {
            String msg = (char*)payload;

            Serial.printf("[%u] %s\n", num, msg.c_str());

            // Browser is ready
            if (msg.indexOf("\"status\":\"READY\"") >= 0)
            {
                Serial.println("Browser Ready.");

                currentClient = num;

                // Play current video
                webSocket.sendTXT(num,
                    R"({"command":"PLAY"})");

                Serial.println("PLAY sent.");

            
            }

            if (msg.indexOf("\"status\":\"PLAYING\"") >= 0)
            {
                Serial.println("Video is now playing.");

                playStartTime = millis();

                waitingForNext = true;
            }

            break;
        }

        case WStype_ERROR:
        {
            Serial.printf("[%u] ERROR\n", num);
            break;
        }

        case WStype_BIN:
        {
            Serial.printf("[%u] Binary\n", num);
            break;
        }

        default:
        {
            Serial.printf("[%u] Event %d\n", num, type);
            break;
        }

    }
}

// ----------------------------------------------------

void setup()
{
    Serial.begin(115200);

    WiFi.mode(WIFI_STA);

    WiFi.begin(ssid, password);

    Serial.print("Connecting");

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println();
    Serial.println("WiFi Connected");

    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    webSocket.begin();

    webSocket.onEvent(webSocketEvent);

    Serial.println("WebSocket Server Started");
}

// ----------------------------------------------------

void loop()
{
    webSocket.loop();

    if (waitingForNext)
    {
        if (millis() - playStartTime >= videoDuration)
        {
            waitingForNext = false;

            currentVideo++;

            if (currentVideo >= playlistSize)
                currentVideo = 0;

            sendOpenNew(currentClient);
        }
    }
}