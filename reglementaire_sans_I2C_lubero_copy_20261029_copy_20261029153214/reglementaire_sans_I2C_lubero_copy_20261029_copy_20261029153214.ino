/*
====================================================================
          REGLEMENTAIRE SCOLAIRE AUTOMATIQUE
====================================================================

                    ARDUINO MEGA
              DS1302 + LCD 16x2 PARALLELE

====================================================================
CABLAGE
====================================================================

LED BLEUE       -> D2
LED JAUNE       -> D3
LED ROUGE       -> D4

BOUTON 1        -> D5
BOUTON 2        -> D6
BOUTON 3        -> D7

RELAIS          -> D9

DS1302 CE/RST   -> D10
DS1302 I/O      -> D11
DS1302 SCLK     -> D12

LCD RS          -> D22
LCD E           -> D23
LCD D4          -> D24
LCD D5          -> D25
LCD D6          -> D26
LCD D7          -> D27

====================================================================
*/

#include <LiquidCrystal.h>
#include <ThreeWire.h>
#include <RtcDS1302.h>


// ==================================================================
// LCD
// ==================================================================

LiquidCrystal lcd(
    22,
    23,
    24,
    25,
    26,
    27
);


// ==================================================================
// DS1302
// ThreeWire(IO, SCLK, CE)
// ==================================================================

ThreeWire myWire(
    11,     // IO
    12,     // SCLK
    10      // CE / RST
);

RtcDS1302<ThreeWire> rtc(myWire);


// ==================================================================
// BROCHES
// ==================================================================

const byte LED_BLEUE = 2;
const byte LED_JAUNE = 3;
const byte LED_ROUGE = 4;

const byte BTN1 = 5;
const byte BTN2 = 6;
const byte BTN3 = 7;

const byte RELAIS = 9;


// ==================================================================
// RELAIS
// ==================================================================

const bool RELAIS_ACTIF_HIGH = true;


// ==================================================================
// DUREES
// ==================================================================

const unsigned long DUREE_SONNERIE = 10000UL;

const unsigned long DUREE_TEST = 5000UL;

const unsigned long DUREE_URGENCE = 5000UL;

const unsigned long PAUSE_URGENCE = 5000UL;

// IMPORTANT : 3 secondes
const unsigned long DUREE_APPUI_REGLAGE = 3000UL;


// ==================================================================
// MODES
// ==================================================================

enum ModeSysteme
{
    MODE_SCOLAIRE,
    MODE_PRIERE
};

ModeSysteme modeSysteme = MODE_SCOLAIRE;


// ==================================================================
// SONNERIE AUTOMATIQUE
// ==================================================================

bool sonnerieActive = false;

unsigned long debutSonnerie = 0;


// ==================================================================
// TEST
// ==================================================================

bool testActif = false;

unsigned long debutTest = 0;


// ==================================================================
// URGENCE
// ==================================================================

bool urgenceActive = false;

byte etapeUrgence = 0;

unsigned long debutUrgence = 0;


// ==================================================================
// ANTI DOUBLE SONNERIE
// ==================================================================

int derniereAnneeSonnee = -1;
int dernierMoisSonnee = -1;
int dernierJourSonnee = -1;
int derniereHeureSonnee = -1;
int derniereMinuteSonnee = -1;


// ==================================================================
// REGLAGE RTC
// ==================================================================

bool modeReglage = false;

byte champReglage = 0;


// ==================================================================
// VALEURS TEMPORAIRES
// ==================================================================

int regHeure = 0;
int regMinute = 0;
int regSeconde = 0;

int regJour = 1;
int regMois = 1;
int regAnnee = 2026;


// ==================================================================
// STRUCTURE BOUTON
// ==================================================================

struct EtatBouton
{
    byte pin;
    bool dernierEtat;
    bool etatStable;
    unsigned long dernierChangement;
};


EtatBouton bouton1 =
{
    BTN1,
    LOW,
    LOW,
    0
};


EtatBouton bouton2 =
{
    BTN2,
    LOW,
    LOW,
    0
};


EtatBouton bouton3 =
{
    BTN3,
    LOW,
    LOW,
    0
};


// ==================================================================
// COMBINAISON B1 + B3
// ==================================================================

unsigned long debutAppuiReglage = 0;

bool combinaisonReglageDetectee = false;

bool attendreRelacheReglage = false;


// ==================================================================
// RELAIS - SIRENE ON
// ==================================================================

void relaisON()
{
    if (RELAIS_ACTIF_HIGH)
    {
        digitalWrite(RELAIS, LOW);
    }
    else
    {
        digitalWrite(RELAIS, HIGH);
    }
}


// ==================================================================
// RELAIS - SIRENE OFF
// ==================================================================

void relaisOFF()
{
    if (RELAIS_ACTIF_HIGH)
    {
        digitalWrite(RELAIS, HIGH);
    }
    else
    {
        digitalWrite(RELAIS, LOW);
    }
}


// ==================================================================
// LCD
// ==================================================================

void effacerLigne(byte ligne)
{
    lcd.setCursor(0, ligne);
    lcd.print("                ");
}


void afficherMessage(
    const char* ligne1,
    const char* ligne2
)
{
    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print(ligne1);

    lcd.setCursor(0, 1);
    lcd.print(ligne2);
}


// ==================================================================
// NOM JOUR
// ==================================================================

