load('api_config.js');
load('api_arduino_ssd1306.js');
load('api_timer.js');
load('api_gpio.js');
load('api_mqtt.js');
load('api_sys.js');
load("api_esp32.js");
load('api_math.js');

// FFI Wrappers

Adafruit_SSD1306._spl = ffi('void ssd1306_splash(void *, void *)');
Adafruit_SSD1306._proto.splash = function (data) { Adafruit_SSD1306._spl(this.ssd, data); };
Adafruit_SSD1306._dbm = ffi('void ssd1306_drawBitmap(void *, void *, int, int, int, int)');
Adafruit_SSD1306._proto.drawBitmap = function (data, x, y, w, h) { Adafruit_SSD1306._dbm(this.ssd, data, x, y, w, h); };
let free = ffi('void free(void *)');
let mqtt_connected = ffi('int mqtt_connected(void)');
let distance_error = ffi('int distance_error(void)');
let temperature_error = ffi('int temperature_error(void)');
let get_distance = ffi('void get_distance(char *)');
let get_temperature = ffi('void get_temperature(char *)');
let get_distance_number = ffi('double get_distance_number(void)');
let get_temperature_number = ffi('double get_temperature_number(void)');

// Polyfills
let atob = atob || function (str) { return ffi('void *atob(void *, int)')(str, str.length); };

// Display 

let d = Adafruit_SSD1306.create_i2c(Cfg.get('app.ssd1306_reset_pin'), Adafruit_SSD1306.RES_128_64);
d.begin(Adafruit_SSD1306.SWITCHCAPVCC, 0x3C, true);

let logo = atob("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAHAAAAAAAAAAAAAAAAAAAADwAAAAAAAAAAAAAAAAAAAA8AAAAAAAAAAAAAAAAAAAAfAAAAAAAAAAAAAAAAAAAAPwAAAAAAAAAAAAAAAAAAAH4AAAAAAAAAAAAAAAAAAAD+AAAAAAAAAAAAAAAAAAAB/gAAAAAAAAAAAAAAAAAAA/4AAAAAAAAAAAAAAAAAAAP8AAAAAAAA7gAQGAAAAAAD/AAAAAAAANgAABgAAAAA//wAAAAAAHn8+fH4AAAAA//4AAAAAADN2ZvTOAAAAA//+AAAAAAAhtsbFhgAAAB///wAAAAAAYbbCxYYAAAA///+AAAAAAGG2wsWGAAAAH///wAAAAAAhtsbFhgAAAAMAP+AAAAAAMzZuxM4AAAAAAB/wAAAAAB82PsT+AAAAAAAH+AAAAAAAAEIAAAAAAAAAB/wAAAAAAABmAAAAAAAAAAP8AAAAAAAAPAAAAAAAAAAB/gAAAAAAAAAAAAAAAAAAAf8AAAAAAAAAAAAAAAAAAAH/gAAAAAAAAAAAAAAAAAAA/4AAAAAAAAAAAAAAAAAAAP/AAAAAgAAAYAAAAAAAAAD/wAAAAYAAAGAAAAAAAAAA/+AA/HvxiefjeAAAAAAAAH/gAO7dkZs35swAAAAAAAB/4ADHjZueFmzEAAAAAAAAf/AAx/2b1h54eAAAAAAAAH/wAMf9ivYefDwAAAAAAAB/8ADHhY52HmbGAAAAAAAAf/AAxs2OYzZjxgAAAAAAAB/wAMb4xmPmY3wAAAAAAAABAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAgBACAAAAAAAAAAAAAAAAD/n/P+P8AAAAAAAAAAAAAB///////gAAAAAAAAAAAAAf//////4AAAAAAAAAAAAAHj/H/P+eAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA==");
let logo2 = atob("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAADgAAAAAAAAAeAAAAAAAAAD4AAAAAAAAAfgAAAAAAAAD8AAAAAAAAAfwAAAAAAAAB/AAAAAAAAAP4AAAAAAAAB/gAAAAAAAAP+AAAAAAAAAf4AAAAAAAB//AAAAAAAA//8AAAAAAAP//wAAAAAAD///AAAAAAAf//+AAAAAAB///8AAAAAAAYA/4AAAAAAAAA/wAAAAAAAAB/gAAAAAAAAD/AAAAAAAAAH+AAAAAAAAAf8AAAAAAAAA/4AAAAAAAAD/gAAAAAAAAP/AAAAAAAAAf8AAAAAAAAB/4AAAAAAAAH/gAAAAAAAAf/AAAAAAAAB/8AAAAAAAAD/wAAAAAAAAP/gAAAAAAAA/+AAAAAAAAD/4AAAAAAAAD/gAAAAAAAAAgAAAAAAAAAAAAAAAAAAAAAAAAAAQAgBACAAAAf8/5/z/gAAD///////AAAP//////8AAA8f4/x/jwAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=");
let wifi_logo = atob("AAAAAAAAB+AcODAOB/AMGAHAA+AAAACAAYAAAAAAAAA=");
let ultrasonic_logo = atob("AAAAAAAAAAA//AAADjgSbDNEEmwMOAAAP/w//AAAAAA=");
let temperature_logo = atob("AAAAHA/QGHwRcBBwEGARYBBgEWAQYBBgEWAYYA/AAAA=");
let mqtt_logo = atob("AAAAAAAAAAAAAAwYGjwxxiHCMcYeLAwYAAAAAAAAAAA=");
let mqtt_error = true;

// Main
function main() {

  let distance = "      ";
  let temperature = "      ";
  get_distance(distance);
  get_temperature(temperature);
  d.clearDisplay();
  d.drawBitmap(logo2, 0, 0, 64, 64);
  d.setTextColor(Adafruit_SSD1306.WHITE);
  d.setTextSize(2);
  d.setCursor(65, 30)
  d.write(distance);
  d.setTextSize(1);
  d.setCursor(100, 45)
  d.write("in")

  d.setTextSize(1);
  d.setCursor(12, 57)
  d.write(temperature);
  d.write("F")

  if (mqtt_connected()) {
    d.drawBitmap(wifi_logo, 128 - 16, 0, 16, 16);
  }

  if (!distance_error()) {
    d.drawBitmap(ultrasonic_logo, 128 - 32, 0, 16, 16);
  }

  if (!temperature_error()) {
    d.drawBitmap(temperature_logo, 128 - 48, 0, 16, 16);
  }

  if (!mqtt_error) {
    d.drawBitmap(mqtt_logo, 128 - 64, 0, 16, 16);
  }

  d.display();
}


let getInfo = function () {
  return JSON.stringify({ data: { Temperature: get_temperature_number(), Distance: get_distance_number(), total_ram: Sys.total_ram(), free_ram: Sys.free_ram(), TemperatureError: temperature_error(), DistanceError: distance_error() } });
};

// Publish to MQTT topic on a button press. Button is wired to GPIO pin 0
function publish() {
  let topic = '/losant/' + Cfg.get('device.id') + '/state';
  let message = getInfo();
  mqtt_error = !MQTT.pub(topic, message, 1);
}

// Init
function init() {
  d.clearDisplay();
  d.splash(logo);
  d.display();
  Timer.set(1000, true, main, null);
  Timer.set(60000, true, publish, null);
}

init();

let LED = ffi('int get_led_gpio_pin(void)')(), BUTTON = 0;

// Publish to MQTT topic on a button press. Button is wired to GPIO pin 0
GPIO.set_button_handler(BUTTON, GPIO.PULL_UP, GPIO.INT_EDGE_NEG, 200, publish, null);