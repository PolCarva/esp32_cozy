#include <Arduino.h>
#include <ArduinoJson.h>
#if defined(ESP32) || defined(ARDUINO_RASPBERRY_PI_PICO_W)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif

#include <FirebaseClient.h>
#include <WiFiClientSecure.h>

#include <Adafruit_NeoPixel.h>

#include <time.h>

#define WIFI_SSID "Pol"
#define WIFI_PASSWORD "unodostres"

#define DATABASE_SECRET "AIzaSyDAj4u7i2qP_3B9i1t87-Lqa1fHJIPzLYE"
#define DATABASE_URL "https://proyectomultimedia1-default-rtdb.firebaseio.com/"

WiFiClientSecure ssl;
DefaultNetwork network;
AsyncClientClass client(ssl, getNetwork(network));

FirebaseApp app;
RealtimeDatabase Database;
AsyncResult result;
LegacyToken dbSecret(DATABASE_SECRET);

String config;

// Time variables
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -10800; // Uruguay GMT Offset (UTC-3)
const int daylightOffset_sec = 0;  // Daylight savings time offset

int horaAcostarse = 25;
int minutoAcostarse = 61;
int horaDespertarse = 25;
int minutoDespertarse = 61;

struct Color
{
    float r;
    float g;
    float b;
    float alpha;
};

/* PINES YA USADOS: 39, 35, 33, 32, 23, 22, 21, 19, 18, 13*/

// ------ ON OFF ------ //
const int onOffBtnPin = 21;   // Pin del botón táctil para ON/OFF
bool onOffState = false;      // Estado actual del ON/OFF (false = apagado, true = encendido)
bool lastOnOffBtnState = LOW; // Último estado del botón de ON/OFF
// ------ END ON OFF ------ //

// ------  AROMA ------ //
const int aromaPin = 18;    // Pin para el controlador de aroma
const int aromaBtnPin = 19; // Pin del botón táctil para aroma

bool aromaState = false;      // Estado actual del aroma (false = apagado, true = encendido)
bool lastAromaBtnState = LOW; // Último estado del botón de aroma

bool aromaFlag = false;

int aromaAcostarseDesde = 5;
int aromaAcostarseHasta = 2;
bool isAromaActiveAcostarse = true;

int aromaDespertarseDesde = 5;
int aromaDespertarseHasta = 2;
bool isAromaActiveDespertarse = true;
// ------ END AROMA ------ //

// ------  LEDS ------ //
#define NUMPIXELS 30
const int mainLedPin = 33;   // Pin de LED principal
const int secondLedPin = 32; // Pin de LED secundario
const int ledBtnPin = 35;    // Pin del botón táctil para LEDs

bool ledState = false;      // Estado actual de los LEDs (false = apagado, true = encendido)
bool lastLedBtnState = LOW; // Último estado del botón táctil para LEDs

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, mainLedPin, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel pixels2 = Adafruit_NeoPixel(NUMPIXELS, secondLedPin, NEO_GRB + NEO_KHZ800);

Color colorAcostarse;
Color colorDespertar;

bool luzAmbientalFlag = false;

int luzAmbientalAcostarseDesde = 5;
int luzAmbientalAcostarseHasta = 2;
bool isLuzAmbientalActive = true;

int luzAmbientalDespertarseDesde = 5;
int luzAmbientalDespertarseHasta = 2;
bool isLuzAmbientalActiveDespertarse = true;

// ----END LED------ //

// ------ SENSOR ------ //
const int sensorTrigPin = 23; // Pin del sensor de ultrasonido (TRIG)
const int sensorEchoPin = 22; // Pin del sensor de ultrasonido (ECHO)

float sensorDuration, sensorDistance;

// ------ END SENSOR ------ //

// ------ SONIDO ------ //
const int soundPlayPouseBtnPin = 13; // Pin del botón táctil para reproducir/pausar sonido
const int soundVolumePin = 39;       // Pin del potenciómetro para controlar el volumen del sonido
int sonidoAlarma = 1;                // Sonido actual de la alarma
int sonidoAcostarse = 1;             // Sonido actual del despertar

bool seApagoLaAlarma = false;

bool soundPlayState = true;    // Estado actual del sonido (false = pausado, true = reproduciendo)
bool lastSoundPlayState = LOW; // Último estado del botón táctil para reproducir/pausar sonido
const int soundVolume = 0;     // Volumen actual del sonido
int lastSoundVolume = 0;       // Último volumen del sonido

bool sonidoFlag = false;

int sonidoAcostarseDesde = 5;
int sonidoAcostarseHasta = 2;
bool isSonidoActive = true;

