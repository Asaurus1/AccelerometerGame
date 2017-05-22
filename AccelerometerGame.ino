#include <LinkedList.h>
#include <Matrix.h>
#include <TFT_ST7735.h>

//#define SHOWRAW

#define _CS 10
#define _DC 9
#define XREAD A1
#define YREAD A2
#define ZREAD A3
#define POSCOLOR WHITE
#define NEGCOLOR RED
bool showSlidingDot_toggle = true;

/////////// Dot Object Declaration ///////////////
#define DOT_XSTART 64
#define DOT_YSTART 64
#define DOT_SIZE 5
#define DOT_COLOR RED
#define DOT_XMIN 0
#define DOT_XMAX 127
#define DOT_YMIN 10
#define DOT_YMAX 127

struct obj_dot {
  Matrix position;
  Matrix velocity;
  Matrix accl;
} dot = { Matrix(3,1), Matrix(3,1), Matrix(3,1) };

// Other drawing vectors
Matrix AcclEndPoint(3,1);
Matrix VelEndPoint(3,1);

double accelerationFactor = 1.5;
double dampingFactor = 0.9;
double dotMaxVel = 10;

/////////// Food Object Declaration
#define FOOD_SIZE 2
#define FOOD_COLOR YELLOW

struct obj_food {
  // Constructor
  obj_food()  {position = Matrix(3,1);}
  obj_food(int x, int y) { position = Matrix(3,1); position(0) = x; position(1) = y; }

  // Members  
  Matrix position;
  char   color = BLUE;
  bool   isWithinRadius(int x, int y, int r) { return pow(x-position(0),2) + pow(y-position(1),2) <= pow(r,2); }
  static obj_food& createFood(int x, int y);
  static void destroyFood(int index);
};

LinkedList<obj_food*> foodInstances;

obj_food& obj_food::createFood(int x, int y) { 
  obj_food* newFood = new obj_food(x, y);
  foodInstances.add(newFood);
  return *newFood;
}

void obj_food::destroyFood(int index) {
  obj_food* deletingFood = foodInstances.remove(index);
  delete deletingFood;
}

/////////// Game Variables
#define   TIMEPERROUND 1000
int       score = 0;
long      timeRemaining;
enum      gs_var {RUNNING, GAMEOVER} gameState = RUNNING;

/////////// Min and max ADC values { X, Y, Z };
const int PROGMEM MINV[3] = {370, 413, 426};
const int PROGMEM MAXV[3] = {650, 613, 625};

////////// Actual accelerometer variables
int x_vread;
int y_vread;
int z_vread;
float x_accl;
float y_accl;
float z_accl;

TFT_ST7735 tft = TFT_ST7735(_CS,_DC);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  tft.begin();
  tft.setTextScale(2);

  resetGame();  
}

void resetGame() {
  // Initialize Dot Position
  dot.position(0) = DOT_XSTART;
  dot.position(1) = DOT_YSTART;

  // Remove existing food
  while (foodInstances.size() > 0) {
    delete foodInstances.pop();
  }

  // Create initial food on screen
  randomSeed(analogRead(A0));
  for (int i = 0; i < 12; i++) { obj_food::createFood(random(128),random(116)+12); }  

  // clear screen
  tft.clearScreen();

  score = 0;
  timeRemaining = TIMEPERROUND;
  gameState = RUNNING;
}

void waitForShake(int timeout) {
  long startTime = millis();
  while (millis() < startTime + timeout && x_accl <1.3 && y_accl < 1.3) {readAcclData(); delay(20);}
}

void loop() {
  readAcclData();

  // Print to screen
  if (gameState == RUNNING) {
    clearSlidingDot();
    dotMatrixMath();
    dotFoodCollisionHandle();
    drawSlidingDot();
    drawFood();
    drawVectors();
    drawScorebar();
 
    // Create new food
    if (random(100)<5) {
      obj_food::createFood(random(128),random(116)+12);
    }

    // Countdown
    timeRemaining -= 1;
    if (timeRemaining <= 0) { gameState = GAMEOVER; }
  } 
  else {
    tft.clearScreen();
    tft.setTextScale(2);
    tft.setTextColor(RED);
    tft.setCursor(CENTER, 50, SCREEN);
    tft.print("GAME OVER\n");
    tft.setTextScale(1);
    tft.setTextColor(WHITE);
    tft.setCursor(CENTER, 80, SCREEN);
    tft.print("SHAKE TO RETRY");
    drawScorebar();
    waitForShake(20000);
    resetGame();
  }

  //Wait till next gamestep
  delay(20);
}

