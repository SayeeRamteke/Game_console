// Arduino Gaming Console with 4 Games (Catch, Shooter, Wall Breaker, Flappy Bird)
#include <Wire.h>
#include <U8g2lib.h>
#include <EEPROM.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);

// Buttons & buzzer
#define BUTTON_LEFT    4
#define BUTTON_RIGHT   5
#define BUTTON_MENU    7
#define BUTTON_SELECT  6
#define BUTTON_UP      2
#define BUTTON_DOWN    3
#define BUZZER_PIN     8

// Menu/state
unsigned long lastSwitch = 0;
int currentPage = 0, menuIndex = 0;
bool inGame = false;
int currentGame = -1;

// —— Catch Game —— 
int paddleX = 54;
const int paddleWidth = 20;
int ballX = 0, ballY = 0, score = 0;
unsigned long lastFall = 0;
const int ballSpeed = 150;

// —— Shooter Game —— 
const int cannonW = 12, cannonH = 6;
bool shooterInit = false;
int cannonX, shooterScore, enemyX, enemyY;
bool bulletActive;
int bulletX, bulletY;
const int bulletSpeed = 4, enemySpeed = 1;

// —— Wall Breaker —— 
const int wb_cols = 8;
const int wb_rows = 2;
bool bricks[wb_rows][wb_cols];
int wb_ballX, wb_ballY, wb_ballVX, wb_ballVY;
int wb_paddleX;
const int wb_paddleW = 30;
bool wbInit = false;

// —— Flappy Bird —— 
float flappyY = 30, flappyVel = 0;
int pipeX = SCREEN_WIDTH, gapY = 20;
bool flapInit = false;
int flapScore = 0;
const float gravity = 0.7;
const int flapStrength = -3;

// —— EEPROM high scores —— 
int highScoreCatch = 0;
int highScoreShooter = 0;
int highScoreWallBreaker = 0;
int highScoreFlappy = 0;

// EEPROM addresses
const int EEPROM_ADDR_CATCH = 0;
const int EEPROM_ADDR_SHOOTER = 4;
const int EEPROM_ADDR_WALL = 8;
const int EEPROM_ADDR_FLAPPY = 12;

// Function prototypes
void drawPage(int);
void beep();
void showGameOverScreen();
void showVictoryScreen();
void playCatchGame();
void playShooterGame();
void playWallBreaker();
void playFlappyBird();