int sonidoDespertarseDesde = 5;
int sonidoDespertarseHasta = 2;
bool isAlarmaActive = true;

bool estaSonando = false;

// ------ END SONIDO ------ //

void connectToWiFi()
{
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(300);
    }
}

void getDbConfig()
{
    if (horaAcostarse == 25) // ----------------------------------------------------------------------------------------------- CAMBIAR ESTO
    {

        // Intenta obtener la configuración como un std::string
        std::string stdConfig = Database.get<std::string>(client, "/Config");

        // Convierte std::string a String de Arduino
        String config = String(stdConfig.c_str());

        // Crea un buffer JSON suficientemente grande
        StaticJsonDocument<1024> doc;

        // Deserializar el JSON
        DeserializationError error = deserializeJson(doc, config);
        if (error)
        {
            return;
        }

        horaAcostarse = doc["acostarse"]["hora"];
        minutoAcostarse = doc["acostarse"]["minuto"];

        horaDespertarse = doc["despertar"]["hora"];
        minutoDespertarse = doc["despertar"]["minuto"];

        //----------------- LEDs -----------------//
        colorAcostarse = {
            doc["luzAmbiental"]["color"]["r"],
            doc["luzAmbiental"]["color"]["g"],
            doc["luzAmbiental"]["color"]["b"],
            doc["luzAmbiental"]["color"]["a"]};

        colorDespertar = {
            doc["luzAmbiental"]["despertar"]["color"]["r"],
            doc["luzAmbiental"]["despertar"]["color"]["g"],
            doc["luzAmbiental"]["despertar"]["color"]["b"],
            doc["luzAmbiental"]["despertar"]["color"]["a"]};

        luzAmbientalAcostarseDesde = doc["luzAmbiental"]["desde"];
        luzAmbientalAcostarseHasta = doc["luzAmbiental"]["hasta"];

        luzAmbientalDespertarseDesde = doc["luzAmbiental"]["despertar"]["desde"];
        luzAmbientalDespertarseHasta = doc["luzAmbiental"]["despertar"]["hasta"];

        isLuzAmbientalActive = doc["luzAmbiental"]["active"];
        isLuzAmbientalActiveDespertarse = doc["luzAmbiental"]["despertar"]["active"];
        //----------------- ALARMA -----------------//
        sonidoAlarma = doc["alarma"]["sonido"];
        isAlarmaActive = doc["alarma"]["active"];

        sonidoAcostarse = doc["sonidoAmbiental"]["sonido"];
        isSonidoActive = doc["sonidoAmbiental"]["active"];

        //----------------- AROMA -----------------//
        aromaAcostarseDesde = doc["aroma"]["desde"];
        aromaAcostarseHasta = doc["aroma"]["hasta"];

        aromaDespertarseDesde = doc["aroma"]["despertar"]["desde"];
        aromaDespertarseHasta = doc["aroma"]["despertar"]["hasta"];

        isAromaActiveDespertarse = doc["aroma"]["despertar"]["active"];
        isAromaActiveAcostarse = doc["aroma"]["active"];
        //----------------- SONIDO -----------------//
        sonidoAcostarseDesde = doc["sonidoAmbiental"]["desde"];
        sonidoAcostarseHasta = doc["sonidoAmbiental"]["hasta"];
    }
}

void initializeFirebase()
{
    Firebase.printf("Firebase Client v%s\n", FIREBASE_CLIENT_VERSION);
}

void configureSSL()
{
    ssl.setInsecure();
#if defined(ESP8266)
    ssl.setBufferSizes(1024, 1024);
#endif
}

void setupFirebaseDatabase()
{
    initializeApp(client, app, getAuth(dbSecret));
    app.getApp<RealtimeDatabase>(Database);
    Database.url(DATABASE_URL);
    client.setAsyncResult(result);
}

void printError(int code, const String &msg)
{
    Firebase.printf("Error, msg: %s, code: %d\n", msg.c_str(), code);
}

void controlSound(bool state)
{
    for (int i = 0; i < 20; i++)
    {
        Serial.println(state ? sonidoAcostarse : 6);
        delay(100);
    }
}

void controlAlarma(bool state)
{
    for (int i = 0; i < 20; i++)
    {
        Serial.println(state ? sonidoAlarma : 6);
        delay(100);
    }
}

