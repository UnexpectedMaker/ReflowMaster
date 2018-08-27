/*
---------------------------------------------------------------------------
Reflow Master Control - v1.0.0 - 01/08/2018

AUTHOR/LICENSE:
Created by Seon Rozenblum - seon@unexpectedmaker.com
Copyright 2016 License: GNU GPL v3 http://www.gnu.org/licenses/gpl-3.0.html

LINKS:
Project home: github.com/unexpectedmaker/reflowmaster
Blog: unexpectedmaker.com

DISCLAIMER:
This software is furnished "as is", without technical support, and with no 
warranty, express or implied, as to its usefulness for any purpose.

PURPOSE:
This controller is the software that runs on the Reflow Master toaster oven controller made by Unexpected Maker

HISTORY:
01/08/2018 v1.0 - Initial release.
13/08/2018 v1.01 - Settings UI button now change to show SELECT or CHANGE depending on what is selected

---------------------------------------------------------------------------
*/

/*
 * NOTE: This is a work in progress...
 */


/**
 * START custom temperature sensor, ADS1x
 */
#include <Wire.h>
#include <Adafruit_ADS1015.h>

// Adafruit_ADS1115 ads;  /* Use this for the 16-bit version */
Adafruit_ADS1015 ads;     /* Use thi for the 12-bit version */

#include <TypeK.h>
TypeK TC_K;
float Tsense;   // variable to store the measured temperature
float Tcjc = 25.3;  // cold junction compensation temperature
float mV   = 41.276;  // thermocouple mV reading

int16_t results;
/* Be sure to update this value based on the IC and the gain settings! */
// float   multiplier = 3.0F;    /* ADS1015 @ +/- 6.144V gain (12-bit results) */
float   multiplier = 0.125;    /* ADS1015 @ +/- 6.144V gain (12-bit results) */
//float multiplier = 0.1875F; /* ADS1115  @ +/- 6.144V gain (16-bit results) */
#define _SDA SDA
#define _SCL SCL
/**
 * END
 */

#include <SPI.h>
#include <spline.h> // http://github.com/kerinin/arduino-splines
#include "Adafruit_GFX.h" // Library Manager
#include "Adafruit_ILI9341.h" // Library Manager
#include "MAX31855.h" // by Rob Tillaart Library Manager
#include "OneButton.h" // Library Manager
#include "ReflowMasterProfile.h" 
// #include <FlashStorage.h> // Library Manager

// used to obtain the size of an array of any type
#define ELEMENTS(x)   (sizeof(x) / sizeof(x[0]))

// used to show or hide serial debug output
#define DEBUG

// TFT SPI pins
#define TFT_DC 16
#define TFT_CS 17
#define TFT_RESET 1

// MAX 31855 Pins
#define MAXDO   11
#define MAXCS   10
#define MAXCLK  12

#define BUTTON0 36 // menu buttons
#define BUTTON1 39 // menu buttons
#define BUTTON2 34 // menu buttons
#define BUTTON3 35 // menu buttons

#define BUZZER 32  // buzzer
#define RELAY  33    // relay control
#define FAN    25     // fan control

// Just a bunch of re-defined colours
#define BLUE      0x001F
#define TEAL      0x0438
#define GREEN     0x07E0
#define CYAN      0x07FF
#define RED       0xF800
#define MAGENTA   0xF81F
#define YELLOW    0xFFE0
#define ORANGE    0xFC00
#define PINK      0xF81F
#define PURPLE    0x8010
#define GREY      0xC618
#define WHITE     0xFFFF
#define BLACK     0x0000
#define DKBLUE    0x000D
#define DKTEAL    0x020C
#define DKGREEN   0x03E0
#define DKCYAN    0x03EF
#define DKRED     0x6000
#define DKMAGENTA 0x8008
#define DKYELLOW  0x8400
#define DKORANGE  0x8200
#define DKPINK    0x9009
#define DKPURPLE  0x4010
#define DKGREY    0x4A49

// @note adding some color defines for customization, add all
#define GRAPHA DKGREY
#define GRAPHB GREY
#define GRAPHC WHITE
#define GRAPHD BLACK

// Save data struct
typedef struct {
  boolean valid = false;
  boolean useFan = true;
  byte paste = 0;
  float power = 1;
  int lookAhead = 12;
  int lookAheadWarm = 1;
  int tempOffset = 0;
  bool startFullBlast = false;
} Settings;

const String ver = "1.01";
bool newSettings = false;

long nextTempRead;
double timeX = 0;
double tempOffset = 60;

byte state; // 0 = ready, 1 = warmup, 2 = reflow, 3 = finished, 10 Menu, 11+ settings
byte state_settings = 0;
byte settings_pointer = 0;

// Initialise an array to hold 4 profiles
// Increase this array if you plan to add more
ReflowGraph solderPaste[4];
// Index into the current profile
int currentGraphIndex = 0;

// Calibration data - currently diabled in this version
int calibrationState = 0;
long calibrationSeconds = 0;
long calibrationStatsUp = 0;
long calibrationStatsDown = 300;
bool calibrationUpMatch = false;
bool calibrationDownMatch = false;
float calibrationDropVal = 0;
float calibrationRiseVal = 0;

// Runtime reflow variables
float currentDuty = 0;
float currentTemp = 0;
float lastTemp = -1;
float currentDetla = 0;
unsigned int currentPlotColor = GREEN;
bool isCuttoff = false;
bool isFanOn = false;
float lastWantedTemp = -1;


// Graph Size for UI
int graphRangeMin_X = 0;
int graphRangeMin_Y = 30;
int graphRangeMax_X = -1;
int graphRangeMax_Y = 165;
int graphRangeStep_X = 30;
int graphRangeStep_Y = 15;

// Create a spline reference for converting profile values to a spline for the graph
Spline baseCurve;

// Initialise the TFT screen
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RESET);

// Initialise the MAX31855 IC for thermocouple tempterature reading
MAX31855 tc(MAXCLK, MAXCS, MAXDO);

// Initialise the buttons using OneButton library
OneButton button0(BUTTON0, false);
OneButton button1(BUTTON1, false);
OneButton button2(BUTTON2, false);
OneButton button3(BUTTON3, false);

// Initiliase a reference for the settings file that we store in flash storage
Settings set;
// Initialise flash storage
// FlashStorage(flash_store, Settings); // @todo ifdef

