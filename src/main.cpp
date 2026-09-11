#include <Arduino.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <driver/rtc_io.h>
#include <WiFi.h>
#include <BluetoothSerial.h>
#include <esp_bt.h>
#include <esp_wifi.h>
#include <PixelOperator88pt.h>
#include <PixelOperator816pt.h>
#include <vector>
#include <Button2.h>
#include <Preferences.h>
#include <wordwrap.h>

Preferences prefs;

// button vars
const int PIN_BUTTON_SLEEP = 1;
const int PIN_BUTTON_UP = 43;
const int PIN_BUTTON_DOWN = 17;
const int PIN_BUTTON_ACCEPT = 44;
const int PIN_BUTTON_BACK = 18;

Button2 buttonSleep;
Button2 buttonUp;
Button2 buttonDown;
Button2 buttonAccept;
Button2 buttonBack;

// important pins
const int PIN_POWER_ON = 15;
const int PIN_BACKLIGHT = 38;

// screen stuff
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite screenSprite = TFT_eSprite(&tft);

// ui settings
const uint16_t FONT_COLOR_SELECTED = tft.color565(34, 202, 254);
const uint16_t FONT_COLOR_DIM = tft.color565(17, 101, 127);
bool showDebugUI = false;
const int SMALL_FONT_MAX_COLUMNS = 25;
const int LARGE_FONT_MAX_COLUMNS = 12;

// menu stuff
typedef void (*MenuAction)();

struct MenuItem {
  String label;
  void* action;
  std::vector<MenuItem*> submenus;
  bool smallMenuFont;
  bool disableScrolling;
};

int currentMenuIndex = 0;
std::vector<MenuItem*> menuHistory;
std::vector<int> menuIndexHistory;

// menu setup (ignore examples these were just for testing)
void exampleFunc1() {Serial.println("hi 1");};
void exampleFunc2() {Serial.println("hi 2");};

MenuItem menuExample2 = {
  "example2", 
  (void*)exampleFunc2,
  {},
  false,
  false
};

MenuItem menuExample = {
  "example", 
  (void*)exampleFunc1,
  {&menuExample2, &menuExample2, &menuExample2},
  false,
  false
};

MenuItem menuNotImplemented = {
  "not implemented yet, sorry!", 
  nullptr,
  {},
  false,
  false
};

MenuItem menuEmpty = {
  "", 
  nullptr,
  {},
  false,
  false
};

void prescript();

MenuItem prescriptButton = {
  "_PRESS ACCEPT FOR PRESCRIPT_",
  (void*)prescript,
  {},
  true,
  true
};

MenuItem menuPrescript = {
  "Prescript", 
  nullptr,
  {&prescriptButton},
  true,
  true
};

MenuItem menuCoinFlip = {
  "Coin Flip", 
  nullptr,
  {&menuNotImplemented},
  true,
  false
};

void changeBrightness1() {
  analogWrite(TFT_BL, 3);
  prefs.putInt("brightness", 3);
}

void changeBrightness10() {
  analogWrite(TFT_BL, 26);
  prefs.putInt("brightness", 10);
}

void changeBrightness25() {
  analogWrite(TFT_BL, 64);
  prefs.putInt("brightness", 64);
}

void changeBrightness50() {
  analogWrite(TFT_BL, 128);
  prefs.putInt("brightness", 128);
}

void changeBrightness100() {
  analogWrite(TFT_BL, 255);
  prefs.putInt("brightness", 255);
}

MenuItem menuSettingsBrightness1 = {
  "Brightness 1%", 
  (void*)changeBrightness1,
  {},
  false,
  false
};

MenuItem menuSettingsBrightness10 = {
  "Brightness 10%", 
  (void*)changeBrightness10,
  {},
  false,
  false
};

MenuItem menuSettingsBrightness25 = {
  "Brightness 25%", 
  (void*)changeBrightness25,
  {},
  false,
  false
};

MenuItem menuSettingsBrightness50 = {
  "Brightness 50%", 
  (void*)changeBrightness50,
  {},
  false,
  false
};

MenuItem menuSettingsBrightness100 = {
  "Brightness 100%", 
  (void*)changeBrightness100,
  {},
  false,
  false
};

void toggleDebug() {
  showDebugUI = !showDebugUI;
  prefs.putBool("debugUI", showDebugUI);
}

MenuItem menuSettingsDebugToggle = {
  "Debug Overlays", 
  (void*)toggleDebug,
  {},
  false,
  false
};

