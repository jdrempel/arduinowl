// ARDUINOWL V2
// New algorithm, more or less from scratch

// IMPORTS
#include <Button.h>
#include <LiquidCrystal.h>
#include <Adafruit_NeoPixel.h>

// HARDWARE: Prepare pin definitions and hardware constants.
LiquidCrystal lcd(2, 3, 4, 5, 6, 7);

Button modBtn(9);
Button fwdBtn(8);
Button rwdBtn(10);

#define SERIAL_BUFFER_SIZE 256
#define SERIAL_BUFFER_SIZE_RX 256
#define LED_COUNT 2
#define DAL       0x0C2340
#define PHI       0xFF9E1B
#define HOU       0x97D700
#define BOS       0x174B7A
#define NYE       0x171C68
#define SFS       0xFC4C02
#define VAL       0x5AB719
#define GLA       0x3C1053
#define FLA       0xFEDA00
#define SHD       0xD22630
#define SEO       0xAA8A00
#define LDN       0x59CBE8
#define ATL       0xC4C4C4
#define CDH       0xFDA100
#define GZC       0x25F2D4
#define HZS       0xFA7298
#define PAR       0x2F3D57
#define TOR       0xC20022
#define VAN       0x2FB228
#define WAS       0x990034

Adafruit_NeoPixel leds = Adafruit_NeoPixel(LED_COUNT, 12, NEO_GRB + NEO_KHZ800);

// STARTUP: What happens when the Arduino first boots?
const int LEAGUE_POP = 20; // how many teams are in the league?
const int MATCH_QUEUE = 3; // how many matches are we looking ahead (including current one)?
int mode = 0; // 0 = matches, 1 = overall ranks
int rankNum = 0; // represents currently displayed rank.
int matchNum = 0; // represents currently displayed schedule/match info
int titleDelay = 750; // How long to display the mode titles for (ms)

// Each team has a 3-digit abbreviated team code.
//   TODO: Should this be a 'string' instead of "String"?
String teamcode[LEAGUE_POP] = {"DAL", "PHI", "HOU", "BOS", "NYE", "SFS", "VAL", "GLA", "FLA", "SHD", "SEO", "LDN", "ATL", "CDH", "GZC", "HZS", "PAR", "TOR", "VAN", "WAS"};
// Title strings for each display mode
String dispmode[2] = {"Match Schedule", "Overall Rankings"};

// char endMarker = '*';
const int MAX_LENGTH = 16; // maximum possible line length (2x16 LCD screen)
// char strBuffer[MAX_LENGTH]; // 16-character strBuffer for Serial input
// int bufferIdx = 0;
const int NUM_DISPS = LEAGUE_POP + 3; // The total number of possible text screens (n.i. titles)
String pythonInput = "";
String pythonStrings[2 * NUM_DISPS];
String matchLines[2 * MATCH_QUEUE];
String rankLines[2 * LEAGUE_POP];
String firstLines[NUM_DISPS]; // string array of all the first lines of LCD output
String secondLines[NUM_DISPS]; // string array of all the second lines of LCD output

// UTILITY FUNCTIONS: Functions to simplify life later on.

String getValue(String data, char separator, int index) {
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? (i + 1) : i;
    }
  }

  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

// Set one of the LEDs to the given team colour
void setLED(int LED, unsigned long TEAM) {
//  clearLEDs();
  leds.setPixelColor(LED, TEAM);
  leds.show();
  delay(5);
}

// Set both LEDs to the same team colour simultaneously
void setBothLEDs(unsigned long TEAM) {
  setLED(0, TEAM);
  setLED(1, TEAM);
}

void clearLEDs() {
  for (int i = 0; i < LED_COUNT; i++)
  {
    leds.setPixelColor(i, 0);
  }
}

// Refresh the display to a new mode
void setDispMode(int newMode) {
  lcd.setCursor(0,0);
  lcd.print(dispmode[newMode]);
  delay(titleDelay);
  // lcd.clear();
  int idx = matchNum;
  if (newMode) {
    idx = rankNum;
  }
  printMatch(idx, newMode);
}

void printMatch(int idx, int mode) {
  int modeOffset = 0;
  if (mode) {
    // modeOffset = MATCH_QUEUE;
    for (int k = 0; k < LEAGUE_POP; k++) {
      firstLines[k] = rankLines[2*k];
      secondLines[k] = rankLines[2*k + 1];
    }
  } else {
    for (int k = 0; k < MATCH_QUEUE; k++) {
      firstLines[k] = matchLines[2*k];
      secondLines[k] = matchLines[2*k + 1];
    }
    String teamName[2] = {firstLines[idx].substring(6,9), firstLines[idx].substring(13)};
    int teamNum[2] = {0, 0};
    for (int team = 0; team < 2; team++) {
      for (int T = 0; T < LEAGUE_POP; T++) {
        if (teamName[team] == teamcode[T]) {
          teamNum[team] = T;
        }
      }
      setLED(team, getTeamCode(teamNum[team]));
    }
  }
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(firstLines[modeOffset+idx]);
  lcd.setCursor(0,1);
  lcd.print(secondLines[modeOffset+idx]);
}

// Moves to the next screen of the same mode
void cycleForward(int mode) {
  if (!mode) {
    if (matchNum < MATCH_QUEUE - 1) {
      matchNum++;
    } else {
      matchNum = 0;
    }
  } else {
    if (rankNum < LEAGUE_POP - 1) {
      rankNum++;
    } else {
      rankNum = 0;
    }
  }
}