void controlLed(bool state)
{
    uint32_t color = state ? pixels.Color(colorAcostarse.r * colorAcostarse.alpha, colorAcostarse.g * colorAcostarse.alpha, colorAcostarse.b * colorAcostarse.alpha) : 0;
    for (int i = 0; i < NUMPIXELS; i++)
    {
        pixels.setPixelColor(i, color);
        pixels2.setPixelColor(i, color);
    }
    pixels.show();
    pixels2.show();
}
void controlLedDespertarse(bool state)
{
    uint32_t color = state ? pixels.Color(colorDespertar.r * colorDespertar.alpha, colorDespertar.g * colorDespertar.alpha, colorDespertar.b * colorDespertar.alpha) : 0;
    for (int i = 0; i < NUMPIXELS; i++)
    {
        pixels.setPixelColor(i, color);
        pixels2.setPixelColor(i, color);
    }
    pixels.show();
    pixels2.show();
}

void controlAroma(bool state)
{
    if (state)
    {
        digitalWrite(aromaPin, HIGH);
        delay(100);
        digitalWrite(aromaPin, LOW);
    }
    else
    {
        digitalWrite(aromaPin, HIGH);
        delay(100);
        digitalWrite(aromaPin, LOW);
        delay(100);
        digitalWrite(aromaPin, HIGH);
        delay(100);
        digitalWrite(aromaPin, LOW);
    }
}

void turnOffAll()
{
    if (ledState)
        controlLed(!ledState);

    if (soundPlayState)
        controlSound(!soundPlayState);

    if (seApagoLaAlarma)
        seApagoLaAlarma = false;
}

void controlOnOff(bool state)
{
    if (!state)
    {
        turnOffAll();
    }
    else
    {
        horaAcostarse = 25;
    }
}

bool handleButton(int pin, bool &lastState, void (*controlFunction)(bool), bool &deviceState)
{
    bool currentState = digitalRead(pin) == HIGH;
    if (currentState != lastState)
    {
        lastState = currentState; // Actualizar el último estado
        if (currentState == HIGH)
        {
            deviceState = !deviceState;
            controlFunction(deviceState); // Cambiar el estado
            return true;                  // Indicar que el estado ha cambiado
        }
    }
    return false; // No hubo cambio
}

void handleSensor()
{
    if (onOffState)
    {
        digitalWrite(sensorTrigPin, LOW);
        delayMicroseconds(2);
        digitalWrite(sensorTrigPin, HIGH);
        delayMicroseconds(10);
        digitalWrite(sensorTrigPin, LOW);

        sensorDuration = pulseIn(sensorEchoPin, HIGH);
        sensorDistance = (sensorDuration * .0343) / 2;
        if (sensorDistance > 100 && sensorDistance < 800 && !seApagoLaAlarma && estaSonando)
        {
            Serial.println("6");
            soundPlayState = false;
            seApagoLaAlarma = true;
            estaSonando = false;
        }
    }
}

void handleSoundVolume()
{
    int volume = analogRead(soundVolumePin);
    /* promediar cada 10 voumenes y ahi pasar a newvolume*/
    static int volumeBuffer[10] = {0};
    static int bufferIndex = 0;
    static int bufferSum = 0;

    // Add the new volume to the buffer
    bufferSum -= volumeBuffer[bufferIndex];
    volumeBuffer[bufferIndex] = volume;
    bufferSum += volumeBuffer[bufferIndex];

    // Increment the buffer index
    bufferIndex = (bufferIndex + 1) % 10;

    // Calculate the average volume
    int newVolume = bufferSum / 10;
    newVolume = map(newVolume, 0, 4095, 0, 10);

    if (soundPlayState)
    {
        if (newVolume > lastSoundVolume)
        {
            Serial.println("9");
            lastSoundVolume = newVolume;
        }
        else if (newVolume < lastSoundVolume)
        {
            Serial.println("0");
            lastSoundVolume = newVolume;
        }
    }
}

void setLedColor(Color color)
{
    uint32_t colorValue = pixels.Color(color.r, color.g, color.b);
    for (int i = 0; i < NUMPIXELS; i++)
    {
        pixels.setPixelColor(i, colorValue);
        pixels2.setPixelColor(i, colorValue);
    }
    pixels.show();
    pixels2.show();
}

