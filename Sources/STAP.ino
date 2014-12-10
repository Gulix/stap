// Bibliothèque pour le LCD
#include <LiquidCrystal.h>

// "Constantes" représentant les différentes broches utilisées
#define _PIN_LEDVERTE 10
#define _PIN_LEDROUGE 8
#define _PIN_BOUTONPRINCIPAL 13
#define _PIN_BOUTONPLUS 9
#define _PIN_BOUTONMOINS 6
#define _PIN_LCD_RS 12
#define _PIN_LCD_ENABLE 11
#define _PIN_LCD_D4 5
#define _PIN_LCD_D5 4
#define _PIN_LCD_D6 3
#define _PIN_LCD_D7 2
#define _PIN_PIEZO 7

// Etat de l'application
#define _ETAT_ACCUEIL 1
#define _ETAT_CHOIX_MODE 2
#define _ETAT_VALEUR_CHRONO 3
#define _ETAT_LIMITE_MANCHE 4
#define _ETAT_NOMBRE_MANCHES 5
#define _ETAT_NOMBRE_JOUEURS 6
#define _ETAT_RECAPITULATIF 7
#define _ETAT_CHRONO_ATTENTE 8
#define _ETAT_CHRONO_ENCOURS 9
#define _ETAT_CHRONO_TERMINE 10
#define _ETAT_PASENCORE 99
int _etat;

// Modes prédéfinis
#define _NOMBRE_PREDEFINIS 1
#define _PREDEFINI_BLOODBOWL_4MIN 1
int _modePredefiniSelectionne;

// Récapitulatif - Eléments à afficher
#define _RECAP_MODE 1
#define _RECAP_CHRONO 2
#define _RECAP_LIMITE 3
#define _RECAP_MANCHES 4
#define _RECAP_JOUEURS 5
int _elementRecapAffiche = 2;

// "Constantes" de temps pour gérer le timing des actions
#define _TEMPS_ANNULATION_SEC 3
#define _TEMPS_CLIGN_LED_MS 400
#define _HEURES_EN_MS 3600000
#define _MINUTES_EN_MS 60000
#define _SECONDES_EN_MS 1000
#define _TEMPSMAX_ETATTERMINE_MS 3000

// Gestion du temps
unsigned long _millisPrecedent;
unsigned long _tempsBoutonPrincipalPresse;
unsigned long _tempsBoutonPlusPresse;
unsigned long _tempsBoutonMoinsPresse;
unsigned long _ecartPassage;
unsigned long _tempsEtatTermine;

// Gestion des LED
int _etatLedVerte;
int _etatLedRouge;
long _intervalleClignotement;

// Etat des boutons
int _precEtatBoutonPrincipal;
int _precEtatBoutonPlus;
int _precEtatBoutonMoins;
int _isBoutonPrincipalClick;
int _isBoutonPlusClick;
int _isBoutonMoinsClick;

// Gestion du rafraichissement de l'écran
int _isRefreshLCD;

// Chronos et mode sélectionné
unsigned long _tempsAffiche;
unsigned long _valeurChrono;


// Initialisation de la bibliothèque LCD
LiquidCrystal lcd(_PIN_LCD_RS, _PIN_LCD_ENABLE, _PIN_LCD_D4, _PIN_LCD_D5, _PIN_LCD_D6, _PIN_LCD_D7);

