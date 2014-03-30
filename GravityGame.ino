
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_LEDBackpack.h>
#include <Adafruit_GFX.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_LSM303_U.h>

Adafruit_BicolorMatrix    display;
Adafruit_LSM303_Accel_Unified    accel = Adafruit_LSM303_Accel_Unified(0xdeadbeef);
sensors_vec_t    center;

#define    ENABLE_UART    0

#if ENABLE_UART
#define    DELAY    500
#else
#define    DELAY    100
#endif

void    setup()
{
#if ENABLE_UART
    Serial.begin(9600);
#endif

    display.begin();
    if( !accel.begin()  && Serial) {
        Serial.println("Crap! Accelerometer failed to initialized");
    }
    
    sensors_event_t    event;
    accel.getEvent(&event);
    center = event.acceleration;
    
    display.fillScreen(LED_YELLOW);
    display.writeDisplay();
}


void     PrintAccel( const char *title, const sensors_vec_t &accel )
{
    if ( Serial ) {
        if ( title ) { Serial.print(title); Serial.print(": "); }
        Serial.print("x="); Serial.print(accel.x); Serial.print(", y="); Serial.println(accel.y);
    }
}

const float kDefaultAlpha = 0.5;
float lowPassFilter( float value, float old_value, float alpha = kDefaultAlpha )
{ 
    alpha = constrain(alpha,0.0,1.0); 
    return (old_value * alpha) + ((1.0 - alpha) * value); 
}

#define    kBallSize (2)
#define    kBoardSize (8)

void    drawBallAt(uint16_t x, uint16_t y, uint16_t color )
{
    x = constrain(x,(kBallSize>>1),kBoardSize-(kBallSize>>1)) - (kBallSize>>1);
    y = constrain(y,(kBallSize>>1),kBoardSize-(kBallSize>>1)) - (kBallSize>>1);

    display.fillRect(x,y,kBallSize,kBallSize,color);

    if ( Serial ) {
        Serial.println(String("drawBallAt: x=")+String(x)+", y=" + String(y));
    }
}

enum {
    kLeftWall = 1 << 0,
    kRightWall = 1 << 1,
    kTopWall = 1 << 2,
    kBottomWall = 1 << 3
};

void    drawWallCollision( int wall, uint16_t color ) {
    if ( wall & kLeftWall ) display.drawFastVLine(0,0,8,color);
    if ( wall & kRightWall) display.drawFastVLine(7,0,8,color);
    if ( wall & kTopWall ) display.drawFastHLine(0,0,8,color);
    if ( wall & kBottomWall) display.drawFastHLine(0,7,8,color);
}

uint16_t colorForOffset( float x, float y )
{
    uint16_t color = LED_GREEN;
#if 0
    const float kYellowDist = float(kBoardSize>>3);
    const float kRedDist = float(kBoardSize>>2);
    if ( abs(x)>kRedDist || abs(y)>kRedDist ) color = LED_RED;
    else if ( abs(x)>kYellowDist || abs(y)>kYellowDist ) color = LED_YELLOW;
#endif    
    if ( Serial ) {
        Serial.println("----------------"); Serial.print("x="); Serial.print(x); Serial.print(", y="); Serial.println(y);
    }
    return color;
}

void    loop()
{
     display.clear();

    sensors_event_t    event;
    accel.getEvent(&event);

    static sensors_vec_t    previous = center;
    static float x = 0.0;
    static float y = 0.0;
    static int    lives = 5;
    const float xCenter = kBoardSize/2.0;
    const float yCenter = kBoardSize/2.0;
    
    
    if ( lives > 0 ) {
        drawBallAt(int(xCenter+x),int(yCenter+y),colorForOffset(x,y));
        
        boolean hitVWall = abs(y) >= kBoardSize/2.0;
        boolean hitHWall = abs(x)>= kBoardSize/2.0;
        if ( hitVWall || hitHWall ) {
            int wall = 0;
            if ( hitHWall ) {
                wall |= (x/abs(x) > 0.0) ? kRightWall : kLeftWall;
            }
            if ( hitVWall ) {
                wall |= (y/abs(y)>0.0) ? kBottomWall : kTopWall;
            }
            drawWallCollision(wall,LED_RED);
        }
     
        display.writeDisplay();
        
        if ( hitHWall || hitVWall ) {
            --lives;
            previous = center;
            x = y = 0.0;
            delay(DELAY*4);    // extra delay
        }
        else {       
            sensors_vec_t accelData = event.acceleration;
            #define    DELTA_SCALE(f)    (f/4.0)
            float dx = DELTA_SCALE(center.x - (previous.x = lowPassFilter( accelData.x, previous.x)));
            float dy = DELTA_SCALE(center.y - (previous.y = lowPassFilter( accelData.y, previous.y)));
            
        //    PrintAccel("Center",center);
        //    PrintAccel("Previous",previous);
        //    PrintAccel("Current",accelData);
        
            x = constrain(x+dx,-float(kBoardSize>>1),float(kBoardSize>>1)); 
            y = constrain(y-dy,-float(kBoardSize>>1),float(kBoardSize>>1));
        }
        delay(DELAY);
   }
    else if ( lives == 0 ) {
        --lives;
        display.setTextWrap(false);
        display.setTextSize(1);
        display.setTextColor(LED_RED);
        for ( int16_t x = 8;x>=-64;--x ) {
            display.clear();
            display.setCursor(x,0);
            display.print("Game Over!");
            display.writeDisplay();
            delay(100);
        }
        
static const uint8_t PROGMEM frown_bmp[] =
              { B00111100,
                B01000010,
                B10100101,
                B10000001,
                B10011001,
                B10100101,
                B01000010,
                B00111100 };

          display.clear();
          display.drawBitmap(0, 0, frown_bmp, 8, 8, LED_RED);
          display.writeDisplay();
    }
}

