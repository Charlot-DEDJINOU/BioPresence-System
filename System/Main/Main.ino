#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_Fingerprint.h>
#include <Keypad_I2C.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Configuration WiFi
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

// URL de base de l'API
const String API_BASE_URL = "YOUR_API_BASE_URL";

// Broches du capteur (Serial2)
#define FINGERPRINT_RX 17
#define FINGERPRINT_TX 16

// Adresses I2C
#define LCD_ADDR    0x25
#define KEYPAD_ADDR 0x20

// Capacité max du capteur
#define MAX_TEMPLATES 300

// Objet écran LCD 16×2
LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);
// Serial2 pour le lecteur
HardwareSerial fingerSerial(2);
Adafruit_Fingerprint finger(&fingerSerial);

const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

byte rowPins[ROWS] = {7, 6, 5, 4};
byte colPins[COLS] = {3, 2, 1, 0};
Keypad_I2C keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS, KEYPAD_ADDR);

void setup() {
  Serial.begin(115200);
  Wire.begin();
  lcd.init();
  lcd.backlight();

  // Connexion WiFi
  WiFi.begin(ssid, password);
  lcd.setCursor(0,0);
  lcd.print("Connexion WiFi..");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connexion WiFi...");
  }
  
  lcd.clear();
  lcd.print("WiFi connecte");
  Serial.println("WiFi connecté");
  Serial.print("Adresse IP: ");
  Serial.println(WiFi.localIP());
  delay(2000);

  // Initialisation du capteur d'empreintes
  fingerSerial.begin(57600, SERIAL_8N1, FINGERPRINT_RX, FINGERPRINT_TX);
  delay(1000);
  finger.begin(57600);
  
  if (!finger.verifyPassword()) {
    lcd.setCursor(0,0);
    lcd.print("Erreur capteur");
    Serial.println("Erreur: Impossible de communiquer avec le capteur");
    while (true);
  }

  keypad.begin();
  
  // Affiche le menu principal
  afficherMenu();
  Serial.println("Système prêt");
}

void loop() {
  char key = keypad.getKey();
  if (key) {
    lcd.clear();
    if (key == 'A') {
      enregistrerEmpreintes();
    } 
    else if (key == 'B') {
      verifierPresence();
    }
    delay(2000);
    afficherMenu();
  }
}

void afficherMenu() {
  lcd.clear();
  lcd.print("A:Enregistrer");
  lcd.setCursor(0,1);
  lcd.print("B:Marquer pres.");
}

// Fonction pour saisir un code numérique
String saisirCode() {
  String code = "";
  lcd.clear();
  lcd.print("Tapez code:");
  lcd.setCursor(0,1);
  
  while (true) {
    char key = keypad.getKey();
    if (key) {
      if (key >= '0' && key <= '9') {
        code += key;
        lcd.print("*");
        Serial.print("Code saisi: ");
        Serial.println(code);
      } else if (key == '#') {
        // Validation du code
        break;
      } else if (key == '*') {
        // Effacer le dernier caractère
        if (code.length() > 0) {
          code.remove(code.length() - 1);
          lcd.setCursor(0,1);
          lcd.print("                "); // Effacer la ligne
          lcd.setCursor(0,1);
          for (int i = 0; i < code.length(); i++) {
            lcd.print("*");
          }
        }
      }
    }
    delay(100);
  }
  
  return code;
}

// Fonction pour faire un appel HTTP GET
String httpGET(String url) {
  HTTPClient http;
  http.begin(url);
  http.addHeader("accept", "application/json");
  
  int httpResponseCode = http.GET();
  String response = "";
  
  if (httpResponseCode > 0) {
    response = http.getString();
    Serial.print("Réponse HTTP GET (");
    Serial.print(httpResponseCode);
    Serial.print("): ");
    Serial.println(response);
  } else {
    Serial.print("Erreur HTTP GET: ");
    Serial.println(httpResponseCode);
  }
  
  http.end();
  return response;
}

// Fonction pour faire un appel HTTP PUT
bool httpPUT(String url, String jsonData) {
  HTTPClient http;
  http.begin(url);
  http.addHeader("accept", "application/json");
  http.addHeader("Content-Type", "application/json");
  
  int httpResponseCode = http.PUT(jsonData);
  bool success = (httpResponseCode == 200);
  
  Serial.print("Réponse HTTP PUT (");
  Serial.print(httpResponseCode);
  Serial.print("): ");
  Serial.println(http.getString());
  
  http.end();
  return success;
}

