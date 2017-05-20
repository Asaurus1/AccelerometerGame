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

/////////// Matrix Stuff for Sliding Dot ///////////////
bool showSlidingDot_toggle = true;

                           Matrix grav(3,1);
double dp[] = {64, 64, 0}; Matrix dotPos(3,1,dp);
                           Matrix dotVel(3,1);
                           Matrix dotAccl(3,1);
                           Matrix AcclEndPoint(3,1);
                           Matrix VelEndPoint(3,1);

double accelerationFactor = 2;
double dampingFactor = 0.9;
double dotMaxVel = 10;

/////////// Min and max ADC values { X, Y, Z };
static const int PROGMEM MINV[3] = {370, 413, 426};
static const int PROGMEM MAXV[3] = {650, 613, 625};

////////// Actually accelerometer variables
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
}

void loop() {
  // put your main code here, to run repeatedly:
  x_vread = analogRead(XREAD);
  y_vread = analogRead(YREAD);
  z_vread = analogRead(ZREAD);
  x_accl = (float)map((x_vread*100), (MINV[0]*100), (MAXV[0]*100), 100, -100)/100.0;
  y_accl = (float)map((y_vread*100), (MINV[1]*100), (MAXV[1]*100), 100, -100)/100.0;
  z_accl = (float)map((z_vread*100), (MINV[2]*100), (MAXV[2]*100), 100, -100)/100.0;
    
  Serial.print("X: "); Serial.print(x_accl); Serial.print(", Y: "); Serial.print(y_accl); Serial.print(", Z: "); Serial.println(z_accl);
  if (Serial.available()) {
     if (Serial.read() == 's') {showSlidingDot_toggle=!showSlidingDot_toggle; Serial.print(showSlidingDot_toggle);}
  }
  // Print out data
  if (!showSlidingDot_toggle) {
    showTextOutput();
  }  else {
    clearSlidingDot();
    dotMatrixMath();
    showSlidingDot();
  }
  delay(30);

  if (abs(z_accl > 2.0)) { tft.setCursor(0,80); tft.setTextColor(GREEN); tft.print("Z-accl was\nOVER 9000"); }

}

// Draws the accelerometer values to the display
void showTextOutput() {
  //tft.clearScreen();              // Clear the entire screen
  tft.setCursor(0,0);               // Reset cursor back to top of the screen
  tft.fillRect(100,0,28,50,BLACK);  // Clear the edge of the screen

  setTextColorBasedOnSign(x_accl);
  tft.print("X-Accl: ");
  tft.println(x_accl);
    #ifdef SHOWRAW
      tft.print("X-Raw: ");
      tft.println(x_vread);
    #endif
  setTextColorBasedOnSign(y_accl);
  tft.print("Y-Accl: ");
  tft.println(y_accl);
    #ifdef SHOWRAW
      tft.print("Y-Raw: ");
      tft.println(y_vread);
    #endif
  setTextColorBasedOnSign(z_accl);
  tft.print("Z-Accl: ");
  tft.println(z_accl);
    #ifdef SHOWRAW
      tft.print("Z-Raw: ");
      tft.println(z_vread);
    #endif
}

void showSlidingDot() {
  tft.fillCircle(dotPos(0),dotPos(1),5,RED);
  tft.drawLine(dotPos(0),dotPos(1),AcclEndPoint(0),AcclEndPoint(1),GREEN);
  tft.drawLine(dotPos(0),dotPos(1),VelEndPoint(0),VelEndPoint(1),BLUE);
}

void clearSlidingDot() {
  tft.fillCircle(dotPos(0),dotPos(1),max(7,max(dotVel.norm()*6,dotAccl.norm()*21)),BLACK);
}

void dotMatrixMath() {
  // Set Gravity
  grav(0) = x_accl; grav(1) = y_accl; grav(2) = z_accl;

  // Calculate the normal acceleration vector
  dotAccl = -grav; dotAccl(2) = 0;
  dotAccl(0) = -dotAccl(0); // Fix Coordinate System difference between screen and accelerometer

  dotVel = dotVel*dampingFactor;
  dotVel += dotAccl*accelerationFactor;
  dotVel(0) = constrain(dotVel(0),-dotMaxVel,dotMaxVel);
  dotVel(1) = constrain(dotVel(1),-dotMaxVel,dotMaxVel);
  
  dotPos += dotVel;
  dotWallCollisionHandle();

  AcclEndPoint = dotPos + dotAccl*20;
  VelEndPoint = dotPos + dotVel*5;
}

void dotWallCollisionHandle() {
  dotPos(0) = constrain(dotPos(0),0,128);
  dotPos(1) = constrain(dotPos(1),0,128);

  if (dotPos(0) == 0 && dotVel(0)<0) {dotVel(0) = 0;}
  if (dotPos(1) == 0 && dotVel(1)<0) {dotVel(1) = 0;}
  if (dotPos(0) == 128 && dotVel(0)>0) {dotVel(0) = 0;}
  if (dotPos(1) == 128 && dotVel(1)>0) {dotVel(1) = 0;}
}

inline void setTextColorBasedOnSign(float n) {
  if (n < 0) {
      tft.setTextColor(NEGCOLOR,BLACK);
  } else {
      tft.setTextColor(POSCOLOR,BLACK);
  }
}

