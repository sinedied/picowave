#define USE_SPIFFS            true
#define ESP_DRD_USE_EEPROM    true

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <driver/i2s.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EasyButton.h>

// #define DRD_TIMEOUT             10
// #define DRD_ADDRESS             0
#include <ESP_DoubleResetDetector.h> 
// DoubleResetDetector* drd;
// bool      initialConfig = false;

#define SCREEN_WIDTH 64 // OLED display width, in pixels
#define SCREEN_HEIGHT 48 // OLED display height, in pixels

#define SAMPLE_BUFFER_SIZE 512
#define SAMPLE_RATE 8000
// most microphones will probably default to left channel but you may need to tie the L/R pin low
#define I2S_MIC_CHANNEL I2S_CHANNEL_FMT_ONLY_LEFT
// either wire your microphone to the same pins or change these to match your wiring
#define I2S_MIC_LEFT_RIGHT_CLOCK D5
#define I2S_MIC_SERIAL_DATA D6
#define I2S_MIC_SERIAL_CLOCK D7

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET 0
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

template <typename TYPE> class doubleBuffer{
    public:
        volatile TYPE *readBuffer, *writeBuffer;
        volatile bool swap_ready = false;
        void swap(){
            volatile TYPE *temp = readBuffer;
            readBuffer = writeBuffer;
            writeBuffer = temp;
        }
        doubleBuffer(int size){
            readBuffer = (TYPE*)calloc(size, sizeof(TYPE));
            writeBuffer = (TYPE*)calloc(size, sizeof(TYPE));
        }
};
doubleBuffer<uint8_t> screenBuffer(SCREEN_WIDTH * SCREEN_HEIGHT);


#define SAMPLING_FREQ   44100         // Hz, must be 40000 or less due to ADC conversion time. Determines maximum frequency that can be analysed by the FFT Fmax=sampleF/2.
#define AUDIO_IN_PIN    A0
#define BUTTON_PIN      D8

EasyButton modeBtn(BUTTON_PIN);

bool ledOn = false;

// don't mess around with this
i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 4,
    .dma_buf_len = 1024,
    .use_apll = false,
    .tx_desc_auto_clear = false,
    .fixed_mclk = 0};

// and don't mess around with this
i2s_pin_config_t i2s_mic_pins = {
    .bck_io_num = I2S_MIC_SERIAL_CLOCK,
    .ws_io_num = I2S_MIC_LEFT_RIGHT_CLOCK,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_MIC_SERIAL_DATA};

void printDouble( double val, unsigned int precision){
// prints val with number of decimal places determine by precision
// NOTE: precision is 1 followed by the number of zeros for the desired number of decimial places
// example: printDouble( 3.1415, 100); // prints 3.14 (two decimal places)

    Serial.print (int(val));  //prints the int part
    Serial.print("."); // print the decimal point
    unsigned int frac;
    if(val >= 0)
      frac = (val - int(val)) * precision;
    else
       frac = (int(val)- val ) * precision;
    int frac1 = frac;
    while( frac1 /= 10 )
        precision /= 10;
    precision /= 10;
    while(  precision /= 10)
        Serial.print("0");

    Serial.println(frac,DEC) ;
}

void changeMode() {
  // Serial.println("Button pressed");
  ledOn = !ledOn;
}

int32_t raw_samples[SAMPLE_BUFFER_SIZE];

void setup() {
  // Serial.begin(9600);
  Serial.begin(115200);

  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &i2s_mic_pins);

  // put your setup code here, to run once:
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  // Serial.println(F("Init ok"));
  pinMode(LED_BUILTIN, OUTPUT);
  // pinMode(BUTTON_PIN, INPUT_PULLUP);

  modeBtn.begin();
  modeBtn.onPressed(changeMode);

  // drd = new DoubleResetDetector(DRD_TIMEOUT, DRD_ADDRESS);
  // if (drd->detectDoubleReset()) { 
  //   initialConfig = !initialConfig;
  // }

  // if (initialConfig) {
  //   digitalWrite(LED_BUILTIN, HIGH);
  // } else {
  //   digitalWrite(LED_BUILTIN, LOW);
  // }


}

