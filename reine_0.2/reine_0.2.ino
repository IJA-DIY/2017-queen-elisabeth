#include "Compiler_Errors.h"

// touch includes
#include <MPR121.h>
#include <Wire.h>
#define MPR121_ADDR 0x5C
#define MPR121_INT 4

// mp3 includes
#include <SPI.h>
#include <SdFat.h>
#include <SdFatUtil.h>
#include <SFEMP3Shield.h>

// mp3 variables
SFEMP3Shield MP3player;
byte test_MP3;

// sd card instantiation
SdFat sd;
SdFile file;

unsigned long delta_time; // Variable delta_time, de même type que la fonction millis()

int numElectrodes = 12; //nombre de electrodes
int numElectrodes_Active = 13;// 13 indique que aucune pin est toucher
char nomFichierSelec[50]; // Variable permettant d'avoir le nom du fichier en variable globale sinon cela ne fonctionnait pas, mais vu que maintenant on appele correctement les char, cette varible peut etre remplacer par une locale

/* Utilisation du capteur Ultrason HC-SR04 */
// définition des broches utilisées
int trig = 11;
int echo = 12;
long lecture_echo;
long cm;
long distance_act = 200; // Distance en cm pour activer les phrases

int ancienne_Question = 0; // Premet de ne pas poser deux fois la même questin d'affiler
int attente_time = 10000; // Attente en seconde avant de redire une phrase lors du mode dectetion
int attente_chg_Question = 5000;// Attente en seconde de maintient de lappuie sur une partie du corps pour activer le mode question
int attente_chg_Chapeau = 5000; // Idem pour pour alterner en chapeau/diademe
boolean diademe = true;// Indique si le chapeau ou le diademe est activé.

void setup() {
  Serial.begin(57600); // 57600 bits/s
  pinMode(LED_BUILTIN, OUTPUT); //Important
  randomSeed(analogRead(0)); // initialise le générateur de nombre aléatoire en lisant broche analogique
  Serial.println("Initialisation de l'interface Carte SD :");
  if (!sd.begin(SD_SEL, SPI_HALF_SPEED)) sd.initErrorHalt();
  else Serial.println("Initialisation OK");
  MP3player.begin(); //Init module mp3
  MP3player.setVolume(10, 10); //Init volume L-R, 0 = max, 255 = minimun
  if (!MPR121.begin(MPR121_ADDR)) Serial.println("error setting up MPR121");
  MPR121.setInterruptPin(MPR121_INT);
  // C'est le seuil tactile - le réglage bas rend plus comme un déclencheur de proximité
  // La valeur par défaut est de 40 pour le contact
  MPR121.setTouchThreshold(40);

  // C'est le seuil de sortie - TOUJOURS doit être plus petit que le seuil tactile
  // 40/2 = 20, seuil par défaut
  MPR121.setReleaseThreshold(20);

  // Init info sur les pins
  MPR121.updateTouchData();
  // On lance la fonction détection, c'est un doublon par rapport a la ligne du dessus
  detection_PinToucher();
  //On init les pins pour le capteur ultrason
  pinMode(trig, OUTPUT);
  digitalWrite(trig, LOW);
  pinMode(echo, INPUT);
  //Init delta_time pour calculer un delta de temps comme un chrono
  delta_time = millis();
}