const char* nomJour(byte jour)
{
    switch (jour)
    {
        case 0:
            return "Dim";

        case 1:
            return "Lun";

        case 2:
            return "Mar";

        case 3:
            return "Mer";

        case 4:
            return "Jeu";

        case 5:
            return "Ven";

        case 6:
            return "Sam";
    }

    return "---";
}


// ==================================================================
// AFFICHAGE HEURE
// ==================================================================

void afficherHeure(const RtcDateTime& now)
{
    char ligne1[17];

    snprintf(
        ligne1,
        sizeof(ligne1),
        "%s %02u/%02u/%02u",
        nomJour(now.DayOfWeek()),
        now.Day(),
        now.Month(),
        now.Year() % 100
    );

    char ligne2[17];

    snprintf(
        ligne2,
        sizeof(ligne2),
        "%02u:%02u:%02u",
        now.Hour(),
        now.Minute(),
        now.Second()
    );

    lcd.setCursor(0, 0);
    lcd.print("                ");

    lcd.setCursor(0, 0);
    lcd.print(ligne1);

    lcd.setCursor(0, 1);
    lcd.print("                ");

    lcd.setCursor(0, 1);
    lcd.print(ligne2);

    lcd.setCursor(9, 1);

    if (modeSysteme == MODE_SCOLAIRE)
    {
        lcd.print("ECOLE");
    }
    else
    {
        lcd.print("PRIERE");
    }
}


// ==================================================================
// LEDS NORMAL
// ==================================================================

void ledsNormal()
{
    if (modeSysteme == MODE_SCOLAIRE)
    {
        digitalWrite(LED_BLEUE, HIGH);
        digitalWrite(LED_JAUNE, LOW);
    }
    else
    {
        digitalWrite(LED_BLEUE, LOW);
        digitalWrite(LED_JAUNE, HIGH);
    }

    digitalWrite(LED_ROUGE, LOW);
}


// ==================================================================
// DEMARRER SONNERIE
// ==================================================================

void demarrerSonnerie()
{
    if (sonnerieActive)
        return;

    sonnerieActive = true;

    debutSonnerie = millis();

    relaisON();

    digitalWrite(LED_JAUNE, HIGH);
    digitalWrite(LED_ROUGE, HIGH);

    afficherMessage(
        "  SONNERIE !!!",
        "   EN COURS"
    );

    Serial.println("SONNERIE AUTOMATIQUE");
}


// ==================================================================
// ARRETER SONNERIE
// ==================================================================

void arreterSonnerie()
{
    sonnerieActive = false;

    relaisOFF();

    digitalWrite(LED_JAUNE, LOW);
    digitalWrite(LED_ROUGE, LOW);

    ledsNormal();
}


// ==================================================================
// GERER SONNERIE
// ==================================================================

void gererSonnerie()
{
    if (!sonnerieActive)
        return;

    if (
        millis() - debutSonnerie >=
        DUREE_SONNERIE
    )
    {
        arreterSonnerie();
    }
}


// ==================================================================
// TEST
// ==================================================================

void demarrerTest()
{
    if (modeReglage)
        return;

    if (urgenceActive)
        return;

    if (sonnerieActive)
        return;

    testActif = true;

    debutTest = millis();

    relaisON();

    digitalWrite(LED_JAUNE, HIGH);
    digitalWrite(LED_ROUGE, HIGH);

    afficherMessage(
        " TEST SONNERIE",
        "    5 SEC"
    );

    Serial.println("TEST SONNERIE");
}


void arreterTest()
{
    testActif = false;

    relaisOFF();

    digitalWrite(LED_JAUNE, LOW);
    digitalWrite(LED_ROUGE, LOW);

    ledsNormal();
}


void gererTest()
{
    if (!testActif)
        return;

    if (
        millis() - debutTest >=
        DUREE_TEST
    )
    {
        arreterTest();
    }
}


// ==================================================================
// URGENCE
// ==================================================================

void demarrerUrgence()
{
    if (modeReglage)
        return;

    if (urgenceActive)
        return;

    if (sonnerieActive)
    {
        arreterSonnerie();
    }

    if (testActif)
    {
        arreterTest();
    }

    urgenceActive = true;

    etapeUrgence = 1;

    debutUrgence = millis();

    relaisON();

    digitalWrite(LED_BLEUE, LOW);
    digitalWrite(LED_JAUNE, HIGH);
    digitalWrite(LED_ROUGE, HIGH);

    afficherMessage(
        "!!! URGENCE !!!",
        "  SONNERIE"
    );

    Serial.println("URGENCE ACTIVEE");
}


void arreterUrgence()
{
    urgenceActive = false;

    etapeUrgence = 0;

    relaisOFF();

    digitalWrite(LED_JAUNE, LOW);
    digitalWrite(LED_ROUGE, LOW);

    ledsNormal();
}


void gererUrgence()
{
    if (!urgenceActive)
        return;

    if (etapeUrgence == 1)
    {
        if (
            millis() - debutUrgence >=
            DUREE_URGENCE
        )
        {
            relaisOFF();

            digitalWrite(
                LED_JAUNE,
                LOW
            );

            etapeUrgence = 2;

            debutUrgence = millis();
        }
    }

    else if (etapeUrgence == 2)
    {
        if (
            millis() - debutUrgence >=
            PAUSE_URGENCE
        )
        {
            relaisON();

            digitalWrite(
                LED_JAUNE,
                HIGH
            );

            etapeUrgence = 3;

            debutUrgence = millis();
        }
    }

    else if (etapeUrgence == 3)
    {
        if (
            millis() - debutUrgence >=
            DUREE_URGENCE
        )
        {
            arreterUrgence();
        }
    }
}