void loop() {
  modeBtn.read();

  size_t bytes_read = 0;
  i2s_read(I2S_NUM_0, raw_samples, sizeof(int32_t) * SAMPLE_BUFFER_SIZE, &bytes_read, portMAX_DELAY);
  int samples_read = bytes_read / sizeof(int32_t);
  // dump the samples out to the serial channel.
  for (int i = 0; i < samples_read; i++)
  {
    Serial.printf("%ld\n", raw_samples[i]);
  }


  // put your main code here, to run repeatedly:
  // Serial.println(F("loop"));

  display.clearDisplay();
  // display.drawPixel(0, 0, WHITE);
  // display.drawPixel(32, 16, WHITE);
  // display.drawPixel(63, 47, WHITE);
  // display.drawPixel(95, 63, WHITE);
  // display.drawPixel(127, 63, WHITE);


  display.setTextSize(1);
  display.setCursor(0, 0);
  display.setTextColor(WHITE); 
  display.println("Hello,");
  display.println("World!");
  display.setTextSize(0);
  display.setCursor(0, 24);
  display.println("1234567890");
  display.display();

  // double v = analogRead(AUDIO_IN_PIN);
  // Serial.println(F("loop"));

  // if (v > 0) {
  //   Serial.print(F("v: "));
  //   Serial.println(v);
  //   printDouble(v, 100);
  // }

  if (ledOn) {
    digitalWrite(LED_BUILTIN, HIGH);
  } else {
    digitalWrite(LED_BUILTIN, LOW);
  }



  // delay(100);
  // display.invertDisplay(true);
  // delay(500);
  // display.invertDisplay(false);

  // digitalWrite(LED_BUILTIN, LOW);
  // digitalWrite(D4, LOW);
  // delay(20);
  // digitalWrite(LED_BUILTIN, HIGH);
  // digitalWrite(D4, HIGH);
  // delay(500);
  // drd->loop();
}


// #define NUMFLAKES 10
// #define XPOS 0
// #define YPOS 1
// #define DELTAY 2


// #define LOGO16_GLCD_HEIGHT 16
// #define LOGO16_GLCD_WIDTH  16
// static const unsigned char PROGMEM logo16_glcd_bmp[] =
// { B00000000, B11000000,
//   B00000001, B11000000,
//   B00000001, B11000000,
//   B00000011, B11100000,
//   B11110011, B11100000,
//   B11111110, B11111000,
//   B01111110, B11111111,
//   B00110011, B10011111,
//   B00011111, B11111100,
//   B00001101, B01110000,
//   B00011011, B10100000,
//   B00111111, B11100000,
//   B00111111, B11110000,
//   B01111100, B11110000,
//   B01110000, B01110000,
//   B00000000, B00110000 };

// #if (SSD1306_LCDHEIGHT != 48)
// #error("Height incorrect, please fix Adafruit_SSD1306.h!");
// #endif


// void testdrawbitmap(const uint8_t *bitmap, uint8_t w, uint8_t h) {
//   uint8_t icons[NUMFLAKES][3];

//   // initialize
//   for (uint8_t f=0; f< NUMFLAKES; f++) {
//     icons[f][XPOS] = random(display.width());
//     icons[f][YPOS] = 0;
//     icons[f][DELTAY] = random(5) + 1;

//     Serial.print("x: ");
//     Serial.print(icons[f][XPOS], DEC);
//     Serial.print(" y: ");
//     Serial.print(icons[f][YPOS], DEC);
//     Serial.print(" dy: ");
//     Serial.println(icons[f][DELTAY], DEC);
//   }

//   while (1) {
//     // draw each icon
//     for (uint8_t f=0; f< NUMFLAKES; f++) {
//       display.drawBitmap(icons[f][XPOS], icons[f][YPOS], bitmap, w, h, WHITE);
//     }
//     display.display();
//     delay(200);

//     // then erase it + move it
//     for (uint8_t f=0; f< NUMFLAKES; f++) {
//       display.drawBitmap(icons[f][XPOS], icons[f][YPOS], bitmap, w, h, BLACK);
//       // move it
//       icons[f][YPOS] += icons[f][DELTAY];
//       // if its gone, reinit
//       if (icons[f][YPOS] > display.height()) {
//         icons[f][XPOS] = random(display.width());
//         icons[f][YPOS] = 0;
//         icons[f][DELTAY] = random(5) + 1;
//       }
//     }
//    }
// }


// void testdrawchar(void) {
//   display.setTextSize(1);
//   display.setTextColor(WHITE);
//   display.setCursor(0,0);

//   for (uint8_t i=0; i < 168; i++) {
//     if (i == '\n') continue;
//     display.write(i);
//     if ((i > 0) && (i % 21 == 0))
//       display.println();
//   }
//   display.display();
//   delay(1);
// }

// void testdrawcircle(void) {
//   for (int16_t i=0; i<display.height(); i+=2) {
//     display.drawCircle(display.width()/2, display.height()/2, i, WHITE);
//     display.display();
//     delay(1);
//   }
// }

// void testfillrect(void) {
//   uint8_t color = 1;
//   for (int16_t i=0; i<display.height()/2; i+=3) {
//     // alternate colors
//     display.fillRect(i, i, display.width()-i*2, display.height()-i*2, color%2);
//     display.display();
//     delay(1);
//     color++;
//   }
// }

