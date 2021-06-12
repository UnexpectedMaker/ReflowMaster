/*
  ---------------------------------------------------------------------------
  Reflow Master Control - v2.02 - 20/12/2020.

  AUTHOR/LICENSE:
  Created by Seon Rozenblum - seon@unexpectedmaker.com
  Copyright 2016 License: GNU GPL v3 http://www.gnu.org/licenses/gpl-3.0.html

  LINKS:
  Project home: github.com/unexpectedmaker/reflowmaster
  Blog: unexpectedmaker.com

  PURPOSE:
  This controller is the software that runs on the Reflow Master toaster oven controller made by Unexpected Maker

  HISTORY:
  01/08/2018 v1.0   - Initial release.
  13/08/2018 v1.01  - Settings UI button now change to show SELECT or CHANGE depending on what is selected
  27/08/2018 v1.02  - Added tangents to the curve for ESP32 support, Improved graph curves, fixed some UI glitches, made end graph time value be the end profile time
  28/08/2018 v1.03  - Added some graph smoothing
  20/05/2019 v1.04  - Increased max curve to support profiles up to 8mins
                    - Added fan on time after reflow for cooldown settings
                    - Added extra profile for Ju Feng Medium temp paste
  07/09/2019 v1.05  - Fixed some bugs, Thanks Tablatronix!
  16/09/2019 v1.06  - Fixed probe offset temp not changing in settings
  02/07/2020 v1.07  - Cleaned up some Fan control and tracking code
                    - Cleaned up some debug messages
  13/07/2020 v1.08  - Fixed bug in DEBUG mode
  17/07/2020 v1.09  - Added TC error display in menu if an error is found
                    - Prevent starting reflow or oven check if there is a TC error
                    - Code refactor for easier reading? maybe?
  19/12/2020 v2.00  - Added baking mode
                    - Lots of code cleanup and simplification
                    - Included the more obscure libraries inside the sketch
  20/12/2020 v2.01  - Forgot to hookup minBakeTemp, minBakeTime, maxBakeTemp, maxBakeTemp variables to buttons
                    - Increased max bake time to 3 hours
                    - Added long press for Bake Time & Temp to quickly change values, clamped at max, so it won't loop
                    - Oven was not turned off correctly after the bake ended
  20/12/2020 v2.02  - Prevent going into the Bake menu when there is a thermocouple error
                    - Set FAN to Off by default
                    - Fixed some incorrect comments 
                    - Made TC error more visible
  21/12/2020 v2.02  - Fixed UI glitch in main menu with TC error display
  ---------------------------------------------------------------------------
*/

/*
   NOTE: This is a work in progress...
*/

#include <SPI.h>
#include "spline.h"
#include "MAX31855.h"
#include "OneButton.h" // Add from Library Manager
#include "ReflowMasterProfile.h"
#include "FlashStorage.h"

#include "tft_display.h"
#include "menu_settings.h"

// used to obtain the size of an array of any type
#define ELEMENTS(x)   (sizeof(x) / sizeof(x[0]))

// used to show or hide serial debug output
#define DEBUG

// TFT SPI pins
#define TFT_DC 0
#define TFT_CS 3
#define TFT_RESET 1

// MAX 31855 Pins
#define MAXDO   11
#define MAXCS   10
#define MAXCLK  12

#define BUTTON0 A0 // menu buttons
#define BUTTON1 A1 // menu buttons
#define BUTTON2 A2 // menu buttons
#define BUTTON3 A3 // menu buttons

#define BUZZER A4  // buzzer
#define RELAY 5    // relay control
#define FAN A5     // fan control

// UI and runtime states
enum states {
  BOOT = 0,
  WARMUP = 1,
  REFLOW = 2,
  FINISHED = 3,
  MENU = 10,
  SETTINGS = 11,
  SETTINGS_PASTE = 12,
  SETTINGS_RESET = 13,
  OVENCHECK = 15,
  OVENCHECK_START = 16,
  BAKE_MENU = 20,
  BAKE = 21,
  BAKE_DONE = 22,
  ABORT = 99
} state;

const String ver = "2.02";
bool newSettings = false;

// TC variables
unsigned long nextTempRead;
unsigned long nextTempAvgRead;
int avgReadCount = 0;

unsigned long keepFanOnTime = 0;

double timeX = 0;
double tempOffset = 60;

// Bake variables
long currentBakeTime = 0; // Used to countdown the bake time
byte currentBakeTimeCounter = 0;
int lastTempDirection = 0;
long minBakeTime = 600; // 10 mins in seconds
long maxBakeTime = 10800; // 3 hours in seconds
float minBakeTemp = 45; // 45 Degrees C
float maxBakeTemp = 100; // 100 Degrees C

// Current index in the settings screen
int settings_pointer = 0;

// Initialise an array to hold 5 profiles
// Increase this array if you plan to add more
ReflowGraph solderPaste[5];
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
int tcError = 0;
bool tcWasError = false;
bool tcWasGood = true;
float currentDuty = 0;
float currentTemp = 0;
float cachedCurrentTemp = 0;
float currentTempAvg = 0;
float lastTemp = -1;
float currentDetla = 0;
unsigned int currentPlotColor = GREEN;
bool isCuttoff = false;
bool isFanOn = false;
float lastWantedTemp = -1;
int buzzerCount = 5;

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

// UI button positions and sizes
const unsigned int buttonPosY[] = { 19, 74, 129, 184 };
const unsigned int buttonHeight = 16;
const unsigned int buttonWidth = 4;
const uint32_t ButtonColor[] = { GREEN, RED, BLUE, YELLOW };
const unsigned int NumButtons = ELEMENTS(buttonPosY);
String buttonText[NumButtons];