// This is where we initialise each of the profiles that will get loaded into the Reflkow Master
void LoadPaste()
{
/*
  Each profile is initialised with the follow data:

      Paste name ( String )
      Paste type ( String )
      Paste Reflow Temperature ( int )
      Profile graph X values - time
      Profile graph Y values - temperature
      Length of the graph  ( int, how long if the graph array )
      Fan kick in time if using a fan ( int, seconds )
      Open door message time ( int, seconds )
 */
  float baseGraphX[7] = { 1, 90, 180, 210, 240, 270, 300 }; // time
  float baseGraphY[7] = { 27, 90, 130, 138, 165, 138, 27 }; // value

  solderPaste[0] = ReflowGraph( "CHIPQUIK", "No-Clean Sn42/Bi57.6/Ag0.4", 138, baseGraphX, baseGraphY, ELEMENTS(baseGraphX), 240, 270 );

  float baseGraphX2[7] = { 1, 90, 180, 225, 240, 270, 300 }; // time
  float baseGraphY2[7] = { 25, 150, 175, 190, 210, 125, 50 }; // value

  solderPaste[1] = ReflowGraph( "CHEMTOOLS L", "No Clean 63CR218 Sn63/Pb37", 183, baseGraphX2, baseGraphY2, ELEMENTS(baseGraphX2), 240, 270 );

  float baseGraphX3[6] = { 1, 75, 130, 180, 210, 250 }; // time
  float baseGraphY3[6] = { 25, 150, 175, 210, 150, 50 }; // value

  solderPaste[2] = ReflowGraph( "CHEMTOOLS S", "No Clean 63CR218 Sn63/Pb37", 183, baseGraphX3, baseGraphY3, ELEMENTS(baseGraphX3), 180, 210 );

  float baseGraphX4[7] = { 1, 60, 120, 160, 210, 260, 310 }; // time
  float baseGraphY4[7] = { 25, 105, 150, 150, 220, 150, 20 }; // value

  solderPaste[3] = ReflowGraph( "DOC SOLDER", "No Clean Sn63/Pb36/Ag2", 187, baseGraphX4, baseGraphY4, ELEMENTS(baseGraphX4), 210, 260 );

  //TODO: Think of a better way to initalise these baseGraph arrays to not need unique array creation
}

// Obtain the temp value of the current profile at time X
int GetGraphValue( int x )
{
  return ( CurrentGraph().reflowGraphY[x] );
}

int GetGraphTime( int x )
{
  return ( CurrentGraph().reflowGraphX[x] );
}

// Obtain the current profile
ReflowGraph CurrentGraph()
{
  return solderPaste[ currentGraphIndex ];
}

// Set the current profile via the array index
void SetCurrentGraph( int index )
{
  currentGraphIndex = index;
  graphRangeMax_X = CurrentGraph().MaxTime();
  graphRangeMax_Y = CurrentGraph().MaxValue() + 5; // extra padding
  graphRangeMin_Y = CurrentGraph().MinValue();

#ifdef DEBUG
  Serial.print("Setting Paste: ");
  Serial.println( CurrentGraph().n );
  Serial.println( CurrentGraph().t );
  Serial.println( CurrentGraph().len );
#endif

  // Initialise the spline for the profile to allow for smooth graph display on UI
  timeX = 0;
  // baseCurve.setPoints(CurrentGraph().reflowGraphX, CurrentGraph().reflowGraphY, CurrentGraph().len);
  baseCurve.setPoints(CurrentGraph().reflowGraphX, CurrentGraph().reflowGraphY,  CurrentGraph().tangents, CurrentGraph().len);
  baseCurve.setDegree( Hermite );

  // Re-interpolate data based on spline
  for( int ii = 0; ii <= graphRangeMax_X; ii+= 1 )
  {
    Serial.println(ii);
    Serial.println(baseCurve.value(ii));
    solderPaste[ currentGraphIndex ].wantedCurve[ii] = baseCurve.value(ii);
    // solderPaste[ currentGraphIndex ].wantedCurve[ii] = random(200);
    delay(0);
  }

  Serial.print("DONE ");
  // calculate the biggest graph movement delta
  float lastWanted = -1;
  for ( int i = 0; i < solderPaste[ currentGraphIndex ].offTime; i++ )
  {
      float wantedTemp = solderPaste[ currentGraphIndex ].wantedCurve[ i ];

      if ( lastWanted > -1 )
      {
        float wantedDiff = (wantedTemp - lastWanted );
  
        if ( wantedDiff > solderPaste[ currentGraphIndex ].maxWantedDelta )
          solderPaste[ currentGraphIndex ].maxWantedDelta = wantedDiff;
      }
      lastWanted  = wantedTemp;
      delay(0);
  }
}

// @todo abstract pwm relay control, esp32 use sigmadelta or ledc
void setupRelay(){
    sigmaDeltaSetup(0, 312500);
    //attach pin 18 to channel 0
    sigmaDeltaAttachPin(RELAY,0);
    //initialize channel 0 to off
    sigmaDeltaWrite(0, 0);
}

void writeRelay(int value){
    sigmaDeltaWrite(0,value);
}

void setup()
{

  initTemp();

  // Setup all GPIO
  pinMode( BUZZER, OUTPUT );

  // pinMode( RELAY, OUTPUT );
  setupRelay();
  pinMode( FAN, OUTPUT );
  
  pinMode( BUTTON0, INPUT );
  pinMode( BUTTON1, INPUT );
  pinMode( BUTTON2, INPUT );
  pinMode( BUTTON3, INPUT );

  // Turn off the SSR - duty cycle of 0
  SetRelayFrequency( 0 );

#ifdef DEBUG
  Serial.begin(115200);
#endif

  // just wait a bit before we try to load settings from FLASH
  delay(500);

  // load settings from FLASH
  // set = flash_store.read(); // @todo abstract

  // If no settings were loaded, initialise data and save
  if ( !set.valid )
  {
    SetDefaults();
    newSettings = true;
    // flash_store.write(set); // @todo abstract
  }

  // Attatch button IO for OneButton
  button0.attachClick(button0Press);
  button1.attachClick(button1Press);
  button2.attachClick(button2Press);
  button3.attachClick(button3Press);

#ifdef DEBUG
  Serial.println("TFT Begin...");
#endif

  // Start up the TFT and show the boot screen
  tft.begin();
  BootScreen();

  delay(500);

// Start up the MAX31855
#ifdef DEBUG
  Serial.println("Thermocouple Begin...");
#endif
  tc.begin();

  // delay for initial temp probe read to be garbage
  delay(500);

  // Load up the profiles
  LoadPaste();
  // Set the current profile based on last selected
  SetCurrentGraph( set.paste );

  delay(500);

  // Show the main menu
  ShowMenu();
  delay(1000);
  StartReflow();  
}

// @note modified for dev testing
// @todo refactor to allow dev simulations
void _loop(){
  delay(1000);
  currentTemp = 28;
  lastTemp = currentTemp;
  StartWarmup();
  delay(1000);
  state = 2;
  StartReflow();  
}