void setup() {
  u8g2.begin();
  pinMode(BUTTON_LEFT, INPUT_PULLUP);
  pinMode(BUTTON_RIGHT, INPUT_PULLUP);
  pinMode(BUTTON_MENU, INPUT_PULLUP);
  pinMode(BUTTON_SELECT, INPUT_PULLUP);
  pinMode(BUTTON_UP, INPUT_PULLUP);
  pinMode(BUTTON_DOWN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  randomSeed(analogRead(0));

  EEPROM.get(EEPROM_ADDR_CATCH, highScoreCatch);
  EEPROM.get(EEPROM_ADDR_SHOOTER, highScoreShooter);
  EEPROM.get(EEPROM_ADDR_WALL, highScoreWallBreaker);
  EEPROM.get(EEPROM_ADDR_FLAPPY, highScoreFlappy);
}

void drawPage(int page) {
  u8g2.clearBuffer();
  if (page == 0) {
    u8g2.setFont(u8g2_font_ncenB10_tr);
    u8g2.drawStr(3,30,"Gaming Console");
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.drawStr(15,50,"Press SELECT to Play");
  } else if (page == 1) {
    u8g2.setFont(u8g2_font_ncenB10_tr);
    u8g2.drawStr(0,15,"Controls:");
    u8g2.setFont(u8g2_font_5x8_tr);
    u8g2.drawStr(0,32,"> L/R: Move");
    u8g2.drawStr(0,42,"> SELECT: Action");
    u8g2.drawStr(0,52,"> MENU: Exit");
  } else {
    const char* games[] = {
      "1. Catch Game",
      "2. Shooter Game",
      "3. Wall Breaker",
      "4. Flappy Bird"
    };
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.drawStr(20,10,"Select Game:");
    u8g2.setFont(u8g2_font_5x8_tr);
    for (int i = 0; i < 4; i++) {
      if (i == menuIndex) u8g2.drawStr(10,25 + i*12, ">");
      u8g2.drawStr(20,25 + i*12, games[i]);
    }
  }
  u8g2.sendBuffer();
}

void beep() {
  tone(BUZZER_PIN,1000,80);
  delay(80);
  noTone(BUZZER_PIN);
}

// ———————————————— FIXED SECTION ————————————————
void showGameOverScreen() {
  int finalScore;
  int* gameHighScore;
  int eepromAddr;

  switch (currentGame) {
    case 0: finalScore = score; gameHighScore = &highScoreCatch; eepromAddr = EEPROM_ADDR_CATCH; break;
    case 1: finalScore = shooterScore; gameHighScore = &highScoreShooter; eepromAddr = EEPROM_ADDR_SHOOTER; break;
    case 2: finalScore = score; gameHighScore = &highScoreWallBreaker; eepromAddr = EEPROM_ADDR_WALL; break;
    case 3: finalScore = flapScore; gameHighScore = &highScoreFlappy; eepromAddr = EEPROM_ADDR_FLAPPY; break;
    default: finalScore = 0; gameHighScore = &highScoreCatch; eepromAddr = EEPROM_ADDR_CATCH;
  }

  if (finalScore > *gameHighScore) {
    *gameHighScore = finalScore;
    EEPROM.put(eepromAddr, *gameHighScore);
  }

  // Display scores
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB10_tr);
  u8g2.drawStr(20,25,"Game Over");
  char buf[20];
  u8g2.setFont(u8g2_font_5x8_tr);
  sprintf(buf,"Score:%d", finalScore);
  u8g2.drawStr(20,40,buf);
  sprintf(buf,"High:%d", *gameHighScore);
  u8g2.drawStr(20,50,buf);
  u8g2.drawStr(10,60,"Press MENU");
  u8g2.sendBuffer();

  unsigned long t = millis();
  while (millis() - t < 5000) {
    if (!digitalRead(BUTTON_MENU)) {
      beep();
      delay(200);
      break;
    }
  }
}
// ———————————————————————————————————————————————

void showVictoryScreen() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB10_tr);
  u8g2.drawStr(20,25,"You Win!");
  
  char buf[20];
  sprintf(buf,"Score:%d", score);
  u8g2.setFont(u8g2_font_5x8_tr);
  u8g2.drawStr(20,40,buf);

  sprintf(buf,"High:%d", highScoreCatch);
  u8g2.drawStr(20,50,buf);

  u8g2.drawStr(10,60,"Press MENU");
  u8g2.sendBuffer();

  unsigned long t = millis();
  while (millis() - t < 5000) {
    if (!digitalRead(BUTTON_MENU)) {
      beep();
      delay(200);
      break;
    }
  }
}

void playCatchGame() {
  if (!digitalRead(BUTTON_MENU)) { inGame = false; currentGame = -1; currentPage = 2; beep(); delay(200); return; }
  if (!digitalRead(BUTTON_LEFT) && paddleX > 0) paddleX -= 3;
  if (!digitalRead(BUTTON_RIGHT) && paddleX < SCREEN_WIDTH - paddleWidth) paddleX += 3;
  if (millis() - lastFall > ballSpeed) { ballY += 3; lastFall = millis(); }
  if (ballY >= 60 && ballX >= paddleX && ballX <= paddleX + paddleWidth) {
    ballY = 0; ballX = random(0, SCREEN_WIDTH - 5); score++;
  }
  if (ballY > SCREEN_HEIGHT) { beep(); showGameOverScreen(); inGame = false; currentGame = -1; currentPage = 2; return; }
  u8g2.clearBuffer();
  char buf[20];
  sprintf(buf,"S:%d H:%d", score, highScoreCatch);
  u8g2.setFont(u8g2_font_5x8_tr);
  u8g2.drawStr(0,10,buf);
  u8g2.drawBox(paddleX,60,paddleWidth,3);
  u8g2.drawBox(ballX,ballY,5,5);
  u8g2.sendBuffer();
}