void drawScorebar() {
  tft.fillRect(60,0,128,10,BLACK);
  
  tft.setTextScale(1);
  tft.setTextColor(WHITE,BLACK);
  tft.setCursor(0,0);
  tft.print("Score: ");
  tft.print(score);

  tft.setCursor(60,0);
  tft.print(timeRemaining);
}

void drawSlidingDot() {
  tft.fillCircle(dot.position(0),dot.position(1),DOT_SIZE,DOT_COLOR);
}

void drawVectors() {
  tft.drawLine(dot.position(0),dot.position(1),AcclEndPoint(0),AcclEndPoint(1),GREEN);
  tft.drawLine(dot.position(0),dot.position(1),VelEndPoint(0),VelEndPoint(1),BLUE);
}

void drawFood() {
  int fSize = foodInstances.size();
  for (int i = 0; i < fSize; i++) {
    obj_food* f = foodInstances.get(i);
    tft.fillCircle(f->position(0), f->position(1), FOOD_SIZE, FOOD_COLOR);
  }
}

void clearSlidingDot() {
  tft.fillCircle(dot.position(0),dot.position(1),max(7,max(dot.velocity.norm()*6,dot.accl.norm()*21)),BLACK);
}

void dotMatrixMath() {
  // Set Gravity
  dot.accl(0) = x_accl; 
  dot.accl(1) = -y_accl; 

  // Integrate to Velocity
  dot.velocity = dot.velocity*dampingFactor;
  dot.velocity += dot.accl*accelerationFactor;
  dot.velocity(0) = constrain(dot.velocity(0),-dotMaxVel,dotMaxVel);
  dot.velocity(1) = constrain(dot.velocity(1),-dotMaxVel,dotMaxVel);
  
  // Integrate to Position
  dot.position += dot.velocity;
  dotWallCollisionHandle();

  // Update vector drawing points
  AcclEndPoint = dot.position + dot.accl*20;
  VelEndPoint = dot.position + dot.velocity*5;
}

void dotWallCollisionHandle() {
  dot.position(0) = constrain(dot.position(0),DOT_XMIN+DOT_SIZE,DOT_XMAX-DOT_SIZE);
  dot.position(1) = constrain(dot.position(1),DOT_YMIN+DOT_SIZE,DOT_YMAX-DOT_SIZE);

  if (dot.position(0) == DOT_XMIN+DOT_SIZE && dot.velocity(0) < 0) {dot.velocity(0) = 0;}
  if (dot.position(1) == DOT_YMIN+DOT_SIZE && dot.velocity(1) < 0) {dot.velocity(1) = 0;}
  if (dot.position(0) == DOT_XMAX-DOT_SIZE && dot.velocity(0) > 0) {dot.velocity(0) = 0;}
  if (dot.position(1) == DOT_YMAX-DOT_SIZE && dot.velocity(1)> 0) {dot.velocity(1) = 0;}
}

void dotFoodCollisionHandle() {
  for (int i = 0; i < foodInstances.size(); i++) {
    obj_food* f = foodInstances.get(i);
    if (f->isWithinRadius(dot.position(0),dot.position(1),DOT_SIZE+FOOD_SIZE)) {
      tft.fillCircle(f->position(0), f->position(1), FOOD_SIZE, BLACK);
      obj_food::destroyFood(i);
      score++;
    }
  }
}

void readAcclData() {
  // Read accelerometer voltages from ADC and convert to -1 to 1 scale.
  x_vread = analogRead(XREAD);
  y_vread = analogRead(YREAD);
  z_vread = analogRead(ZREAD);
  x_accl = (float)map((x_vread*100), (MINV[0]*100), (MAXV[0]*100), 100, -100)/100.0;
  y_accl = (float)map((y_vread*100), (MINV[1]*100), (MAXV[1]*100), 100, -100)/100.0;
  z_accl = (float)map((z_vread*100), (MINV[2]*100), (MAXV[2]*100), 100, -100)/100.0;
  
  // Print Raw Acceleration Data 
  Serial.print("X: "); Serial.print(x_accl); Serial.print(", Y: "); Serial.print(y_accl); Serial.print(", Z: "); Serial.println(z_accl);
}

inline void setTextColorBasedOnSign(float n) {
  if (n < 0) {
      tft.setTextColor(NEGCOLOR,BLACK);
  } else {
      tft.setTextColor(POSCOLOR,BLACK);
  }
}