void loop()
{
  // Used by OneButton to poll for button inputs
  button0.tick();
  button1.tick();
  button2.tick();
  button3.tick();
  // Current activity state machine
  if ( state == 0 ) // BOOT
  {
    return;
  }
  else if ( state == 1 ) // WARMUP - We sit here until the probe reaches the starting temp for the profile
  {
    if ( nextTempRead < millis() ) // we only read the probe every second
    {
      nextTempRead = millis() + 1000;
      
      ReadCurrentTemp();
      MatchTemp();
      if ( currentTemp >= GetGraphValue(0) )
      {
        // We have reached the starting temp for the profile, so lets start baking our boards!
        lastTemp = currentTemp;
        StartReflow();
      }
      else if ( currentTemp > 0 )
      {
        // Show the current probe temp so we can watch as it reaches the minimum starting value
        tft.setTextColor( YELLOW, BLACK );
        tft.setTextSize(5);
        println_Center( tft, "  "+String( round( currentTemp ) )+"c  ", tft.width() / 2, ( tft.height() / 2 ) + 10 );
      }
    }
    return;
  }
  else if ( state == 3 ) // FINISHED
  {
    // Currently not used
    return;
  }
  else if ( state == 11 || state == 12 || state == 13 ) // SETTINGS
  {
    // Currently not used
    return;
  }
  else if ( state == 10 ) // MENU
  {
    if ( nextTempRead < millis() )
    {
      nextTempRead = millis() + 1000;

      // We show the current probe temp in the men screen just for info
      ReadCurrentTemp();

      if ( currentTemp > 0 )
      {
        tft.setTextColor( YELLOW, BLACK );
        tft.setTextSize(5);
        int third = tft.width()/4;
        println_Center( tft, "  "+String( round( currentTemp ) )+"c  ", tft.width() / 2, ( tft.height() / 2 ) + 10 );
      }
    }
    return;
  }
  else if ( state == 15 )
  {
    // Currently not used
    return;
  }
  else if ( state == 16 ) // calibration - not currently used
  {
    if ( nextTempRead < millis() )
    {
      nextTempRead = millis() + 1000;
  
      ReadCurrentTemp();

      MatchCalibrationTemp();
  
      if ( calibrationState < 2 )
      {
        tft.setTextColor( CYAN, BLACK );
        tft.setTextSize(2);

        if ( calibrationState == 0 )
        {
          if ( currentTemp < GetGraphValue(0) )
            println_Center( tft, "WARMING UP", tft.width() / 2, ( tft.height() / 2 ) - 15 );
          else
            println_Center( tft, "HEAT UP SPEED", tft.width() / 2, ( tft.height() / 2 ) - 15 );
            
          println_Center( tft, "TARGET "+String( GetGraphValue(1) )+"c in " + String( GetGraphTime(1) ) +"s", tft.width() / 2, ( tft.height() -18 ) );
        }
        else if ( calibrationState == 1 )
        {
          println_Center( tft, "COOL DOWN LEVEL", tft.width() / 2, ( tft.height() / 2 ) - 15 );
          tft.fillRect( 0, tft.height()-30, tft.width(), 30, BLACK );
        }


        // only show the timer when we have hit the profile starting temp
        if (currentTemp >= GetGraphValue(0) )
        {
          // adjust the timer colour based on good or bad values
          if ( calibrationState == 0 )
          {
            if ( calibrationSeconds <= GetGraphTime(1) )
              tft.setTextColor( WHITE, BLACK );
            else
              tft.setTextColor( ORANGE, BLACK );
          }
          else
          {
            tft.setTextColor( WHITE, BLACK );
          }
          
          tft.setTextSize(4);
          println_Center( tft, " "+String( calibrationSeconds )+" secs ", tft.width() / 2, ( tft.height() / 2 ) +20 );
        }
        tft.setTextSize(5);
        tft.setTextColor( YELLOW, BLACK );
        println_Center( tft, " "+String( round( currentTemp ) )+"c ", tft.width() / 2, ( tft.height() / 2 ) + 65 );

        
      }
      else if ( calibrationState == 2 )
      {
        calibrationState = 3;
        
        tft.setTextColor( GREEN, BLACK );
        tft.setTextSize(2);
        tft.fillRect( 0, (tft.height() / 2 ) - 45, tft.width(), (tft.height() / 2 ) + 45, BLACK );
        println_Center( tft, "RESULTS!", tft.width() / 2, ( tft.height() / 2 ) - 45 );

        tft.setTextColor( WHITE, BLACK );
        tft.setCursor( 20, ( tft.height() / 2 ) - 10 );
        tft.print( "RISE " );
        if ( calibrationUpMatch )
        {
          tft.setTextColor( GREEN, BLACK );
          tft.print( "PASS" );
        }
        else
        {
          tft.setTextColor( ORANGE, BLACK );
          tft.print( "FAIL " );
          tft.setTextColor( WHITE, BLACK );
          tft.print( "REACHED " + String( round(calibrationRiseVal * 100) ) +"%") ;
        }

        tft.setTextColor( WHITE, BLACK );
        tft.setCursor( 20, ( tft.height() / 2 ) + 20 );
        tft.print( "DROP " );
        if ( calibrationDownMatch )
        {
          tft.setTextColor( GREEN, BLACK );
          tft.print( "PASS" );
          tft.setTextColor( WHITE, BLACK );
          tft.print( "DROPPED " + String( round(calibrationDropVal * 100) ) +"%") ;
        }
        else
        {
          tft.setTextColor( ORANGE, BLACK );
          tft.print( "FAIL " );
          tft.setTextColor( WHITE, BLACK );
          tft.print( "DROPPED " + String( round(calibrationDropVal * 100) ) +"%") ;

          tft.setTextColor( WHITE, BLACK );
          tft.setCursor( 20, ( tft.height() / 2 ) + 40 );
          tft.print( "RECOMMEND ADDING FAN") ;
        }
      }
    }
  }
  else // state is 2 - REFLOW
  {
    if ( nextTempRead < millis() )
    {
      nextTempRead = millis() + 1000;
  
      ReadCurrentTemp();
      MatchTemp();
  
      if ( currentTemp > 0 )
      {
        timeX++;

        if ( timeX > 300 )
        {
          EndReflow();
        }
        else 
        {
          Graph(tft, timeX, currentTemp, 30, 220, 270, 180 );

          if ( !isCuttoff )
          {
            float wantedTemp = CurrentGraph().wantedCurve[ (int)timeX ];
            DrawHeading( String( round( currentTemp ) ) + "/" + String( (int)wantedTemp )+"c", currentPlotColor, BLACK );

#ifdef DEBUG
            tft.setCursor( 60, 40 );
            tft.setTextSize(2);
            tft.fillRect( 60, 40, 80, 20, BLACK );
            tft.println( String( round((currentDuty/256) * 100 )) +"%" );
#endif
          }
        }
      }
    }
  }
}

// This is where the SSR is controlled via PWM
void SetRelayFrequency( int duty )
{
  // calculate the wanted duty based on settings power override
  currentDuty = ((float)duty * set.power );

  // Write the clamped duty cycle to the RELAY GPIO
  // analogWrite( RELAY, constrain( round( currentDuty ), 0, 255) );
  sigmaDeltaWrite(0,constrain( round( currentDuty ), 0, 255)); // @todo abstract, NOT TESTED

#ifdef DEBUG
  Serial.print("RELAY Duty Cycle: ");
  Serial.println( String( ( currentDuty / 256.0 ) * 100) + "%" +" Using Power: " + String( round( set.power * 100 )) + "%" );
#endif
}

/*
 * SOME CALIBRATION CODE THAT IS CURRENTLY USED FOR THE OVEN CHECK SYSTEM
 * Oven Check currently shows you hoe fast your oven can reach the initial pre-soak temp for your selected profile
 */
 