void loop() {
  long dist = ultrason_Acquisition();
  if (dist <= distance_act ) { // Si c'est dans le périmetre
    if ((millis() - delta_time) >= 10000) { // Si le temps d'attente est ok
      lecture_Aleatoire("ultrason"); // On lit un fichier dans le dossier "ultrason
      delta_time = millis(); // On re-init delta_time
    }

  }
  if ( pin_Toucher() == 6) { // Si diademe toucher
    if (temps_Appuyer(6, attente_chg_Chapeau) == true) { // Si on touche le diademe pendant x seconde, on active le diademe pour les questions
      diademe = true;
      lecture_Aleatoire("diademe");
    }// On lit un fichier dans le dossier diademe
    else {
      lecture_Aleatoire("diademe");

    }
  }
  if ( pin_Toucher() == 7) { // Identique que au dessus mais avec le chapeau, diademe = false indique chapeau activé
    if (temps_Appuyer(7, attente_chg_Chapeau) == true) {
      diademe = false;
      lecture_Aleatoire("chapeau");
    }
    else {
      lecture_Aleatoire("chapeau");

    }
  }
  if ( pin_Toucher() == 1) { // Si bras droit toucher
    // Passage mode question
    if (temps_Appuyer(1, attente_chg_Question) == true) { // Si le bras droit toucher x seconde
      lecture_Fichier("arm.mp3", "bip"); // On indique de c'est validé, on attend 5 secondes le temps d'appuyer sur le bras G
      delay(5000);
      if (temps_Appuyer(0, attente_chg_Question) == true) { //Si le bras gauche toucher x seconde, on indique par 2 "bip" que le mode question est activer
        lecture_Fichier("arm.mp3", "bip");
        while (MP3player.isPlaying()); // Permet d'attendre que le son est terminé d'être lu
        lecture_Fichier("arm.mp3", "bip");
        while (MP3player.isPlaying());
        for (int i_alea_question = 0; i_alea_question < 5; i_alea_question++) { // On lance 5 questions sur les vetements puis on quitte le mode
          sequencage_question_Vetements();

        }
      }
    }
    else {
      lecture_Aleatoire("brasd");
    }

  }
  if ( pin_Toucher() == 0) { // Identique que le dessus mais au lieu du bras droit puis gauche, c'est gauche puis droite et on active le mode question Corps
    if (temps_Appuyer(0, attente_chg_Question ) == true) {
      lecture_Fichier("arm.mp3", "bip");
      delay(5000);
      if (temps_Appuyer(1, attente_chg_Question) == true) {
        lecture_Fichier("arm.mp3", "bip");
        while (MP3player.isPlaying());
        lecture_Fichier("arm.mp3", "bip");
        while (MP3player.isPlaying());
        for (int i_alea_question = 0; i_alea_question < 5; i_alea_question++) {
          sequencage_question_Corps();
        }
      }
    }
    else {
      lecture_Aleatoire("brasg");
    }
  }
  // Cela permet de jouer le son correspondant a l'emplacement toucher.
  if ( pin_Toucher() == 2) {
    lecture_Aleatoire("maing");
  }
  if ( pin_Toucher() == 3) {
    lecture_Aleatoire("maind");
  }
  if ( pin_Toucher() == 4) {
    lecture_Aleatoire("torse");
  }
  if ( pin_Toucher() == 5) {
    lecture_Aleatoire("epaules");
  }
  if ( pin_Toucher() == 8) {
    lecture_Aleatoire("piedd");
  }
  if ( pin_Toucher() == 9) {
    lecture_Aleatoire("piedg");
  }
  if ( pin_Toucher() == 10) {
    lecture_Aleatoire("cheveux");
  }
  if ( pin_Toucher() == 11) {
    lecture_Aleatoire("sac");
  }
}
void fonction_Question(int electrode, char dossierQuestion[], char dossierJuste[], char dossierFaux[]) {
  // Fonction qui lit une question suivant l'electrode, le dossier de la question, le dossier du juste et le dossier du faux
  lecture_Aleatoire(dossierQuestion); // Une lit une question aléatoire mais d'un emplacement définie
  while (MP3player.isPlaying());
  delta_time = millis(); // On init delta_time
  while (millis() - delta_time <= attente_time) { // On a x seconde pour répondre
    int electrodeTemp = pin_Toucher(); // On récupère la broche toucher
    if (electrodeTemp == electrode) { // Si c'est la bonne, on lit le dossier juste et on quitte la boucle
      while (MP3player.isPlaying());
      lecture_Aleatoire(dossierJuste);
      break;
    } // Si c'est faux, on lit dossier faux et on re-init delta_time
    else if (electrodeTemp != 13) {
      lecture_Aleatoire(dossierFaux);
      delta_time = millis();
    }
  }// Si le temps est dépasser on quitte la fonction. Mettre une info vocale comme quoi on quitte la fonction question aurait été juicieux
  while (MP3player.isPlaying());
}

