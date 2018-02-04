#include <stdlib.h>
#include "mgos_arduino_ssd1306.h"
#include "mgos.h"
#include "mgos_mqtt.h"
#include "mgos_gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "mgos_onewire.h"

// STATIC SENSOR READING VARIABLES

static double distance;
static bool distanceError;
static bool temperatureError;
static float temperature = 20;

// TEMPERATURE DS18B20 ONE-WIRE

struct ds18b20_result {
    uint8_t rom[8];
    char *mac;
    float temp;
};

typedef void (*ds18b20_read_t)(struct ds18b20_result *result);

// Helper for allocating new things
#define new(what) (what *)malloc(sizeof(what))

// Helper for allocating strings
#define new_string(len) (char *)malloc(len * sizeof(char))

// Converts a uint8_t rom address to a MAC address string
#define to_mac(r, str) sprintf(str, "%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x", r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7])

// Read first temperatures
void ds18b20_read_first(int pin, int res, ds18b20_read_t callback) {
  uint8_t rom[8], data[9];
  int16_t raw;
  int us, cfg;
  struct mgos_onewire *ow;
  struct ds18b20_result *temp, *list = NULL;

  // Step 1: Determine config
  if ( res == 9 )       { cfg=0x1F;  us=93750;  } // 9-bit resolution (93.75ms delay)
  else if ( res == 10 ) { cfg=0x3F;  us=187500; } // 10-bit resolution (187.5ms delay)
  else if ( res == 11 ) { cfg=0x5F;  us=375000; } // 11-bit resolution (375ms delay)
  else                  { cfg=0x7F;  us=750000; } // 12-bit resolution (750ms delay)

  // Step 2: Find all the sensors
  ow = mgos_onewire_create(pin);
  mgos_onewire_search_clean(ow);
  while ( mgos_onewire_next(ow, rom, 1) ) {       // Loop over all devices
      if (rom[0] != 0x28) continue;               // Skip devices that are not DS18B20's
      temp = new(struct ds18b20_result);
      if ( temp == NULL ) {
          printf("Memory allocation failure!"); 
          exit(1);  
      }
      memcpy(temp->rom, rom, 8);
      temp->mac = new_string(23); 
      to_mac(rom, temp->mac);
      list = temp; 
      break;
  }

  // Step 3: Write the configuration
  mgos_onewire_reset(ow);
  mgos_onewire_write(ow, 0xCC); 
  mgos_onewire_write(ow, 0x4E);
  mgos_onewire_write(ow, 0x00);
  mgos_onewire_write(ow, 0x00); 
  mgos_onewire_write(ow, cfg);
  mgos_onewire_write(ow, 0x48);
  
  // Step 4: Start temperature conversion
  mgos_onewire_reset(ow);
  mgos_onewire_write(ow, 0xCC);
  mgos_onewire_write(ow, 0x44); 
  mgos_usleep(us); 

  // Step 5: Read the temperatures
  temp = list; 
  if ( temp != NULL ) {  
      mgos_onewire_reset(ow);
      mgos_onewire_select(ow, temp->rom);
      mgos_onewire_write(ow, 0xBE);
      mgos_onewire_read_bytes(ow, data, 9); 
      raw = (data[1] << 8) | data[0];
      cfg = (data[4] & 0x60);
      if (cfg == 0x00)      raw = raw & ~7;
      else if (cfg == 0x20) raw = raw & ~3;
      else if (cfg == 0x40) raw = raw & ~1;
      temp->temp = (float) raw / 16.0; 
  }

  // Step 6: Invoke the callback
  callback(list);

  // Step 7: Cleanup
  if ( list != NULL ) {  
        free(list->mac);
      free(list);    
  }
  mgos_onewire_close(ow);
}

static void temperatures_cb(struct ds18b20_result *results) {
  if ( results != NULL ) {
      temperature = results->temp;  /* Celsius */
      temperatureError = false;
  } else
  {
    temperatureError = true;
  }
}

static void temperatures_probe(void *arg) {
  temperatureError = true;
  ds18b20_read_first(17 /* GPIO */, 9 /* resolution */, temperatures_cb);
}

static int ON_BOARD_LED = 25; /* Heltec ESP32 */
bool mqtt_conn_flag = false;
static uint8_t led_timer_ticks = 0;  

// ** HC-SR04

mgos_gpio_int_handler_f interrupt_handler_pos = 0;
mgos_gpio_int_handler_f interrupt_handler_neg = 0;

static double startTime;

void get_distance(char * out) {
  sprintf(out, "%.2f", distance);;
}

void get_temperature(char * out) {
  sprintf(out, "%.1f", ((temperature * 1.8) + 32));
}

double get_distance_number() {
  return (double)roundf(distance * 100) / 100;
}