void MatchCalibrationTemp()
{
  if ( calibrationState == 0 ) // temp speed
  {
    // Set SSR to full duty cycle - 100%
    SetRelayFrequency( 255 );

    // Only count seconds from when we reach the profile starting temp
    if ( currentTemp >= GetGraphValue(0) )
        calibrationSeconds ++;

    if ( currentTemp >= GetGraphValue(1) )
    {
#ifdef DEBUG
      Serial.println("Cal Heat Up Speed " + String( calibrationSeconds ) );
#endif

      calibrationRiseVal =  ( (float)currentTemp / (float)( GetGraphValue(1) ) );
      calibrationUpMatch = ( calibrationSeconds <= GetGraphTime(1) );


      calibrationState = 1; // cooldown
      SetRelayFrequency( 0 );
      StartFan( false );
      Buzzer( 2000, 50 );
    }  
  }
  else if ( calibrationState == 1 )
  {
    calibrationSeconds --;
    SetRelayFrequency( 0 );

    if ( calibrationSeconds <= 0 )                                                                                               
    {
      Buzzer( 2000, 50 );
#ifdef DEBUG
      Serial.println("Cal Cool Down Temp " + String( currentTemp ) );
#endif

      // calc calibration drop percentage value
      calibrationDropVal = ( (float)( GetGraphValue(1) - currentTemp ) / (float)GetGraphValue(1) );
      // Did we drop in temp > 33% of our target temp? If not, recomment using a fan!
      calibrationDownMatch = ( calibrationDropVal > 0.33 );

      calibrationState = 2; // finished
      StartFan( true );
    }
  }
}

/*
 * END
 * SOME CALIBRATION CODE THAT IS CURRENTLY USED FOR THE OVEN CHECK SYSTEM
 */

// Read the temp probe
void ReadCurrentTemp()
{
  // int status = tc.read();
  // float internal = tc.getInternal();
  // currentTemp = tc.getTemperature() + set.tempOffset;
  updateTemp(); // @note replacing with tests and adc testing (not reliant on max 31855)
}

// This is where the magic happens for temperature matching
void MatchTemp()
{
  float duty = 0;
  float wantedTemp = 0;
  float wantedDiff = 0;
  float tempDiff = 0;
  float perc = 0;

  // if we are still before the main flow cut-off time (last peak)
  if ( timeX < CurrentGraph().offTime )
  {
    // We are looking XXX steps ahead of the ideal graph to compensate for slow movement of oven temp
    if ( timeX < CurrentGraph().reflowGraphX[2] )
      wantedTemp = CurrentGraph().wantedCurve[ (int)timeX + set.lookAheadWarm ];
    else
      wantedTemp = CurrentGraph().wantedCurve[ (int)timeX + set.lookAhead ];
    
    wantedDiff = (wantedTemp - lastWantedTemp );
    lastWantedTemp = wantedTemp;
    
    tempDiff = ( currentTemp - lastTemp ); 
    lastTemp = currentTemp;
    
    perc = wantedDiff - tempDiff;

#ifdef DEBUG
    Serial.print( "T: " );
    Serial.print( timeX );
    
    Serial.print( "  Current: " );
    Serial.print( currentTemp );
    
    Serial.print( "  Wanted: " );
    Serial.print( wantedTemp );
    
    Serial.print( "  T Diff: " );
    Serial.print( tempDiff  );
    
    Serial.print( "  W Diff: " );
    Serial.print( wantedDiff );
    
    Serial.print( "  Perc: " );
    Serial.print( perc );
#endif
    
    isCuttoff = false;

    // have to passed the fan turn on time?
    if ( !isFanOn && timeX >= CurrentGraph().fanTime )
    {
#ifdef DEBUG
      Serial.print( "COOLDOWN: " );
#endif
      // If we are usng the fan, turn it on 
      if ( set.useFan )
      {
        isFanOn = true;
        DrawHeading( "COOLDOWN!", GREEN, BLACK );
        Buzzer( 1000, 2000 );
  
        StartFan ( true );
      }
      else // otherwise YELL at the user to open the oven door
      {
        DrawHeading( "OPEN OVEN", RED, BLACK );
        Buzzer( 1000, 2000 );
      }
    }
  }
  else
  {
    // YELL at the user to open the oven door
    if ( !isCuttoff )
    {
      DrawHeading( "OPEN OVEN", GREEN, BLACK );
      Buzzer( 1000, 2000 );

#ifdef DEBUG
      Serial.print( "CUTOFF: " );
#endif      
    }

    isFanOn = false;
    isCuttoff = true;
    StartFan ( false );
  }
  
  currentDetla = (wantedTemp - currentTemp);

#ifdef DEBUG
  Serial.print( "  Delta: " );
  Serial.print( currentDetla );
#endif
  
  float base = 128;
  
  if ( currentDetla >= 0 )
  {
      base = 128 + ( currentDetla * 5 );
  }
  else if ( currentDetla < 0 )
  {
      base = 32 + ( currentDetla * 15 );
  }

  base = constrain( base, 0, 256 );

#ifdef DEBUG
  Serial.print("  Base: ");
  Serial.print( base );
  Serial.print( " -> " );
#endif
  
  duty = base + ( 172 * perc );
  duty = constrain( duty, 0, 256 );

  // override for full blast at start
  if ( set.startFullBlast && timeX < CurrentGraph().reflowGraphX[1] )
    duty = 256;
    
  currentPlotColor = GREEN;

  SetRelayFrequency( duty );
}

void StartFan ( bool start )
{
#ifdef DEBUG
  Serial.print("***** FAN Use? ");
  Serial.print( set.useFan );
  Serial.print( " start? ");
  Serial.println( start );
#endif
  
  if ( set.useFan )
  {
    digitalWrite ( FAN, ( start ? HIGH : LOW ) );
  }
}

void DrawHeading( String lbl, unsigned int acolor, unsigned int bcolor )
{
    tft.setTextSize(4);
    tft.setTextColor(acolor , bcolor);
    tft.setCursor(0,0);
    tft.fillRect( 0, 0, 220, 40, BLACK );
    tft.println( String(lbl) );
}

// buzz he buzzer
void Buzzer( int hertz, int len )
{
   // tone( BUZZER, hertz, len); // @todo esp32 no pwn or tone
}


double ox , oy ;

void DrawBaseGraph()
{
  oy = 220;
  ox = 30;
  timeX = 0;
  
  for( int ii = 0; ii <= graphRangeMax_X; ii+= 5 )
  {
    GraphDefault(tft, ii, CurrentGraph().wantedCurve[ii], 30, 220, 270, 180, PINK );
  }

  ox = 30;
  oy = 220;
  timeX = 0;
}

void BootScreen()
{
  tft.setRotation(1);
  tft.fillScreen(BLACK);

  tft.setTextColor( GREEN, BLACK );
  tft.setTextSize(3);
  println_Center( tft, "REFLOW MASTER", tft.width() / 2, ( tft.height() / 2 ) - 8 );

  tft.setTextSize(2);
  tft.setTextColor( WHITE, BLACK );
  println_Center( tft, "unexpectedmaker.com", tft.width() / 2, ( tft.height() / 2 ) + 20 );
  tft.setTextSize(1);
  println_Center( tft, "PCB v2018-2, Code v" + ver, tft.width() / 2, tft.height() - 20 );

  state = 10;
}