// ==================================================================
// CORRESPONDANCE HEURE
// ==================================================================

bool heureCorrespond(
    const RtcDateTime& now,
    byte heure,
    byte minute
)
{
    return
        now.Hour() == heure &&
        now.Minute() == minute &&
        now.Second() <= 2;
}


// ==================================================================
// ANTI DOUBLE
// ==================================================================

bool dejaSonnee(
    const RtcDateTime& now
)
{
    return
        derniereAnneeSonnee == now.Year() &&
        dernierMoisSonnee == now.Month() &&
        dernierJourSonnee == now.Day() &&
        derniereHeureSonnee == now.Hour() &&
        derniereMinuteSonnee == now.Minute();
}


void enregistrerSonnerie(
    const RtcDateTime& now
)
{
    derniereAnneeSonnee = now.Year();
    dernierMoisSonnee = now.Month();
    dernierJourSonnee = now.Day();
    derniereHeureSonnee = now.Hour();
    derniereMinuteSonnee = now.Minute();
}


// ==================================================================
// SONNERIES SCOLAIRES
// ==================================================================

void verifierSonneriesScolaires(
    const RtcDateTime& now
)
{
    byte jour = now.DayOfWeek();

    // SAMEDI
    if (jour == 6)
        return;

    // DIMANCHE
    if (jour == 0)
    {
        if (
            heureCorrespond(now, 7, 30) ||
            heureCorrespond(now, 7, 45) ||
            heureCorrespond(now, 8, 30) ||
            heureCorrespond(now, 9, 15) ||
            heureCorrespond(now, 10, 0) ||
            heureCorrespond(now, 10, 45) ||
            heureCorrespond(now, 11, 5) ||
            heureCorrespond(now, 11, 50) ||
            heureCorrespond(now, 12, 30) ||
            heureCorrespond(now, 13, 15)
        )
        {
            if (!dejaSonnee(now))
            {
                enregistrerSonnerie(now);
                demarrerSonnerie();
            }
        }

        return;
    }

    // MERCREDI + VENDREDI
    if (
        jour == 3 ||
        jour == 5
    )
    {
        if (
            heureCorrespond(now, 7, 30) ||
            heureCorrespond(now, 7, 45) ||
            heureCorrespond(now, 8, 35) ||
            heureCorrespond(now, 9, 25) ||
            heureCorrespond(now, 10, 15) ||
            heureCorrespond(now, 11, 5) ||
            heureCorrespond(now, 11, 20) ||
            heureCorrespond(now, 12, 10) ||
            heureCorrespond(now, 13, 0) ||
            heureCorrespond(now, 13, 50)
        )
        {
            if (!dejaSonnee(now))
            {
                enregistrerSonnerie(now);
                demarrerSonnerie();
            }
        }

        return;
    }

    // JEUDI
    if (jour == 4)
    {
        if (
            heureCorrespond(now, 7, 30) ||
            heureCorrespond(now, 7, 45) ||
            heureCorrespond(now, 8, 30) ||
            heureCorrespond(now, 9, 15) ||
            heureCorrespond(now, 10, 0) ||
            heureCorrespond(now, 10, 45) ||
            heureCorrespond(now, 11, 5) ||
            heureCorrespond(now, 11, 50) ||
            heureCorrespond(now, 12, 30) ||
            heureCorrespond(now, 13, 15) ||
            heureCorrespond(now, 14, 15)
        )
        {
            if (!dejaSonnee(now))
            {
                enregistrerSonnerie(now);
                demarrerSonnerie();
            }
        }

        return;
    }

    // LUNDI + MARDI
    if (
        jour == 1 ||
        jour == 2
    )
    {
        if (
            heureCorrespond(now, 7, 30) ||
            heureCorrespond(now, 7, 45) ||
            heureCorrespond(now, 8, 35) ||
            heureCorrespond(now, 9, 25) ||
            heureCorrespond(now, 10, 15) ||
            heureCorrespond(now, 11, 5) ||
            heureCorrespond(now, 11, 20) ||
            heureCorrespond(now, 12, 10) ||
            heureCorrespond(now, 13, 0) ||
            heureCorrespond(now, 13, 50) ||
            heureCorrespond(now, 14, 30)
        )
        {
            if (!dejaSonnee(now))
            {
                enregistrerSonnerie(now);
                demarrerSonnerie();
            }
        }

        return;
    }
}


// ==================================================================
// MODE PRIERE
// ==================================================================

void verifierSonneriesPriiere(
    const RtcDateTime& now
)
{
    if (now.DayOfWeek() == 6)
        return;

    if (
        heureCorrespond(now, 7, 30) ||
        heureCorrespond(now, 7, 45) ||
        heureCorrespond(now, 8, 30) ||
        heureCorrespond(now, 9, 15) ||
        heureCorrespond(now, 10, 0) ||
        heureCorrespond(now, 10, 45) ||
        heureCorrespond(now, 11, 35) ||
        heureCorrespond(now, 11, 50) ||
        heureCorrespond(now, 12, 35) ||
        heureCorrespond(now, 13, 20) ||
        heureCorrespond(now, 14, 5)
    )
    {
        if (!dejaSonnee(now))
        {
            enregistrerSonnerie(now);
            demarrerSonnerie();
        }
    }
}