double get_temperature_number(char * out) {
  return (double)roundf(((temperature * 1.8) + 32) * 100) / 100;
}
  
int distance_error(void) {
	return (int) distanceError;
}

int temperature_error(void) {
	return (int) temperatureError;
}

static void echo_handler_pos(int pin, void *arg)
{
  startTime = mg_time();
  mgos_gpio_remove_int_handler(pin, &interrupt_handler_pos, &arg);
  mgos_gpio_set_int_handler_isr(pin, MGOS_GPIO_INT_EDGE_NEG, interrupt_handler_neg, arg);
  mgos_gpio_enable_int(pin);
}

static void echo_handler_neg(int pin, void *arg)
{
  double now = mg_time();
  double duration = (now - startTime);
  mgos_gpio_remove_int_handler(pin, &interrupt_handler_neg, &arg);

  double speedSound = (331 + ( 0.6 * temperature)) / 10;
  double d = ((duration * 1000) / 2) * speedSound / 2.54;
  if ((d < 50) && (d > 1))
  {
    distance = d;
    distanceError = false;
  }
}

void setupUltraSonic()
{
  distanceError = false;
  mgos_gpio_set_mode(13, MGOS_GPIO_MODE_OUTPUT);
  mgos_gpio_set_mode(12, MGOS_GPIO_MODE_INPUT);
  interrupt_handler_pos = echo_handler_pos;
  interrupt_handler_neg = echo_handler_neg;
}

static void pulseUltraSonic(void *arg)
{
  distanceError = true;
  mgos_gpio_set_int_handler_isr(12, MGOS_GPIO_INT_EDGE_POS, interrupt_handler_pos, NULL); 
  mgos_gpio_enable_int(12);
  mgos_gpio_write(13, 1);
  mgos_usleep(10);
  mgos_gpio_write(13, 0);
}

void *atob(unsigned char *data, int inputLength) {
  void *result;
  int outputLength = ((inputLength)/4)*3;

  result = (void *) calloc(outputLength + 1, 1);
  cs_base64_decode(data, inputLength, (char *)result, NULL);
  return result;
}
  
void ssd1306_splash(Adafruit_SSD1306 *ssd, void *data) {
  if (ssd == nullptr) return;
  uint8_t * data_c = reinterpret_cast<uint8_t *>(data);
  int h = mgos_ssd1306_height(ssd);
  int w = mgos_ssd1306_width(ssd);
  ssd->drawBitmap(0, 0, data_c, w, h, WHITE);
}

void ssd1306_drawBitmap(Adafruit_SSD1306 *ssd, void *data, int x, int y, int w, int h) {
  if (ssd == nullptr) return;
  uint8_t * data_c = reinterpret_cast<uint8_t *>(data);
  ssd->drawBitmap(x, y, data_c, w, h, WHITE);
}

int get_led_gpio_pin(void) {
  return ON_BOARD_LED;
}

int mqtt_connected(void) {
	return (int) mqtt_conn_flag;
}

static void blink_on_board_led_cb(void *arg) {
    static uint8_t remainder;

    if (mqtt_conn_flag) {
        remainder = (++led_timer_ticks % 40);
        if (remainder == 0) {
            led_timer_ticks = 0;
            mgos_gpio_write(ON_BOARD_LED, 0);  // on
        } else if (remainder == 1) {
            mgos_gpio_write(ON_BOARD_LED, 1);  // off
        }
    } else {
        mgos_gpio_toggle(ON_BOARD_LED);
    }
    (void) arg;
}

static void mqtt_ev_handler(struct mg_connection *c, int ev, void *p, void *user_data) {
  struct mg_mqtt_message *msg = (struct mg_mqtt_message *) p;
  if (ev == MG_EV_MQTT_CONNACK) {
    LOG(LL_INFO, ("CONNACK: %d", msg->connack_ret_code));
    mqtt_conn_flag = true;
  } else if (ev == MG_EV_CLOSE) {
      mqtt_conn_flag = false;
  }
  (void) user_data;
  (void) c;
}  

enum mgos_app_init_result mgos_app_init(void) {
  mgos_gpio_set_mode(ON_BOARD_LED, MGOS_GPIO_MODE_OUTPUT);
  mgos_set_timer(100 /* ms */, true /* repeat */, blink_on_board_led_cb, NULL);
  mgos_mqtt_add_global_handler(mqtt_ev_handler, NULL);
  mgos_set_timer(1000 /* ms */, true /* repeat */, pulseUltraSonic, NULL);
  mgos_set_timer(10000 /* ms */, true /* repeat */, temperatures_probe, NULL);
  mgos_set_timer(100 /* ms */, false /* repeat */, temperatures_probe, NULL);
  
  setupUltraSonic();
  
  return MGOS_APP_INIT_SUCCESS;
}
  
#ifdef __cplusplus
}
#endif