void ShowMenu()
{
  state = 10;
  SetRelayFrequency( 0 );

  // set = flash_store.read(); // @todo abstract
    
  tft.fillScreen(BLACK);

  tft.setTextColor( WHITE, BLACK );
  tft.setTextSize(2);
  tft.setCursor( 20, 20 );
  tft.println( "CURRENT PASTE" );

  tft.setTextColor( YELLOW, BLACK );
  tft.setCursor( 20, 40 ); 
  tft.println( CurrentGraph().n );
  tft.setCursor( 20, 60 );
  tft.println( String(CurrentGraph().tempDeg) +"deg");

  if ( newSettings )
  {
    tft.setTextColor( CYAN, BLACK );
    println_Center( tft, "Settings Stomped!!", tft.width() / 2, tft.height() - 80 );
  }

  tft.setTextSize(1);
  tft.setTextColor( WHITE, BLACK );
  println_Center( tft, "Reflow Master - PCB v2018-2, Code v" + ver, tft.width() / 2, tft.height() - 20 );

  ShowMenuOptions( true );
}

void ShowSettings()
{
  state = 11;
  SetRelayFrequency( 0 );

  newSettings = false;

  int posY = 50;
  int incY = 20;

  tft.setTextColor( BLUE, BLACK );
  tft.fillScreen(BLACK);


  tft.setTextColor( BLUE, BLACK );
  tft.setTextSize(2);
  tft.setCursor( 20, 20 );
  tft.println( "SETTINGS" );

  tft.setTextColor( WHITE, BLACK );
  tft.setCursor( 20, posY );
  tft.print( "SWITCH PASTE" );

  posY += incY;

  // Fan is y 70
  UpdateSettingsFan( posY );

  posY += incY;

  // Power is 90
  UpdateSettingsPower( posY );

  posY += incY;

  UpdateSettingsLookAhead( posY );
  
  posY += incY;

  UpdateSettingsTempOffset( posY );
  
  posY += incY;

  UpdateSettingsStartFullBlast( posY );
  
  posY += incY;

  tft.setTextColor( WHITE, BLACK );
  tft.setCursor( 20, posY );
  tft.print( "RESET TO DEFAULTS" );

  posY += incY;
  
  ShowMenuOptions( true );
}

void ShowPaste()
{
  state = 12;
  SetRelayFrequency( 0 );

  tft.fillScreen(BLACK);

  tft.setTextColor( BLUE, BLACK );
  tft.setTextSize(2);
  tft.setCursor( 20, 20 );
  tft.println( "SWITCH PASTE" );

  tft.setTextColor( WHITE, BLACK );

  int y = 50;

  for( int i = 0; i < ELEMENTS(solderPaste); i++ )
  {
      if ( i == set.paste )
        tft.setTextColor( YELLOW, BLACK );
      else
        tft.setTextColor( WHITE, BLACK );

      tft.setTextSize(2);
      tft.setCursor( 20, y );

      tft.println( String( solderPaste[i].tempDeg) +"d " + solderPaste[i].n );
      tft.setTextSize(1);
      tft.setCursor( 20, y + 17 );
      tft.println( solderPaste[i].t );
      tft.setTextColor( GREY, BLACK );
      
      y+= 40;
  }

   ShowMenuOptions( true );
}

void ShowMenuOptions( bool clearAll )
{
  int buttonPosY[] = { 19, 74, 129, 184 };
  int buttonHeight = 16;
  int buttonWidth = 4;

  tft.setTextColor( WHITE, BLACK );
  tft.setTextSize(2);

  if ( clearAll )
  {
    // Clear All
    for ( int i = 0; i < 4; i++ )
      tft.fillRect( tft.width()-100,  buttonPosY[i]-2, 100, buttonHeight+4, BLACK );
  }
  
  if ( state == 10 )
  {
    // button 0
    tft.fillRect( tft.width()-5,  buttonPosY[0], buttonWidth, buttonHeight, GREEN );
    println_Right( tft, "START", tft.width()- 27, buttonPosY[0] + 8 );
  
     // button 1
    tft.fillRect( tft.width()-5,  buttonPosY[1], buttonWidth, buttonHeight, RED );
    println_Right( tft, "SETTINGS", tft.width()- 27, buttonPosY[1] + 8 );

    // button 3
    tft.fillRect( tft.width()-5,  buttonPosY[3], buttonWidth, buttonHeight, YELLOW );
    println_Right( tft, "OVEN CHECK", tft.width()- 27, buttonPosY[3] + 8 );
  }
  else if ( state == 11 )
  {
    // button 0
    tft.fillRect( tft.width()-100,  buttonPosY[0]-2, 100, buttonHeight+4, BLACK );
    tft.fillRect( tft.width()-5,  buttonPosY[0], buttonWidth, buttonHeight, GREEN );
    switch ( settings_pointer )
      {

        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
          println_Right( tft, "CHANGE", tft.width()- 27, buttonPosY[0] + 8 );
          break;

        default:
          println_Right( tft, "SELECT", tft.width()- 27, buttonPosY[0] + 8 );
          break;
      }
  
     // button 1
    tft.fillRect( tft.width()-5,  buttonPosY[1], buttonWidth, buttonHeight, RED );
    println_Right( tft, "BACK", tft.width()- 27, buttonPosY[1] + 8 );

    // button 2
    tft.fillRect( tft.width()-5,  buttonPosY[2], buttonWidth, buttonHeight, BLUE );
    println_Right( tft, "/\\", tft.width()- 27, buttonPosY[2] + 8 );

    // button 3
    tft.fillRect( tft.width()-5,  buttonPosY[3], buttonWidth, buttonHeight, YELLOW );
    println_Right( tft, "\\/", tft.width()- 27, buttonPosY[3] + 8 );

    UpdateSettingsPointer();
  }
  else if ( state == 12 )
  {
    // button 0
    tft.fillRect( tft.width()-5,  buttonPosY[0], buttonWidth, buttonHeight, GREEN );
    println_Right( tft, "SELECT", tft.width()- 27, buttonPosY[0] + 8 );

     // button 1
    tft.fillRect( tft.width()-5,  buttonPosY[1], buttonWidth, buttonHeight, RED );
    println_Right( tft, "BACK", tft.width()- 27, buttonPosY[1] + 8 );

    // button 2
    tft.fillRect( tft.width()-5,  buttonPosY[2], buttonWidth, buttonHeight, BLUE );
    println_Right( tft, "/\\", tft.width()- 27, buttonPosY[2] + 8 );

    // button 3
    tft.fillRect( tft.width()-5,  buttonPosY[3], buttonWidth, buttonHeight, YELLOW );
    println_Right( tft, "\\/", tft.width()- 27, buttonPosY[3] + 8 );

    UpdateSettingsPointer();
  }
  else if ( state == 13 ) // restore settings to default
  {
    // button 0
    tft.fillRect( tft.width()-5,  buttonPosY[0], buttonWidth, buttonHeight, GREEN );
    println_Right( tft, "YES", tft.width()- 27, buttonPosY[0] + 8 );
  
     // button 1
    tft.fillRect( tft.width()-5,  buttonPosY[1], buttonWidth, buttonHeight, RED );
    println_Right( tft, "NO", tft.width()- 27, buttonPosY[1] + 8 );
  }
  else if ( state == 1 || state == 2 || state == 16 ) // warmup, reflow, calibration
  {
    // button 0
    tft.fillRect( tft.width()-5,  buttonPosY[0], buttonWidth, buttonHeight, GREEN );
    println_Right( tft, "ABORT", tft.width()- 27, buttonPosY[0] + 8 );
  }
  else if ( state == 3 ) // Finished
  {
     tft.fillRect( tft.width()-100,  buttonPosY[0]-2, 100, buttonHeight+4, BLACK );
    
    // button 0
    tft.fillRect( tft.width()-5,  buttonPosY[0], buttonWidth, buttonHeight, GREEN );
    println_Right( tft, "MENU", tft.width()- 27, buttonPosY[0] + 8 );
  }
  else if ( state == 15 )
  {
    // button 0
    tft.fillRect( tft.width()-5,  buttonPosY[0], buttonWidth, buttonHeight, GREEN );
    println_Right( tft, "START", tft.width()- 27, buttonPosY[0] + 8 );
  
     // button 1
    tft.fillRect( tft.width()-5,  buttonPosY[1], buttonWidth, buttonHeight, RED );
    println_Right( tft, "BACK", tft.width()- 27, buttonPosY[1] + 8 );
  } 
}