// Fonction pour faire un appel HTTP POST
bool httpPOST(String url, String jsonData) {
  HTTPClient http;
  http.begin(url);
  http.addHeader("accept", "application/json");
  http.addHeader("Content-Type", "application/json");
  
  int httpResponseCode = http.POST(jsonData);
  bool success = (httpResponseCode == 200);
  
  Serial.print("Réponse HTTP POST (");
  Serial.print(httpResponseCode);
  Serial.print("): ");
  Serial.println(http.getString());
  
  http.end();
  return success;
}

// Renvoie le premier ID libre (1..MAX_TEMPLATES), ou 0 si plein
uint8_t getFreeID() {
  int p;
  for (uint8_t id = 1; id <= MAX_TEMPLATES; id++) {
    p = finger.loadModel(id);
    if (p == FINGERPRINT_OK) {
      continue; // emplacement occupé
    } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
      Serial.print("Erreur communication pour ID ");
      Serial.println(id);
      continue; // Considérer comme occupé en cas d'erreur
    } else {
      Serial.print("ID libre trouvé: ");
      Serial.println(id);
      return id; // libre
    }
  }
  return 0; // mémoire pleine
}

// Enregistre deux empreintes pour un utilisateur
void enregistrerEmpreintes() {
  // a. Demander le code à l'utilisateur
  String code = saisirCode();
  
  if (code.length() == 0) {
    lcd.clear();
    lcd.print("Code requis");
    return;
  }

  // b. Envoyer le code au serveur
  lcd.clear();
  lcd.print("Verification...");
  String url = API_BASE_URL + "/employes/by-code/" + code;
  String response = httpGET(url);
  
  if (response.length() == 0) {
    lcd.clear();
    lcd.print("Erreur serveur");
    return;
  }

  // Parser la réponse JSON
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, response);
  
  String user_id = doc["user_id"];
  String full_name = doc["full_name"];
  
  // c. Vérifier si user_id est null
  if (user_id == "null" || user_id.length() == 0) {
    lcd.clear();
    lcd.print("Code invalide");
    lcd.setCursor(0,1);
    lcd.print("Utilisat. incon.");
    return;
  }
  
  lcd.clear();
  lcd.print("Utilisateur:");
  lcd.setCursor(0,1);
  lcd.print(full_name);
  delay(2000);

  // d. Enregistrer les empreintes
  uint8_t id1 = getFreeID();
  if (id1 == 0) {
    lcd.clear(); 
    lcd.print("Memo pleine"); 
    return;
  }
  
  if (!enrollFingerprint(id1)) {
    lcd.clear(); 
    lcd.print("Echec ID1"); 
    return;
  }
  
  lcd.clear();
  lcd.print("ID1="); lcd.print(id1); lcd.print(" OK");
  delay(1500);

  uint8_t id2 = getFreeID();
  if (id2 == 0) {
    lcd.clear(); 
    lcd.print("Memo pleine"); 
    return;
  }
  
  if (!enrollFingerprint(id2)) {
    lcd.clear(); 
    lcd.print("Echec ID2"); 
    return;
  }
  
  lcd.clear();
  lcd.print("ID2="); lcd.print(id2); lcd.print(" OK");
  delay(1500);

  // e. Envoyer les IDs au serveur
  lcd.clear();
  lcd.print("Envoi serveur...");
  
  String putUrl = API_BASE_URL + "/employes/" + user_id + "/empreintes";
  String jsonData = "{\"empreinte_ids\":[" + String(id1) + "," + String(id2) + "]}";
  
  // f. Vérifier la réponse
  if (httpPUT(putUrl, jsonData)) {
    lcd.clear();
    lcd.print("Enregistrement");
    lcd.setCursor(0,1);
    lcd.print("reussi!");
  } else {
    lcd.clear();
    lcd.print("Erreur serveur");
    lcd.setCursor(0,1);
    lcd.print("Enreg echoue");
  }
}