void setup() {
  // Initialisation des variables
  _millisPrecedent = 0;
  _tempsBoutonPrincipalPresse = 0;
  _tempsBoutonPlusPresse = 0;
  _tempsBoutonMoinsPresse = 0;
  _etat = _ETAT_ACCUEIL;

  // Etat des boutons
  _precEtatBoutonPrincipal = LOW;
  _precEtatBoutonPlus = LOW;
  _precEtatBoutonMoins = LOW;
  
  // Valeurs par défaut
  _modePredefiniSelectionne = _PREDEFINI_BLOODBOWL_4MIN;

  // initialisation du LCD
  lcd.begin(16, 2);
  _isRefreshLCD = true;
  
  // initialisation des pins
  pinMode(_PIN_BOUTONPRINCIPAL,INPUT);
  pinMode(_PIN_BOUTONPLUS,INPUT);
  pinMode(_PIN_BOUTONMOINS,INPUT);
  pinMode(_PIN_LEDVERTE,OUTPUT);
  pinMode(_PIN_LEDROUGE,OUTPUT);
  pinMode(_PIN_PIEZO, OUTPUT);
}

void loop() {
  
  RecupererEtatBoutons();
  DetecterAnnulation();
  PasserTemps();
  
  AnalyserModificationsEtat();
  
  if (_isRefreshLCD)
    RafraichirAffichageLCD();
  _isRefreshLCD = false;
  
  GererLEDs();
  GererPiezo();
}

void RecupererEtatBoutons()
{
  int etatBoutonPrincipal = digitalRead(_PIN_BOUTONPRINCIPAL);
  int etatBoutonPlus = digitalRead(_PIN_BOUTONPLUS);
  int etatBoutonMoins = digitalRead(_PIN_BOUTONMOINS);
  
  _isBoutonPrincipalClick = (etatBoutonPrincipal == LOW) && (_precEtatBoutonPrincipal == HIGH);
  _isBoutonPlusClick = (etatBoutonPlus == LOW) && (_precEtatBoutonPlus == HIGH);
  _isBoutonMoinsClick = (etatBoutonMoins == LOW) && (_precEtatBoutonMoins == HIGH);
  
  _precEtatBoutonPrincipal = etatBoutonPrincipal;
  _precEtatBoutonPlus = etatBoutonPlus;
  _precEtatBoutonMoins = etatBoutonMoins;
}

void PasserTemps()
{
  unsigned long currentMillis = millis();
  _ecartPassage = currentMillis - _millisPrecedent;
  
  // Temps d'appui sur bouton principal
  if (_precEtatBoutonPrincipal == HIGH)
    _tempsBoutonPrincipalPresse += _ecartPassage;
  else if (_tempsBoutonPrincipalPresse != 0)
    _tempsBoutonPrincipalPresse = 0;
    
  // Temps d'appui sur bouton "+"
  if (_precEtatBoutonPlus == HIGH)
    _tempsBoutonPlusPresse += _ecartPassage;
  else if (_tempsBoutonPlusPresse != 0)
    _tempsBoutonPlusPresse = 0;
    
  // Temps d'appui sur bouton "-"
  if (_precEtatBoutonMoins == HIGH)
    _tempsBoutonMoinsPresse += _ecartPassage;
  else if (_tempsBoutonMoinsPresse != 0)
    _tempsBoutonMoinsPresse = 0;
	
  // Timer courant
  if (_etat == _ETAT_CHRONO_ENCOURS)
  {
    if (_tempsAffiche < _ecartPassage)
    {
      _etat = _ETAT_CHRONO_TERMINE;
      _tempsEtatTermine = 0;
	  _isRefreshLCD = true;
    }
    else 
      _tempsAffiche -= _ecartPassage;
  }
  // Limite de l'état "Terminé" pour le buzzer
  else if (_etat == _ETAT_CHRONO_TERMINE)
  {
    if (_tempsEtatTermine < _TEMPSMAX_ETATTERMINE_MS)
      _tempsEtatTermine += _ecartPassage;
  }
  
  _millisPrecedent = currentMillis;
}

void DetecterAnnulation()
{
  // Si le bouton principal est relâché après un appui long, 
  // on annule ce qui est en cours, et on revient à l'accueil
  if (!_isBoutonPrincipalClick)
    return;
    
  if (_tempsBoutonPrincipalPresse < (_TEMPS_ANNULATION_SEC * 1000))
    return;
    
  // Retour à l'état "Accueil"
  _etat = _ETAT_ACCUEIL;
  _isBoutonPrincipalClick = !_isBoutonPrincipalClick;
  _isRefreshLCD = true;
}