void UpdateSettingsPointer()
{
  if ( state == 11 )
  {
      tft.setTextColor( BLUE, BLACK );
      tft.setTextSize(2);
      tft.fillRect( 0, 20, 20, tft.height()-20, BLACK );
      tft.setCursor( 5, ( 50 + ( 20 * settings_pointer ) ) );
      tft.println(">");


      tft.setTextSize(1);
      tft.setTextColor( GREEN, BLACK );
      tft.fillRect( 0, tft.height()-40, tft.width(), 40, BLACK );
      switch ( settings_pointer )
      {
          case 0:
          println_Center( tft, "Select which profile to reflow", tft.width() / 2, tft.height() - 20 );
          break;

          case 1:
          println_Center( tft, "Enable fan for end of reflow, requires 5V DC fan", tft.width() / 2, tft.height() - 20 );
          break;

          case 2:
          println_Center( tft, "Adjust the power boost", tft.width() / 2, tft.height() - 20 );
          break;

          case 3:
          println_Center( tft, "Soak and Reflow look ahead for rate change speed", tft.width() / 2, tft.height() - 20 );
          break;

          case 4:
          println_Center( tft, "Adjust temp probe reading offset                                                                                               ", tft.width() / 2, tft.height() - 20 );
          break;

          case 5:
          println_Center( tft, "Force full power on initial ramp-up - be careful!", tft.width() / 2, tft.height() - 20 );
          break;

          case 6:
          println_Center( tft, "Reset to default settings", tft.width() / 2, tft.height() - 20 );
          break;

          default:
          //println_Center( tft, "", tft.width() / 2, tft.height() - 20 );
          break;
      }
      tft.setTextSize(2);
  }
  else if ( state == 12 )
  {
      tft.setTextColor( BLUE, BLACK );
      tft.setTextSize(2);
      tft.fillRect( 0, 20, 20, tft.height()-20, BLACK );
      tft.setCursor( 5, ( 50 + ( 20 * ( settings_pointer * 2 ) ) ) );
      tft.println(">");
  }
}



void StartWarmup()
{
  tft.fillScreen(BLACK);

  state = 1;
  timeX = 0;
  ShowMenuOptions( true );
  lastWantedTemp = -1;

  tft.setTextColor( GREEN, BLACK );
  tft.setTextSize(3);
  println_Center( tft, "WARMING UP", tft.width() / 2, ( tft.height() / 2 ) - 30 );

  tft.setTextColor( WHITE, BLACK );
  println_Center( tft, "START @ " + String( GetGraphValue(0) ) +"c", tft.width() / 2, ( tft.height() / 2 ) + 50 );
}

void StartReflow()
{
  tft.fillScreen(BLACK);

  state = 2;
   ShowMenuOptions( true );
  
  timeX = 0;
  SetupGraph(tft, 0, 0, 30, 220, 270, 180, graphRangeMin_X, graphRangeMax_X, graphRangeStep_X, graphRangeMin_Y, graphRangeMax_Y, graphRangeStep_Y, "Reflow Temp", " Time [s]", "deg [C]", GRAPHA,GRAPHB,GRAPHC,GRAPHD );
  
  DrawHeading( "READY", WHITE, BLACK );
  DrawBaseGraph();
}

void AbortReflow()
{
  if ( state == 1 || state == 2 || state == 16 ) // if we are in warmup or reflow states
  {
    state = 99;
    DrawHeading( "ABORT", RED, BLACK );

    StartFan( false );

    delay(1000);

    state = 10;
    ShowMenu();
     
  }
}

void EndReflow()
{
  if ( state == 2 )
  {
    SetRelayFrequency( 0 );
    state = 3;

    Buzzer( 1000, 1000 );

    DrawHeading( "DONE!", WHITE, BLACK );

    ShowMenuOptions( false );

    StartFan( false );
  }
}

void SetDefaults()
{
  // Default settings values
  set.valid = true;
  set.power = 1;
  set.paste = 0;
  set.useFan = false;
  set.lookAhead = 12;
  set.lookAheadWarm = 12;
  set.startFullBlast = false;
  set.tempOffset = 0;
}

void ResetSettingsToDefault()
{
  // set default values again and save
  SetDefaults();
  // flash_store.write(set); // @todo abstract

  // load the default paste
  SetCurrentGraph( set.paste );

  // show settings
  settings_pointer = 0;
  ShowSettings();
}


/*
 * OVEN CHECK CODE NOT CURRENTLY USED
 * START
 */
void StartOvenCheck()
{
#ifdef DEBUG
  Serial.println("Oven Check Start Temp " + String( currentTemp ) );
#endif
  state = 16;
  calibrationSeconds = 0;
  calibrationState = 0;
  calibrationStatsUp = 0;
  calibrationStatsDown = 300;
  calibrationUpMatch = false;
  calibrationDownMatch = false;
  calibrationDropVal = 0;
  calibrationRiseVal = 0;
  SetRelayFrequency( 0 );
  StartFan( false );

#ifdef DEBUG
  Serial.println("Running Oven Check");
#endif
  tft.fillScreen(BLACK);
  tft.setTextColor( CYAN, BLACK );
  tft.setTextSize(2);
  tft.setCursor( 20, 20 );
  tft.println( "OVEN CHECK" );

  tft.setTextColor( YELLOW, BLACK );
  tft.setCursor( 20, 40 ); 
  tft.println( CurrentGraph().n );
  tft.setCursor( 20, 60 );
  tft.println( String(CurrentGraph().tempDeg) +"deg");

  ShowMenuOptions( true );
}

