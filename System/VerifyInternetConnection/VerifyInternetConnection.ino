#include <WiFi.h>
#include <HTTPClient.h>

// Remplacez par le nom et le mot de passe de votre réseau
const char* ssid     = "your SSID";
const char* password = "your password";

// URL de test : retourne une 204 No Content si la connexion Internet fonctionne
const char* testUrl = "http://clients3.google.com/generate_204";

void setup() {
  Serial.begin(115200);
  delay(100);

  Serial.printf("Connexion au réseau %s\n", ssid);
  WiFi.begin(ssid, password);

  // Attente de la connexion Wi-Fi
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi‑Fi connecté !");
  Serial.print("IP locale : ");
  Serial.println(WiFi.localIP());
}

// Renvoie true si la requête HTTP renvoie le code 204
bool internetOK() {
  if (WiFi.status() != WL_CONNECTED) {
    return false; 
  }

  HTTPClient http;
  http.begin(testUrl);
  int httpCode = http.GET();
  http.end();

  return (httpCode == 204);
}

void loop() {
  if (internetOK()) {
    Serial.println("✅ Internet opérationnel");
  } else {
    Serial.println("❌ Pas d'accès Internet !");
  }
  delay(10000); // vérification toutes les 10 secondes
}