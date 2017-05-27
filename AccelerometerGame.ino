#include <LinkedList.h>
#include <Matrix.h>
//#define ST7735_DISPL
#define HX8357_DISPL

#if defined(ST7735_DISPL)
  #include <TFT_ST7735.h>
#elif defined(HX8357_DISPL)
  #include <HX8357_t3.h>
#endif

#define SHOWVECTORS

#define _CS 10
#define _DC 9
#define XREAD A1
#define YREAD A2
#define ZREAD A3
#define POSCOLOR WHITE
#define NEGCOLOR RED

/////////// Screen Stuff
#if defined(ST7735_DISPL)
  TFT_ST7735 tft = TFT_ST7735(_CS,_DC);
  #define SCREENSIZE_X 128
  #define SCREENSIZE_Y 128
#elif defined(HX8357_DISPL)
  HX8357_t3 tft = HX8357_t3(_CS,_DC);
  #define BLACK       HX8357_BLACK      
  #define NAVY        HX8357_NAVY       
  #define DARKGREEN   HX8357_DARKGREEN  
  #define DARKCYAN    HX8357_DARKCYAN   
  #define MAROON      HX8357_MAROON     
  #define PURPLE      HX8357_PURPLE     
  #define OLIVE       HX8357_OLIVE      
  #define LIGHTGREY   HX8357_LIGHTGREY  
  #define DARKGREY    HX8357_DARKGREY   
  #define BLUE        HX8357_BLUE       
  #define GREEN       HX8357_GREEN      
  #define CYAN        HX8357_CYAN       
  #define RED         HX8357_RED        
  #define MAGENTA     HX8357_MAGENTA    
  #define YELLOW      HX8357_YELLOW     
  #define WHITE       HX8357_WHITE      
  #define ORANGE      HX8357_ORANGE     
  #define GREENYELLOW HX8357_GREENYELLOW
  #define PINK        HX8357_PINK   
  #define setTextScale setTextSize   // Just a quick fix cause the libraries don't quite match up
  #define SCREENSIZE_X HX8357_TFTWIDTH
  #define SCREENSIZE_Y HX8357_TFTHEIGHT
#endif

/////////// Dot Object Declaration ///////////////
#define DOT_XSTART SCREENSIZE_X/2
#define DOT_YSTART SCREENSIZE_Y/2
#define DOT_SIZE 15
#define DOT_COLOR RED
#define DOT_XMIN 0
#define DOT_XMAX SCREENSIZE_X-1
#define DOT_YMIN 10
#define DOT_YMAX SCREENSIZE_Y-1

struct obj_dot {
  Matrix position;
  Matrix velocity;
  Matrix accl;
} dot = { Matrix(3,1), Matrix(3,1), Matrix(3,1) };

// Other drawing vectors
Matrix AcclEndPoint(3,1);
Matrix VelEndPoint(3,1);

double accelerationFactor = 3;
double dampingFactor = 0.9;
double dotMaxVel = 20;

/////////// Food Object Declaration
#define FOOD_SIZE 5
#define FOOD_COLOR YELLOW

struct obj_food {
  // Constructor
  obj_food()  {position = Matrix(3,1);}
  obj_food(int x, int y) { position = Matrix(3,1); position(0) = x; position(1) = y; }

  // Members  
  Matrix position;
  char   color = YELLOW;
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
  for (int i = 0; i < 12; i++) { obj_food::createFood(random(DOT_XMAX),random(DOT_YMAX-12)+12); }  

  // clear screen
  tft.fillScreen(BLACK);
  tft.setCursor(0,0);

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
      obj_food::createFood(random(DOT_XMAX),random(DOT_YMAX-12)+12);
    }

    // Countdown
    timeRemaining -= 1;
    if (timeRemaining <= 0) { gameState = GAMEOVER; }
  } 
  else {
    tft.fillScreen(BLACK);
    tft.setCursor(0,0);
    tft.setTextScale(2);
    tft.setTextColor(RED);
    tft.setCursor(0, 50);
    tft.print("GAME OVER\n");
    tft.setTextScale(1);
    tft.setTextColor(WHITE);
    tft.setCursor(0,80);
    tft.print("SHAKE TO RETRY");
    drawScorebar();
    waitForShake(20000);
    resetGame();
  }

  //Wait till next gamestep
  delay(20);
}

void drawScorebar() {
  tft.fillRect(SCREENSIZE_X/2,0,SCREENSIZE_X,15,BLACK);
  tft.setTextScale(2);
  tft.setTextColor(WHITE,BLACK);
  tft.setCursor(0,0);
  tft.print("Score: ");
  tft.print(score);

  tft.setCursor(SCREENSIZE_X/2,0);
  tft.print(timeRemaining);
}

void drawSlidingDot() {
  tft.fillCircle(dot.position(0),dot.position(1),DOT_SIZE,DOT_COLOR);
}

void drawVectors() {
  tft.drawLine(max(0,dot.position(0)),max(0,dot.position(1)),max(0,AcclEndPoint(0)),max(0,AcclEndPoint(1)),GREEN);
  tft.drawLine(max(0,dot.position(0)),max(0,dot.position(1)),max(0,VelEndPoint(0)),max(0,VelEndPoint(1)),BLUE);
}

void drawFood() {
  int fSize = foodInstances.size();
  for (int i = 0; i < fSize; i++) {
    obj_food* f = foodInstances.get(i);
    tft.fillCircle(f->position(0), f->position(1), FOOD_SIZE, FOOD_COLOR);
  }
}

void clearSlidingDot() {
  tft.fillCircle(dot.position(0),dot.position(1),max(DOT_SIZE+1,max(dot.velocity.norm()*6,dot.accl.norm()*21)),BLACK);
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