void ShowOvenCheck()
{
  state = 15;
  SetRelayFrequency( 0 );
  StartFan( true );

  int posY = 50;
  int incY = 20;

#ifdef DEBUG
  Serial.println("Oven Check");
#endif
  
  tft.fillScreen(BLACK);
  tft.setTextColor( CYAN, BLACK );
  tft.setTextSize(2);
  tft.setCursor( 20, 20 );
  tft.println( "OVEN CHECK" );


  tft.setTextColor( WHITE, BLACK );

  ShowMenuOptions( true );

  tft.setTextSize(2);
  tft.setCursor( 0, 60 );
  tft.setTextColor( YELLOW, BLACK );

  tft.println( " Oven Check allows");
  tft.println( " you to measure your");
  tft.println( " oven's rate of heat");
  tft.println( " up & cool down to");
  tft.println( " see if your oven");
  tft.println( " is powerful enough");
  tft.println( " to reflow your ");
  tft.println( " selected profile.");
  tft.println( "");

  tft.setTextColor( YELLOW, RED );
  tft.println( " EMPTY YOUR OVEN FIRST!");
}

/*
 * END
 * OVEN CHECK CODE NOT CURRENTLY USED/WORKING
 */


/*
 * Lots of UI code here
 */

void ShowResetDefaults()
{
  tft.fillScreen(BLACK);
  tft.setTextColor( WHITE, BLACK );
  tft.setTextSize(2);
  tft.setCursor( 20, 90 );
      
  tft.setTextColor( WHITE, BLACK );
  tft.print( "RESET SETTINGS" );
  tft.setTextSize(3);
  tft.setCursor( 20, 120 );
  tft.println( "ARE YOU SURE?" );

  state = 13;
  ShowMenuOptions( false );

  tft.setTextSize(1);
  tft.setTextColor( GREEN, BLACK );
  tft.fillRect( 0, tft.height()-40, tft.width(), 40, BLACK );
  println_Center( tft, "Settings restore cannot be undone!", tft.width() / 2, tft.height() - 20 );
}
 
void UpdateSettingsFan( int posY )
{
  tft.fillRect( 15,  posY-5, 200, 20, BLACK );
  tft.setTextColor( WHITE, BLACK );
  tft.setCursor( 20, posY );

  tft.setTextColor( WHITE, BLACK );
  tft.print( "USE FAN " );
  tft.setTextColor( YELLOW, BLACK );
    
  if ( set.useFan )
  {
    tft.println( "ON" );
  }
  else
  {
    tft.println( "OFF" );
  }
  tft.setTextColor( WHITE, BLACK );
}


void UpdateSettingsStartFullBlast( int posY )
{
  tft.fillRect( 15,  posY-5, 240, 20, BLACK );
  tft.setTextColor( WHITE, BLACK );
  tft.setCursor( 20, posY );
  tft.print( "START RAMP 100% " );
  tft.setTextColor( YELLOW, BLACK );
    
  if ( set.startFullBlast )
  {
    tft.println( "ON" );
  }
  else
  {
    tft.println( "OFF" );
  }
  tft.setTextColor( WHITE, BLACK );
}

void UpdateSettingsPower( int posY )
{
  tft.fillRect( 15,  posY-5, 240, 20, BLACK );
  tft.setTextColor( WHITE, BLACK );
  
  tft.setCursor( 20, posY );
  tft.print( "POWER ");
  tft.setTextColor( YELLOW, BLACK );
  tft.println( String( round((set.power * 100))) +"%");
  tft.setTextColor( WHITE, BLACK );
}

void UpdateSettingsLookAhead( int posY )
{
  tft.fillRect( 15,  posY-5, 260, 20, BLACK );
  tft.setTextColor( WHITE, BLACK );
  
  tft.setCursor( 20, posY );
  tft.print( "GRAPH LOOK AHEAD ");
  tft.setTextColor( YELLOW, BLACK );
  tft.println( String( set.lookAhead) );
  tft.setTextColor( WHITE, BLACK );
}

//void UpdateSettingsLookAheadWarm( int posY )
//{
//  tft.fillRect( 15,  posY-5, 220, 20, BLACK );
//  tft.setTextColor( WHITE, BLACK );
//  
//  tft.setCursor( 20, posY );
//  tft.print( "GRAPH LOOK AHEAD  ");
//  tft.setTextColor( YELLOW, BLACK );
//  tft.println( String( set.lookAheadWarm) );
//  tft.setTextColor( WHITE, BLACK );
//}

void UpdateSettingsTempOffset( int posY )
{
  tft.fillRect( 15,  posY-5, 220, 20, BLACK );
  tft.setTextColor( WHITE, BLACK );
  
  tft.setCursor( 20, posY );
  tft.print( "TEMP OFFSET ");
  tft.setTextColor( YELLOW, BLACK );
  tft.println( String( set.tempOffset) );
  tft.setTextColor( WHITE, BLACK );
}



/*
 * Button press code here
 */

long nextButtonPress = 0;

void button0Press()
{
  // @todo use state machine instead, use flags for start reflow, remove from button callback scope
  // so that these are all in global scope and can be refactored,methodized and unit tested
  // maybe add enums for states for clarity
  if ( nextButtonPress < millis() )
  {
    nextButtonPress = millis() + 20; 
    Buzzer( 2000, 50 );
    
    if ( state == 10 )
    {
      StartWarmup();
    }
    else if ( state == 1 || state == 2 || state == 16 )
    {
      AbortReflow();
    }
    else if ( state == 3 )
    {
      ShowMenu();
    }
    else if ( state == 11 )
    {
      if ( settings_pointer == 0 )  // change paste
      {
        settings_pointer = set.paste;
        ShowPaste();
      }
      else if ( settings_pointer == 1 )  // switch fan use
      {
        set.useFan = !set.useFan;

        UpdateSettingsFan( 70 );
      }
      else if ( settings_pointer == 2 ) // change power
      {
        set.power += 0.1;
        if ( set.power > 1.55 )
          set.power = 0.5;

        UpdateSettingsPower( 90 );
      }
      else if ( settings_pointer == 3 ) // change lookahead for reflow
      {
        set.lookAhead += 1;
        if ( set.lookAhead > 15 )
          set.lookAhead = 1;

        UpdateSettingsLookAhead( 110 );
      }
      else if ( settings_pointer == 4 ) // change temp probe offset
      {
        set.tempOffset += 1;
        if ( set.tempOffset > 15 )
          set.tempOffset = -15;

        UpdateSettingsTempOffset( 130 );
      }
      else if ( settings_pointer == 5 ) // change use full power on initial ramp
      {
        set.startFullBlast = !set.startFullBlast;

        UpdateSettingsStartFullBlast( 150 );
      }
      else if ( settings_pointer == 6 ) // reset defaults
      {
        ShowResetDefaults();
      }
    }
    else if ( state == 12 )
    {
      if ( set.paste != settings_pointer )
      {
        set.paste = settings_pointer;
        SetCurrentGraph( set.paste );
        ShowPaste();
      }
    }
    else if ( state == 13 )
    {
      ResetSettingsToDefault();
    }
    else if ( state == 15 )
    {
      StartOvenCheck();
    }
  }
}