// Procédure d'enregistrement d'une empreinte
bool enrollFingerprint(uint8_t id) {
  int p = -1;
  int tentatives = 0;
  const int MAX_TENTATIVES = 50;
  
  // 1re capture
  lcd.clear();
  lcd.print("Enreg ID:");
  lcd.print(id);
  lcd.setCursor(0,1);
  lcd.print("Placer doigt");
  
  while (p != FINGERPRINT_OK && tentatives < MAX_TENTATIVES) {
    p = finger.getImage();
    if (p == FINGERPRINT_OK) {
      break;
    } else if (p == FINGERPRINT_NOFINGER) {
      delay(200);
      tentatives++;
      continue;
    } else {
      return false;
    }
  }
  
  if (tentatives >= MAX_TENTATIVES) {
    return false;
  }

  p = finger.image2Tz(1);
  if (p != FINGERPRINT_OK) {
    return false;
  }

  // Retirer puis reposer le doigt
  lcd.setCursor(0,1);
  lcd.print("Retirer...");
  delay(2000);
  
  while (finger.getImage() == FINGERPRINT_OK) {
    delay(100);
  }
  
  lcd.setCursor(0,1);
  lcd.print("Reposer...");

  p = -1;
  tentatives = 0;
  while (p != FINGERPRINT_OK && tentatives < MAX_TENTATIVES) {
    p = finger.getImage();
    if (p == FINGERPRINT_OK) {
      break;
    } else if (p == FINGERPRINT_NOFINGER) {
      delay(200);
      tentatives++;
      continue;
    } else {
      return false;
    }
  }
  
  if (tentatives >= MAX_TENTATIVES) {
    return false;
  }

  p = finger.image2Tz(2);
  if (p != FINGERPRINT_OK) {
    return false;
  }

  p = finger.createModel();
  if (p != FINGERPRINT_OK) {
    return false;
  }

  p = finger.storeModel(id);
  if (p != FINGERPRINT_OK) {
    return false;
  }
  
  return true;
}

// Nouvelle fonction pour vérifier la présence
void verifierPresence() {
  // a. Demander le code à l'utilisateur
  String code = saisirCode();
  
  if (code.length() == 0) {
    lcd.clear();
    lcd.print("Code requis");
    return;
  }

  // b. Récupérer les IDs d'empreintes pour ce code
  lcd.clear();
  lcd.print("Verification...");
  String url = API_BASE_URL + "/employes/" + code + "/empreintes";
  String response = httpGET(url);
  
  if (response.length() == 0) {
    lcd.clear();
    lcd.print("Erreur serveur");
    return;
  }

  // Parser la réponse JSON
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, response);
  
  JsonArray empreinte_ids = doc["empreinte_ids"];
  
  // c. Vérifier si le tableau est vide
  if (empreinte_ids.size() == 0) {
    lcd.clear();
    lcd.print("Aucune empreinte");
    lcd.setCursor(0,1);
    lcd.print("Utiliser opt. A");
    return;
  }

  // d. Scanner l'empreinte
  lcd.clear();
  lcd.print("Placer doigt...");
  
  int p = finger.getImage();
  if (p != FINGERPRINT_OK) {
    lcd.clear();
    lcd.print("Pas de doigt");
    return;
  }
  
  p = finger.image2Tz(1);
  if (p != FINGERPRINT_OK) {
    lcd.clear();
    lcd.print("Erreur lecture");
    return;
  }
  
  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK) {
    lcd.clear();
    lcd.print("Empreinte");
    lcd.setCursor(0,1);
    lcd.print("non reconnue");
    return;
  }

  // Vérifier si l'ID trouvé est dans le tableau
  uint8_t detectedID = finger.fingerID;
  bool idTrouve = false;
  
  for (JsonVariant id : empreinte_ids) {
    if (id.as<int>() == detectedID) {
      idTrouve = true;
      break;
    }
  }

  // e. ID non autorisé
  if (!idTrouve) {
    lcd.clear();
    lcd.print("Verification");
    lcd.setCursor(0,1);
    lcd.print("echouee");
    return;
  }

  // f. ID autorisé - signaler la présence
  lcd.clear();
  lcd.print("Envoi presence..");
  
  String postUrl = API_BASE_URL + "/employes/presences/signal";
  String jsonData = "{\"employee_code\":\"" + code + "\"}";
  
  // g. Vérifier la réponse du serveur
  if (httpPOST(postUrl, jsonData)) {
    lcd.clear();
    lcd.print("Presence");
    lcd.setCursor(0,1);
    lcd.print("enregistree!");
  } else {
    lcd.clear();
    lcd.print("Echec presence");
  }
}