void AnalyserModificationsEtat()
{
  // Pour chaque état, on procède à l'analyse des entrées
  // Le cas échéant, on procède à un changement d'état
  
  switch(_etat)
  {
    case _ETAT_ACCUEIL:
      AnalyserModificationsEtat_Accueil();
      break;
    case _ETAT_RECAPITULATIF:
      AnalyserModificationsEtat_Recapitulatif();
      break;
	case _ETAT_CHRONO_ATTENTE:
	  AnalyserModificationsEtat_ChronoAttente();
	  break;
	case _ETAT_CHRONO_ENCOURS:
	  AnalyserModificationsEtat_ChronoEnCours();
	  break;
	case _ETAT_PASENCORE:
	  AnalyserModificationsEtat_PasEncore();
	  break;
    case _ETAT_CHRONO_TERMINE:
      AnalyserModificationsEtat_Termine();
  }
}

void AnalyserModificationsEtat_Accueil()
{
  // On navigue sur le choix d'un chrono "Custom" (0)
  // ou une liste de chronos prédéfinis (1..x)
  if(_isBoutonPlusClick)
  {
    _modePredefiniSelectionne++;
    if (_modePredefiniSelectionne > _NOMBRE_PREDEFINIS)
      _modePredefiniSelectionne = 0;
    _isRefreshLCD = true;
  }
  else if (_isBoutonMoinsClick)
  {
    _modePredefiniSelectionne--;
    if (_modePredefiniSelectionne < 0)
      _modePredefiniSelectionne = _NOMBRE_PREDEFINIS;
    _isRefreshLCD = true;
  }
  else if (_isBoutonPrincipalClick)
  {
    if (_modePredefiniSelectionne == 0)
    {
      //_etat = _ETAT_CHOIX_MODE;
	  _etat = _ETAT_PASENCORE;
      _isRefreshLCD = true;
    }
    else 
    {
      _etat = _ETAT_RECAPITULATIF;
      _isRefreshLCD = true;
      if (_modePredefiniSelectionne == _PREDEFINI_BLOODBOWL_4MIN)
      {
        _valeurChrono = 4 * 60000;
		//_valeurChrono = 45000;
      }
    }
  }
}

void AnalyserModificationsEtat_Recapitulatif()
{
  // Le récapitulatif sert de "tampon" avant d'utiliser vraiment le chrono
  if (_isBoutonPrincipalClick)
  {
    _etat = _ETAT_CHRONO_ATTENTE;
    _isRefreshLCD = true;
  }
}

void AnalyserModificationsEtat_ChronoAttente()
{
  if (_isBoutonPrincipalClick)
  {
    _etat = _ETAT_CHRONO_ENCOURS;
	_isRefreshLCD = true;
	_tempsAffiche = _valeurChrono;
  }
}

void AnalyserModificationsEtat_ChronoEnCours()
{
  if (_isBoutonPrincipalClick)
  {
    _etat = _ETAT_CHRONO_ATTENTE;
	_isRefreshLCD = true;
  } 
  else 
  {
    // Doit-on faire un refresh du LCD ?
	if (_tempsAffiche > 10 * 1000) // > 10s, on vérifie le changement de seconde
	  _isRefreshLCD = (_tempsAffiche / 1000) != ((_tempsAffiche + _ecartPassage) / 1000);
	else // <= 10s, on vérifie le changement de dixième
	  _isRefreshLCD = (_tempsAffiche / 100) != ((_tempsAffiche + _ecartPassage) / 100);
  }
   
}

void AnalyserModificationsEtat_PasEncore()
{
  // Ecran pour les fonctionnalités non-développées
  if (_isBoutonPrincipalClick)
  {
    _etat = _ETAT_ACCUEIL;
	_isRefreshLCD = true;
  }
}