MenuItem menuSettingsBrightness = {
  "Brightness", 
  nullptr,
  {&menuSettingsBrightness1, &menuSettingsBrightness10, &menuSettingsBrightness25, &menuSettingsBrightness50, &menuSettingsBrightness100},
  true,
  false
};

MenuItem menuSettings = {
  "Settings", 
  nullptr,
  {&menuSettingsBrightness, &menuSettingsDebugToggle},
  false,
  false
};

MenuItem menuMain = {
  "Main Menu", 
  nullptr,
  {&menuPrescript, &menuCoinFlip, &menuSettings},
  false,
  false
};

MenuItem* currentMenu = &menuMain;

bool menuIsAnimating = false;
const int SCREEN_HEIGHT = 170; 
const int MENU_ITEM_SPACING = 75;
float menuScrollY = 0;
int menuTargetScrollY = 0;

// prescript stuff
const String PRESCRIPT_SCRAMBLED_CHARS = " abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890!@#$%^&*()-_=+,./|\\";
const int PRESCIPT_MAX_ATTEMPTS = 2;
const float PRESCRIPT_DELAY = 0.05f;
const String PRESCRIPTS_LIST[] = {
  "Slam Down with Weight, Topple the Body",
  "Lay Vertical The End, Insert Up to the Wick",
  "Lay the Blade on its Side, Slice Like a Severed Breath",
  "Swing to Fell, Have it Meet the Ground",
  "Aim Toward a Point, Let it Echo Within",
  "Carve at a Low Slant, Peel What Remains",
  "Destroy the Sound, Crush Flat the Thought",
  "Stab the Silence's Heart, Penetrate the Memory",
  "With Tempered Secret, Cut the Form",
  "Enwrap 330 times in Long Swaths of Frozen Blood",
  "Revel with Soundless Applause, Impale in Voiceless Sorrow",
  "Raise and Laugh the Blade, Cry the Waterfall Like the Scent of Fallen Leaves"
};

// function declarations
void sleepButtonPress(Button2& btn);
void upButtonPress(Button2& btn);
void downButtonPress(Button2& btn);
void acceptButtonPress(Button2& btn);
void backButtonPress(Button2& btn);

String prescriptScramble(String line);

String wrapText(String text, int endX);

void updateMenu();
void drawDebugUI();
void drawMenu();

void setup() {
  Serial.begin(115200);
  prefs.begin("prescript");

  // deep sleep wake up stuff
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0) {
    // run wake up stuff here if needed
  }

  // disable wifi and bluetooth as it is not needed
  WiFi.mode(WIFI_OFF);
  WiFi.disconnect(true);
  esp_wifi_stop();
  btStop();
  esp_bt_controller_disable();

  // initialize important pins
  pinMode(PIN_POWER_ON, OUTPUT);
  digitalWrite(PIN_POWER_ON, HIGH);
  pinMode(PIN_BACKLIGHT, OUTPUT);
  digitalWrite(PIN_BACKLIGHT, HIGH); 

  // set up buttons
  buttonSleep.begin(PIN_BUTTON_SLEEP);
  buttonSleep.setPressedHandler(sleepButtonPress);
  buttonUp.begin(PIN_BUTTON_UP);
  buttonUp.setClickHandler(upButtonPress);
  buttonDown.begin(PIN_BUTTON_DOWN);
  buttonDown.setClickHandler(downButtonPress);
  buttonAccept.begin(PIN_BUTTON_ACCEPT);
  buttonAccept.setClickHandler(acceptButtonPress);
  buttonBack.begin(PIN_BUTTON_BACK);
  buttonBack.setClickHandler(backButtonPress);

  // set up lcd
  tft.init();
  tft.setRotation(3);

  // load settings
  analogWrite(TFT_BL, prefs.getInt("brightness", 255));
  showDebugUI = prefs.getBool("debugUI", false);

  // set up screen sprite
  screenSprite.createSprite(tft.width(), tft.height()); 
  screenSprite.setTextColor(FONT_COLOR_SELECTED);
  screenSprite.setFreeFont(&PixelOperator816pt7b);
  screenSprite.setTextWrap(false, false);

  drawMenu();
}

void loop() {
  buttonSleep.loop();
  buttonUp.loop(); 
  buttonDown.loop();
  buttonAccept.loop();
  buttonBack.loop();
  updateMenu();
}

