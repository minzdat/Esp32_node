#ifndef CONNECT_WIFI_H
#define CONNECT_WIFI_H

typedef enum {
    Internet     = 1,
    NoInternet  = 0
}INTERNET_CHECK;

void wifi_init_sta(const char* w_ssid,const char* w_pass);
void wifi_init_softap(const char* w_ssid,const char* w_pass);
void https_ping(const char* urrl);
// void https_pingg(void *pvParameters);
void https_request_check(const char* urrl);

void https_cofig(const char* urrl);
int check_internet(void);
void set_wifi_max_tx_power(void);


#endif