// Initiliase a reference for the settings file that we store in flash storage
Settings set;
// Initialise flash storage
FlashStorage(flash_store, Settings);

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
  */
  float baseGraphX[7] = { 1, 90, 180, 210, 240, 270, 300 }; // time
  float baseGraphY[7] = { 27, 90, 130, 138, 165, 138, 27 }; // value

  solderPaste[0] = ReflowGraph( "CHIPQUIK", "No-Clean Sn42/Bi57.6/Ag0.4", 138, baseGraphX, baseGraphY, ELEMENTS(baseGraphX) );

  float baseGraphX1[7] = { 1, 90, 180, 225, 240, 270, 300 }; // time
  float baseGraphY1[7] = { 25, 150, 175, 190, 210, 125, 50 }; // value

  solderPaste[1] = ReflowGraph( "CHEMTOOLS L", "No Clean 63CR218 Sn63/Pb37", 183, baseGraphX1, baseGraphY1, ELEMENTS(baseGraphX1) );

  float baseGraphX2[6] = { 1, 75, 130, 180, 210, 250 }; // time
  float baseGraphY2[6] = { 25, 150, 175, 210, 150, 50 }; // value

  solderPaste[2] = ReflowGraph( "CHEMTOOLS S", "No Clean 63CR218 Sn63/Pb37", 183, baseGraphX2, baseGraphY2, ELEMENTS(baseGraphX2) );

  float baseGraphX3[7] = { 1, 60, 120, 160, 210, 260, 310 }; // time
  float baseGraphY3[7] = { 25, 105, 150, 150, 220, 150, 20 }; // value

  solderPaste[3] = ReflowGraph( "DOC SOLDER", "No Clean Sn63/Pb36/Ag2", 187, baseGraphX3, baseGraphY3, ELEMENTS(baseGraphX3) );

  float baseGraphX4[6] = { 1, 90, 165, 225, 330, 360 }; // time
  float baseGraphY4[6] = { 25, 150, 175, 235, 100, 25 }; // value

  solderPaste[4] = ReflowGraph( "CHEMTOOLS SAC305 HD", "Sn96.5/Ag3.0/Cu0.5", 225, baseGraphX4, baseGraphY4, ELEMENTS(baseGraphX4) );

  //TODO: Think of a better way to initalise these baseGraph arrays to not need unique array creation
}

// Obtain the temp value of the current profile at time X
int GetGraphValue( int x )
{
  return ( CurrentGraph().reflowTemp[x] );
}

int GetGraphTime( int x )
{
  return ( CurrentGraph().reflowTime[x] );
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
  graphRangeMax_Y = CurrentGraph().MaxTempValue() + 5; // extra padding
  graphRangeMin_Y = CurrentGraph().MinTempValue();

  debug_print("Setting Paste: ");
  debug_println( CurrentGraph().n );
  debug_println( CurrentGraph().t );

  // Initialise the spline for the profile to allow for smooth graph display on UI
  timeX = 0;
  baseCurve.setPoints(CurrentGraph().reflowTime, CurrentGraph().reflowTemp, CurrentGraph().reflowTangents, CurrentGraph().len);
  baseCurve.setDegree( Hermite );

  // Re-interpolate data based on spline
  for ( int ii = 0; ii <= graphRangeMax_X; ii += 1 )
  {
    solderPaste[ currentGraphIndex ].wantedCurve[ii] = baseCurve.value(ii);
  }

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
  }
}

void setup()
{
  // Setup all GPIO
  pinMode( BUZZER, OUTPUT );
  pinMode( RELAY, OUTPUT );
  pinMode( FAN, OUTPUT );
  pinMode( BUTTON0, INPUT );
  pinMode( BUTTON1, INPUT );
  pinMode( BUTTON2, INPUT );
  pinMode( BUTTON3, INPUT );

  // Turn of Green Debug LED
  pinMode( 13, INPUT );

  // Turn off the SSR - duty cycle of 0
  SetRelayFrequency( 0 );

#ifdef DEBUG
  Serial.begin(115200);
#endif

  // just wait a bit before we try to load settings from FLASH
  delay(500);

  // load settings from FLASH
  set = flash_store.read();

  // If no settings were loaded, initialise data and save
  if ( !set.valid )
  {
    SetDefaults();
    newSettings = true;
    flash_store.write(set);
  }

  // Attatch button IO for OneButton
  button0.attachClick(button0Press);
  button1.attachClick(button1Press);
  button2.attachClick(button2Press);
  button3.attachClick(button3Press);

  // New long press buttons for changing bake time and temp values
  button2.attachLongPressStart(button2LongPressStart);
  button2.attachDuringLongPress(button2LongPress);
  button3.attachLongPressStart(button3LongPressStart);
  button3.attachDuringLongPress(button3LongPress);

  debug_println("TFT Begin...");

  // Start up the TFT and show the boot screen
  tft.begin(32000000);
  BootScreen();
  BuzzerStart();

  delay(200);

  // Start up the MAX31855
  debug_println("Thermocouple Begin...");
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
}

// Helper method to display the temperature on the TFT
void DisplayTemp( bool center = false )
{
  if ( center )
  {
    tft.setTextColor( YELLOW, BLACK );
    tft.setTextSize(5);
    println_Center( tft, "  " + String( round( currentTemp ) ) + "c  ", tft.width() / 2, ( tft.height() / 2 ) + 10 );
  }
  else
  {
    if ( round( currentTemp ) != round( cachedCurrentTemp ) )
    {
      // display the previous temp in black to clear it
      tft.setTextColor( BLACK, BLACK );
      tft.setTextSize(6);
      tft.setCursor( 20,  ( tft.height() / 2 ) - 25 );
      tft.println( String( round( cachedCurrentTemp ) ) + "c" );

      // display the new temp in green
      tft.setTextColor( GREEN, BLACK );
      tft.setCursor( 20,  ( tft.height() / 2 ) - 25 );
      tft.println( String( round( currentTemp ) ) + "c" );

      // cache the current temp
      cachedCurrentTemp = currentTemp;
    }
  }
}