void lecture_Aleatoire(char dossier[]) {// Fonction qui choisit un fichier aléatoire dans un dossier et le lit
  random_Fichier(dossier);
  lecture_Fichier(nomFichierSelec, dossier);

}

void random_Fichier(char repertoire[]) {
  /*Fonction du code d'exemple permettant de lire un fichier aléatoire dans un dossier.
    La fonction va parcourir le dossier pour compter le nombre de fichier puis initialiser un random pour choisir le x ème fichier dans la liste */


sd.chdir(); // set working directory back to root (in case we were anywhere else)
if (!sd.chdir(repertoire)) { // select our directory
  Serial.println("error selecting directory"); // error message if reqd.
}

size_t nomFichierLongueur;
char* matchPtr;
unsigned int nombrefichierMP3 = 0;

// On compte le nombre de fichier MP3 dans le dossier
while (file.openNext(sd.vwd(), O_READ)) {
  file.getFilename(nomFichierSelec);
  file.close();
  // sorry to do this all without the String object, but it was
  // causing strange lockup issues
  nomFichierLongueur = strlen(nomFichierSelec);
  matchPtr = strstr(nomFichierSelec, ".MP3");
  // basically, if the filename ends in .MP3, we increment our MP3 count
  if (matchPtr - nomFichierSelec == nomFichierLongueur - 4) nombrefichierMP3++;
}

// generate a random number, representing the file we will play
unsigned int fichierSelectionner = random(nombrefichierMP3);

// loop through files again - it's repetitive, but saves
// the RAM we would need to save all the filenames for subsequent access
unsigned int fileCtr = 0;

sd.chdir(); // set working directory back to root (to reset the file crawler below)
if (!sd.chdir(repertoire)) { // select our directory (again)
  Serial.println("error selecting directory"); // error message if reqd.
}

while (file.openNext(sd.vwd(), O_READ)) {
  file.getFilename(nomFichierSelec);
  file.close();

  nomFichierLongueur = strlen(nomFichierSelec);
  matchPtr = strstr(nomFichierSelec, ".MP3");
  // this time, if we find an MP3 file...
  if (matchPtr - nomFichierSelec == nomFichierLongueur - 4) {
    // ...we check if it's the one we want, and if so play it...
    if (fileCtr == fichierSelectionner) {
      // this only works because we're in the correct directory
      // (via sd.chdir() and only because sd is shared with the MP3 player)

      break;
    } else {
      // ...otherwise we increment our counter
      fileCtr++;
    }
  }
}
}

void lecture_Fichier(char nom_Fichier[], char repertoire_Fichier[]) {
  //On lit un fichier en particuler en indiquant son nom et son emplacement
  sd.chdir(); //Racine Carte SD
  if (!sd.chdir(repertoire_Fichier)) { // selection repertoire
    Serial.println("Erreur sélection du répertoire");
  }
  MP3player.playMP3(nom_Fichier); //Lecture
  while (MP3player.isPlaying()); // On attend la fin de la lecture
  MP3player.stopTrack(); // On stop le module MP3 pour eviter tout conflit
}


void detection_PinToucher() {
  // Detection de la pin toucher, c'est aussi une fonction d'exemple, qui vérifie les pins dans l'ordre.
  // Inconvéniant, si on en touche deux, il va prendre uniquement en compte la dernière dans la liste. Mettre un tableau a la place d'un int serait plus judicieux la aussi.
  if (MPR121.touchStatusChanged()) {
    MPR121.updateTouchData();
    for (int i = 0; i < numElectrodes; i++) {
      if (MPR121.isNewTouch(i)) {
        numElectrodes_Active = i;
      } else if (MPR121.isNewRelease(i)) {
        numElectrodes_Active = 13;
      }
    }
  }
}