// function definitions
void sleepButtonPress(Button2& btn) {
  // wait for button to be released first so it doesn't turn right back on immediately
  while(digitalRead(PIN_BUTTON_SLEEP) == LOW) { delay(10); }
  delay(200);

  // tell display to sleep then turn off important pins, since the sleep command doesn't turn off the backlight
  tft.writecommand(0x10); 
  digitalWrite(PIN_BACKLIGHT, LOW);
  //digitalWrite(PIN_POWER_ON, LOW); // i dont think this is necessary, but im leaving it here but commented in case it is

  // set up the sleep button so it can turn the system back on
  rtc_gpio_pullup_en((gpio_num_t)PIN_BUTTON_SLEEP);
  rtc_gpio_pulldown_dis((gpio_num_t)PIN_BUTTON_SLEEP);
  esp_sleep_enable_ext0_wakeup((gpio_num_t)PIN_BUTTON_SLEEP, 0);

  esp_deep_sleep_start();
}

void upButtonPress(Button2& btn) {
  if (currentMenu->disableScrolling == true) return;
  currentMenuIndex = constrain(currentMenuIndex - 1, 0, currentMenu->submenus.size() - 1);
  menuIsAnimating = true;
}

void downButtonPress(Button2& btn) {
  if (currentMenu->disableScrolling == true) return;
  currentMenuIndex = constrain(currentMenuIndex + 1, 0, currentMenu->submenus.size() - 1);
  menuIsAnimating = true;
}

void acceptButtonPress(Button2& btn) {
  if (currentMenu->submenus[currentMenuIndex]->submenus.size() > 0) {
    menuHistory.insert(menuHistory.begin(), currentMenu);
    menuIndexHistory.insert(menuIndexHistory.begin(), currentMenuIndex);
    currentMenu = currentMenu->submenus[currentMenuIndex];
    currentMenuIndex = 0;
  }
  else if (currentMenu->submenus[currentMenuIndex]->action != nullptr) {
    MenuAction action = reinterpret_cast<MenuAction>(currentMenu->submenus[currentMenuIndex]->action);
    action();
  }
  menuIsAnimating = true;
}

void backButtonPress(Button2& btn) {
  if (menuHistory.size() > 0 && menuIndexHistory.size() > 0) {
    currentMenu = menuHistory[0];
    menuHistory.erase(menuHistory.begin());
    currentMenuIndex = menuIndexHistory[0];
    menuIndexHistory.erase(menuIndexHistory.begin());
  }
  menuIsAnimating = true;
}

void drawDebugUI() {
  int xPosition = 10;
  int yPosition = 25;
  int extraSpacing = 2;

  screenSprite.setFreeFont(&PixelOperator88pt7b);
  screenSprite.setTextColor(TFT_WHITE);
  screenSprite.setCursor(xPosition, yPosition);
  screenSprite.print("curMenu: " + currentMenu->label);
  screenSprite.setCursor(xPosition, yPosition + screenSprite.fontHeight() + extraSpacing);
  screenSprite.print("isAnimating: " + String(menuIsAnimating));
  screenSprite.setCursor(xPosition, yPosition + (screenSprite.fontHeight() * 2) + extraSpacing);
  screenSprite.print("freeRAM: " + String(ESP.getFreeHeap()));
  screenSprite.setCursor(xPosition, yPosition + (screenSprite.fontHeight() * 3) + extraSpacing);
  screenSprite.print("menuHistory: m" + String(menuHistory.size()) + " i" + String(menuIndexHistory.size()));
}

void updateMenu() {
  if (!menuIsAnimating) return;

  // lerp scrollY to targetScrollY
  menuTargetScrollY = currentMenuIndex * MENU_ITEM_SPACING;
  menuScrollY += (menuTargetScrollY - menuScrollY) * 0.15;

  // check if the difference is close enough to 0 so it can stop animating
  if (abs(menuTargetScrollY - menuScrollY) < 0.5) {
    menuScrollY = menuTargetScrollY;
    menuIsAnimating = false; 
  }

  drawMenu();
}

