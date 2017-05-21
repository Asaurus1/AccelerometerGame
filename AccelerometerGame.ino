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

double accelerationFactor = 2;
double dampingFactor = 0.9;
double dotMaxVel = 10;

/////////// Food Object Declaration
struct obj_food {
  Matrix position;
  char   color = BLUE;
};

obj_food foodInstances[50];
int      foodCount = 0;


/////////// Game Variables
int       score = 0;
long      timeRemaining = 1000;

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

  // Initialize Dot Position
  dot.position(0) = DOT_XSTART;
  dot.position(1) = DOT_YSTART;
}

void loop() {
  readAcclData();

  // Print to screen
  clearSlidingDot();
  dotMatrixMath();
  drawSlidingDot();
  drawVectors();
  drawScorebar();

  // Countdown
  timeRemaining -= 1;

  //Wait till next gamestep
  delay(30);
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
  tft.fillCircle(dot.position(0),dot.position(1),DOT_SIZE,RED);
}

void drawVectors() {
  tft.drawLine(dot.position(0),dot.position(1),AcclEndPoint(0),AcclEndPoint(1),GREEN);
  tft.drawLine(dot.position(0),dot.position(1),VelEndPoint(0),VelEndPoint(1),BLUE);
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

