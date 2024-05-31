// REmove the temperature sensor and other led . 
// Improving with addiing medicine featurs

#include <Wire.h>
#include <WiFi.h>
#include <DHTesp.h>
#include <LiquidCrystal_I2C.h>

// Parameters for NTP
#define NTP_SERVER     "pool.ntp.org"
#define UTC_OFFSET_DST 0

// Pin definitions 
#define LED_1 15   //Drawer 1 led 
#define LED_2 2    // Drawer 2 led
#define LED_3 4    // Drawer 3 led
#define hm_1 26   //Drawer 1 led 
#define hm_2 27    // Drawer 2 led
#define hm_3 14    // Drawer 3 led

#define PB_CANCEL 34
#define PB_UP 35
#define PB_DOWN 32
#define PB_OK 33
#define BUZZER 18
#define DHT_PIN 12

// Object declarations
LiquidCrystal_I2C lcd(0x27, 16, 2); // Address 0x27 for I2C communication, 16 columns and 2 rows

DHTesp dhtSensor;

// Variables to configure time zone
int offset_hours = 0, offset_minutes = 0;
unsigned long utc_offset = 19800;

// Variables to keep time
int seconds = 0, minutes = 0, hours = 0, days = 0;

// Variables to ring buzzer
int n_notes = 8;
int C = 262;
int D = 294;
int E = 330;
int F = 349;
int G = 392;
int A = 440;
int B = 494;
int C1 = 523;
int alarmTone[] = {C, D, E, F, G, A, B, C1};
int startTone[] = {B, A, B, A, G};

// Variables for menu
int current_mode = 0;
int n_modes = 4;
String menu[] = {"Medicine Time 1", "Medicine Time 2", "Medicine Time 3", "4 - Remove Alarms"};
int current_alarm_index = 0;

// Structure to hold information for each medicine time
bool alarms_enabled = true;
int n_alarms = 3;
bool alarm_triggered[] = {false, false, false};



struct MedicineTime {
  int alarm_hours;
  int alarm_minutes;
  struct Drawer {
    int drawer_led;
    int hm; //haptic motor
    int no_of_pills;
  } drawers[3]; // Assuming there can be up to 3 drawers
};

// Array to hold information for each medicine time
MedicineTime medicine_times[] = {
  {9, 27, {{LED_1,hm_1, 2}, {LED_2,hm_2, 2}, {LED_3,hm_3, 5}}}, // Medicineb Time 1
  {9, 27, {{LED_1,hm_1, 2}, {LED_2,hm_2, 2}, {LED_3,hm_3, 5}}}, // Medicineb Time 2
  {9, 27, {{LED_1,hm_1, 2}, {LED_2,hm_2, 2}, {LED_3,hm_3, 5}}} // Medicineb Time 3
};

void setup() {
  Serial.begin(9600);

  pinMode(16,OUTPUT);
  pinMode(LED_1, OUTPUT);
  pinMode(LED_2, OUTPUT);
  pinMode(LED_3, OUTPUT);  
  pinMode(hm_1, OUTPUT);
  pinMode(hm_2, OUTPUT);
  pinMode(hm_3, OUTPUT);  

  pinMode(PB_CANCEL, INPUT);
  pinMode(PB_UP, INPUT);
  pinMode(PB_DOWN, INPUT);
  pinMode(PB_OK, INPUT);
  pinMode(BUZZER, OUTPUT);

  dhtSensor.setup(DHT_PIN, DHTesp::DHT22); // Set up DHT22 sensor

  lcd.init();                      // Initialize the LCD
  lcd.backlight();                 // Turn on backlight

  lcd.clear();
  lcd.print("Medicine Box");
  delay(100);

  // Play the start tone using buzzer
  for (int i = 0; i < n_notes; ++i) {
    tone(BUZZER, startTone[i]);
    delay(300);
    noTone(BUZZER);
    delay(2);
  }

  // Connect to wifi
  WiFi.begin("Husna", "password");
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    lcd.clear();
    lcd.print("Connecting to WiFi");
  }

  lcd.clear();
  lcd.print("Connected to WiFi");
  lcd.clear();

  // Configure current time over wifi
  configTime(utc_offset, UTC_OFFSET_DST, NTP_SERVER);
}

void loop() {
  update_time_with_check_alarm();

  // Go to menu if OK button pressed
  if (digitalRead(PB_OK) == LOW) {
    delay(1000);
    go_to_menu();
  }

  check_temp();
}

// Function to update current time over wifi
void update_time(void) {
  // Save current time in a tm struct over wifi
  struct tm timeinfo;
  getLocalTime(&timeinfo);

  // Update variables saving time
  char str_hours[8];
  strftime(str_hours, 8, "%H", &timeinfo);
  hours = atoi(str_hours);

  char str_minutes[8];
  strftime(str_minutes, 8, "%M", &timeinfo);
  minutes = atoi(str_minutes);

  char str_seconds[8];
  strftime(str_seconds, 8, "%S", &timeinfo);
  seconds = atoi(str_seconds);

  char str_days[8];
  strftime(str_days, 8, "%d", &timeinfo);
  days = atoi(str_days);
}

