#include <DS3231.h>
#include <Wire.h>
#include <LiquidCrystal.h>

// define pins
#define moist A1
#define light A2
#define waterRelay 2
#define airTemp A3
#define soilTemp A6
#define SDA A4
#define SCL A5

// define LCD controller buttons
#define btnRIGHT  0
#define btnUP     1
#define btnDOWN   2
#define btnLEFT   3
#define btnSELECT 4
#define btnNONE   5

#define timePAGE 0
#define humidityLightPAGE 1
#define tempPAGE 2
#define checkForWateringPAGE 3
#define waterWithoutCheckingPAGE 4
#define setclockPAGE 5

#define noOfMenuPages 5
int toDrawPage = 0;


LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

DS3231 Clock;
bool h12 = false;
bool PM = false;


void setup()
{
  lcd.begin(16, 2);              // start the library
  lcd.setCursor(0, 0);
  lcd.print("Hello world!");

  // set_time(16,34,1);
  Serial.begin(9600);
  pinMode(moist, INPUT);
  pinMode(waterRelay, OUTPUT);
  Wire.begin();
}

int get_soil_temp()
{
  int raw = analogRead(soilTemp);
  float temp = raw * 0.217226044 - 61.1111111;
  float voltage = raw*5.0/1024.0;
  float soil_temp = (voltage  - 1.375)/0.0225;
  return soil_temp;
}

int read_LCD_button_down()
{
  int adc_key_in = analogRead(0);      // read the value from the sensor

  if (adc_key_in < 50)   return btnRIGHT;
  if (adc_key_in < 195)  return btnUP;
  if (adc_key_in < 380)  return btnDOWN;
  if (adc_key_in < 555)  return btnLEFT;
  if (adc_key_in < 790)  return btnSELECT;

  return btnNONE;  // when all others fail, return this...
}

int read_LCD_button_down2()
{
  int adc_key_in = analogRead(0);      // read the value from the sensor

  if (adc_key_in < 50)
  {
    delay(60);
    adc_key_in = analogRead(0);
    if (adc_key_in < 50)
      return btnRIGHT;
  }

  if (adc_key_in < 195)
  {
    delay(60);
    adc_key_in = analogRead(0);
    if (adc_key_in < 195)
      return btnUP;

  }
  if (adc_key_in < 380)
  {
    delay(60);
    adc_key_in = analogRead(0);
    if (adc_key_in < 380)
      return btnDOWN;
  }
  if (adc_key_in < 555)
  {

    delay(60);
    adc_key_in = analogRead(0);
    if (adc_key_in < 555)
      return btnLEFT;
  }

  if (adc_key_in < 790)
  {
    delay(60);
    adc_key_in = analogRead(0);
    if (adc_key_in < 790)
      return btnSELECT;
  }


  return btnNONE;  // when all others fail, return this...
}



void set_time()
{
  int h = 0, m = 0, s = 0;
  int to_be_changed = 0; //0 - changes hour; 1 - changes minute; 2 - changes seconds; 3- cancel
  while (true)
  {
    Serial.print("To be changed: ");
    Serial.println(to_be_changed);
    int button = read_LCD_button_down2();
    if (button == btnSELECT) //sets time and jumps to print time screen
    {
      if (to_be_changed == 2)
      {
        toDrawPage = 0;
        break;
      } else
      {
        Clock.setHour(h);
        Clock.setMinute(m);
        Clock.setSecond(0);
        toDrawPage = 0;
        break;
      }
    }
    if (button == btnUP)
    {
      switch (to_be_changed)
      {
        case 0:
          h++;
          if (h > 23)
            h = 0;
          break;
        case 1:
          m++;
          if (m > 59)
            m = 0;
          break;
      }
    }
    if (button == btnDOWN)
    {
      switch (to_be_changed)
      {
        case 0:
          h--;
          if (h < 0)
            h = 23;
          break;
        case 1:
          m--;
          if (m < 0)
            m = 59;
          break;
      }

    }
    if (button == btnLEFT)
    {
      switch (to_be_changed)
      {
        case 0:
          to_be_changed = 2;
          break;
        case 1:
          to_be_changed--;
          break;
        case 2:
          to_be_changed--;
          break;
      }
    }
    if (button == btnRIGHT)
    {
      switch (to_be_changed)
      {
        case 0:
          to_be_changed++;
          break;
        case 1:
          to_be_changed++;
          break;
        case 2:
          to_be_changed = 0;
          break;
      }

    }

    lcd.clear();
    lcd.setCursor(0, 1);
    String hour = String(h);
    String minute = String(m);
    String second = String(s);
    if (to_be_changed == 0)
    {
      lcd.print(hour + "* :: " + minute +  " cancel");
    }
    else if (to_be_changed == 1)
    {
      lcd.print(hour + " :: " + minute +  "* cancel");
    }
    else if (to_be_changed == 2)
    {
      lcd.print(hour + " :: " + minute +  " cancel*");
    }

  }
}