void loop()
{
  // Used by OneButton to poll for button inputs
  button0.tick();
  button1.tick();
  button2.tick();
  button3.tick();

  // Current activity state machine
  if ( state == BOOT ) // BOOT
  {
    return;
  }
  else if ( state == WARMUP ) // WARMUP - We sit here until the probe reaches the starting temp for the profile
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
        avgReadCount = 0;
        currentTempAvg = 0;

        StartReflow();
      }
      else if ( currentTemp > 0 )
      {
        // Show the current probe temp so we can watch as it reaches the minimum starting value
        DisplayTemp( true );
      }
    }
    return;
  }
  else if ( state == FINISHED ) // FINISHED
  {
    // do we keep the fan on after reflow finishes to help cooldown?
    KeepFanOnCheck();
    return;
  }
  else if ( state == SETTINGS || state == SETTINGS_PASTE || state == SETTINGS_RESET ) // SETTINGS
  {
    // Currently not used
    return;
  }
  else if ( state == MENU ) // MENU
  {
    if ( nextTempRead < millis() )
    {
      nextTempRead = millis() + 1000;

      // We show the current probe temp in the men screen just for info
      ReadCurrentTemp();

      if ( tcError > 0 )
      {
        // Clear TC Temp background if there was one!
        if ( tcWasGood )
        {
          tcWasGood = false;
          tft.setTextColor( BLACK, BLACK );
          tft.setTextSize(6);
          tft.setCursor( 20,  ( tft.height() / 2 ) - 25 );
          tft.println( String( round( cachedCurrentTemp ) ) + "c" );

          tft.fillRect( 5, tft.height() / 2 - 20, 180, 31, RED );
        }
        tcWasError = true;
        tft.setTextColor( WHITE, RED );
        tft.setTextSize(3);        
        tft.setCursor( 10, ( tft.height() / 2 ) - 15 );
        tft.println( "TC ERR #" + String( tcError ) );
      }
      else if ( currentTemp > 0 )
      {
        // Clear error background if there was one!
        if ( tcWasError )
        {
          cachedCurrentTemp = 0;
          tcWasError = false;
          tft.fillRect( 0, tft.height() / 2 - 20, 200, 32, BLACK );
        }
        tcWasGood = true;
        DisplayTemp();
      }
    }

    // do we keep the fan on after reflow finishes to help cooldown?
    KeepFanOnCheck();
    return;
  }
  else if ( state == BAKE_MENU || state == BAKE_DONE )
  {
    // do nothing in these states
  }
  else if ( state == BAKE )
  {
    if ( currentBakeTime > 0 )
    {
      if ( nextTempAvgRead < millis() )
      {
        nextTempAvgRead = millis() + 100;
        ReadCurrentTempAvg();
      }

      if ( nextTempRead < millis() )
      {
        nextTempRead = millis() + 1000;

        // Set the temp from the average
        currentTemp = ( currentTempAvg / avgReadCount );
        // clear the variables for next run
        avgReadCount = 0;
        currentTempAvg = 0;

        // Control the SSR
        MatchTemp_Bake();

        if ( currentTemp > 0 )
          currentBakeTime--;
      }

      UpdateBake();
    }
    else
    {
      BakeDone();
    }
  }
  else if ( state == OVENCHECK )
  {
    // Currently not used
    return;
  }
  else if ( state == OVENCHECK_START ) // calibration - not currently used
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

          println_Center( tft, "TARGET " + String( GetGraphValue(1) ) + "c in " + String( GetGraphTime(1) ) + "s", tft.width() / 2, ( tft.height() - 18 ) );
        }
        else if ( calibrationState == 1 )
        {
          println_Center( tft, "COOL DOWN LEVEL", tft.width() / 2, ( tft.height() / 2 ) - 15 );
          tft.fillRect( 0, tft.height() - 30, tft.width(), 30, BLACK );
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
          println_Center( tft, " " + String( calibrationSeconds ) + " secs ", tft.width() / 2, ( tft.height() / 2 ) + 20 );
        }
        tft.setTextSize(5);
        tft.setTextColor( YELLOW, BLACK );
        println_Center( tft, " " + String( round( currentTemp ) ) + "c ", tft.width() / 2, ( tft.height() / 2 ) + 65 );


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
          tft.print( "REACHED " + String( round(calibrationRiseVal * 100) ) + "%") ;
        }

        tft.setTextColor( WHITE, BLACK );
        tft.setCursor( 20, ( tft.height() / 2 ) + 20 );
        tft.print( "DROP " );
        if ( calibrationDownMatch )
        {
          tft.setTextColor( GREEN, BLACK );
          tft.print( "PASS" );
          tft.setTextColor( WHITE, BLACK );
          tft.print( "DROPPED " + String( round(calibrationDropVal * 100) ) + "%") ;
        }
        else
        {
          tft.setTextColor( ORANGE, BLACK );
          tft.print( "FAIL " );
          tft.setTextColor( WHITE, BLACK );
          tft.print( "DROPPED " + String( round(calibrationDropVal * 100) ) + "%") ;

          tft.setTextColor( WHITE, BLACK );
          tft.setCursor( 20, ( tft.height() / 2 ) + 40 );
          tft.print( "RECOMMEND ADDING FAN") ;
        }
      }
    }
  }
  else // state is REFLOW
  {
    if ( nextTempAvgRead < millis() )
    {
      nextTempAvgRead = millis() + 100;
      ReadCurrentTempAvg();
    }
    if ( nextTempRead < millis() )
    {
      nextTempRead = millis() + 1000;

      // Set the temp from the average
      currentTemp = ( currentTempAvg / avgReadCount );
      // clear the variables for next run
      avgReadCount = 0;
      currentTempAvg = 0;

      // Control the SSR
      MatchTemp();

      if ( currentTemp > 0 )
      {
        timeX++;

        if ( timeX > CurrentGraph().completeTime )
        {
          EndReflow();
        }
        else
        {
          Graph(tft, timeX, currentTemp, 30, 220, 270, 180 );

          if ( timeX < CurrentGraph().fanTime )
          {
            float wantedTemp = CurrentGraph().wantedCurve[ (int)timeX ];
            DrawHeading( String( round( currentTemp ) ) + "/" + String( (int)wantedTemp ) + "c", currentPlotColor, BLACK );
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
  analogWrite( RELAY, constrain( round( currentDuty ), 0, 255) );

  debug_print("RELAY Duty Cycle: ");
  debug_println( String( ( currentDuty / 256.0 ) * 100) + "%" + " Using Settings Power: " + String( round( set.power * 100 )) + "%" );
}

/*
   SOME CALIBRATION CODE THAT IS CURRENTLY USED FOR THE OVEN CHECK SYSTEM
   Oven Check currently shows you hoe fast your oven can reach the initial pre-soak temp for your selected profile
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
      debug_println("Cal Heat Up Speed " + String( calibrationSeconds ) );

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
      debug_println("Cal Cool Down Temp " + String( currentTemp ) );

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
   END
   SOME CALIBRATION CODE THAT IS CURRENTLY USED FOR THE OVEN CHECK SYSTEM
*/

void KeepFanOnCheck()
{
  // do we keep the fan on after reflow finishes to help cooldown?
  if ( set.useFan && millis() < keepFanOnTime )
    StartFan( true );
  else
    StartFan( false );
}

void ReadCurrentTempAvg()
{
  int status = tc.read();

  if (status != 0 )
  {
    tcError = status;
    debug_print("TC Read Error Status: ");
    debug_println( status );
  }
  else
  {
    tcError = 0;
    currentTempAvg += tc.getTemperature() + set.tempOffset;
    avgReadCount++;
  }
}

// Read the temp probe
void ReadCurrentTemp()
{
  int status = tc.read();
  if (status != 0 )
  {
    tcError = status;
    debug_print("TC Read Error Status: ");
    debug_println( status );
  }
  else
  {
    tcError = 0;
    currentTemp = tc.getTemperature() + set.tempOffset;
    currentTemp =  constrain(currentTemp, -10, 350);

    debug_print("TC Read: ");
    debug_println( currentTemp );
  }
}

void MatchTemp_Bake()
{
  float duty = 0;
  float tempDiff = 0;
  float perc = 0;

  float wantedTemp = set.bakeTemp - set.bakeTempGap;

  // If the last temperature direction change is dropping, we want to aim for the bake temp without the gap
  if ( currentTemp < set.bakeTemp && lastTempDirection < 0 )
    wantedTemp = set.bakeTemp + 1; // We need to ramp a little more as we've likely been off for a while relying on thermal mass to hold temp

  debug_print( "Bake T: " );
  debug_print( currentBakeTime);
  debug_print( "  Current: " );
  debug_print( currentTemp );
  debug_print( "  LastDir: " );
  debug_print( lastTempDirection );

  if ( currentTemp >= wantedTemp)
  {
    duty = 0;
    perc = 0;

    if ( set.useFan )
    {
      if (currentTemp > set.bakeTemp + 3)
        StartFan( true );
      else
        StartFan( false );
    }
  }
  else
  {
    // if current temp is less thn half of wanted, then boost at 100%
    if ( currentTemp < ( wantedTemp / 2 ) )
      perc = 1;
    else
    {
      tempDiff = wantedTemp - currentTemp;
      perc = tempDiff / wantedTemp;
    }

    duty = 256 * perc;
  }

  debug_print( "  Perc: " );
  debug_print( perc );
  debug_print( "  Duty: " );
  debug_print( duty );
  debug_print( "  " );

  duty = constrain( duty, 0, 256 );
  currentPlotColor = GREEN;
  SetRelayFrequency( duty );
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
    if ( timeX < CurrentGraph().reflowTime[2] )
      wantedTemp = CurrentGraph().wantedCurve[ (int)timeX + set.lookAheadWarm ];
    else
      wantedTemp = CurrentGraph().wantedCurve[ (int)timeX + set.lookAhead ];

    wantedDiff = (wantedTemp - lastWantedTemp );
    lastWantedTemp = wantedTemp;

    tempDiff = ( currentTemp - lastTemp );
    lastTemp = currentTemp;

    perc = wantedDiff - tempDiff;

    debug_print( "T: " );
    debug_print( timeX );

    debug_print( "  Current: " );
    debug_print( currentTemp );

    debug_print( "  Wanted: " );
    debug_print( wantedTemp );

    debug_print( "  T Diff: " );
    debug_print( tempDiff  );

    debug_print( "  W Diff: " );
    debug_print( wantedDiff );

    debug_print( "  Perc: " );
    debug_print( perc );

    isCuttoff = false;

    // have to passed the fan turn on time?
    if ( !isFanOn && timeX >= CurrentGraph().fanTime )
    {
      debug_print( "COOLDOWN: " );

      // If we are usng the fan, turn it on
      if ( set.useFan )
      {
        DrawHeading( "COOLDOWN!", GREEN, BLACK );
        Buzzer( 2000, 2000 );

        StartFan ( true );
      }
      else // otherwise YELL at the user to open the oven door
      {
        if ( buzzerCount > 0 )
        {
          DrawHeading( "OPEN OVEN", RED, BLACK );
          Buzzer( 2000, 2000 );
          buzzerCount--;
        }
      }
    }
  }
  else
  {
    // YELL at the user to open the oven door
    if ( !isCuttoff && set.useFan )
    {
      DrawHeading( "OPEN OVEN", GREEN, BLACK );
      Buzzer( 2000, 2000 );

      debug_print( "CUTOFF: " );
    }
    else if ( !set.useFan )
    {
      StartFan ( false );
    }

    isCuttoff = true;
  }

  currentDetla = (wantedTemp - currentTemp);

  debug_print( "  Delta: " );
  debug_print( currentDetla );

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

  debug_print("  Base: ");
  debug_print( base );
  debug_print( " -> " );

  duty = base + ( 172 * perc );
  duty = constrain( duty, 0, 256 );

  // override for full blast at start only if the current Temp is less than the wanted Temp, and it's in the ram before pre-soak starts.
  if ( set.startFullBlast && timeX < CurrentGraph().reflowTime[1] && currentTemp < wantedTemp )
    duty = 256;

  currentPlotColor = GREEN;

  SetRelayFrequency( duty );
}

void StartFan ( bool start )
{
  if ( set.useFan )
  {
    bool isOn = digitalRead( FAN );
    if ( start != isOn )
    {
      debug_print("* Use FAN? ");
      debug_print( set.useFan );
      debug_print( " Should Start? ");
      debug_println( start );

      digitalWrite ( FAN, ( start ? HIGH : LOW ) );
    }
    isFanOn = start;
  }
  else
  {
    isFanOn = false;
  }
}

void DrawHeading( String lbl, unsigned int acolor, unsigned int bcolor )
{
  tft.setTextSize(4);
  tft.setTextColor(acolor , bcolor);
  tft.setCursor(0, 0);
  tft.fillRect( 0, 0, 220, 40, BLACK );
  tft.println( String(lbl) );
}

// buzz the buzzer
void Buzzer( int hertz, int len )
{
  // Exit early if no beep is set in settings
  if (!set.beep )
    return;

  tone( BUZZER, hertz, len);
}

// Startup Tune
void BuzzerStart()
{
  tone( BUZZER, 262, 200);
  delay(210);
  tone( BUZZER, 523, 100);
  delay(150);
  tone( BUZZER, 523, 100);
  delay(150);
  noTone(BUZZER);
}


double ox , oy ;

void DrawBaseGraph()
{
  oy = 220;
  ox = 30;
  timeX = 0;

  for ( int ii = 0; ii <= graphRangeMax_X; ii += 5 )
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

  tft.drawBitmap( 115, ( tft.height() / 2 ) +20 , UM_Logo, 90, 49, WHITE);

  tft.setTextColor( GREEN, BLACK );
  tft.setTextSize(3);
  println_Center( tft, "REFLOW MASTER", tft.width() / 2, ( tft.height() / 2 ) - 38 );

  tft.setTextSize(2);
  tft.setTextColor( WHITE, BLACK );
  println_Center( tft, "unexpectedmaker.com", tft.width() / 2, ( tft.height() / 2 ) - 10 );
  tft.setTextSize(1);
  println_Center( tft, "Code v" + ver, tft.width() / 2, tft.height() - 20 );

  state = MENU;
}

void ShowMenu()
{
  state = MENU;

  cachedCurrentTemp = 0;

  SetRelayFrequency( 0 );

  set = flash_store.read();

  tft.fillScreen(BLACK);

  tft.setTextColor( WHITE, BLACK );
  tft.setTextSize(2);
  tft.setCursor( 20, 20 );
  tft.println( "CURRENT PASTE" );

  tft.setTextColor( YELLOW, BLACK );
  tft.setCursor( 20, 40 );
  tft.println( CurrentGraph().n );
  tft.setCursor( 20, 60 );
  tft.println( String(CurrentGraph().tempDeg) + "deg");

  if ( newSettings )
  {
    tft.setTextColor( CYAN, BLACK );
    tft.setCursor( 20, tft.height() - 80 );
    tft.println("Settings");
    tft.setCursor( 20, tft.height() - 55 );
    tft.println("Stomped!!");
  }

  tft.setTextSize(1);
  tft.setTextColor( WHITE, BLACK );
  tft.setCursor( 20, tft.height() - 20 );
  tft.println("Reflow Master - Code v" + String(ver));

  ShowButtonOptions( true );
}

void ShowSettings()
{
  state = SETTINGS;
  SetRelayFrequency( 0 );

  newSettings = false;

  SettingsPage::drawPage(settings_pointer);

  ShowButtonOptions( false );
  DrawSettingsPointer();

  SettingsPage::drawScrollIndicator();  // needs to be redrawn on top of buttons
}

void ShowPaste()
{
  state = SETTINGS_PASTE;
  SetRelayFrequency( 0 );

  tft.fillScreen(BLACK);

  tft.setTextColor( BLUE, BLACK );
  tft.setTextSize(2);
  tft.setCursor( 20, 20 );
  tft.println( "SWITCH PASTE" );

  tft.setTextColor( WHITE, BLACK );

  int y = 50;

  for ( size_t i = 0; i < ELEMENTS(solderPaste); i++ )
  {
    if ( i == set.paste )
      tft.setTextColor( YELLOW, BLACK );
    else
      tft.setTextColor( WHITE, BLACK );

    tft.setTextSize(2);
    tft.setCursor( 20, y );

    tft.println( String( solderPaste[i].tempDeg) + "d " + solderPaste[i].n );
    tft.setTextSize(1);
    tft.setCursor( 20, y + 17 );
    tft.println( solderPaste[i].t );
    tft.setTextColor( GREY, BLACK );

    y += 40;
  }

  ShowButtonOptions( true );
}

void DrawButton(uint8_t index, const String& str, uint32_t textColor = WHITE, uint32_t boxColor = DEFAULT_COLOR);

void DrawButton(uint8_t index, const String& str, uint32_t textColor, uint32_t boxColor)
{
  if (index >= NumButtons) return;

  // if default, set colors based on button index
  if (boxColor == DEFAULT_COLOR) boxColor = ButtonColor[index];

  tft.setTextColor(textColor, BLACK);
  tft.setTextSize(2);

  buttonText[index] = str;  // save text for reference
  tft.fillRect(tft.width() - 5, buttonPosY[index], buttonWidth, buttonHeight, boxColor);  // box
  println_Right(tft, str, tft.width() - 27, buttonPosY[index] + 9);  // text
}

void ShowButtonOptions( bool clearAll )
{
  tft.setTextColor( WHITE, BLACK );
  tft.setTextSize(2);

  if ( clearAll )
  {
    // Clear All
    for ( int i = 0; i < 4; i++ )
      tft.fillRect( tft.width() - 95,  buttonPosY[i] - 2, 95, buttonHeight + 4, BLACK );
  }

  if ( state == MENU )
  {
    DrawButton(0, "REFLOW");
    DrawButton(1, "BAKE");
    DrawButton(2, "SETTINGS");
    DrawButton(3, "OVEN CHECK");
  }
  else if ( state == SETTINGS )
  {
    DrawButton(0, SettingsPage::getButtonText(settings_pointer));
    DrawButton(1, "BACK");
    DrawButton(2, "/\\");
    DrawButton(3, "\\/");
  }
  else if ( state == SETTINGS_PASTE )
  {
    DrawButton(0, "SELECT");
    DrawButton(1, "BACK");
    DrawButton(2, "/\\");
    DrawButton(3, "\\/");

    DrawSettingsPointer();
  }
  else if ( state == SETTINGS_RESET ) // restore settings to default
  {
    DrawButton(0, "YES");
    DrawButton(1, "NO");
  }
  else if ( state == WARMUP || state == REFLOW || state == OVENCHECK_START ) // warmup, reflow, calibration
  {
    DrawButton(0, "ABORT");
  }
  else if ( state == FINISHED ) // Finished
  {
    tft.fillRect( tft.width() - 100,  buttonPosY[0] - 2, 100, buttonHeight + 4, BLACK );
    DrawButton(0, "MENU");
  }
  else if ( state == OVENCHECK )
  {
    DrawButton(0, "START");
    DrawButton(1, "BACK");
  }
}

void FlashButtonText(unsigned int index, uint32_t colorSteady = WHITE, uint32_t colorBlink = RED, unsigned int hertz = 5, unsigned int count = 2);

void FlashButtonText(unsigned int index, uint32_t colorSteady, uint32_t colorBlink, unsigned int hertz, unsigned int count)
{
  if (index >= NumButtons) return;  // out of range
  const unsigned long period = 1000 / (hertz * 2);  // in ms

  for (unsigned int i = 0; i < count; i++) {
    DrawButton(index, buttonText[index], colorBlink);
    delay(period);
    DrawButton(index, buttonText[index], colorSteady);
    if(i != count - 1) delay(period);  // delay if not on last loop
  }
}

void DrawScrollIndicator(float pct, uint32_t color)
{
	     if (pct < 0.0) pct = 0.0;
	else if (pct > 1.0) pct = 1.0;

	const unsigned int Width = 15;  // all values in pixels
	const unsigned int Height = 3;
	const unsigned int MarginY = 2;

	const unsigned int XPos = tft.width() - (Width * 2);
	const unsigned int YMin = buttonPosY[2] + buttonHeight + MarginY;  // inside edge
	const unsigned int YMax = buttonPosY[3] - MarginY;  // outside edge

	const unsigned int YRange = YMax - YMin;  // total range + number of options

	const unsigned int yPos = (pct * (float) YRange) + YMin;  // position of indicator

	tft.fillRect(XPos, YMin, Width, YMax - YMin + Height, BLACK);  // clear area
	tft.fillRect(XPos + Width / 2, YMin, 1, YMax - YMin + Height, WHITE);  // draw center line
	tft.fillRect(XPos, yPos, Width, Height, color);  // draw indicator
}

void DrawPageIndicator(unsigned int page, unsigned int numPages, uint32_t color)
{
	const unsigned int TextWidth = 31;  // total width from right edge, ballpark
	const unsigned int TextHeight = 8;  // with text size '1'
	const unsigned int XPos = tft.width() - TextWidth;  // left edge
	const unsigned int YPos = (buttonPosY[2] + buttonPosY[3]) / 2 + (TextHeight / 2);

	tft.fillRect(XPos, YPos, TextWidth, TextHeight, BLACK);  // clear area

	tft.setTextColor(color, BLACK);
	tft.setTextSize(1);
	tft.setCursor(XPos, YPos);

	tft.print(page);
	tft.print('/');
	tft.print(numPages);
}

void UpdateBakeMenu()
{
  tft.setTextColor( YELLOW, BLACK );
  tft.setTextSize(5);
  tft.setCursor( 20, 82 );
  tft.println(String(round(set.bakeTemp)) + "c ");
  tft.setCursor( 20, 157 );
  tft.println(String(set.bakeTime / 60) + "min ");
}

void ShowBakeMenu()
{
  state = BAKE_MENU;

  tft.fillScreen(BLACK);
  tft.setTextColor( BLUE, BLACK );
  tft.setTextSize(3);
  tft.setCursor( 20, 20 );
  tft.println( "LET'S BAKE!" );

  tft.setTextColor( WHITE, BLACK );
  tft.setCursor( 20, 60 );
  tft.setTextSize(2);
  tft.println("TEMPERATURE");
  tft.setCursor( 20, 135 );
  tft.println( "TIME");

  tft.setTextColor( WHITE, BLACK );
  tft.setTextSize(2);


  tft.fillRect( tft.width() - 100,  buttonPosY[0] - 2, 100, buttonHeight + 4, BLACK );

  DrawButton(0, "START");
  DrawButton(1, "BACK");
  DrawButton(2, "+TEMP");
  DrawButton(3, "+TIME");

  UpdateBakeMenu();
}

void UpdateBake()
{
  tft.setTextColor( BLUE, BLACK );
  tft.setTextSize(3);
  tft.setCursor( 20, 20 );

  switch (currentBakeTimeCounter)
  {
    case 0:
      tft.println( "BAKING   " );
      break;
    case 3:
      tft.println( "BAKING.  " );
      break;
    case 6:
      tft.println( "BAKING.. " );
      break;
    case 9:
      tft.println( "BAKING..." );
      break;
    default:
      // do nothing
      break;
  }

  currentBakeTimeCounter++;
  if (currentBakeTimeCounter == 12)
    currentBakeTimeCounter = 0;

  tft.setTextColor( YELLOW, BLACK );
  tft.setTextSize(5);
  tft.setCursor( 20, 82 );
  tft.println(String(round(currentTemp * 10) / 10) + "/" + String(round(set.bakeTemp)) + "c");
  tft.setCursor( 20, 157 );
  tft.println(String(round(currentBakeTime / 60 + 0.5)) + "/" + String(set.bakeTime / 60) + "min ");

  if ( round( currentTemp ) > round( cachedCurrentTemp ) )
    lastTempDirection = 1;
  else if ( round( currentTemp ) < round( cachedCurrentTemp ) )
    lastTempDirection = -1;

  cachedCurrentTemp = currentTemp;
}

void StartBake()
{
  currentBakeTime = set.bakeTime;
  currentBakeTimeCounter = 0;

  tft.fillScreen(BLACK);

  //  tft.setTextColor( BLUE, BLACK );
  //  tft.setTextSize(3);
  //  tft.setCursor( 20, 20 );
  //  tft.println( "BAKING..." );

  tft.setTextColor( WHITE, BLACK );
  tft.setCursor( 20, 60 );
  tft.setTextSize(2);
  tft.println("CURRENT TEMP");
  tft.setCursor( 20, 135 );
  tft.println( "TIME LEFT");

  DrawButton(0, "ABORT");

  UpdateBake();
}

void BakeDone()
{
  state = BAKE_DONE;
  
  SetRelayFrequency(0); // Turn the SSR off immediately

  Buzzer( 2000, 500 );

  if ( set.useFan && set.fanTimeAfterReflow > 0 )
  {
    keepFanOnTime = millis() + set.fanTimeAfterReflow * 1000;
    StartFan( true );
  }
  else
  {
    StartFan( false );
  }

  tft.fillScreen(BLACK);

  tft.setTextColor( BLUE, BLACK );
  tft.setTextSize(3);
  println_Center( tft, "BAKING DONE!", tft.width() / 2, ( tft.height() / 2 ) );

  tft.setTextColor( WHITE, BLACK );
  tft.setTextSize(2);

  DrawButton(0, "MENU");

  delay(750);
  Buzzer( 2000, 500 );

}

void DrawSettingsPointer()
{
  if ( state == SETTINGS )
  {
    SettingsPage::drawCursor(settings_pointer);
  }
  else if ( state == SETTINGS_PASTE )
  {
    tft.setTextColor( BLUE, BLACK );
    tft.setTextSize(2);
    tft.fillRect( 0, 20, 20, tft.height() - 20, BLACK );
    tft.setCursor( 5, ( 50 + ( 20 * ( settings_pointer * 2 ) ) ) );
    tft.println(">");
  }
}

void StartWarmup()
{
  tft.fillScreen(BLACK);

  state = WARMUP;
  timeX = 0;
  ShowButtonOptions( true );
  lastWantedTemp = -1;
  buzzerCount = 5;
  keepFanOnTime = 0;
  StartFan( false );

  tft.setTextColor( BLUE, BLACK );
  tft.setTextSize(3);
  tft.setCursor( 20, 20 );
  tft.println( "REFLOW..." );

  tft.setTextColor( GREEN, BLACK );
  tft.setTextSize(3);
  println_Center( tft, "WARMING UP", tft.width() / 2, ( tft.height() / 2 ) - 30 );

  tft.setTextColor( WHITE, BLACK );
  println_Center( tft, "START @ " + String( GetGraphValue(0) ) + "c", tft.width() / 2, ( tft.height() / 2 ) + 50 );
}

void StartReflow()
{
  tft.fillScreen(BLACK);

  state = REFLOW;
  ShowButtonOptions( true );

  timeX = 0;
  SetupGraph(tft, 0, 0, 30, 220, 270, 180, graphRangeMin_X, graphRangeMax_X, graphRangeStep_X, graphRangeMin_Y, graphRangeMax_Y, graphRangeStep_Y, "Reflow Temp", " Time [s]", "deg [C]", DKBLUE, BLUE, WHITE, BLACK );

  DrawHeading( "READY", WHITE, BLACK );
  DrawBaseGraph();
}

void AbortReflow()
{

  if ( state == WARMUP || state == REFLOW || state == OVENCHECK_START || state == BAKE ) // if we are in warmup or reflow states
  {
    state = ABORT;

    SetRelayFrequency(0); // Turn the SSR off immediately

    tft.fillScreen(BLACK);
    tft.setTextColor( RED, BLACK );
    tft.setTextSize(6);
    println_Center( tft, "ABORT", tft.width() / 2, ( tft.height() / 2 ) );

    if ( set.useFan && set.fanTimeAfterReflow > 0 )
    {
      keepFanOnTime = millis() + set.fanTimeAfterReflow * 1000;
    }
    else
    {
      StartFan( false );
    }

    delay(1000);

    state = MENU;
    ShowMenu();
  }
}

void EndReflow()
{
  if ( state == REFLOW )
  {
    SetRelayFrequency( 0 );
    state = FINISHED;

    Buzzer( 2000, 500 );

    DrawHeading( "DONE!", WHITE, BLACK );

    ShowButtonOptions( false );

    if ( set.useFan && set.fanTimeAfterReflow > 0 )
    {
      keepFanOnTime = millis() + set.fanTimeAfterReflow * 1000;
    }
    else
    {
      StartFan( false );
    }

    delay(750);
    Buzzer( 2000, 500 );
  }
}

void SetDefaults()
{
  // recreate struct using default constructor
  set = Settings();
}

void ResetSettingsToDefault()
{
  // set default values again and save
  SetDefaults();
  flash_store.write(set);

  // load the default paste
  SetCurrentGraph( set.paste );

  // show settings
  settings_pointer = 0;
  ShowSettings();
}


/*
   OVEN CHECK CODE NOT CURRENTLY USED
   START
*/
void StartOvenCheck()
{
  debug_println("Oven Check Start Temp " + String( currentTemp ) );

  state = OVENCHECK_START;
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

  debug_println("Running Oven Check");

  tft.fillScreen(BLACK);
  tft.setTextColor( CYAN, BLACK );
  tft.setTextSize(2);
  tft.setCursor( 20, 20 );
  tft.println( "OVEN CHECK" );

  tft.setTextColor( YELLOW, BLACK );
  tft.setCursor( 20, 40 );
  tft.println( CurrentGraph().n );
  tft.setCursor( 20, 60 );
  tft.println( String(CurrentGraph().tempDeg) + "deg");

  ShowButtonOptions( true );
}

void ShowOvenCheck()
{
  state = OVENCHECK;
  SetRelayFrequency( 0 );
  StartFan( true );

  debug_println("Oven Check");

  tft.fillScreen(BLACK);
  tft.setTextColor( CYAN, BLACK );
  tft.setTextSize(2);
  tft.setCursor( 20, 20 );
  tft.println( "OVEN CHECK" );


  tft.setTextColor( WHITE, BLACK );

  ShowButtonOptions( true );

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
   END
   OVEN CHECK CODE NOT CURRENTLY USED/WORKING
*/


/*
   Lots of UI code here
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

  state = SETTINGS_RESET;
  ShowButtonOptions( false );

  tft.setTextSize(1);
  tft.setTextColor( GREEN, BLACK );
  tft.fillRect( 0, tft.height() - 40, tft.width(), 40, BLACK );
  println_Center( tft, "Settings restore cannot be undone!", tft.width() / 2, tft.height() - 20 );
}

/*
   Button press code here
*/

void KeyTone(int hertz, int len)
{
  if (!set.keyTone) return;
  Buzzer(hertz, len);
}

// returns the a value limited to the min and max ranges, rolling over
// values below and above the provided range as if it was continuous
int constrainLoop(int value, int min, int max)
{
	// rollover up
	if (value > max) {
		int diff = (value - max - 1) % (max - min + 1);
		return constrain(min + diff, min, max);
	}
	// rollover down
	else if (value < min) {
		int diff = (value - min + 1) % (max - min + 1);
		return constrain(max + diff, min, max);
	}
	return value;
}

unsigned long nextButtonPress = 0;

void button0Press()
{
  if ( nextButtonPress < millis() )
  {
    nextButtonPress = millis() + 20;
    KeyTone( 2000, 50 );

    if ( state == MENU )
    {
      // Only allow reflow start if there is no TC error
      if ( tcError == 0 )
        StartWarmup();
      else {
        Buzzer(100, 250);
        FlashButtonText(0);
      }
    }
    else if ( state == BAKE_MENU )
    {
      flash_store.write(set);
      state = BAKE;
      StartBake();
    }
    else if ( state == WARMUP || state == REFLOW || state == OVENCHECK_START || state == BAKE )
    {
      AbortReflow();
    }
    else if ( state == FINISHED || state == BAKE_DONE )
    {
      ShowMenu();
    }
    else if ( state == SETTINGS )
    {
      SettingsPage::changeOption(settings_pointer);
    }
    else if ( state == SETTINGS_PASTE )
    {
      if ( set.paste != settings_pointer )
      {
        set.paste = settings_pointer;
        SetCurrentGraph( set.paste );
        ShowPaste();
      }
    }
    else if ( state == SETTINGS_RESET )
    {
      ResetSettingsToDefault();
    }
    else if ( state == OVENCHECK )
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
    KeyTone( 2000, 50 );

    if ( state == MENU )
    {
      // Only allow reflow start if there is no TC error
      if ( tcError == 0 )
        ShowBakeMenu();
      else {
        Buzzer( 100, 250 );
        FlashButtonText(1);
      }
    }
    else if ( state == SETTINGS ) // leaving settings so save
    {
      // save data in flash
      flash_store.write(set);
      ShowMenu();
    }
    else if ( state == SETTINGS_PASTE)
    {
      settings_pointer = 0;  // reset pointer so we stop at the top of the settings list
      ShowSettings();
    }
    else if (state == SETTINGS_RESET)  // cancel settings reset
    {
      ShowSettings();  // do *not* reset pointer so we stay at the same menu item
    }
    else if ( state == OVENCHECK ) // cancel oven check
    {
      ShowMenu();
    }
    else if ( state == BAKE_MENU) // cancel oven check
    {
      flash_store.write(set);
      ShowMenu();
    }
  }
}

void button2Press()
{
  if ( nextButtonPress < millis() )
  {
    nextButtonPress = millis() + 20;
    KeyTone( 2000, 50 );

    if ( state == MENU )
    {
      settings_pointer = 0;
      ShowSettings();
    }
    else if ( state == BAKE_MENU )
    {
      set.bakeTemp += 1;
      if ( set.bakeTemp > maxBakeTemp )
        set.bakeTemp = minBakeTemp;

      UpdateBakeMenu();
    }
    else if ( state == SETTINGS )
    {
      settings_pointer = constrainLoop( settings_pointer - 1, 0, (int) SettingsOption::getCount() - 1 );
      ShowButtonOptions( false );
      DrawSettingsPointer();
    }
    else if ( state == SETTINGS_PASTE )
    {
      settings_pointer = constrain( settings_pointer - 1, 0, (int) ELEMENTS(solderPaste) - 1 );
      DrawSettingsPointer();
    }
  }
}


void button3Press()
{
  if ( nextButtonPress < millis() )
  {
    nextButtonPress = millis() + 20;
    KeyTone( 2000, 50 );

    if ( state == MENU )
    {
      // Only allow reflow start if there is no TC error
      if ( tcError == 0 )
        ShowOvenCheck();
      else {
        Buzzer( 100, 250 );
        FlashButtonText(3);
      }
    }
    else if ( state == BAKE_MENU )
    {
      set.bakeTime += 300;
      if ( set.bakeTime > maxBakeTime )
        set.bakeTime = minBakeTime;

      UpdateBakeMenu();
    }
    else if ( state == SETTINGS )
    {
      settings_pointer = constrainLoop( settings_pointer + 1, 0, (int) SettingsOption::getCount() - 1);
      ShowButtonOptions( false );
      DrawSettingsPointer();
    }
    else if ( state == SETTINGS_PASTE )
    {
      settings_pointer = constrain( settings_pointer + 1, 0, (int) ELEMENTS(solderPaste) - 1 );
      DrawSettingsPointer();
    }
  }
}


void button2LongPressStart()
{
  if ( nextButtonPress < millis() )
  {
    nextButtonPress = millis() + 10;
    KeyTone( 2000, 10 );
    delay(50);
    KeyTone( 2000, 10 );
  }
}

void button2LongPress()
{
  if ( state == BAKE_MENU )
  {
    if ( nextButtonPress < millis() )
    {
      nextButtonPress = millis() +10;

      set.bakeTemp += 1;
      if ( set.bakeTemp > maxBakeTemp )
        set.bakeTemp = maxBakeTemp;

      UpdateBakeMenu();
    }
  }
}


void button3LongPressStart()
{
  if ( nextButtonPress < millis() )
  {
    nextButtonPress = millis() + 20;
    KeyTone( 2000, 10 );
    delay(50);
    KeyTone( 2000, 10 );
  }
}

void button3LongPress()
{
  if ( state == BAKE_MENU )
  {
    if ( nextButtonPress < millis() )
    {
      nextButtonPress = millis() + 20;

      set.bakeTime += 300;
      if ( set.bakeTime > maxBakeTime )
        set.bakeTime = maxBakeTime;

      UpdateBakeMenu();
    }
  }
}

/*
   Graph drawing code here
   Special thanks to Kris Kasprzak for his free graphing code that I derived mine from
   https://www.youtube.com/watch?v=YejRbIKe6e0
*/

void SetupGraph(Adafruit_ILI9341 &d, double x, double y, double gx, double gy, double w, double h, double xlo, double xhi, double xinc, double ylo, double yhi, double yinc, String title, String xlabel, String ylabel, unsigned int gcolor, unsigned int acolor, unsigned int tcolor, unsigned int bcolor )
{
  double i;
  int temp;

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
    d.setCursor(gx - 25, temp);
    println_Right( d, String(round(i)), gx - 25, temp );
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

    if ( i <= xhi - xinc )
      println_Center(d, String(round(i)), temp, gy + 10 );
    else
      println_Center(d, String(round(xhi)), temp, gy + 10 );
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

  if ( timeX < 2 )
    oy = min( oy, y );

  y = min( y, 220 ); // bottom of graph!

  //  d.fillRect( ox-1, oy-1, 3, 3, currentPlotColor );

  d.drawLine(ox, oy + 1, x, y + 1, currentPlotColor);
  d.drawLine(ox, oy - 1, x, y - 1, currentPlotColor);
  d.drawLine(ox, oy, x, y, currentPlotColor );
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

void println_Center( Adafruit_ILI9341 &d, String heading, int centerX, int centerY )
{
  int x = 0;
  int y = 0;
  int16_t  x1, y1;
  uint16_t ww, hh;

  d.getTextBounds( heading.c_str(), x, y, &x1, &y1, &ww, &hh );
  d.setCursor( centerX - ww / 2 + 2, centerY - hh / 2);
  d.println( heading );
}

void println_Right( Adafruit_ILI9341 &d, String heading, int centerX, int centerY )
{
  int x = 0;
  int y = 0;
  int16_t  x1, y1;
  uint16_t ww, hh;

  d.getTextBounds( heading.c_str(), x, y, &x1, &y1, &ww, &hh );
  d.setCursor( centerX + ( 18 - ww ), centerY - hh / 2);
  d.println( heading );
}


// Debug printing functions
void debug_print(String txt)
{
#ifdef DEBUG
  Serial.print(txt);
#endif
}

void debug_print(int txt)
{
#ifdef DEBUG
  Serial.print(txt);
#endif
}

void debug_println(String txt)
{
#ifdef DEBUG
  Serial.println(txt);
#endif
}

void debug_println(int txt)
{
#ifdef DEBUG
  Serial.println(txt);
#endif
}