int pin_Toucher() {
  // Fonction qui permet de rendre le code plus lissible mais doublon, si on transforme void detection_PinToucher() en int detection_PinToucher()
  detection_PinToucher();
  return numElectrodes_Active;
}

int random_new_Question(int nb) {
  // Fonction qui initialise la fonction random, et qui verifie que c'est une nouvelle question
  int new_Question = random(1, (nb + 1));// nb + 1 est important car un random(1,4); va rendre soit 1, 2 ou 3.
  if (new_Question == ancienne_Question) {
    while (!new_Question != ancienne_Question) {
      new_Question = random(1, (nb + 1));
    }
  }
  return new_Question;
}

void sequencage_question_Corps() {
  //Fonction question corps, on choisit une question avec le random, et on lance la fonction_Question()
  int rep = random_new_Question(9);
  switch (rep) {
    case 1:
      //Bras G
      fonction_Question(0, "c/brasg", "juste", "faux");
      break;
    case 2:
      //Bras D
      fonction_Question(1, "c/brasd", "juste", "faux");
      break;
    case 3:
      //Main G
      fonction_Question(2, "c/maing", "juste", "faux");
      break;
    case 4:
      //Main D
      fonction_Question(3, "c/maind", "juste", "faux");
      break;
    case 5:
      //Torse
      fonction_Question(4, "c/torse", "juste", "faux");
      break;
    case 6:
      //Pied G
      fonction_Question(9, "c/piedg", "juste", "faux");
      break;
    case 7:
      //Pied D
      fonction_Question(8, "c/piedd", "juste", "faux");
      break;
    case 8:
      //Epaules
      fonction_Question(5, "c/epaules", "juste", "faux");
      break;
    case 9:
      //Cheveux
      fonction_Question(10, "c/cheveux", "juste", "faux");
      break;

    default:
      break;
  }
}

void sequencage_question_Vetements() {
  //Idem mais avec les vetements
  switch (random_new_Question(6)) {
    case 1:
      //Manche D
      fonction_Question(1, "v/manched", "juste", "faux");
      break;
    case 2:
      //Manche G
      fonction_Question(0, "v/mancheg", "juste", "faux");
      break;
    case 3:
      //Gant D
      fonction_Question(3, "v/gantd", "juste", "faux");
      break;
    case 4:
      //Gant G
      fonction_Question(2, "v/gantg", "juste", "faux");
      break;
    case 5:
      //Chaussure G
      fonction_Question(9, "v/chaussureg", "juste", "faux");
      break;
    case 6:
      //Chaussure D
      fonction_Question(8, "v/chaussured", "juste", "faux");
      break;
    case 7:
      //Diademe/Chapeau
      if (diademe == true) {
        fonction_Question(7, "v/diademe", "juste", "faux");
      }
      else {
        fonction_Question(6, "v/chapeau", "juste", "faux");
      }
      break;
    case 8:
      //Sac
      fonction_Question(11, "v/sac", "juste", "faux");
      break;
    default:
      break;
  }
}

long ultrason_Acquisition() {
  // Fonction qui rend la distance entre le capteur et la personne devant elle
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);
  lecture_echo = pulseIn(echo, HIGH);
  cm = lecture_echo / 58;
  return cm;
}

boolean temps_Appuyer(int pin_verif, int delay_attente) {
  // Fonction essentiel qui permet de verifier que on appuie x seconde sans interuption sur le capteur en question
  boolean ok = true;
  delta_time = millis();
  while (ok == true) {
    if (pin_verif != pin_Toucher()) {
      ok = false;
    }
    else if ((millis() - delta_time) >= delay_attente) {
      break;
    }
  }
  return ok;
}