// ==================================================================
// CHANGEMENT MODE
// ==================================================================

void changerMode()
{
    if (
        modeSysteme ==
        MODE_SCOLAIRE
    )
    {
        modeSysteme = MODE_PRIERE;

        digitalWrite(
            LED_BLEUE,
            LOW
        );

        digitalWrite(
            LED_JAUNE,
            HIGH
        );

        afficherMessage(
            "MODE PRIERE",
            "    ACTIVE"
        );

        Serial.println(
            "MODE PRIERE"
        );
    }
    else
    {
        modeSysteme = MODE_SCOLAIRE;

        digitalWrite(
            LED_JAUNE,
            LOW
        );

        digitalWrite(
            LED_BLEUE,
            HIGH
        );

        afficherMessage(
            "MODE SCOLAIRE",
            "    ACTIVE"
        );

        Serial.println(
            "MODE SCOLAIRE"
        );
    }

    delay(1000);

    lcd.clear();
}


// ==================================================================
// BOUTON ANTI-REBOND
// ==================================================================

bool boutonAppuye(
    EtatBouton& bouton
)
{
    bool lecture =
        digitalRead(bouton.pin);

    if (
        lecture !=
        bouton.dernierEtat
    )
    {
        bouton.dernierChangement =
            millis();

        bouton.dernierEtat =
            lecture;
    }

    if (
        millis() -
        bouton.dernierChangement >= 50
    )
    {
        if (
            lecture !=
            bouton.etatStable
        )
        {
            bouton.etatStable =
                lecture;

            if (
                bouton.etatStable ==
                HIGH
            )
            {
                return true;
            }
        }
    }

    return false;
}


// ==================================================================
// PREPARER REGLAGE
// ==================================================================

void preparerReglage()
{
    RtcDateTime now =
        rtc.GetDateTime();

    regHeure = now.Hour();
    regMinute = now.Minute();
    regSeconde = now.Second();

    regJour = now.Day();
    regMois = now.Month();
    regAnnee = now.Year();

    // Sécurité si RTC renvoie une date invalide
    if (
        regAnnee < 2020 ||
        regAnnee > 2099 ||
        regMois < 1 ||
        regMois > 12 ||
        regJour < 1 ||
        regJour > 31
    )
    {
        regHeure = 0;
        regMinute = 0;
        regSeconde = 0;

        regJour = 1;
        regMois = 1;
        regAnnee = 2026;
    }

    champReglage = 0;

    modeReglage = true;

    relaisOFF();

    sonnerieActive = false;
    testActif = false;
    urgenceActive = false;
    etapeUrgence = 0;

    digitalWrite(
        LED_BLEUE,
        LOW
    );

    digitalWrite(
        LED_JAUNE,
        HIGH
    );

    digitalWrite(
        LED_ROUGE,
        LOW
    );

    lcd.clear();

    Serial.println();
    Serial.println(
        "============================"
    );
    Serial.println(
        "MODE REGLAGE HORLOGE"
    );
    Serial.println(
        "============================"
    );

    Serial.println(
        "B1 = champ suivant"
    );

    Serial.println(
        "B2 = augmenter"
    );

    Serial.println(
        "B3 = diminuer"
    );

    Serial.println(
        "B1 sur SAUVEGARDER = OK"
    );

    attendreRelacheReglage = true;
}


// ==================================================================
// ANNEE BISSEXTILE
// ==================================================================

bool estBissextile(
    int annee
)
{
    return
        (annee % 400 == 0) ||
        (
            annee % 4 == 0 &&
            annee % 100 != 0
        );
}


// ==================================================================
// JOURS MOIS
// ==================================================================

int joursDansMois(
    int mois,
    int annee
)
{
    switch (mois)
    {
        case 1:
        case 3:
        case 5:
        case 7:
        case 8:
        case 10:
        case 12:
            return 31;

        case 4:
        case 6:
        case 9:
        case 11:
            return 30;

        case 2:
            return estBissextile(annee)
                ? 29
                : 28;
    }

    return 31;
}


// ==================================================================
// CORRIGER JOUR
// ==================================================================

void corrigerJour()
{
    int maximum =
        joursDansMois(
            regMois,
            regAnnee
        );

    if (regJour > maximum)
        regJour = maximum;

    if (regJour < 1)
        regJour = 1;
}


// ==================================================================
// AFFICHER CHAMP REGLAGE
// ==================================================================

void afficherChampReglage()
{
    lcd.clear();

    switch (champReglage)
    {
        case 0:

            lcd.setCursor(0, 0);
            lcd.print("HEURE");

            lcd.setCursor(0, 1);

            if (regHeure < 10)
                lcd.print("0");

            lcd.print(regHeure);
            lcd.print(":");

            if (regMinute < 10)
                lcd.print("0");

            lcd.print(regMinute);
            lcd.print(":");

            if (regSeconde < 10)
                lcd.print("0");

            lcd.print(regSeconde);

            break;


        case 1:

            lcd.setCursor(0, 0);
            lcd.print("MINUTE");

            lcd.setCursor(0, 1);

            if (regMinute < 10)
                lcd.print("0");

            lcd.print(regMinute);

            break;


        case 2:

            lcd.setCursor(0, 0);
            lcd.print("SECONDE");

            lcd.setCursor(0, 1);

            if (regSeconde < 10)
                lcd.print("0");

            lcd.print(regSeconde);

            break;


        case 3:

            lcd.setCursor(0, 0);
            lcd.print("JOUR");

            lcd.setCursor(0, 1);

            if (regJour < 10)
                lcd.print("0");

            lcd.print(regJour);

            break;


        case 4:

            lcd.setCursor(0, 0);
            lcd.print("MOIS");

            lcd.setCursor(0, 1);

            if (regMois < 10)
                lcd.print("0");

            lcd.print(regMois);

            break;


        case 5:

            lcd.setCursor(0, 0);
            lcd.print("ANNEE");

            lcd.setCursor(0, 1);
            lcd.print(regAnnee);

            break;


        case 6:

            lcd.setCursor(0, 0);
            lcd.print("SAUVEGARDER ?");

            lcd.setCursor(0, 1);
            lcd.print("B1=OUI");

            break;
    }
}