void button1Press()
{
  if ( nextButtonPress < millis() )
  {
    nextButtonPress = millis() + 20;
    Buzzer( 2000, 50 );

    if ( state == 10 )
    {
      settings_pointer = 0;
      ShowSettings();
    }
    else if ( state == 11 ) // leaving settings so save
    {
      // save data in flash
      // flash_store.write(set); // @todo abstract
      ShowMenu();
    }
    else if ( state == 12 || state == 13 )
    {
      settings_pointer = 0;
      ShowSettings();
    }
    else if ( state == 15 ) // cancel oven check
    {
      ShowMenu();
    }
  }
}

void button2Press()
{
  if ( nextButtonPress < millis() )
  {
    nextButtonPress = millis() + 20;
    Buzzer( 2000, 50 );

    if ( state == 11 )
    {
      settings_pointer = constrain( settings_pointer -1, 0, 6 );
      ShowMenuOptions( false );
      //UpdateSettingsPointer();
    }
    else if ( state == 12 )
    {
      settings_pointer = constrain( settings_pointer -1, 0, ELEMENTS(solderPaste)-1 );
      UpdateSettingsPointer();
    }
  }
}


void button3Press()
{
  if ( nextButtonPress < millis() )
  {
    nextButtonPress = millis() + 20;
    Buzzer( 2000, 50 );

    if ( state == 10 )
    {
      ShowOvenCheck();
    }
    else if ( state == 11 )
    {
      settings_pointer = constrain( settings_pointer +1, 0, 6 );
      ShowMenuOptions( false );
      //UpdateSettingsPointer();
    }
    else if ( state == 12 )
    {
      settings_pointer = constrain( settings_pointer +1, 0, ELEMENTS(solderPaste)-1 );
      UpdateSettingsPointer();
    }
  }
}

/*
 * Graph drawing code here
 * Special thanks to Kris Kasprzak for his free graphing code that I derived mine from
 * https://www.youtube.com/watch?v=YejRbIKe6e0
 */

void SetupGraph(Adafruit_ILI9341 &d, double x, double y, double gx, double gy, double w, double h, double xlo, double xhi, double xinc, double ylo, double yhi, double yinc, String title, String xlabel, String ylabel, unsigned int gcolor, unsigned int acolor, unsigned int tcolor, unsigned int bcolor )
{
  double ydiv, xdiv;
  double i;
  int temp;
  int rot, newrot;

    ox = (x - xlo) * ( w) / (xhi - xlo) + gx;
    oy = (y - ylo) * (gy - h - gy) / (yhi - ylo) + gy;
    // draw y scale
    for ( i = ylo; i <= yhi; i += yinc)
    {
      // compute the transform
      temp =  (i - ylo) * (gy - h - gy) / (yhi - ylo) + gy;

      if (i == ylo) {
        d.drawLine(gx, temp, gx + w, temp, acolor);
      }
      else {
        d.drawLine(gx, temp, gx + w, temp, gcolor);
      }

      d.setTextSize(1);
      d.setTextColor(tcolor, bcolor);
      d.setCursor(gx-25, temp);
      println_Right( d, String(round(i)), gx-25, temp );
    }
    
    // draw x scale
    for (i = xlo; i <= xhi; i += xinc)
    {
      temp =  (i - xlo) * ( w) / (xhi - xlo) + gx;
      if (i == 0)
      {
        d.drawLine(temp, gy, temp, gy - h, acolor);
      }
      else
      {
        d.drawLine(temp, gy, temp, gy - h, gcolor);
      }

      d.setTextSize(1);
      d.setTextColor(tcolor, bcolor);
      d.setCursor(temp, gy + 10);
      println_Center(d, String(round(i)), temp, gy + 10 );
    }

    //now draw the labels
    d.setTextSize(2);
    d.setTextColor(tcolor, bcolor);
    d.setCursor(gx , gy - h - 30);
    d.println(title);

    d.setTextSize(1);
    d.setTextColor(acolor, bcolor);
    d.setCursor(w - 25 , gy - 10);
    d.println(xlabel);

    tft.setRotation(0);
    d.setTextSize(1);
    d.setTextColor(acolor, bcolor);
    d.setCursor(w - 116, 34 );
    d.println(ylabel);
    tft.setRotation(1);
}

void Graph(Adafruit_ILI9341 &d, double x, double y, double gx, double gy, double w, double h )
{
  // recall that ox and oy are initialized as static above
  x =  (x - graphRangeMin_X) * ( w) / (graphRangeMax_X - graphRangeMin_X) + gx;
  y =  (y - graphRangeMin_Y) * (gy - h - gy) / (graphRangeMax_Y - graphRangeMin_Y) + gy;

  oy = min( oy, y );
  y = min( y, 220 ); // bottom of graph!

  d.drawLine(ox, oy, x, y, currentPlotColor );
  d.drawLine(ox, oy + 1, x, y + 1, currentPlotColor);
  d.drawLine(ox, oy - 1, x, y - 1, currentPlotColor);
  ox = x;
  oy = y;
}

void GraphDefault(Adafruit_ILI9341 &d, double x, double y, double gx, double gy, double w, double h, unsigned int pcolor )
{
  // recall that ox and oy are initialized as static above
  x =  (x - graphRangeMin_X) * ( w) / (graphRangeMax_X - graphRangeMin_X) + gx;
  y =  (y - graphRangeMin_Y) * (gy - h - gy) / (graphRangeMax_Y - graphRangeMin_Y) + gy;

  //Serial.println( oy );
  d.drawLine(ox, oy, x, y, pcolor);
  d.drawLine(ox, oy + 1, x, y + 1, pcolor);
  d.drawLine(ox, oy - 1, x, y - 1, pcolor);
  ox = x;
  oy = y;
}

char* string2char(String command)
{
    if(command.length()!=0)
    {
        char *p = const_cast<char*>(command.c_str());
        return p;
    }
}

void println_Center( Adafruit_ILI9341 &d, String heading, int centerX, int centerY )
{
    int x = 0;
    int y = 0;
    int16_t  x1, y1;
    uint16_t ww, hh;

    d.getTextBounds( string2char(heading), x, y, &x1, &y1, &ww, &hh );
    d.setCursor( centerX - ww/2 + 2, centerY - hh / 2);
    d.println( heading );   
}

void println_Right( Adafruit_ILI9341 &d, String heading, int centerX, int centerY )
{
    int x = 0;
    int y = 0;
    int16_t  x1, y1;
    uint16_t ww, hh;

    d.getTextBounds( string2char(heading), x, y, &x1, &y1, &ww, &hh );
    d.setCursor( centerX + ( 18 - ww ), centerY - hh / 2);
    d.println( heading );
}


void tester(){
  currentTemp = 28;
  // state = 2; // start
  // if(state != 2) StartWarmup();
}

void updateTemp(){
  results = ads.readADC_Differential_0_1();
  mV      = results * multiplier;
  Tsense  = TC_K.Temp_C(mV, Tcjc); // C compensated
  currentTemp = round(Tsense + set.tempOffset);
  Serial.println(currentTemp);
}

void initTemp(){
  ads.setGain(GAIN_SIXTEEN);    // 16x gain  +/- 0.256V  1 bit = 0.125mV  0.0078125mV  
  ads.begin();
  Wire.begin(_SDA,_SCL);
}
