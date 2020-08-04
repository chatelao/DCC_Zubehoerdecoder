/* DIY Zubehördecoder
 *
 * Klassen für die einzelnen Funktionen FSERVO, FCOIL, FSIGNAL, FSTATIC
 * Für jede im Konfig-File definierte Funktion muss ein passendes Objekt instanziiert werden.
 * Die Instanziierung muss im setup() mit 'new' erfolgen.
 */
#include <MobaTools.h>
#include "../Interface.h"

#define PPWA 3                  // Zahl der Pins je Weichenadresse

// Offset der CV-Adresse bei den Funktionsspezifschen CV-Werten
const byte MODE=0, PAR1=1, PAR2=2, PAR3=3, STATE=4 ;

// Werden die Klassen nicht im Zusammenhang mit der nmradcc Lib verwendet,
// so kann der Zugriff auf die Konfigurationsvariablen hier angepasst werden
// ( gegebenenfalls auch direkter EEPROM Zugriff )
#define NMRA
#ifdef NMRA
    #define getParam( parAdr ) ifc_getCV( _cvAdr+parAdr )
    #define setParam( parAdr, value ) ifc_setCV( _cvAdr+parAdr, value )
    #define setState( value )  ifc_setCV( _cvAdr+STATE, value )
#else
    // Zugriff auf die Konfig-Parameter direkt über EEPROM:
    // ( nicht möglich bei LocoNet Interface )
    #include <EEPROM.h>
    #define getParam( parAdr ) EEPROM.read( _cvAdr+parAdr )
    #define setParam( parAdr, value ) EEPROM.update( _cvAdr+parAdr, value )
    #define setState( value )  EEPROM.update( _cvAdr+STATE, value )
#endif
//======================  allgemeine Hilfsfunktionen ==================================
// Ausblenden der nicht belegten (NC) Ports
/* verschoben nach interface.h
#ifdef __STM32F1__
void _pinMode( byte port, WiringPinMode mode );
#else
void _pinMode( byte port, byte mode );
#endif

void _digitalWrite( byte port, byte state ) ;
*/
//========================= Funktionsklassen  =========================================

//---------------------- FCOIL -------------------------------------------
// Flags für CV 'MODE'
#define CAUTOOFF 0x01   // Die Impulsdauer wird intern begrenzt
#define NOPOSCHK 0x08   // Die Ausgänge reagieren auch auf einen Befehl, wenn die aktuelle
                        // Postion nicht verändert wird.
 
 class Fcoil {
     public:
    Fcoil( int cvAdr, uint8_t out1P[] );
    void process();
    void set( uint8_t sollWert, uint8_t outState );       // neuen Schaltbefehl erhalten

    
    private:
    MoToTimer _pulseT;
    struct {
        bool pulseON   :1 ;            // Flag ob Pausentimer läuft
        bool sollOut   :1;
        bool sollCoil  :1;       // anzusteuernde Spule
        bool sollAct   :1;       // Sollwert wurde noch nicht übernommen
        bool istCoil   :1;
    }_flags;
    #define GERADE  0x0             // Bit 0
    #define ABZW    0x1             // Bit 0
    
    uint16_t _cvAdr = 0;            // Adresse des CV-Blocks mit den Funktionsparametern
    uint8_t *_outP;           // Pins der Ausgange

 };

//------------------------FSTATIC -------------------------------------------- 
// Flags für CV 'MODE':
#define BLKMODE 0x01    // Ausgänge blinken
#define BLKSTRT 0x02    // Starten mit beide Ausgängen EIN
#define BLKSOFT 0x04    // Ausgänge als Softleds


 class Fstatic {
    // statisches oder blinkendes Ansteuern von Led's
    public:
    Fstatic( int cvAdr, uint8_t ledP[]  );
    void process( );
    void set( bool sollWert );       // neuen Schaltbefehl erhalten

    private:
    void _setLedPin( uint8_t ledI, uint8_t sollWert );
    EggTimer _pulseT;
    
    uint16_t _cvAdr;            // Adresse des CV-Blocks mit den Funktionsparametern
    SoftLed *_ledS[2] = { NULL, NULL };      // Softled-Objekte
    uint8_t *_ledP;           // Pins der Leds
    struct {
        bool blkOn :1;      // blinkende Led ist EIN
        bool isOn  :1;      // Funktion is eingeschaltet
    } _flags;       
    
 };

//------------------------ FSERVO -------------------------------------------- 
// Flags für CV 'MODE'
#define SAUTOOFF 0x01   // Impulse werden nach erreichen der Endlage abgeschaltet
#define SDIRECT  0x02   // Der Servo reagiert auch während der Bewegung auf einen Umschaltbefehl
#define NOPOSCHK 0x08   // Die Ausgänge reagieren auch auf einen Befehl, wenn die aktuelle
                        // Postion nicht verändert wird.
#define SAUTOBACK 0x04  // Servo fährt automatisch in die Grundstellung zurück (nach Zeitablauf)
#define SAUTOTIME 2000 // Defaultwert ( wenn State-Parameter = 0 ist )