// ==================================================================
// AUGMENTER
// ==================================================================

void augmenterValeur()
{
    switch (champReglage)
    {
        case 0:

            regHeure++;

            if (regHeure > 23)
                regHeure = 0;

            break;


        case 1:

            regMinute++;

            if (regMinute > 59)
                regMinute = 0;

            break;


        case 2:

            regSeconde++;

            if (regSeconde > 59)
                regSeconde = 0;

            break;


        case 3:

            regJour++;

            if (
                regJour >
                joursDansMois(
                    regMois,
                    regAnnee
                )
            )
            {
                regJour = 1;
            }

            break;


        case 4:

            regMois++;

            if (regMois > 12)
                regMois = 1;

            corrigerJour();

            break;


        case 5:

            regAnnee++;

            if (regAnnee > 2099)
                regAnnee = 2020;

            corrigerJour();

            break;
    }

    afficherChampReglage();
}


// ==================================================================
// DIMINUER
// ==================================================================

void diminuerValeur()
{
    switch (champReglage)
    {
        case 0:

            regHeure--;

            if (regHeure < 0)
                regHeure = 23;

            break;


        case 1:

            regMinute--;

            if (regMinute < 0)
                regMinute = 59;

            break;


        case 2:

            regSeconde--;

            if (regSeconde < 0)
                regSeconde = 59;

            break;


        case 3:

            regJour--;

            if (regJour < 1)
            {
                regJour =
                    joursDansMois(
                        regMois,
                        regAnnee
                    );
            }

            break;


        case 4:

            regMois--;

            if (regMois < 1)
                regMois = 12;

            corrigerJour();

            break;


        case 5:

            regAnnee--;

            if (regAnnee < 2020)
                regAnnee = 2099;

            corrigerJour();

            break;
    }

    afficherChampReglage();
}


// ==================================================================
// DIAGNOSTIC RTC
// ==================================================================

void afficherDiagnosticRTC(
    const RtcDateTime& verification
)
{
    Serial.println();
    Serial.println(
        "================================"
    );

    Serial.println(
        "       DIAGNOSTIC DS1302"
    );

    Serial.println(
        "================================"
    );

    Serial.print(
        "Date lue : "
    );

    Serial.print(
        verification.Day()
    );

    Serial.print("/");

    Serial.print(
        verification.Month()
    );

    Serial.print("/");

    Serial.println(
        verification.Year()
    );


    Serial.print(
        "Heure lue : "
    );

    if (verification.Hour() < 10)
        Serial.print("0");

    Serial.print(
        verification.Hour()
    );

    Serial.print(":");

    if (verification.Minute() < 10)
        Serial.print("0");

    Serial.print(
        verification.Minute()
    );

    Serial.print(":");

    if (verification.Second() < 10)
        Serial.print("0");

    Serial.println(
        verification.Second()
    );


    Serial.print(
        "RTC running : "
    );

    Serial.println(
        rtc.GetIsRunning()
        ? "OUI"
        : "NON"
    );


    Serial.print(
        "RTC valide : "
    );

    Serial.println(
        rtc.IsDateTimeValid()
        ? "OUI"
        : "NON"
    );

    Serial.println(
        "================================"
    );
}


// ==================================================================
// SAUVEGARDER RTC
// ==================================================================