void controlTimeEvent(int startHour, int startMinute, int minsFrom, int minsTo, bool &flag, bool active, void (*eventHandler)(bool))
{
    if (!active)
        return;

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        return;
    }

    // Calcular la hora y minuto de inicio del evento
    int eventStartHour = startHour;
    int eventStartMinute = startMinute + minsFrom;

    if (eventStartMinute >= 60)
    {
        eventStartHour += eventStartMinute / 60;
        eventStartMinute %= 60;
    }
    else if (eventStartMinute < 0)
    {
        eventStartHour--;
        eventStartMinute += 60;
    }

    // Normalizar la hora de inicio
    if (eventStartHour >= 24)
        eventStartHour %= 24;
    if (eventStartHour < 0)
        eventStartHour += 24;

    // Calcular la hora y minuto de fin del evento
    int eventEndHour = startHour;
    int eventEndMinute = startMinute + minsTo;

    if (eventEndMinute >= 60)
    {
        eventEndHour += eventEndMinute / 60;
        eventEndMinute %= 60;
    }
    else if (eventEndMinute < 0)
    {
        eventEndHour--;
        eventEndMinute += 60;
    }

    // Normalizar la hora de fin
    if (eventEndHour >= 24)
        eventEndHour %= 24;
    if (eventEndHour < 0)
        eventEndHour += 24;

    // Comprobar si la hora actual es la hora de inicio para encender el evento
    if (timeinfo.tm_hour == eventStartHour && timeinfo.tm_min == eventStartMinute && !flag)
    {
        eventHandler(true);
        flag = true;
    }

    // Comprobar si la hora actual es la hora de fin para apagar el evento
    if (timeinfo.tm_hour == eventEndHour && timeinfo.tm_min == eventEndMinute && flag)
    {
        eventHandler(false);
        flag = false;
    }
}

void setup()
{
    // ---- AROMA ------ //
    pinMode(aromaPin, OUTPUT);
    pinMode(aromaBtnPin, INPUT);
    controlAroma(false);

    // ---- END AROMA ------ //

    // ---- LEDS ------ //
    pinMode(ledBtnPin, INPUT);
    // ---- END LEDS ------ //

    // ---- SENSOR ------ //
    pinMode(sensorTrigPin, OUTPUT);
    pinMode(sensorEchoPin, INPUT);
    // ---- END SENSOR ------ //

    Serial.begin(115200);
    connectToWiFi();
    initializeFirebase();
    configureSSL();
    setupFirebaseDatabase();

    /* TIME */
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    pixels.begin();
    pixels2.begin();
    pixels.setBrightness(50);
    pixels2.setBrightness(50);
    pixels.show();
    pixels2.show();
}

void checkAlarm()
{
    if (onOffState && isAlarmaActive)
    {
        struct tm timeinfo;
        if (!getLocalTime(&timeinfo))
        {
            return;
        }

        if (!seApagoLaAlarma && timeinfo.tm_hour == horaDespertarse && timeinfo.tm_min == minutoDespertarse && !estaSonando)
        {
            Serial.println(sonidoAlarma);
            estaSonando = true;
            handleSensor();
        }
        else if (estaSonando)
        {
            handleSensor();
        }
    }
}

void loop()
{
    if (onOffState)
    {
        // Obtener configuración de la base de datos
        getDbConfig();

        // ------- TIME ------- //
        /* LUZ */
        controlTimeEvent(horaAcostarse, minutoAcostarse, luzAmbientalAcostarseDesde * -1, luzAmbientalAcostarseHasta, luzAmbientalFlag, isLuzAmbientalActive, controlLed);
        controlTimeEvent(horaDespertarse, minutoDespertarse, luzAmbientalDespertarseDesde * -1, luzAmbientalDespertarseHasta, luzAmbientalFlag, isLuzAmbientalActiveDespertarse, controlLedDespertarse);
        /* Aroma */
        if (!aromaState)
        {
            controlTimeEvent(horaAcostarse, minutoAcostarse, aromaAcostarseDesde * -1, aromaAcostarseHasta, aromaFlag, isAromaActiveAcostarse, controlAroma);
            controlTimeEvent(horaDespertarse, minutoDespertarse, aromaDespertarseDesde * -1, aromaDespertarseHasta, aromaFlag, isAromaActiveDespertarse, controlAroma);
        }
        /* Sonido */
        controlTimeEvent(horaAcostarse, minutoAcostarse, sonidoAcostarseDesde * -1, sonidoAcostarseHasta, sonidoFlag, isSonidoActive, controlSound);
        checkAlarm();
        //---- END TIME -------//

        // Manejar botón para aroma
        handleButton(aromaBtnPin, lastAromaBtnState, controlAroma, aromaState);

        // Manejar botón para LEDs
        handleButton(ledBtnPin, lastLedBtnState, controlLed, ledState);

        // Manejar play/pause de sonido
        handleButton(soundPlayPouseBtnPin, lastSoundPlayState, controlSound, soundPlayState);

        // Manejar volumen de sonido
        handleSoundVolume();
    }
    // Manejar botón para ON/OFF
    handleButton(onOffBtnPin, lastOnOffBtnState, controlOnOff, onOffState);
    delay(50);
}