void AnalyserModificationsEtat_Termine()
{
  if (_isBoutonPrincipalClick)
  {
    _etat = _ETAT_CHRONO_ATTENTE;
	_isRefreshLCD = true;
  }
}

void RafraichirAffichageLCD()
{
  lcd.noDisplay();
  lcd.clear();
  lcd.home();
  
  // Affichage selon l'état
  switch(_etat)
  {
    case _ETAT_ACCUEIL:
      AffichageLCD_Accueil();
      break;
    case _ETAT_RECAPITULATIF:
      AffichageLCD_Recapitulatif();
      break;
	case _ETAT_CHRONO_ATTENTE:
	  AffichageLCD_ChronoAttente();
	  break;
	case _ETAT_CHRONO_ENCOURS:
	  AffichageLCD_ChronoEnCours();
	  break;
    case _ETAT_CHRONO_TERMINE:
      AffichageLCD_ChronoTermine();
      break;
	case _ETAT_PASENCORE:
	  AffichageLCD_PasEncore();
	  break;
  }
  
  lcd.display();
}

void AffichageLCD_Accueil()
{
  if (_modePredefiniSelectionne == 0)
  {
    lcd.print(F("Chrono"));
    lcd.setCursor(0,1);
    lcd.print(F("     Mode custom"));
  }
  else if (_modePredefiniSelectionne == _PREDEFINI_BLOODBOWL_4MIN)
  {
    lcd.print(F("Blood Bowl"));
    lcd.setCursor(0,1);
    lcd.print(F("           4 min"));
  }
}

void AffichageLCD_Recapitulatif()
{
  if (_modePredefiniSelectionne == 0)
    lcd.print(F("Mode custom"));    
  else if (_modePredefiniSelectionne == _PREDEFINI_BLOODBOWL_4MIN)
    lcd.print(F("Blood Bowl  4min"));
  
  lcd.setCursor(0,1);
  
  // On peut afficher les différentes infos avec +/-
  
}

void AffichageLCD_ChronoAttente()
{
  lcd.print(F("  Chrono pret  "));
  
}

void AffichageLCD_ChronoEnCours()
{
  int nbHeures = _tempsAffiche / _HEURES_EN_MS;
  int nbMinutes = (_tempsAffiche % _HEURES_EN_MS) / _MINUTES_EN_MS;
  int nbSecondes = (_tempsAffiche % _MINUTES_EN_MS) / _SECONDES_EN_MS;
  int nbDixiemes = (_tempsAffiche % _SECONDES_EN_MS) / 100;
  
  if ((nbHeures > 0) || (nbMinutes > 30))
    lcd.print("    "
	          + ((nbHeures > 9) ? String(nbHeures) : ("0" + String(nbHeures))) + ":" 
	          + ((nbMinutes > 9) ? String(nbMinutes) : ("0" + String(nbMinutes))) + ":"
	          + ((nbSecondes > 9) ? String(nbSecondes) : ("0" + String(nbSecondes))));
  else if ((nbMinutes > 0) || (nbSecondes > 10))
    lcd.print("     " 
	          + ((nbMinutes > 9) ? String(nbMinutes) : ("0" + String(nbMinutes))) + ":"
	          + ((nbSecondes > 9) ? String(nbSecondes) : ("0" + String(nbSecondes))));
  else
    lcd.print("      " 
	          + ((nbSecondes > 9) ? String(nbSecondes) : ("0" + String(nbSecondes))) + ":"
	          + String(nbDixiemes) + "0");
}

void AffichageLCD_ChronoTermine()
{
  lcd.print(F("  Temps ecoule  "));
}

void AffichageLCD_PasEncore()
{
  lcd.print(F("Pas encore ..."));
  lcd.setCursor(0,1);
  lcd.print(F("       Bientot ?"));
}