// Moves to the previous screen of the same mode
void cycleBackward(int mode) {
  if (!mode) {
    if (matchNum > 0) {
      matchNum--;
    } else {
      matchNum = MATCH_QUEUE - 1;
    }
  } else {
    if (rankNum > 0) {
      rankNum--;
    } else {
      rankNum = LEAGUE_POP - 1;
    }
  }
}

void readPython() {
  // While there is Serial input, stop and listen
  // Check each bit. If a bit is one of the newline markers, put the string so far into an array.
  // Otherwise add the bit to the existing string strBuffer.
  char strBuffer[MAX_LENGTH] = {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '};
  char endMarker = '*';
  char firstLineMarker = '<'; // Marks the beginning of a 1st-line string
  char secondLineMarker = '>'; // Marks the beginning of a 2nd-line string
  int bufferIdx = 0;
  bool readingFirstLine = false;
  bool readingSecondLine = false;
  int dispIndex = 0;
  char inChar;

  while (Serial.available() > 0) {
    delay(1);
    inChar = Serial.read();
    pythonInput.concat(inChar);
  }

  k = 0;
  do {
    pythonStrings[k] = getValue(pythonInput, endMarker, k);
    k++;
  } while (pythonStrings[k-1] != "");
  int maxK;
  if (k > 2) {
    maxK = k - 1;
  } else {
    maxK = 2;
  }
  int line;
  for (line = 0; line < 2 * MATCH_QUEUE; line++) {
    matchLines[line] = pythonStrings[line];
  }
  for ( ; line < NUM_DISPS; line++) {
    rankLines[line - 2 * MATCH_QUEUE] = pythonStrings[line];
  }

  // while (Serial.available() > 0) {
  //   delay(1);
  //   char inChar = Serial.read();
  //   if (inChar == firstLineMarker) {
  //     readingFirstLine = true;
  //     readingSecondLine = false;
  //   } else if (inChar == secondLineMarker) {
  //     readingFirstLine = false;
  //     readingSecondLine = true;
  //   } else if (inChar == endMarker) {
  //     // Put strBuffer into array and clear strBuffer
  //     // String strToPush = new String(strBuffer);
  //     if (readingFirstLine) { // This is a first line
  //       firstLines[dispIndex] = strBuffer;
  //       readingFirstLine = false;
  //     } else if (readingSecondLine) { // This is a second line
  //       secondLines[dispIndex] = strBuffer;
  //       readingSecondLine = false;
  //       dispIndex++; // Now that we have the second line, move to the next display index
  //     } else {
  //       setBothLEDs(SHD); // For debug purposes only... hopefully this isn't a terrible idea
  //     }
  //     Serial.println(strBuffer);
  //     for (int i=0; i < MAX_LENGTH; i++) {
  //       strBuffer[i] = ' '; // Clearing the strBuffer
  //     }
  //     bufferIdx = 0;
  //   } else {
  //     // Add to strBuffer if this is not a special character
  //     if (readingFirstLine || readingSecondLine) {
  //       strBuffer[bufferIdx] = inChar;
  //       bufferIdx++;
  //     }
  //   }
  // }
}

// Returns an unsigned long corresponding to the colour of each team
unsigned long getTeamCode(int num) {
  switch (num) {
    case 0:
    return DAL; break;
    case 1:
    return PHI; break;
    case 2:
    return HOU; break;
    case 3:
    return BOS; break;
    case 4:
    return NYE; break;
    case 5:
    return SFS; break;
    case 6:
    return VAL; break;
    case 7:
    return GLA; break;
    case 8:
    return FLA; break;
    case 9:
    return SHD; break;
    case 10:
    return SEO; break;
    case 11:
    return LDN; break;
    case 12:
    return ATL; break;
    case 13:
    return CDH; break;
    case 14:
    return GZC; break;
    case 15:
    return HZS; break;
    case 16:
    return PAR; break;
    case 17:
    return TOR; break;
    case 18: 
    return VAN; break;
    case 19:
    return WAS; break;
  }
}

void setup() {
//   TODO: Should this be a higher baud rate? Can PySerial keep up if it is?
  Serial.begin(9600);
  Serial.setTimeout(1500);

// Initialized LCD display
  lcd.begin(16, 2);
  lcd.print("Loading...");
// Initialize push buttons
  modBtn.begin();
  fwdBtn.begin();
  rwdBtn.begin();
// Initialize LEDs
  leds.begin();
  clearLEDs();

  // setDispMode(mode);
}

void loop() {
  // Check for Serial input
  if (Serial.available() <= 0) {
  // NORMAL OPERATION: On an average loop what is the Arduino doing?
    // Create a flag for button pressed - default will be true
    bool inputCheck = true;
    // Check for user input
    if (modBtn.pressed()) {
      // Change display modes
      mode = 1 - mode;
      setDispMode(mode);
    } else if (fwdBtn.pressed()) {
      // Cycle displayed data forward
      cycleForward(mode);
    } else if (rwdBtn.pressed()) {
      // Cycle displayed data backward
      cycleBackward(mode);
    } else {
      // No input
      inputCheck = false;
    }
    
    if (inputCheck) {
      // Some kind of refresh on the LCD?
      // setDispMode(mode); // Should just refresh the screen
    }

  } else {
  // RX AND PARSING: How does the Arduino respond to Serial input?
    readPython();
    // setDispMode(mode); // Should just refresh the screen
  }



}