void sauvegarderRTC()
{
    corrigerJour();


    Serial.println();
    Serial.println(
        "================================"
    );

    Serial.println(
        "      ECRITURE DS1302"
    );

    Serial.println(
        "================================"
    );


    Serial.print(
        "Date demandee : "
    );

    Serial.print(regJour);
    Serial.print("/");

    Serial.print(regMois);
    Serial.print("/");

    Serial.println(regAnnee);


    Serial.print(
        "Heure demandee : "
    );

    if (regHeure < 10)
        Serial.print("0");

    Serial.print(regHeure);
    Serial.print(":");

    if (regMinute < 10)
        Serial.print("0");

    Serial.print(regMinute);
    Serial.print(":");

    if (regSeconde < 10)
        Serial.print("0");

    Serial.println(regSeconde);


    // --------------------------------------------------------------
    // AUTORISER ECRITURE
    // --------------------------------------------------------------

    Serial.println(
        "Desactivation protection..."
    );

    rtc.SetIsWriteProtected(false);


    // --------------------------------------------------------------
    // DEMARRER RTC
    // --------------------------------------------------------------

    Serial.println(
        "Activation oscillateur..."
    );

    rtc.SetIsRunning(true);


    // --------------------------------------------------------------
    // CREATION DATE
    // --------------------------------------------------------------

    RtcDateTime nouvelleDate(
        regAnnee,
        regMois,
        regJour,
        regHeure,
        regMinute,
        regSeconde
    );


    // --------------------------------------------------------------
    // PREMIERE ECRITURE
    // --------------------------------------------------------------

    Serial.println(
        "Ecriture #1..."
    );

    rtc.SetDateTime(
        nouvelleDate
    );


    delay(300);


    // --------------------------------------------------------------
    // PREMIERE LECTURE
    // --------------------------------------------------------------

    RtcDateTime verification =
        rtc.GetDateTime();


    afficherDiagnosticRTC(
        verification
    );


    bool sauvegardeOK =
        verification.Year() == regAnnee &&
        verification.Month() == regMois &&
        verification.Day() == regJour &&
        verification.Hour() == regHeure &&
        verification.Minute() == regMinute &&
        verification.Second() == regSeconde;


    // --------------------------------------------------------------
    // DEUXIEME TENTATIVE
    // --------------------------------------------------------------

    if (!sauvegardeOK)
    {
        Serial.println();
        Serial.println(
            "ATTENTION : PREMIERE ECRITURE"
        );

        Serial.println(
            "NON CONFIRMEE."
        );

        Serial.println(
            "Nouvelle tentative..."
        );


        rtc.SetIsWriteProtected(false);

        rtc.SetIsRunning(true);


        delay(100);


        rtc.SetDateTime(
            nouvelleDate
        );


        delay(500);


        verification =
            rtc.GetDateTime();


        afficherDiagnosticRTC(
            verification
        );


        sauvegardeOK =
            verification.Year() == regAnnee &&
            verification.Month() == regMois &&
            verification.Day() == regJour &&
            verification.Hour() == regHeure &&
            verification.Minute() == regMinute &&
            verification.Second() == regSeconde;
    }


    // --------------------------------------------------------------
    // RESULTAT
    // --------------------------------------------------------------

    Serial.println();

    if (sauvegardeOK)
    {
        Serial.println(
            ">>> RTC : SAUVEGARDE REUSSIE"
        );


        lcd.clear();

        lcd.setCursor(0, 0);

        lcd.print(
            "HEURE SAUVEE"
        );


        lcd.setCursor(0, 1);

        if (
            verification.Hour() < 10
        )
        {
            lcd.print("0");
        }

        lcd.print(
            verification.Hour()
        );

        lcd.print(":");


        if (
            verification.Minute() < 10
        )
        {
            lcd.print("0");
        }

        lcd.print(
            verification.Minute()
        );

        lcd.print(":");


        if (
            verification.Second() < 10
        )
        {
            lcd.print("0");
        }

        lcd.print(
            verification.Second()
        );


        delay(1500);


        modeReglage = false;

        champReglage = 0;

        attendreRelacheReglage = false;


        derniereAnneeSonnee = -1;
        dernierMoisSonnee = -1;
        dernierJourSonnee = -1;
        derniereHeureSonnee = -1;
        derniereMinuteSonnee = -1;


        synchroniserBoutons();


        ledsNormal();

        lcd.clear();


        Serial.println(
            "RTC : MODE NORMAL"
        );
    }
    else
    {
        Serial.println();
        Serial.println(
            ">>> ERREUR : RTC NON SAUVEGARDE"
        );

        Serial.println();

        Serial.println(
            "VERIFIER :"
        );

        Serial.println(
            "1. VCC DS1302 -> 5V"
        );

        Serial.println(
            "2. GND DS1302 -> GND"
        );

        Serial.println(
            "3. CE/RST -> D10"
        );

        Serial.println(
            "4. I/O -> D11"
        );

        Serial.println(
            "5. SCLK -> D12"
        );

        Serial.println(
            "6. Pile du DS1302"
        );

        Serial.println(
            "7. Module DS1302"
        );


        lcd.clear();

        lcd.setCursor(0, 0);

        lcd.print(
            "ERREUR RTC"
        );

        lcd.setCursor(0, 1);

        lcd.print(
            "B1=REESSAYER"
        );


        delay(1500);


        modeReglage = true;

        champReglage = 6;

        attendreRelacheReglage = true;
    }
}


// ==================================================================
// SYNCHRONISER BOUTONS
// ==================================================================

void synchroniserBoutons()
{
    bool b1 =
        digitalRead(BTN1);

    bool b2 =
        digitalRead(BTN2);

    bool b3 =
        digitalRead(BTN3);


    bouton1.dernierEtat = b1;
    bouton1.etatStable = b1;
    bouton1.dernierChangement = millis();


    bouton2.dernierEtat = b2;
    bouton2.etatStable = b2;
    bouton2.dernierChangement = millis();


    bouton3.dernierEtat = b3;
    bouton3.etatStable = b3;
    bouton3.dernierChangement = millis();
}


// ==================================================================
// MENU REGLAGE
// ==================================================================

void gererMenuReglage()
{
    if (!modeReglage)
        return;


    if (attendreRelacheReglage)
    {
        if (
            digitalRead(BTN1) == LOW &&
            digitalRead(BTN3) == LOW
        )
        {
            attendreRelacheReglage = false;

            synchroniserBoutons();

            afficherChampReglage();
        }

        return;
    }


    // B2 = augmenter

    if (
        boutonAppuye(bouton2)
    )
    {
        if (
            champReglage < 6
        )
        {
            augmenterValeur();
        }
    }


    // B3 = diminuer

    if (
        boutonAppuye(bouton3)
    )
    {
        if (
            champReglage < 6
        )
        {
            diminuerValeur();
        }
    }


    // B1

    if (
        boutonAppuye(bouton1)
    )
    {
        if (
            champReglage == 6
        )
        {
            sauvegarderRTC();

            return;
        }


        champReglage++;


        if (
            champReglage > 6
        )
        {
            champReglage = 0;
        }


        afficherChampReglage();
    }
}