// Function to display current time in DD:HH:MM:SS format on display
void print_time_now() {
  String hourStr = String(hours % 12 == 0 ? 12 : hours % 12);
  String minuteStr = String(minutes);
  String secondStr = String(seconds);
  String periodStr = (hours < 12) ? "AM" : "PM";
  String timeString = hourStr + ":" + minuteStr + ":" + secondStr + " " + periodStr;
  lcd.setCursor(0, 0);
  lcd.print(timeString.substring(0, 16));
}

// Function to update current time while checking whether alarm times reached
void update_time_with_check_alarm() {
  update_time();
  print_time_now();

  // Check for alarms
  if (alarms_enabled) {
    for (int i = 0; i < n_alarms; ++i) {
         if (medicine_times[i].alarm_hours == hours && medicine_times[i].alarm_minutes == minutes && alarm_triggered[i] != true) {
        // If alarm time reached and not triggered, ring the alarm
        ring_alarm();
        alarm_triggered[i] = true;
        current_alarm_index = i;
      }
    }
  }
  
}

void blink(int no) {
  digitalWrite(no, HIGH);  // turn the LED on (HIGH is the voltage level)
  delay(1000);                      // wait for a second
  digitalWrite(no, LOW);   // turn the LED off by making the voltage LOW
  delay(1000);   
  digitalWrite(no, HIGH);  // turn the LED on (HIGH is the voltage level)
  delay(1000);                      // wait for a second
  digitalWrite(no, LOW);   // turn the LED off by making the voltage LOW
  delay(1000);  
  digitalWrite(no, HIGH);  // turn the LED on (HIGH is the voltage level)
  delay(1000);                      // wait for a second
  digitalWrite(no, LOW);   // turn the LED off by making the voltage LOW
  delay(1000);                     // wait for a second
}

// Function to display a message, light up a LED and ring the buzzer for an alarm
// Function to ring the alarm and trigger associated actions
void ring_alarm() {
  lcd.clear();
  lcd.print("Medicine Time");

  // Play the alarm tone
  for (int i = 0; i < n_notes; ++i) {
    tone(BUZZER, alarmTone[i]);
    delay(500);
    noTone(BUZZER);
    delay(2);
  }

  // Turn on each drawer LED one by one
  for (int i = 0; i < 3; ++i) {
    digitalWrite(medicine_times[current_alarm_index].drawers[i].drawer_led, HIGH);
     blink(medicine_times[current_alarm_index].drawers[i].hm);
    lcd.clear();
    lcd.print("Pills");
    lcd.setCursor(0, 1);
    lcd.print(medicine_times[current_alarm_index].drawers[i].no_of_pills);
    //delay(1000); // Delay to keep the LED on for 1 second
    while(true){
    if (digitalRead(PB_OK) == LOW) {
      delay(200);
      break;
      }
    }
    alarm_triggered[i] = false;
    digitalWrite(medicine_times[current_alarm_index].drawers[i].drawer_led, LOW);
    lcd.clear();
    delay(200); // Delay between turning off one LED and turning on the next
    
  }

  // Clear the LCD after all actions are completed
  lcd.clear();
  
  update_time_with_check_alarm();

}

// Function to wait till a button pressed an return the pressed button
int wait_for_button_press() {
  while (true) {
    if (digitalRead(PB_UP) == LOW) {
      delay(200); // Button debounce time
      return PB_UP;
    }

    if (digitalRead(PB_DOWN) == LOW) {
      delay(200);
      return PB_DOWN;
    }

    if (digitalRead(PB_CANCEL) == LOW) {
      delay(200);
      return PB_CANCEL;
    }

    if (digitalRead(PB_OK) == LOW) {
      delay(200);
      return PB_OK;
    }

    // Update time while waiting
    update_time();
  }
}

