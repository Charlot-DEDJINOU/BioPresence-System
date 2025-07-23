#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_Fingerprint.h>
#include <Keypad_I2C.h>

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
  Serial.begin(115200); // Ajout pour debug
  Wire.begin();
  lcd.init();
  lcd.backlight();

  // Initialisation du capteur d'empreintes
  fingerSerial.begin(57600, SERIAL_8N1, FINGERPRINT_RX, FINGERPRINT_TX);
  delay(1000); // Délai pour stabilisation
  finger.begin(57600);
  
  if (!finger.verifyPassword()) {
    lcd.setCursor(0,0);
    lcd.print("Erreur capteur");
    Serial.println("Erreur: Impossible de communiquer avec le capteur");
    while (true);
  }

  // Affiche le menu principal
  lcd.clear();
  lcd.print("A:Enregistrer");
  lcd.setCursor(0,1);
  lcd.print("B:Verifier");

  keypad.begin();
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
      verifierEmpreinte();
    }
    delay(2000);
    // Retour au menu
    lcd.clear();
    lcd.print("A:Enregistrer");
    lcd.setCursor(0,1);
    lcd.print("B:Verifier");
  }
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

// Enregistre deux doigts pour un même utilisateur
void enregistrerEmpreintes() {
  lcd.print("Nouvel utilisateur");
  delay(1000);

  // Premier doigt
  uint8_t id1 = getFreeID();
  if (id1 == 0) {
    lcd.clear(); 
    lcd.print("Memo pleine"); 
    Serial.println("Mémoire pleine");
    return;
  }
  
  Serial.print("Tentative enregistrement ID1: ");
  Serial.println(id1);
  
  if (!enrollFingerprint(id1)) {
    lcd.clear(); 
    lcd.print("Echec ID"); 
    lcd.print(id1); 
    Serial.print("Échec enregistrement ID1: ");
    Serial.println(id1);
    return;
  }
  lcd.clear();
  lcd.print("ID1="); lcd.print(id1); lcd.print(" OK");
  Serial.print("ID1 enregistré avec succès: ");
  Serial.println(id1);
  delay(1500);

  // Deuxième doigt
  uint8_t id2 = getFreeID();
  if (id2 == 0) {
    lcd.clear(); 
    lcd.print("Memo pleine"); 
    Serial.println("Mémoire pleine pour ID2");
    return;
  }
  
  Serial.print("Tentative enregistrement ID2: ");
  Serial.println(id2);
  
  if (!enrollFingerprint(id2)) {
    lcd.clear(); 
    lcd.print("Echec ID"); 
    lcd.print(id2); 
    Serial.print("Échec enregistrement ID2: ");
    Serial.println(id2);
    return;
  }
  lcd.clear();
  lcd.print("ID2="); lcd.print(id2); lcd.print(" OK");
  Serial.print("ID2 enregistré avec succès: ");
  Serial.println(id2);
  delay(1500);

  // Confirmation
  lcd.clear();
  lcd.print("Enreg OK:");
  lcd.setCursor(0,1);
  lcd.print(id1); lcd.print(","); lcd.print(id2);
}

// Procédure d'enregistrement d'une empreinte (version améliorée)
bool enrollFingerprint(uint8_t id) {
  int p = -1;
  int tentatives = 0;
  const int MAX_TENTATIVES = 50; // Environ 10 secondes d'attente
  
  // 1re capture
  lcd.clear();
  lcd.print("Enreg ID:");
  lcd.print(id);
  lcd.setCursor(0,1);
  lcd.print("Placer doigt");
  Serial.println("En attente du doigt...");
  
  while (p != FINGERPRINT_OK && tentatives < MAX_TENTATIVES) {
    p = finger.getImage();
    if (p == FINGERPRINT_OK) {
      Serial.println("Image capturée");
      break;
    } else if (p == FINGERPRINT_NOFINGER) {
      delay(200);
      tentatives++;
      continue;
    } else {
      Serial.print("Erreur getImage: ");
      Serial.println(p);
      return false;
    }
  }
  
  if (tentatives >= MAX_TENTATIVES) {
    Serial.println("Timeout - pas de doigt détecté");
    return false;
  }

  p = finger.image2Tz(1);
  if (p != FINGERPRINT_OK) {
    Serial.print("Erreur image2Tz(1): ");
    Serial.println(p);
    return false;
  }
  Serial.println("Template 1 créé");

  // Retirer puis reposer le doigt
  lcd.setCursor(0,1);
  lcd.print("Retirer...");
  Serial.println("Retirer le doigt");
  delay(2000);
  
  // Attendre que le doigt soit retiré
  while (finger.getImage() == FINGERPRINT_OK) {
    delay(100);
  }
  
  lcd.setCursor(0,1);
  lcd.print("Reposer...");
  Serial.println("Reposer le doigt");

  p = -1;
  tentatives = 0;
  while (p != FINGERPRINT_OK && tentatives < MAX_TENTATIVES) {
    p = finger.getImage();
    if (p == FINGERPRINT_OK) {
      Serial.println("2ème image capturée");
      break;
    } else if (p == FINGERPRINT_NOFINGER) {
      delay(200);
      tentatives++;
      continue;
    } else {
      Serial.print("Erreur 2ème getImage: ");
      Serial.println(p);
      return false;
    }
  }
  
  if (tentatives >= MAX_TENTATIVES) {
    Serial.println("Timeout - 2ème capture échouée");
    return false;
  }

  p = finger.image2Tz(2);
  if (p != FINGERPRINT_OK) {
    Serial.print("Erreur image2Tz(2): ");
    Serial.println(p);
    return false;
  }
  Serial.println("Template 2 créé");

  // Création du modèle
  p = finger.createModel();
  if (p != FINGERPRINT_OK) {
    Serial.print("Erreur createModel: ");
    Serial.println(p);
    return false;
  }
  Serial.println("Modèle créé");

  // Stockage du modèle
  p = finger.storeModel(id);
  if (p != FINGERPRINT_OK) {
    Serial.print("Erreur storeModel: ");
    Serial.println(p);
    return false;
  }
  Serial.println("Modèle stocké");
  
  return true;
}

// Vérifie une empreinte et affiche GRANTED ou DENIED
void verifierEmpreinte() {
  lcd.print("Verifier...");
  Serial.println("Vérification d'empreinte");
  
  int p = finger.getImage();
  if (p != FINGERPRINT_OK) {
    lcd.setCursor(0,1); 
    lcd.print("Pas de doigt");
    Serial.print("Erreur getImage: ");
    Serial.println(p);
    return;
  }
  
  p = finger.image2Tz(1);
  if (p != FINGERPRINT_OK) {
    lcd.setCursor(0,1); 
    lcd.print("Erreur TZ");
    Serial.print("Erreur image2Tz: ");
    Serial.println(p);
    return;
  }
  
  p = finger.fingerFastSearch();
  if (p == FINGERPRINT_OK) {
    lcd.setCursor(0,1);
    lcd.print("GRANTED ID:");
    lcd.print(finger.fingerID);
    Serial.print("Empreinte reconnue, ID: ");
    Serial.print(finger.fingerID);
    Serial.print(", Confiance: ");
    Serial.println(finger.confidence);
  } else {
    lcd.setCursor(0,1); 
    lcd.print("DENIED     ");
    Serial.print("Empreinte non reconnue, erreur: ");
    Serial.println(p);
  }
}