// ==================================================================
// DETECTION B1 + B3
// ==================================================================

void detecterCombinaisonReglage()
{
    if (modeReglage)
        return;


    bool b1 =
        digitalRead(BTN1) == HIGH;

    bool b3 =
        digitalRead(BTN3) == HIGH;


    if (
        b1 && b3
    )
    {
        if (
            debutAppuiReglage == 0
        )
        {
            debutAppuiReglage =
                millis();
        }


        if (
            !combinaisonReglageDetectee &&
            millis() -
            debutAppuiReglage >=
            DUREE_APPUI_REGLAGE
        )
        {
            combinaisonReglageDetectee =
                true;


            preparerReglage();


            afficherChampReglage();
        }
    }
    else
    {
        debutAppuiReglage = 0;

        combinaisonReglageDetectee =
            false;
    }
}


// ==================================================================
// INITIALISATION RTC
// ==================================================================

void initialiserRTC()
{
    Serial.println();
    Serial.println(
        "================================"
    );

    Serial.println(
        "       INITIALISATION DS1302"
    );

    Serial.println(
        "================================"
    );


    // --------------------------------------------------------------
    // DEMARRER COMMUNICATION
    // --------------------------------------------------------------

    Serial.println(
        "RTC.Begin()..."
    );

    rtc.Begin();


    // --------------------------------------------------------------
    // ENLEVER PROTECTION
    // --------------------------------------------------------------

    Serial.println(
        "Protection ecriture OFF..."
    );

    rtc.SetIsWriteProtected(false);


    // --------------------------------------------------------------
    // ETAT OSCILLATEUR
    // --------------------------------------------------------------

    Serial.print(
        "RTC running avant : "
    );

    Serial.println(
        rtc.GetIsRunning()
        ? "OUI"
        : "NON"
    );


    // --------------------------------------------------------------
    // VALIDITE
    // --------------------------------------------------------------

    bool valide =
        rtc.IsDateTimeValid();


    Serial.print(
        "Date RTC valide : "
    );

    Serial.println(
        valide
        ? "OUI"
        : "NON"
    );


    // --------------------------------------------------------------
    // SI INVALIDITE
    // --------------------------------------------------------------

    if (!valide)
    {
        Serial.println();
        Serial.println(
            "RTC INVALIDE."
        );

        Serial.println(
            "Tentative initialisation..."
        );


        rtc.SetIsWriteProtected(false);

        rtc.SetIsRunning(true);


        RtcDateTime compilation(
            __DATE__,
            __TIME__
        );


        Serial.println(
            "Date de compilation :"
        );


        afficherSurMoniteurSerie(
            compilation
        );


        Serial.println(
            "Ecriture date compilation..."
        );


        rtc.SetDateTime(
            compilation
        );


        delay(500);
    }


    // --------------------------------------------------------------
    // DEMARRER RTC
    // --------------------------------------------------------------

    if (
        !rtc.GetIsRunning()
    )
    {
        Serial.println(
            "RTC arrete."
        );

        Serial.println(
            "Demarrage..."
        );

        rtc.SetIsRunning(true);

        delay(200);
    }


    // --------------------------------------------------------------
    // RELECTURE
    // --------------------------------------------------------------

    RtcDateTime verification =
        rtc.GetDateTime();


    Serial.println();

    Serial.println(
        "===== RTC INITIALISE ====="
    );


    afficherSurMoniteurSerie(
        verification
    );


    Serial.print(
        "Running : "
    );

    Serial.println(
        rtc.GetIsRunning()
        ? "OUI"
        : "NON"
    );


    Serial.print(
        "Date valide : "
    );

    Serial.println(
        rtc.IsDateTimeValid()
        ? "OUI"
        : "NON"
    );


    Serial.println(
        "=========================="
    );
}


// ==================================================================
// AFFICHAGE SERIE
// ==================================================================

void afficherSurMoniteurSerie(
    const RtcDateTime& now
)
{
    Serial.print(
        nomJour(
            now.DayOfWeek()
        )
    );

    Serial.print(" ");


    if (
        now.Day() < 10
    )
    {
        Serial.print("0");
    }

    Serial.print(
        now.Day()
    );

    Serial.print("/");


    if (
        now.Month() < 10
    )
    {
        Serial.print("0");
    }

    Serial.print(
        now.Month()
    );

    Serial.print("/");


    Serial.print(
        now.Year()
    );

    Serial.print(" ");


    if (
        now.Hour() < 10
    )
    {
        Serial.print("0");
    }

    Serial.print(
        now.Hour()
    );

    Serial.print(":");


    if (
        now.Minute() < 10
    )
    {
        Serial.print("0");
    }

    Serial.print(
        now.Minute()
    );

    Serial.print(":");


    if (
        now.Second() < 10
    )
    {
        Serial.print("0");
    }

    Serial.println(
        now.Second()
    );
}


// ==================================================================
// SETUP
// ==================================================================