void drawMenu() {
  screenSprite.fillSprite(TFT_BLACK);

  if (currentMenu->smallMenuFont == true) {
    screenSprite.setFreeFont(&PixelOperator88pt7b);
  }
  else {
    screenSprite.setFreeFont(&PixelOperator816pt7b);
  }

  for (size_t i = 0; i < currentMenu->submenus.size(); i++) {    
    int yPosition = ((SCREEN_HEIGHT / 2) + (screenSprite.fontHeight() / 2)) + (i * MENU_ITEM_SPACING) - (int)menuScrollY;

    if (i == currentMenuIndex) {
      screenSprite.setTextColor(FONT_COLOR_SELECTED);
    } else {
      screenSprite.setTextColor(FONT_COLOR_DIM);
    }
    screenSprite.setCursor(10, yPosition);

    if (currentMenu->smallMenuFont == true) {
      screenSprite.print(wordwrap.wrap(currentMenu->submenus[i]->label, SMALL_FONT_MAX_COLUMNS));
    }
    else {
      screenSprite.print(wordwrap.wrap(currentMenu->submenus[i]->label, LARGE_FONT_MAX_COLUMNS));
    }
    
  }

  // size of the pixels for the arrow "sprite" calculations (position calculations may be slightly off but its okay)
  int pixelSize = 4;
  if (currentMenu->submenus.size() > 1 && currentMenu->disableScrolling == false) {
    if (currentMenuIndex != 0) {
      // up arrow - available
      screenSprite.fillRect(290 + pixelSize * 2, 10, pixelSize, pixelSize, FONT_COLOR_SELECTED); // tip
      screenSprite.fillRect(290 + pixelSize, 10 + pixelSize, pixelSize * 3, pixelSize, FONT_COLOR_SELECTED); // middle
      screenSprite.fillRect(290, 10 + pixelSize * 2, pixelSize * 5, pixelSize, FONT_COLOR_SELECTED); // base
    }
    else {
      // up arrow - unavailable
      screenSprite.fillRect(290 + pixelSize * 2, 10, pixelSize, pixelSize, FONT_COLOR_DIM); // tip
      screenSprite.fillRect(290 + pixelSize, 10 + pixelSize, pixelSize * 3, pixelSize, FONT_COLOR_DIM); // middle
      screenSprite.fillRect(290, 10 + pixelSize * 2, pixelSize * 5, pixelSize, FONT_COLOR_DIM); // base
    }
    if (currentMenuIndex != currentMenu->submenus.size() - 1) {
      // down arrow - available
      screenSprite.fillRect(290, 150, pixelSize * 5, pixelSize, FONT_COLOR_SELECTED); // base
      screenSprite.fillRect(290 + pixelSize, 150 + pixelSize, pixelSize * 3, pixelSize, FONT_COLOR_SELECTED); // middle
      screenSprite.fillRect(290 + pixelSize * 2, 150 + pixelSize * 2, pixelSize, pixelSize, FONT_COLOR_SELECTED); // tip
    }
    else {
      // down arrow - unavailable
      screenSprite.fillRect(290, 150, pixelSize * 5, pixelSize, FONT_COLOR_DIM); // base
      screenSprite.fillRect(290 + pixelSize, 150 + pixelSize, pixelSize * 3, pixelSize, FONT_COLOR_DIM); // middle
      screenSprite.fillRect(290 + pixelSize * 2, 150 + pixelSize * 2, pixelSize, pixelSize, FONT_COLOR_DIM); // tip
    }
  }

  if (showDebugUI == true) {
    drawDebugUI();
  }
  
  screenSprite.pushSprite(0, 0);
}

String prescriptScramble(String line) {
  String finalLine = "";
  for (int i = 0; i < line.length(); i++) {
    finalLine += PRESCRIPT_SCRAMBLED_CHARS[random(0, PRESCRIPT_SCRAMBLED_CHARS.length() - 1)];
  }
  return finalLine;
}

void prescript() {
  // choose a line and scramble it
  String chosenLine = "_" + PRESCRIPTS_LIST[random(0, sizeof(PRESCRIPTS_LIST) / sizeof(PRESCRIPTS_LIST[0]))] + "_";
  String scrambledLine = prescriptScramble(chosenLine);

  // draw initial scramble
  prescriptButton.label = scrambledLine;
  drawMenu();

  // now scramble it back, drawing along the way
  while (scrambledLine != chosenLine) {
    for (int i = 0; i < chosenLine.length(); i++) {
      int attempts = 0;
      String preIndex = scrambledLine.substring(0,i);
      String postIndex = scrambledLine.substring(i + 1);

      while (scrambledLine[i] != chosenLine[i]) {
        delay(PRESCRIPT_DELAY);
        attempts += 1;

        scrambledLine = preIndex + prescriptScramble(chosenLine[i] + postIndex);
        prescriptButton.label = scrambledLine;
        drawMenu();

        if (attempts >= PRESCIPT_MAX_ATTEMPTS) {
          scrambledLine = preIndex + chosenLine[i] + postIndex;
          prescriptButton.label = scrambledLine;
          drawMenu();

          break;
        }
      }
    }
  }
}