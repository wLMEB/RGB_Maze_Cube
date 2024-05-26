#include <Adafruit_NeoPixel.h>
#include "Wire.h" // This library allows you to communicate with I2C devices.

#define LED_PIN 0
#define GRID_WIDTH 8
#define GRID_HEIGHT 16
#define NORTH 0
#define EAST 1
#define SOUTH 2
#define WEST 3
#define show_noglitch() {delay(1);Matrix.show();delay(1);Matrix.show();}


int grid[GRID_WIDTH * GRID_HEIGHT];
const int MPU_ADDR = 0x68; // I2C address of the MPU-6050. If AD0 pin is set to HIGH, the I2C address will be 0x69.

int16_t accelerometer_x, accelerometer_y, accelerometer_z; // variables for accelerometer raw data
int16_t x_offset, y_offset;
char tmp_str[7]; // temporary variable used in convert function
int px = 1;
int py = 1;
int face = 0;

Adafruit_NeoPixel Matrix = Adafruit_NeoPixel(GRID_WIDTH*GRID_HEIGHT,
                           LED_PIN, NEO_GRB + NEO_KHZ800);

char* convert_int16_to_str(int16_t i) { // converts int16 to string. Moreover, resulting strings will have the same length in the debug monitor.
  sprintf(tmp_str, "%6d", i);
  return tmp_str;
}

void ResetGrid() {
  // Fills the grid with walls ('#' characters).
  for (int i = 0; i < GRID_WIDTH * GRID_HEIGHT; ++i) {
    grid[i] = 0;
  }
}

int XYToIndex( int x, int y ) {
  // Converts the two-dimensional index pair (x,y) into a
  // single-dimensional index. The result is y * ROW_WIDTH + x.
  return y * GRID_WIDTH + x;
}

int XYToIndexLED( int x, int y ) {
  int index = 0;
  if (y % 2 == 0) {
    index = y * GRID_WIDTH + (GRID_WIDTH - x - 1);
  } else {
    index = y * GRID_WIDTH + x;
  }
  return index;
}

int IsInBounds( int x, int y ) {
  // Returns "true" if x and y are both in-bounds.
  if (x < 0 || x >= GRID_WIDTH) return false;
  if (y < 0 || y >= GRID_HEIGHT) return false;
  return true;
}

// This is the recursive function we will code in the next project
void Visit( int x, int y ) {
  // Starting at the given index, recursively visits every direction in a
  // randomized order.
  // Set my current location to be an empty passage.
  grid[ XYToIndex(x, y) ] = 1;
  // Create an local array containing the 4 directions and shuffle their order.
  int dirs[4];
  dirs[0] = NORTH;
  dirs[1] = EAST;
  dirs[2] = SOUTH;
  dirs[3] = WEST;
  for (int i = 0; i < 4; ++i) {
    int r = rand() & 3;
    int temp = dirs[r];
    dirs[r] = dirs[i];
    dirs[i] = temp;
  }
  // Loop through every direction and attempt to Visit that direction.
  for (int i = 0; i < 4; ++i) {
    // dx,dy are offsets from current location. Set them based
    // on the next direction I wish to try.
    int dx = 0, dy = 0;
    switch (dirs[i]) {
      case NORTH: dy = -1; break;
      case SOUTH: dy = 1; break;
      case EAST: dx = 1; break;
      case WEST: dx = -1; break;
    }
    // Find the (x,y) coordinates of the grid cell 2 spots
    // away in the given direction.
    int x2 = x + (dx << 1);
    int y2 = y + (dy << 1);
    if (IsInBounds(x2, y2)) {
      if (grid[ XYToIndex(x2, y2) ] == 0) {
        // (x2,y2) has not been visited yet... knock down the
        // wall between my current position and that position
        grid[ XYToIndex(x2 - dx, y2 - dy) ] = 1;
        // Recursively Visit (x2,y2)
        Visit(x2, y2);
      }
    }
  }
}

void PrintGrid() {
  // Displays the finished maze to the screen.
  for (int y = 0; y < GRID_HEIGHT; ++y) {
    for (int x = 0; x < GRID_WIDTH; ++x) {
      if (grid[XYToIndex(x, y)]) {
        Serial.print("   ");
      } else {
        Serial.print(" W ");
      }
    }
    Serial.println();
  }
  Serial.println();
  Serial.println();
}


void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  ResetGrid();
  Visit(px, py);
  PrintGrid();

  Matrix.begin();
  Matrix.setBrightness(5);
  Matrix.clear();
  Matrix.setPixelColor(0, 0);
  show_noglitch();

  Wire.begin(21, 22);
  Wire.beginTransmission(MPU_ADDR); // Begins a transmission to the I2C slave (GY-521 board)
  Wire.write(0x6B); // PWR_MGMT_1 register
  Wire.write(0); // set to zero (wakes up the MPU-6050)
  Wire.endTransmission(true);

}
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
 
  if (abs(accelerometer_x) > 10000 && abs(accelerometer_y) > 4000 && abs(accelerometer_z) > 10000) {
    Serial.println("SHAKE");
    ResetGrid();
    Visit(px, py);
    x_offset = 0;
    y_offset = 0;
    
  } else if (accelerometer_y+y_offset < -8000) {
    Serial.println("UP");
    py--;
    if (py < 0) {
      py = 0;
    }else if(!grid[XYToIndex(px, py)]){
      py++;
    }
    if(face != py/8){
      //x_offset += 1000;
      y_offset -= 8000;
      face = py/8;
    }
  } else if (accelerometer_x+x_offset < -8000 ) {
    Serial.println("RIGHT");
    px++;   
    if (px == GRID_WIDTH) {
      Serial.print("if statement");
      px = GRID_WIDTH - 1;
    }else if(!grid[XYToIndex(px, py)]){
      px--;
    }
  } else if ( accelerometer_y+y_offset > 8000) {
    Serial.println("DOWN");
    py++;
    if (py == GRID_HEIGHT) {
      py = GRID_HEIGHT - 1;
    }else if(!grid[XYToIndex(px, py)]){
      py--;
    }
    if(face != py/8){
      //x_offset += 1000;
      y_offset += 8000;
      face = py/8;
    }
  } else if (accelerometer_x+x_offset > 8000 ) {
    Serial.println("LEFT");
    px--;
    if (px < 0) {
      px = 0;
    }else if(!grid[XYToIndex(px, py)]){
      px++;
    }
    
  } else {
    Serial.println("STILL");
  }
  int i = 0;
  //PrintGrid();
  for (int y = 0; y < GRID_HEIGHT; ++y) {
    for (int x = 0; x < GRID_WIDTH; ++x) {
      int led_index = XYToIndexLED(x, y);
      if (grid[XYToIndex(x, y)]) {
        if (x == px && y == py) {
          Matrix.setPixelColor(led_index, Matrix.Color(255, 0, 0));
        } else {
          Matrix.setPixelColor(led_index, Matrix.Color(0, 0, 0));
        }

      } else {
        Matrix.setPixelColor(led_index, Matrix.Color(0, 255, 0));
      }
    }
  }

  show_noglitch();
  Serial.print(px);
  Serial.print(" ");
  Serial.print(py);
  Serial.println("END OF LOOP");
  delay(1000);
}
