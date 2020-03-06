
# ESP8266 (Espressif 8266) Prometheus Exporter for DS18B20 Temperature Sensors

Exposes a prometheus exporter endpoint endpoint for scraping DS18B20 temperature sensor readings.

Forked from https://github.com/theoretick/esp8266_prometheus_exporter_for_ds18b20_temperature_sensors to convert to a traditional Arduino project.


## Usage

1. Configure `ssid` and `password` for wifi network
2. Build with Arduino IDE
3. hit `/metrics` endpoint for reading temperature (C), temperature (F), device resolution, and device count:

```
❯ curl 10.0.1.1/metrics

# HELP beertemp_device_total A count of probe devices connected.
# TYPE beertemp_device_total gauge
beertemp_device_total 1
# HELP beertemp_device_temperature_celsius Current device temperature in celsius.
# TYPE beertemp_device_temperature_celsius gauge
beertemp_device_temperature_celsius{id="28d1813592066618" resolution="12"} 22.69
# HELP beertemp_device_temperature_fahrenheit Current device temperature in fahrenheit.
# TYPE beertemp_device_temperature_fahrenheit gauge
beertemp_device_temperature_fahrenheit{id="28d1813592066618" resolution="12"} 72.84
```