void setup()
{
    Serial.begin(9600);


    delay(500);


    Serial.println();
    Serial.println(
        "DEMARRAGE SYSTEME"
    );


    // --------------------------------------------------------------
    // LEDS
    // --------------------------------------------------------------

    pinMode(
        LED_BLEUE,
        OUTPUT
    );

    pinMode(
        LED_JAUNE,
        OUTPUT
    );

    pinMode(
        LED_ROUGE,
        OUTPUT
    );


    // --------------------------------------------------------------
    // RELAIS
    // --------------------------------------------------------------

    if (RELAIS_ACTIF_HIGH)
    {
        digitalWrite(
            RELAIS,
            HIGH
        );
    }
    else
    {
        digitalWrite(
            RELAIS,
            LOW
        );
    }


    pinMode(
        RELAIS,
        OUTPUT
    );


    relaisOFF();


    // --------------------------------------------------------------
    // BOUTONS
    // --------------------------------------------------------------

    pinMode(
        BTN1,
        INPUT
    );

    pinMode(
        BTN2,
        INPUT
    );

    pinMode(
        BTN3,
        INPUT
    );


    synchroniserBoutons();


    // --------------------------------------------------------------
    // LCD
    // --------------------------------------------------------------

    lcd.begin(
        16,
        2
    );

    lcd.clear();

    lcd.setCursor(0, 0);

    lcd.print(
        "REGLEMENTAIRE"
    );

    lcd.setCursor(0, 1);

    lcd.print(
        "Initialisation"
    );


    delay(1000);


    // --------------------------------------------------------------
    // RTC
    // --------------------------------------------------------------

    initialiserRTC();


    // --------------------------------------------------------------
    // MODE NORMAL
    // --------------------------------------------------------------

    modeSysteme =
        MODE_SCOLAIRE;


    ledsNormal();


    // --------------------------------------------------------------
    // AFFICHAGE
    // --------------------------------------------------------------

    RtcDateTime now =
        rtc.GetDateTime();


    afficherHeure(
        now
    );


    // --------------------------------------------------------------
    // SERIE
    // --------------------------------------------------------------

    Serial.println();

    Serial.println(
        "=============================="
    );

    Serial.println(
        " REGLEMENTAIRE SCOLAIRE"
    );

    Serial.println(
        "=============================="
    );

    Serial.println(
        "B1 = URGENCE"
    );

    Serial.println(
        "B2 = MODE"
    );

    Serial.println(
        "B3 = TEST"
    );

    Serial.println(
        "B1+B3 3 SEC = REGLAGE RTC"
    );

    Serial.println(
        "RELAIS : COM + NC"
    );

    Serial.println(
        "D9 LOW  = SIRENE ON"
    );

    Serial.println(
        "D9 HIGH = SIRENE OFF"
    );

    Serial.println(
        "=============================="
    );


    afficherSurMoniteurSerie(
        now
    );
}


// ==================================================================
// LOOP
// ==================================================================

void loop()
{
    // --------------------------------------------------------------
    // B1 + B3
    // --------------------------------------------------------------

    detecterCombinaisonReglage();


    // --------------------------------------------------------------
    // MODE REGLAGE
    // --------------------------------------------------------------

    if (modeReglage)
    {
        gererMenuReglage();

        delay(5);

        return;
    }


    // --------------------------------------------------------------
    // RTC
    // --------------------------------------------------------------

    RtcDateTime now =
        rtc.GetDateTime();


    // --------------------------------------------------------------
    // BOUTON 1
    // --------------------------------------------------------------

    if (
        boutonAppuye(bouton1)
    )
    {
        if (
            digitalRead(BTN3) == LOW
        )
        {
            demarrerUrgence();
        }
    }


    // --------------------------------------------------------------
    // BOUTON 2
    // --------------------------------------------------------------

    if (
        boutonAppuye(bouton2)
    )
    {
        if (
            !urgenceActive &&
            !testActif &&
            !sonnerieActive
        )
        {
            changerMode();
        }
    }


    // --------------------------------------------------------------
    // BOUTON 3
    // --------------------------------------------------------------

    if (
        boutonAppuye(bouton3)
    )
    {
        if (
            digitalRead(BTN1) == LOW
        )
        {
            demarrerTest();
        }
    }


    // --------------------------------------------------------------
    // GERER ETATS
    // --------------------------------------------------------------

    gererUrgence();

    gererTest();

    gererSonnerie();


    // --------------------------------------------------------------
    // SONNERIE AUTOMATIQUE
    // --------------------------------------------------------------

    if (
        !urgenceActive &&
        !testActif &&
        !sonnerieActive
    )
    {
        if (
            modeSysteme ==
            MODE_SCOLAIRE
        )
        {
            verifierSonneriesScolaires(
                now
            );
        }
        else
        {
            verifierSonneriesPriiere(
                now
            );
        }
    }


    // --------------------------------------------------------------
    // AFFICHAGE
    // --------------------------------------------------------------

    if (
        !urgenceActive &&
        !testActif &&
        !sonnerieActive
    )
    {
        afficherHeure(
            now
        );
    }


    // --------------------------------------------------------------
    // SERIE CHAQUE SECONDE
    // --------------------------------------------------------------

    static unsigned long
        dernierAffichage = 0;


    if (
        millis() -
        dernierAffichage >=
        1000
    )
    {
        dernierAffichage =
            millis();


        afficherSurMoniteurSerie(
            now
        );
    }


    delay(5);
}