void try_water_now_no_print()
{
  if (get_moisture() < 40)
  {
      water_now();
  }
}

void water_now()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("  Watering now");
  lcd.setCursor(0, 1);
  lcd.print("Wait 10 seconds");

  toggle_relay(waterRelay, HIGH);
  delay(10000);
  toggle_relay(waterRelay, LOW);

}

void try_to_water()
{
  if (get_moisture() < 40)
  {
      water_now();
  }
  else
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("MoistureTooHigh");
    lcd.setCursor(0, 1);
    lcd.print("No need to water");
    delay(3000);
  }

}

void read_LCD_button_pressed()
{
  static int lastBtnState = btnNONE;
  int currentState = read_LCD_button_down();

  if (currentState != lastBtnState)
  {
    lastBtnState = currentState;
    // Serial.println("asdasdasd");
    // do stuff

    if (lastBtnState == btnDOWN)
    {
      if (toDrawPage < noOfMenuPages)
        toDrawPage++;
    }
    if (lastBtnState == btnUP)
    {
      if (toDrawPage > 0)
        toDrawPage--;
    }
    if (lastBtnState == btnSELECT)
    {
      Serial.println("!!!!!DO STUFF!!!!!");

      if (toDrawPage == checkForWateringPAGE)
      {
        try_to_water();
      }
      if (toDrawPage == waterWithoutCheckingPAGE)
      {
        water_now();
      }
      if (toDrawPage == setclockPAGE)
      {
        lcd.clear();
        lcd.setCursor(0, 0);
        set_time();

      }
    }
  }
}



void drawMenu(/*int pageNo*/)
{
  // see menu numbers defined above

  switch (toDrawPage)
  {
    case timePAGE:
      {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("     TIME:      ");
        lcd.setCursor(0, 1);

        bool h12, PM;
        String hour = String(Clock.getHour(h12, PM));
        String minute = String(Clock.getMinute());
        String second = String(Clock.getSecond());
        lcd.print(hour + " :: " + minute + " :: " + second);
        break;
      }
    case humidityLightPAGE:
      {
        lcd.clear();
        lcd.setCursor(0, 0);
        String humidity = String(get_moisture());
        lcd.print(" Humidity: " + humidity + "%");

        lcd.setCursor(0, 1);
        String light = String(get_luminance());
        lcd.print("   Light:  " + light + "%");
        break;
      }
    case tempPAGE:
      {
        lcd.clear();
        lcd.setCursor(0, 0);
        String t1 = String(get_soil_temp());
        lcd.print("Soil temp: " + t1);
        lcd.setCursor(0,1);
        String t2 = String(get_temperature());
        lcd.print("Air temp: " + t2);
        break;
      }
    case checkForWateringPAGE:
      {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("check for water");
        break;
      }
    case waterWithoutCheckingPAGE:
      {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("   Water now");
        lcd.setCursor(0, 1);
        lcd.print(" for 10 seconds");
        break;
      }
    case setclockPAGE:
      {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("set time");
        set_time();
        break;
      }
  }
}

float get_temperature()
{
    int raw = analogRead(airTemp);
    float temp = raw * 0.217226044 - 61.1111111;
    float voltage = raw*5.0/1024.0;
    float air_temp = (voltage  - 1.375)/0.0225;
    return air_temp;
}

int get_moisture()
{
  int raw_moisture_read = analogRead(moist); //raw data from moisture sensor
  if(raw_moisture_read > 800)
    raw_moisture_read = 800;
  int moisture_percentage = map(raw_moisture_read, 0, 800, 0, 100); //value in percentage points
  return moisture_percentage;
}

int get_luminance()
{
  return map(analogRead(light), 0, 1023, 0, 100); //TODO: check in what range it should actually be
}

void toggle_relay(int RelayPin, int mode)
{
  digitalWrite(RelayPin, mode);
}

void print_time()
{
  Serial.print("Time: \t");
  Serial.print("H: ");
  Serial.print(Clock.getHour(h12, PM), DEC);
  Serial.print(" Min: ");
  Serial.print(Clock.getMinute());
  Serial.print(" Sec: ");
  Serial.println(Clock.getSecond());
}

void print_stuff()
{
  Serial.print("Moisture: \t");
  Serial.println(get_moisture());
  Serial.print("Light level: \t");
  Serial.print(get_luminance());
  Serial.print("Temperature: \t");
  Serial.println(-100/*get_temperature()*/);
}

void loop()
{
  int light = get_luminance(); //light level
  float air_temp = get_temperature(); // air
  float soil_temp = get_soil_temp();
  if(soil_temp <= 40 && soil_temp>4 && air_temp<=40 && air_temp>4 && light < 60)
    try_water_now_no_print();

//  Serial.print("Temperature: ");
//  Serial.println(get_soil_temp());
  
  
  read_LCD_button_pressed();
  drawMenu();

  delay(50);
}