void playShooterGame() {
  if (!digitalRead(BUTTON_MENU)) { inGame=false; currentGame=-1; currentPage=2; beep(); delay(200); return; }
  if (!shooterInit) {
    cannonX = (SCREEN_WIDTH-cannonW)/2; bulletActive = false;
    enemyX = random(0,SCREEN_WIDTH-5); enemyY = 0; shooterScore = 0; shooterInit = true;
  }
  if (!digitalRead(BUTTON_LEFT) && cannonX > 0) cannonX -= 3;
  if (!digitalRead(BUTTON_RIGHT) && cannonX < SCREEN_WIDTH - cannonW) cannonX += 3;
  if (!digitalRead(BUTTON_SELECT) && !bulletActive) {
    bulletActive = true; bulletX = cannonX + cannonW / 2; bulletY = SCREEN_HEIGHT - cannonH - 2;
  }
  if (bulletActive) bulletY -= bulletSpeed;
  if (bulletActive && bulletY < 0) bulletActive = false;
  enemyY += enemySpeed;
  if (enemyY > SCREEN_HEIGHT) { beep(); showGameOverScreen(); inGame = false; currentGame = -1; currentPage = 2; return; }
  if (bulletActive && bulletX >= enemyX && bulletX <= enemyX+4 && bulletY <= enemyY+4 && bulletY >= enemyY) {
    shooterScore++; bulletActive = false; enemyY = 0; enemyX = random(0, SCREEN_WIDTH - 5);
  }
  u8g2.clearBuffer();
  char buf[20];
  sprintf(buf,"Sc:%d H:%d", shooterScore, highScoreShooter);
  u8g2.setFont(u8g2_font_5x8_tr);
  u8g2.drawStr(0,10,buf);
  int x0=cannonX,x1=cannonX+cannonW/2,x2=cannonX+cannonW,y0=SCREEN_HEIGHT-1;
  for (int y=0; y<cannonH; y++) {
    int l=x0+y*(x1-x0)/cannonH,r=x2-y*(x2-x1)/cannonH;
    u8g2.drawHLine(l,y0-y,r-l);
  }
  if (bulletActive) u8g2.drawBox(bulletX-1,bulletY,2,4);
  u8g2.drawBox(enemyX,enemyY,5,5);
  u8g2.sendBuffer();
}

void playWallBreaker() {
  if (!digitalRead(BUTTON_MENU)) { inGame=false; currentGame=-1; currentPage=2; beep(); delay(200); return; }
  if (!wbInit) {
    for (int r=0; r<wb_rows; r++) for (int c=0; c<wb_cols; c++) bricks[r][c] = true;
    wb_paddleX = (SCREEN_WIDTH-wb_paddleW)/2;
    wb_ballX = SCREEN_WIDTH/2; wb_ballY = SCREEN_HEIGHT-10;
    wb_ballVX = 2; wb_ballVY = -2;
    wbInit = true;
  }
  if (!digitalRead(BUTTON_LEFT) && wb_paddleX > 0) wb_paddleX -= 4;
  if (!digitalRead(BUTTON_RIGHT) && wb_paddleX < SCREEN_WIDTH - wb_paddleW) wb_paddleX += 4;
  wb_ballX += wb_ballVX; wb_ballY += wb_ballVY;
  if (wb_ballX <= 0 || wb_ballX >= SCREEN_WIDTH-2) wb_ballVX = -wb_ballVX;
  if (wb_ballY <= 0) wb_ballVY = -wb_ballVY;
  if (wb_ballY >= SCREEN_HEIGHT-3) {
    if (wb_ballX >= wb_paddleX && wb_ballX <= wb_paddleX + wb_paddleW) wb_ballVY = -wb_ballVY;
    else { beep(); showGameOverScreen(); inGame = false; currentGame = -1; currentPage = 2; return; }
  }
  int brickW = SCREEN_WIDTH / wb_cols;
  for (int r=0; r<wb_rows; r++) {
    for (int c=0; c<wb_cols; c++) {
      if (bricks[r][c]) {
        int bx = c * brickW, by = 10 + r * 6;
        if (wb_ballX >= bx && wb_ballX < bx + brickW && wb_ballY >= by && wb_ballY < by + 6) {
          bricks[r][c] = false; wb_ballVY = -wb_ballVY; score++;
        }
      }
    }
  }
  bool bricksLeft = false;
  for (int r=0; r<wb_rows; r++)
    for (int c=0; c<wb_cols; c++)
      if (bricks[r][c]) bricksLeft = true;
  if (!bricksLeft) {
    showVictoryScreen();
    inGame = false;
    currentGame = -1;
    currentPage = 2;
    return;
  }

  u8g2.clearBuffer();
  for (int r=0; r<wb_rows; r++) for (int c=0; c<wb_cols; c++) {
    if (bricks[r][c]) {
      int bx = c * brickW, by = 10 + r * 6;
      u8g2.drawBox(bx, by, brickW - 2, 6);
    }
  }
  u8g2.drawBox(wb_paddleX, SCREEN_HEIGHT - 3, wb_paddleW, 3);
  u8g2.drawBox(wb_ballX, wb_ballY, 2, 2);
  char buf[20];
  sprintf(buf,"S:%d H:%d", score, highScoreWallBreaker);
  u8g2.setFont(u8g2_font_5x8_tr);
  u8g2.drawStr(0,10,buf);
  u8g2.sendBuffer();
}