void GererLEDs()
{
  if (_etat == _ETAT_ACCUEIL)
  {
    _etatLedVerte = HIGH;
	_etatLedRouge = LOW;
  }
  else if (_etat == _ETAT_CHRONO_ATTENTE)
  {
    // On fait clignoter les LED l'une après l'autre
    _intervalleClignotement = _intervalleClignotement - _ecartPassage;
	if (_intervalleClignotement < 0)
	{
	  if (_etatLedVerte == _etatLedRouge)
	  {
	    _etatLedVerte = HIGH;
		_etatLedRouge = LOW;
	  } else 
	  {
	    int etatTampon = _etatLedVerte;
	    _etatLedVerte = _etatLedRouge;
	    _etatLedRouge = etatTampon;
	  }
	  _intervalleClignotement = _TEMPS_CLIGN_LED_MS;
	}
  }
  else if (_etat == _ETAT_CHRONO_ENCOURS)
  {
    _etatLedVerte = LOW;
	_etatLedRouge = LOW;
	if (_tempsAffiche > 30000)
      _etatLedVerte = HIGH;
    else if (_tempsAffiche > 28000) // avertissement à 30s
      _etatLedRouge = HIGH;
	else if ((_tempsAffiche < 21000) && (_tempsAffiche > 19000)) // avertissement à 20s
      _etatLedRouge = HIGH;
	else if ((_tempsAffiche < 15500) && (_tempsAffiche > 14500)) // avertissement à 15s
      _etatLedRouge = HIGH;
    else if ((_tempsAffiche < 10000) && (_tempsAffiche > 5000))
	{
	  // Clignotement de 500 ms sur la rouge
	  _etatLedRouge = ((_tempsAffiche % 1000) > 500) ? HIGH : LOW;
	}
	else if (_tempsAffiche <= 5000)
	{
	  // Clignotement de 100 ms sur la rouge
	  _etatLedRouge = ((_tempsAffiche % 200) > 100) ? HIGH : LOW;
	}	
  }
  else if (_etat == _ETAT_CHRONO_TERMINE)
  {
    _etatLedVerte = LOW;
    _etatLedRouge = HIGH;
  }
  
  // Annulation - la led rouge s'allume
  if (_tempsBoutonPrincipalPresse > (_TEMPS_ANNULATION_SEC * 1000))
  {
	_etatLedVerte = LOW;
	_etatLedRouge = HIGH;
  }
  
  digitalWrite(_PIN_LEDVERTE, _etatLedVerte);
  digitalWrite(_PIN_LEDROUGE, _etatLedRouge);
}

void GererPiezo()
{
  int etatPiezo = false;
  if (_etat == _ETAT_CHRONO_ENCOURS)
  {
	if ((_tempsAffiche < 30000) && (_tempsAffiche > 28000)) // avertissement à 30s
      etatPiezo = true;
	else if ((_tempsAffiche < 21000) && (_tempsAffiche > 19000)) // avertissement à 20s
      etatPiezo = true;
	else if ((_tempsAffiche < 15500) && (_tempsAffiche > 14500)) // avertissement à 15s
      etatPiezo = true;
    else if ((_tempsAffiche < 10000) && (_tempsAffiche > 5000))
	{
	  // Clignotement de 500 ms sur la rouge
	  etatPiezo = ((_tempsAffiche % 1000) > 500);
	}
	else if (_tempsAffiche <= 5000)
	{
	  // Clignotement de 100 ms sur la rouge
	  etatPiezo = ((_tempsAffiche % 200) > 100);
	}	
  }
  else if (_etat == _ETAT_CHRONO_TERMINE)
  {
    if (_tempsEtatTermine < _TEMPSMAX_ETATTERMINE_MS)
      etatPiezo = true;
  }
  
  if (etatPiezo == true)
    tone(_PIN_PIEZO, 33);
  else 
    noTone(_PIN_PIEZO);
}
