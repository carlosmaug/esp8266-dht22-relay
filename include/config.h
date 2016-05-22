// Above this temp we turn on ralay
#define DEFAULT_TEMP 40
// Above this humidity we turn on ralay
#define DEFAULT_HUM  60
// If the relay is turned on manually, we turn it off after this number of minutes
#define DEFAULT_TIME 10
// 0 Relay operates normally, 1 relay does not turn on automatically
#define DEFAULT_OFF   0
// Valid values are: 'SENSOR_DHT22' or 'SENSOR_DHT11'
#define SENSORTYPE    SENSOR_DHT22
// Time between readings of sensor
#define POOLTIME  30000

struct config {
	short int checksum;
	short int hum;
	short int temp;
	short int time;
	short int off;
};


void config_init(void); 
int config_save(struct config save); 
struct config config_read(void);