class Fservo {
    public:
    Fservo( int cvAdr, uint8_t pins[], int8_t modeOffs=MODE );
    void process();
    void set( uint8_t sollWert );       // neuen Schaltbefehl erhalten
	bool isMoving ();					// Abfrage ob Servo in Bewegung
	uint8_t getPos();					// aktuelle Positionsnr. des Servo ermitteln 
    uint8_t getCvPos();                 // CV-Wert zur aktuellen Position ermitteln
	void adjust( uint8_t mode, uint8_t value );	// Servoparamter ändern
		#define ADJPOS		0			// für Endagenjustierung: value = neue Position
		#define ADJPOSEND	1			// neue Endlage in CV übernehmen
		#define ADJSPEED	2			// Servogeschwindigkeit ändern
    void center( uint8_t mode );
		#define ABSOLUT	 0 		// Servo auf 90° stellen
		#define RELATIVE 1		// Mittelstellung zwischen den programmierten Endpunkten
        
    private:
    // offset für die CV's der Positionswerte
    static const uint8_t posOffset[6];
    MoToServo  _weicheS;
    MoToTimer  _autoTime;           // zum automatischen Zurückfahren
    uint16_t _cvAdr = 0;            // Adresse des CV-Blocks mit den Funktionsparametern
    int8_t   _modeOffs = 0;         // Offset der CV-Adresse für das Mode-byte. Normalerweise ist dies 0
                                    // Bei der Servo-Kombination für 3begr. Formsignale aber -5 beim 2. Servo,
                                    // da der auch auf das ModeByte des 1. Servo zugreift.
    uint8_t *_outP;           		// Array mit Pins der Ausgänge
		#define SERVOP	0
		#define REL1P	1
		#define REL2P	2
    uint8_t _sollPos;                // Sollposition des Servo ( 0...3 )
    uint8_t _istPos;                 // istposition des Servo
    struct {
        bool relOn    :1 ;           // Ausgangszustand der Relais  
        bool moving   :1 ;           // Servo in Bewegung
        bool sollAct  :1 ;           // Sollwert wurde noch nicht übernommen
    } _flags;
 
 };
//------------------------ FSIGNAL -------------------------------------------- 
//Konstante für Lichtsignalfunktion
// Flags für CV MODE:
#define LEDINVERT 0x80  // FSIGNAL: SoftledAusgänge invertieren (Bit 7 des Modebyte von FSIGNAL2/3)
const byte  LSMODE=0,       BILD1=1,    BILD2=2, VORSIG=3, DARKMASK = 4, 
            SOFTMASK2 = 5,  BILD3=6,    BILD4=7,
            SOFTMASK3=10,   BILD5=11,   BILD6=12;
#define SIG_DARK_TIME   300     // Zeit zwischen Dunkelschalten und Aufblenden des neuen Signalbilds
#define SIG_RISETIME    500     // Auf/Abblendezeit
 
 class Fsignal {
    public:
    Fsignal( int cvAdr, uint8_t pins[], uint8_t adrAnz, Fsignal** vorSig );
		// adrAnz ist die Zahl der Adressen, die das Signal belegt. Daraus ergibt sich
		// auch die Zahl der CV-Parameter und die Zahl der Pins
    void process();                    // muss im loop() regelmäßig aufgerufen werden
    void set( uint8_t sollWert );      // neuen Signalbefehl erhalten
    void setDark  ( bool darkFlg );    // 'true' schaltet das Signal aus(dunkel), 'false' ein
    
    private:
    void    _clrSignal ();             // SoftLed's ausschalten
    void    _setSignal ();             // aktuelles Signalbild einschalten
    uint8_t _getHsMask ();             // Maske für Hard/Soft Umschaltung aller Ausgänge bestimmen
    uint8_t _getSigMask( uint8_t ) ;   // Bitmaske der Ausgänge für aktuelles Signalbild bestimmen
    
    Fsignal **_vorSig;              // Pointer auf Vorsignal am gleichen Mast
    MoToTimer darkT;                 // Dunkelzeit beim Überblenden zwischen Signalbildern
    uint16_t _cvAdr = 0;            // Adresse des CV-Blocks mit den Funktionsparametern
    uint8_t  _pinAnz;               // Zahl der zugeordnten Ausgangspins : 3(PPWA) je CV-Block 
    uint8_t *_outP;           		// Array mit Pins der Ausgänge
    SoftLed **_sigLed;              // Pointer auf Array der Softled Objekte
    struct {
        byte state   :2;            // Status der internen State-Machine
        byte sigBild :3;            // aktuelles Signalbild entsprechend letztem sollwert
        byte dark    :1;            // Signal ist aktuell Dunkelgeschaltet
        byte isVorSig:1;            // das Objekt ist ein Vorsignal
    } _fktStatus;         // interne Status
    
    // aktueller Signalzustand 
    #define SIG_WAIT        0    // Warte auf Signalbefehle
    #define SIG_NEW         2    // neues Signalbild aufblenden

 
 
 };