void playFlappyBird() {
  if (!digitalRead(BUTTON_MENU)) { 
    inGame = false; 
    currentGame = -1; 
    currentPage = 2; 
    beep(); 
    delay(200); 
    return; 
  }

  if (!flapInit) { 
    flappyY = 30.0;
    flappyVel = 0.0;
    pipeX = SCREEN_WIDTH; 
    gapY = random(10,40); 
    flapScore = 0; 
    flapInit = true; 
  }

  if (!digitalRead(BUTTON_SELECT)) { 
    flappyVel = flapStrength; 
  }

  flappyVel += gravity;          
  if (flappyVel > 3.0) flappyVel = 3.0;  
  flappyY += flappyVel;          

  pipeX -= 2;
  if (pipeX < -10) { 
    pipeX = SCREEN_WIDTH; 
    gapY = random(10,40); 
    flapScore++; 
  }

  if ((int)flappyY <= 0 || (int)flappyY >= SCREEN_HEIGHT-1 || 
      (pipeX < 15 && ((int)flappyY < gapY || (int)flappyY > gapY+20))) {
    beep(); showGameOverScreen(); 
    inGame = false; 
    currentGame = -1; 
    currentPage = 2; 
    return;
  }

  u8g2.clearBuffer();
  char buf[20];
  sprintf(buf,"S:%d H:%d", flapScore, highScoreFlappy);
  u8g2.setFont(u8g2_font_5x8_tr);
  u8g2.drawStr(0,10,buf);
  u8g2.drawBox(10, (int)flappyY, 5, 5);
  u8g2.drawBox(pipeX, 0, 10, gapY);
  u8g2.drawBox(pipeX, gapY+20, 10, SCREEN_HEIGHT-gapY-20);
  u8g2.sendBuffer();
}

void loop() {
  if (inGame) {
    switch (currentGame) {
      case 0: playCatchGame(); break;
      case 1: playShooterGame(); break;
      case 2: playWallBreaker(); break;
      case 3: playFlappyBird(); break;
      default: inGame = false; currentGame = -1; currentPage = 2;
    }
    return;
  }

  if (!digitalRead(BUTTON_MENU)) { 
    currentPage = (currentPage + 1) % 3; 
    lastSwitch = millis(); 
    delay(200); 
  }

  if (currentPage < 2) {
    unsigned long span = (currentPage == 0 ? 2000 : 3000);
    if (millis() - lastSwitch < span)
      drawPage(currentPage);
    else {
      currentPage = (currentPage + 1) % 3;
      lastSwitch = millis();
    }
  } else {
    if (!digitalRead(BUTTON_UP)) {
      menuIndex = (menuIndex - 1 + 4) % 4;
      //beep();
      delay(200);
    } else if (!digitalRead(BUTTON_DOWN)) {
      menuIndex = (menuIndex + 1) % 4;
      //beep();
      delay(200);
    } else if (!digitalRead(BUTTON_SELECT)) {
      beep();
      delay(200);
      inGame = true;
      currentGame = menuIndex;
      score = 0;
      shooterInit = false;
      wbInit = false;
      flapInit = false;
      paddleX = 54;
      ballX = random(0, SCREEN_WIDTH - 5);
      ballY = 0;
      return;
    }
    drawPage(currentPage);
  }
}