// void testdrawtriangle(void) {
//   for (int16_t i=0; i<min(display.width(),display.height())/2; i+=5) {
//     display.drawTriangle(display.width()/2, display.height()/2-i,
//                      display.width()/2-i, display.height()/2+i,
//                      display.width()/2+i, display.height()/2+i, WHITE);
//     display.display();
//     delay(1);
//   }
// }

// void testfilltriangle(void) {
//   uint8_t color = WHITE;
//   for (int16_t i=min(display.width(),display.height())/2; i>0; i-=5) {
//     display.fillTriangle(display.width()/2, display.height()/2-i,
//                      display.width()/2-i, display.height()/2+i,
//                      display.width()/2+i, display.height()/2+i, WHITE);
//     if (color == WHITE) color = BLACK;
//     else color = WHITE;
//     display.display();
//     delay(1);
//   }
// }

// void testdrawroundrect(void) {
//   for (int16_t i=0; i<display.height()/2-2; i+=2) {
//     display.drawRoundRect(i, i, display.width()-2*i, display.height()-2*i, display.height()/4, WHITE);
//     display.display();
//     delay(1);
//   }
// }

// void testfillroundrect(void) {
//   uint8_t color = WHITE;
//   for (int16_t i=0; i<display.height()/2-2; i+=2) {
//     display.fillRoundRect(i, i, display.width()-2*i, display.height()-2*i, display.height()/4, color);
//     if (color == WHITE) color = BLACK;
//     else color = WHITE;
//     display.display();
//     delay(1);
//   }
// }

// void testdrawrect(void) {
//   for (int16_t i=0; i<display.height()/2; i+=2) {
//     display.drawRect(i, i, display.width()-2*i, display.height()-2*i, WHITE);
//     display.display();
//     delay(1);
//   }
// }

// void testdrawline() {
//   for (int16_t i=0; i<display.width(); i+=4) {
//     display.drawLine(0, 0, i, display.height()-1, WHITE);
//     display.display();
//     delay(1);
//   }
//   for (int16_t i=0; i<display.height(); i+=4) {
//     display.drawLine(0, 0, display.width()-1, i, WHITE);
//     display.display();
//     delay(1);
//   }
//   delay(250);

//   display.clearDisplay();
//   for (int16_t i=0; i<display.width(); i+=4) {
//     display.drawLine(0, display.height()-1, i, 0, WHITE);
//     display.display();
//     delay(1);
//   }
//   for (int16_t i=display.height()-1; i>=0; i-=4) {
//     display.drawLine(0, display.height()-1, display.width()-1, i, WHITE);
//     display.display();
//     delay(1);
//   }
//   delay(250);

//   display.clearDisplay();
//   for (int16_t i=display.width()-1; i>=0; i-=4) {
//     display.drawLine(display.width()-1, display.height()-1, i, 0, WHITE);
//     display.display();
//     delay(1);
//   }
//   for (int16_t i=display.height()-1; i>=0; i-=4) {
//     display.drawLine(display.width()-1, display.height()-1, 0, i, WHITE);
//     display.display();
//     delay(1);
//   }
//   delay(250);

//   display.clearDisplay();
//   for (int16_t i=0; i<display.height(); i+=4) {
//     display.drawLine(display.width()-1, 0, 0, i, WHITE);
//     display.display();
//     delay(1);
//   }
//   for (int16_t i=0; i<display.width(); i+=4) {
//     display.drawLine(display.width()-1, 0, i, display.height()-1, WHITE);
//     display.display();
//     delay(1);
//   }
//   delay(250);
// }

// void testscrolltext(void) {
//   display.setTextSize(2);
//   display.setTextColor(WHITE);
//   display.setCursor(10,0);
//   display.clearDisplay();
//   display.println("scroll");
//   display.display();
//   delay(1);

//   display.startscrollright(0x00, 0x0F);
//   delay(2000);
//   display.stopscroll();
//   delay(1000);
//   display.startscrollleft(0x00, 0x0F);
//   delay(2000);
//   display.stopscroll();
//   delay(1000);
//   display.startscrolldiagright(0x00, 0x07);
//   delay(2000);
//   display.startscrolldiagleft(0x00, 0x07);
//   delay(2000);
//   display.stopscroll();
// }


//   // draw a single pixel
//   display.drawPixel(10, 10, WHITE);

//   // draw a white circle, 10 pixel radius
//   display.fillCircle(display.width()/2, display.height()/2, 10, WHITE);

//   // text display tests
//   display.setTextSize(1);
//   display.setTextColor(WHITE);
//   display.setCursor(0,0);
//   display.println("Hello, world!");
//   display.setTextColor(BLACK, WHITE); // 'inverted' text
//   display.println(3.141592);
//   display.setTextSize(2);
//   display.setTextColor(WHITE);

