#include <Adafruit_NeoPixel.h>
#include "Wire.h" // This library allows you to communicate with I2C devices.

#define N 8

#define NORTH 0
#define EAST 1
#define SOUTH 2
#define WEST 3

#define FRONT 0
#define RIGHT 1
#define BACK 2
#define LEFT 3
#define TOP 4
#define BOTTOM 5

#define LED_PIN1 0 //Green
#define LED_PIN2 2 //Yellow
#define LED_PIN3 4 //Orange
#define LED_PIN4 27 //Purple
#define LED_PIN5 25 //Brown
#define LED_PIN6 32 // White

#define show_noglitch_1() {delay(1);M1.show();delay(1);M1.show();}
#define show_noglitch_2() {delay(1);M2.show();delay(1);M2.show();}
#define show_noglitch_3() {delay(1);M3.show();delay(1);M3.show();}
#define show_noglitch_4() {delay(1);M4.show();delay(1);M4.show();}
#define show_noglitch_5() {delay(1);M5.show();delay(1);M5.show();}
#define show_noglitch_6() {delay(1);M6.show();delay(1);M6.show();}

const int MPU_ADDR = 0x68; // I2C address of the MPU-6050. If AD0 pin is set to HIGH, the I2C address will be 0x69.

Adafruit_NeoPixel M1 = Adafruit_NeoPixel(64, LED_PIN1, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel M2 = Adafruit_NeoPixel(64, LED_PIN2, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel M3 = Adafruit_NeoPixel(64, LED_PIN3, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel M4 = Adafruit_NeoPixel(64, LED_PIN4, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel M5 = Adafruit_NeoPixel(64, LED_PIN5, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel M6 = Adafruit_NeoPixel(64, LED_PIN6, NEO_GRB + NEO_KHZ800);

int cube[6][N*N];
int face, X, Y;
int wI, wX, wY;
int temp;

int16_t accelerometer_x, accelerometer_y, accelerometer_z; // variables for accelerometer raw data
int16_t horizontal, vertical;
char tmp_str[7]; // temporary variable used in convert function

int dirs[4] = {NORTH, EAST, SOUTH, WEST};

void initCube(){
  for(int i=0; i<6; i++){
    for(int j=0; j<N*N; j++){
      cube[i][j] = 0;
    }
  }
}

char* convert_int16_to_str(int16_t i) { // converts int16 to string. Moreover, resulting strings will have the same length in the debug monitor.
  sprintf(tmp_str, "%6d", i);
  return tmp_str;
}

int XYLED( int x, int y ) {
  int index = 0;
  if (y % 2 == 0) {
    index = y * N + (N - x - 1);
  } else {
    index = y * N + x;
  }
  return index;
}

int XYflat(int x, int y){
  return y*N + x;
}

void printCube() {
    Serial.println();
    for (int y = 0; y < N; y++) {
        Serial.print("                        ");
        for (int x = 0; x < N; x++) {
            if (y == Y && x == X && face == 4) { Serial.print(" x "); } 
            else if (y == wY && x == wX && 4 == wI) { Serial.print(" + "); } 
            else if (cube[4][XYflat(x,y)]){ Serial.print("   "); }
            else { Serial.print("|"); Serial.print(4); Serial.print("|"); }
        }
        Serial.println();
    }
    
    for (int y = 0; y < N; y++) {
        for (int x = 0; x < N*4; x++) {
            int f;
            switch(x){
                case 0 ... 7:   f = LEFT; break;
                case 8 ... 15:  f = FRONT; break;
                case 16 ... 23: f = RIGHT; break;
                case 24 ... 31: f = BACK; break;
            }
            if (y == Y && x%8 == X && f == face) { Serial.print(" x "); }
            else if (y == wY && x%8 == wX && f == wI) { Serial.print(" + "); } 
            else if (cube[f][XYflat(x%8,y)]){ Serial.print("   "); } 
            else { Serial.print("|"); Serial.print(f); Serial.print("|"); }
        }
        Serial.println();
    }
    
    for (int y = 0; y < N; y++) {
        Serial.print("                        ");
        for (int x = 0; x < N; x++) {
            if (y == Y && x == X && face == 5) { Serial.print(" x "); } 
            else if (y == wY && x == wX && 5 == wI) { Serial.print(" + "); } 
            else if (cube[5][XYflat(x,y)]){ Serial.print("   "); }
            else { Serial.print("|"); Serial.print(5); Serial.print("|"); }
        }
        Serial.println();
    }
}


struct coord {
    int i; int x; int y; int dx; int dy;
};

struct coord normCoord(int i, int x, int y, int dx, int dy){
    x+=dx;
    y+=dy;

    switch (i) {
        case FRONT:
            if (y < 0)        { i = TOP; y = N - 1; } 
            else if (y >= N)  { i = BOTTOM; y = 0; } 
            else if (x < 0)   { i = LEFT; x = N - 1; } 
            else if (x >= N)  { i = RIGHT; x = 0; }
            break;
        case RIGHT:
            if (y < 0)        { i = TOP; y = N - 1 - x; x = N - 1; temp = dx; dx = dy; dy = dx; } 
            else if (y >= N)  { i = BOTTOM; y = x; x = N - 1; temp = dx; dx = -dy; dy = dx; } 
            else if (x < 0)   { i = FRONT; x = N - 1; } 
            else if (x >= N)  { i = BACK; x = 0; }
            break;
        case BACK:
            if (y < 0)        { i = TOP; x = N - 1 - x; y = 0; dy = -dy; } 
            else if (y >= N)  { i = BOTTOM; x = N - 1 - x; y = N - 1; dy = -dy; } 
            else if (x < 0)   { i = RIGHT; x = N - 1; } 
            else if (x >= N)  { i = LEFT; x = 0; }
            break;
        case LEFT:
            if (y < 0)        { i = TOP; y = x; x = 0; temp = dx; dx = -dy; dy = dx; } 
            else if (y >= N)  { i = BOTTOM; y = N - 1 - x; x = 0; temp = dx; dx = dy; dy = dx; } 
            else if (x < 0)   { i = BACK; x = N - 1; } 
            else if (x >= N)  { i = FRONT; x = 0; }
            break;
        case TOP:
            if (x < 0)        { i = LEFT; x = y; y = 0; temp = dx; dx = dy; dy = -dx; } 
            else if (x >= N)  { i = RIGHT; x = N - 1 - y; y = 0; temp = dx; dx = dy; dy = dx; } 
            else if (y < 0)   { i = BACK; x = N - 1 - x; y = 0; } 
            else if (y >= N)  { i = FRONT; y = 0; }
            break;
        case BOTTOM:
            if (x < 0)        { i = LEFT; x = N - 1 - y; y = N - 1; temp = dx; dx = dy; dy = dx; } 
            else if (x >= N)  { i = RIGHT; x = y; y = N - 1; temp = dx; dx = dy; dy = -dx; } 
            else if (y < 0)   { i = FRONT; y = N - 1; } 
            else if (y >= N)  { i = BACK; x = N - 1 - x; y = N - 1; }
            break;
    }

    coord new_coord = {i, x, y, dx, dy};
    return new_coord;
}

coord c1, c2, c3, c4;
int numWalls(int i, int x, int y) {
    c1 = normCoord(i,x,y,1,0);
    c2 = normCoord(i,x,y,-1,0);
    c3 = normCoord(i,x,y,0,1);
    c4 = normCoord(i,x,y,0,-1);
    return 4 - (cube[c1.i][XYflat(c1.x, c1.y)] + cube[c2.i][XYflat(c2.x, c2.y)] + cube[c3.i][XYflat(c3.x, c3.y)] + cube[c4.i][XYflat(c4.x, c4.y)]);
}

int r;
coord d1, d2;
void Visit(int i, int x, int y) {  
    cube[i][XYflat(x, y)] = 1;
  
    for (int k = 3; k > 0; --k) {
        r = random(k + 1);
        temp = dirs[k];
        dirs[k] = dirs[r];
        dirs[r] = temp;
    }
      
    for (int k=0; k<4; ++k){
        int dx=0, dy=0;
        switch (dirs[k]) {
          case NORTH: dy = -1; break;
          case EAST:  dx = 1; break;
          case SOUTH: dy = 1; break;
          case WEST:  dx = -1; break;
        }
        
        d1 = normCoord(i, x, y, dx, dy);
        
        d2 = normCoord(d1.i, d1.x, d1.y, d1.dx, d1.dy);
        
        if (cube[d2.i][XYflat(d2.x, d2.y)] == 0 and numWalls(d2.i,d2.x,d2.y) == 4){
            cube[d1.i][XYflat(d1.x, d1.y)] = 1;
            Visit(d2.i,d2.x,d2.y);
        }        
    }
}

coord c;
void move(int dir) {
    int dx, dy;
    dx = 0;
    dy = 0;
    switch(dir) {
        case NORTH: dy = -1; break;
        case SOUTH: dy = 1; break;
        case EAST:  dx = 1; break;
        case WEST:  dx = -1; break;
    }
    
    c = normCoord(face, X, Y, dx, dy);
    
    if (cube[c.i][XYflat(c.x, c.y)]){
        X = c.x;
        Y = c.y;
        face = c.i;
    }
}

int randi, randx, randy, num_walls;
void generateSolution() {
    while(true){
        randi = random(6); randx = random(N); randy = random(N);
        num_walls = numWalls(randi,randx,randy);
        if (cube[randi][XYflat(randx, randy)] && num_walls > 2){
            wI = randi; wX = randx; wY = randy;
            break;
        }
    }
}

void winning(){
  rainbow();
}

uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return M1.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return M1.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return M1.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

void rainbow() {
  int i, j;
  for(j=0; j<256; j++) {
    for(i=0; i<M1.numPixels(); i++) {
      M1.setPixelColor(i, Wheel((i+j) & 255));
      M2.setPixelColor(i, Wheel((i+j) & 255));
      M3.setPixelColor(i, Wheel((i+j) & 255));
      M4.setPixelColor(i, Wheel((i+j) & 255));
      M5.setPixelColor(i, Wheel((i+j) & 255));
      M6.setPixelColor(i, Wheel((i+j) & 255));
    }
    show_noglitch_1();
    show_noglitch_2();
    show_noglitch_3();
    show_noglitch_4();
    show_noglitch_5();
    show_noglitch_6();
    delay(20);
  }
}
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  initCube();
  X=random(N);
  Y=random(N);
  face = 0;
  Visit(face, X, Y);
  generateSolution();

  M1.begin();
  M1.setBrightness(5);
  M1.clear();
  M1.setPixelColor(0, 0);
  show_noglitch_1();
  
  M2.begin();
  M2.setBrightness(5);
  M2.clear();
  M2.setPixelColor(0, 0);
  show_noglitch_2();

  M3.begin();
  M3.setBrightness(5);
  M3.clear();
  M3.setPixelColor(0, 0);
  show_noglitch_3();

  M4.begin();
  M4.setBrightness(5);
  M4.clear();
  M4.setPixelColor(0, 0);
  show_noglitch_4();

  M5.begin();
  M5.setBrightness(5);
  M5.clear();
  M5.setPixelColor(0, 0);
  show_noglitch_5();

  M6.begin();
  M6.setBrightness(5);
  M6.clear();
  M6.setPixelColor(0, 0);
  show_noglitch_6();


  Wire.begin();
  Wire.beginTransmission(MPU_ADDR); // Begins a transmission to the I2C slave (GY-521 board)
  Wire.write(0x6B); // PWR_MGMT_1 register
  Wire.write(0); // set to zero (wakes up the MPU-6050)
  Wire.endTransmission(true);

//  calculate_IMU_error();

}


unsigned long previousTime = 0;
unsigned long currentTime = 0;
unsigned long elapsedTime = 0;
int counter = 0;
unsigned long int shakeTime = 0;
uint32_t wallColor = M1.Color(0,255,0);
uint32_t winColor = M1.Color(255,255,255);
void loop() {
  // put your main code here, to run repeatedly
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B); // starting with register 0x3B (ACCEL_XOUT_H) [MPU-6000 and MPU-6050 Register Map and Descriptions Revision 4.2, p.40]
  Wire.endTransmission(false); // the parameter indicates that the Arduino will send a restart. As a result, the connection is kept active.
  Wire.requestFrom(MPU_ADDR, 7 * 2, true); // request a total of 7*2=14 registers

  accelerometer_x = Wire.read() << 8 | Wire.read(); // reading registers: 0x3B (ACCEL_XOUT_H) and 0x3C (ACCEL_XOUT_L)
  accelerometer_y = Wire.read() << 8 | Wire.read(); // reading registers: 0x3D (ACCEL_YOUT_H) and 0x3E (ACCEL_YOUT_L)
  accelerometer_z = Wire.read() << 8 | Wire.read(); // reading registers: 0x3F (ACCEL_ZOUT_H) and 0x40 (ACCEL_ZOUT_L)
  Serial.print("aX = "); Serial.print(convert_int16_to_str(accelerometer_x));
  Serial.print(" | aY = "); Serial.print(convert_int16_to_str(accelerometer_y));
  Serial.print(" | aZ = "); Serial.print(convert_int16_to_str(accelerometer_z));


  Serial.println(" ");



 int16_t x_offset, y_offset;
// switch(face) {
//        case 0: x_offset = 0; y_offset=0; break;
//        case 1:  x_offset = -16000; y_offset=0; break;
//        case 2:   x_offset = 0; y_offset=8000; break;
//        case 3:   x_offset = -8000; y_offset=0; break;
//        case 4:   x_offset = 0; y_offset=-8000; break;
//        case 5:   x_offset = 0; y_offset=-8000; break;
//    }
  if (accelerometer_z<-22000||accelerometer_y<-22000||accelerometer_x<-22000) {
    Serial.println("SHAKE");
    if(counter ==0){
      shakeTime = millis();
    }
    if(millis()<(shakeTime+5000)){
      counter ++;
      if(counter>3){
      counter=0;
      initCube();
      
      X = random(N);
      Y = random(N);
      Visit(face, X, Y);
      generateSolution();
      wallColor = M1.Color(random(127),random(255),random(255));
     
      
      }
      
    }else{
      counter = 0;
    }
  }
  Serial.print("Winning condition: ");
   Serial.print("face: ");
   Serial.print(wI);
   Serial.print("x: ");
   Serial.print(wX);
   Serial.print("y: ");
   Serial.print(wY);
   Serial.print("wall Color");
   Serial.println(winColor);
  if(face == 2){
    accelerometer_x *=-1;
  }
  if(face ==1 ){
    accelerometer_z *=-1;
  }
  if(face == 4){
    accelerometer_z *=-1;
  }
  if (face == 0 || face ==2){
    horizontal = accelerometer_x;
    vertical = accelerometer_y;
  }

  if (face == 4 || face ==5){
    horizontal = accelerometer_x;
    vertical = accelerometer_z;
  }

  if (face == 1 || face ==3){
    horizontal = accelerometer_z;
    vertical = accelerometer_y;
  }
  
  if (vertical< -8000) {
//    if(face==2){
//      Serial.println("DOWN");
//      move(SOUTH);
//    }else{
    Serial.println(convert_int16_to_str(y_offset));
    Serial.println("UP");
    move(NORTH);
    //}
    
  } else if (horizontal< -8000) {
//    if(face==2){
//      Serial.println("LEFT");
//      move(WEST);
//    }else{
      Serial.println(convert_int16_to_str(x_offset));
      Serial.println("RIGHT");
      move(EAST);
    //}
    
  } else if (vertical> 8000) {
//    if(face==2){
//      Serial.println("UP");
//      move(NORTH);
//    }else{
    Serial.println(convert_int16_to_str(y_offset));
      Serial.println("DOWN");
    move(SOUTH);
    //}
    
  } else if (horizontal> 8000) {
//    if(face==2){
//      Serial.println("RIGHT");
//      move(EAST);
//    }else{
      Serial.println(convert_int16_to_str(x_offset));
      Serial.println("LEFT");
      move(WEST);
    //}
    
  } else {
    Serial.println("STILL");
  }


  for (int y = 0; y < N; ++y) {
    for (int x = 0; x < N; ++x) {
      int led_index = XYLED(x, y);
      if (cube[0][XYflat(x, y)]) {
        if (x == X && y == Y && face == 0) {
          M1.setPixelColor(led_index, M1.Color(255, 0, 0));
        } else if(x ==wX && y == wY && 0 == wI){
          M1.setPixelColor(led_index, winColor);
        } else {
          M1.setPixelColor(led_index, M1.Color(0, 0, 0));
        }

      } else {
        M1.setPixelColor(led_index, wallColor);
      }
    }
  }

  for (int y = 0; y < N; ++y) {
    for (int x = 0; x < N; ++x) {
      int led_index = XYLED(x, y);
      if (cube[1][XYflat(x, y)]) {
        if (x == X && y == Y && face == 1) {
          M2.setPixelColor(led_index, M2.Color(255, 0, 0));
        }  else if(x ==wX && y == wY && 1 == wI){
          M2.setPixelColor(led_index, winColor);
        }else {
          M2.setPixelColor(led_index, M2.Color(0, 0, 0));
        }

      } else {
        M2.setPixelColor(led_index, wallColor);
      }
    }
  }

  for (int y = 0; y < N; ++y) {
    for (int x = 0; x < N; ++x) {
      int led_index = XYLED(x, y);
      if (cube[2][XYflat(x, y)]) {
        if (x == X && y == Y && face == 2) {
          M3.setPixelColor(led_index, M3.Color(255, 0, 0));
        }  else if(x ==wX && y == wY && 2 == wI){
          M3.setPixelColor(led_index, winColor);
        }else {
          M3.setPixelColor(led_index, M3.Color(0, 0, 0));
        }

      } else {
        M3.setPixelColor(led_index, wallColor);
      }
    }
  }

  for (int y = 0; y < N; ++y) {
    for (int x = 0; x < N; ++x) {
      int led_index = XYLED(x, y);
      if (cube[3][XYflat(x, y)]) {
        if (x == X && y == Y && face == 3) {
          M4.setPixelColor(led_index, M4.Color(255, 0, 0));
        }  else if(x ==wX && y == wY && 3 == wI){
          M4.setPixelColor(led_index, winColor);
        }else {
          M4.setPixelColor(led_index, M4.Color(0, 0, 0));
        }

      } else {
        M4.setPixelColor(led_index, wallColor);
      }
    }
  }

  for (int y = 0; y < N; ++y) {
    for (int x = 0; x < N; ++x) {
      int led_index = XYLED(x, y);
      if (cube[4][XYflat(x, y)]) {
        if (x == X && y == Y && face == 4) {
          M5.setPixelColor(led_index, M5.Color(255, 0, 0));
        } else if(x ==wX && y == wY && 4 == wI){
          M5.setPixelColor(led_index, winColor);
        }else{
          M5.setPixelColor(led_index, M5.Color(0, 0, 0));
        }

      } else {
        M5.setPixelColor(led_index,wallColor);
      }
    }
  }

  for (int y = 0; y < N; ++y) {
    for (int x = 0; x < N; ++x) {
      int led_index = XYLED(x, y);
      if (cube[5][XYflat(x, y)]) {
        if (x == X && y == Y && face == 5) {
          M6.setPixelColor(led_index, M6.Color(255, 0, 0));
        }  else if(x ==wX && y == wY && 5 == wI){
          M6.setPixelColor(led_index, winColor);
        }else {
          M6.setPixelColor(led_index, M6.Color(0, 0, 0));
        }

      } else {
        M6.setPixelColor(led_index, wallColor);
      }
    }
  }
  
 

  show_noglitch_1();
  show_noglitch_2();
  show_noglitch_3();
  show_noglitch_4();
  show_noglitch_5();
  show_noglitch_6();
  if(wX==X&& wY==Y&& wI==face){
    winning();
    initCube();
    face = random(6);
    X = random(N);
    Y = random(N);
    Visit(face,X,Y);
    generateSolution();
  }

  Serial.print(face);
  Serial.print(" ");
  Serial.print(X);
  Serial.print(" ");
  Serial.print(Y);
  Serial.println();
  Serial.println("END OF LOOP");

}
