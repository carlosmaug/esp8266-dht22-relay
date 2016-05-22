enum EDhtType{
	SENSOR_DHT11,SENSOR_DHT22
};

struct DhtReading {
	float temperature;
	float humidity;
	uint8_t sensor_id[16];
	BOOL success;
};


void ICACHE_FLASH_ATTR dht(void);
struct DhtReading * ICACHE_FLASH_ATTR dht_read(int force);
void dht_init(enum EDhtType, uint32_t polltime);