// Function to set utc offset by getting it as input
/*void set_time_zone() {
  // Get hours of utc offset as input
  int temp_hours = offset_hours;
  while (true) {
    lcd.clear();
    lcd.print("Enter hours: ");
    lcd.print(temp_hours);

    int button = wait_for_button_press();
    if (button == PB_OK) {
      delay(200);
      offset_hours = temp_hours;
      break;
    }

    if (button == PB_CANCEL) {
      delay(200);
      break;
    }

    if (button == PB_UP) {
      delay(200);
      temp_hours += 1;
      temp_hours %= 24;
    }

    else if (button == PB_DOWN) {
      delay(200);
      temp_hours -= 1;
      if (temp_hours < 0) {
        temp_hours += 24;
      }
    }
  }

  // Get minutes of utc offset as input
  int temp_minutes = offset_minutes;
  while (true) {
    lcd.clear();
    lcd.print("Enter minutes: ");
    lcd.print(temp_minutes);

    int button = wait_for_button_press();
    if (button == PB_OK) {
      delay(200);
      offset_minutes = temp_minutes;
      break;
    }

    if (button == PB_CANCEL) {
      delay(200);
      break;
    }

    if (button == PB_UP) {
      delay(200);
      temp_minutes += 1;
      temp_minutes %= 60;
    }

    else if (button == PB_DOWN) {
      delay(200);
      temp_minutes -= 1;
      if (temp_minutes < 0) {
        temp_minutes += 60;
      }
    }
  }

  utc_offset = offset_hours * 60 * 60 + offset_minutes * 60; // Calculate utc offset in seconds
  configTime(utc_offset, UTC_OFFSET_DST, NTP_SERVER); // Use setted utc offset and configure current time

  lcd.clear();
  lcd.print("Time zone is set");
  delay(1000);
}
*/

// Function to set given alarm by getting alarm time as input
void set_alarm(int alarm) {
  // Get hour as input
  int temp_hours = medicine_times[alarm].alarm_hours;
  while (true) {
    lcd.clear();
    lcd.print("Enter hour: ");
    lcd.print(temp_hours);

    int button = wait_for_button_press();
    if (button == PB_OK) {
      delay(200);
      medicine_times[alarm].alarm_hours = temp_hours;
      break;
    }

    if (button == PB_CANCEL) {
      delay(200);
      break;
    }

    if (button == PB_UP) {
      delay(200);
      temp_hours += 1;
      temp_hours %= 24;
    }

    else if (button == PB_DOWN) {
      delay(200);
      temp_hours -= 1;
      if (temp_hours < 0) {
        temp_hours += 24;
      }
    }
  }

  // Get minutes as input
  int temp_minutes = medicine_times[alarm].alarm_minutes;
  while (true) {
    lcd.clear();
    lcd.print("Enter minute: ");
    lcd.print(temp_minutes);

    int button = wait_for_button_press();
    if (button == PB_OK) {
      delay(200);
      medicine_times[alarm].alarm_minutes = temp_minutes;
      break;
    }

    if (button == PB_CANCEL) {
      delay(200);
      break;
    }

    if (button == PB_UP) {
      delay(200);
      temp_minutes += 1;
      temp_minutes %= 60;
    }

    else if (button == PB_DOWN) {
      delay(200);
      temp_minutes -= 1;
      if (temp_minutes < 0) {
        temp_minutes += 60;
      }
    }
  }

  lcd.clear();
  lcd.print("Alarm is set");
  delay(1000);
  
}

// Function to get current temperature and humidity and check whether they are in healthy limits
void check_temp(void) {
  TempAndHumidity data = dhtSensor.getTempAndHumidity(); // Read temperature and humidity from sensor
  bool all_good = true;

  // Give warnings if healthy limits exceeded
  if (data.temperature > 32) {
    all_good = false;
    digitalWrite(LED_2, HIGH);
    lcd.clear();
    lcd.print("Temperature High");
  }

  else if (data.temperature < 26) {
    all_good = false;
    digitalWrite(LED_2, HIGH);
    lcd.clear();
    lcd.print("Temperature Low");
  }

  if (data.humidity > 80) {
    all_good = false;
    digitalWrite(LED_2, HIGH);
    lcd.clear();
    lcd.print("Humidity High");
  }

  if (data.humidity < 60) {
    all_good = false;
    digitalWrite(LED_2, HIGH);
    lcd.clear();
    lcd.print("Humidity Low");
  }

  if (all_good) {
    digitalWrite(LED_2, LOW);
  }
  else {
    tone(BUZZER, G);
    delay(600);
    noTone(BUZZER);
    delay(2);
  }
}

// Function to navigate through the menu
void go_to_menu(void) {
  while (true) {
    lcd.clear();
    lcd.print("Set");
    lcd.setCursor(0,1);
    lcd.print(menu[current_mode]);

    int button = wait_for_button_press();

    // Navigate through menu based on buttons pressed
    if (button == PB_OK) {
      delay(200);
      run_current_mode(); // current_mode not passed to function. This function reads current mode from global variable
    }

    if (button == PB_CANCEL) {
      delay(200);
      lcd.clear();
      break;
    }

    if (button == PB_UP) {
      delay(200);
      current_mode += 1;
      current_mode %= n_modes;
    }

    else if (button == PB_DOWN) {
      delay(200);
      current_mode -= 1;
      if (current_mode < 0) {
        current_mode += n_modes;
      }
    }
  }
}

// Function to execute the functions related to current mode
void run_current_mode() {
  if (current_mode == 0 || current_mode == 1 || current_mode == 2) {
    set_alarm(current_mode);
  }
  else if (current_mode == 3) {
    alarms_enabled = false;
  }
}

