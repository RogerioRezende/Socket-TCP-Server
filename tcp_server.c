/*
	Autor: Marcio Pasini e Rogerio Rezende
	Objetivo: Apresentar um exemplo programa com ESP32 em modo servidor Socket TCP
	Disciplina: IoT Aplicada
	Curso: Engenharia Mecatrônica
*/

/* BSD Socket API Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

/* Inclusão das Bibliotecas */
#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include <dht.h>
#include <ultrasonic.h>

/* Definições e Constantes */
#define TRUE          		1 
#define FALSE		  		0
#define DEBUG         		TRUE
#define MAX_DISTANCE_CM 	500 // 5m max
#define TRIGGER_GPIO 		GPIO_NUM_0
#define ECHO_GPIO 			GPIO_NUM_2
#define DHT_GPIO			GPIO_NUM_16
#define PORT CONFIG_EXAMPLE_PORT

#ifdef CONFIG_EXAMPLE_IPV4
#define HOST_IP_ADDR CONFIG_EXAMPLE_IPV4_ADDR
#else
#define HOST_IP_ADDR CONFIG_EXAMPLE_IPV6_ADDR
#endif

QueueHandle_t bufferTemperatura; //cria fila temperatira
QueueHandle_t bufferUmidade; //cria fila umidade
QueueHandle_t bufferDistancia; //cria fila distancia

/* Variáveis Globais */
static const char *TAG = "example";
static const dht_sensor_type_t sensor_type = DHT_TYPE_DHT11;
static const gpio_num_t dht_gpio = DHT_GPIO;

static void do_retransmit(const int sock)
{
    int len;
    char rx_buffer[128];

    do {
        len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
        if (len < 0) {
            ESP_LOGE(TAG, "Error occurred during receiving: errno %d", errno);
        } else if (len == 0) {
            ESP_LOGW(TAG, "Connection closed");
        } else {
            rx_buffer[len] = 0; // Null-terminate whatever is received and treat it like a string
            ESP_LOGI(TAG, "Received %d bytes: %s", len, rx_buffer);

            // send() can return less bytes than supplied length.
            // Walk-around for robust implementation. 
            /*int to_write = len;
            while (to_write > 0) {
                int written = send(sock, rx_buffer + (len - to_write), to_write, 0);
                if (written < 0) {
                    ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
                }
                to_write -= written;
            }*/

            uint16_t temp;
            uint16_t umid;
            uint16_t dist;
            char stringUmidade[10];
            char stringTemperatura[10];
            char stringDistancia[10];
			int tamanho = 0;

            if(len>=1 && (rx_buffer[0]=='T' || rx_buffer[0]=='t'))
			{
				xQueueReceive(bufferTemperatura,&temp,pdMS_TO_TICKS(0));
				
				if(temp < 10)
					tamanho = 1;
				else if(temp >= 10 && temp < 100)
					tamanho = 2;
				else if(temp >= 100 && temp < 1000)
					tamanho = 3;
				
                sprintf(stringTemperatura,"%d",temp);
                send(sock, stringTemperatura, tamanho, 0);
				send(sock, "C  ", 3, 0);
				
			}

            else if(len>=1 && (rx_buffer[0]=='U' || rx_buffer[0]=='u'))
			{
				xQueueReceive(bufferUmidade,&umid,pdMS_TO_TICKS(0));
				
				if(umid < 10)
					tamanho = 1;
				else if(umid >= 10 && umid < 100)
					tamanho = 2;
				else if(umid >= 100 && umid < 1000)
					tamanho = 3;
				
                sprintf(stringUmidade,"%d",umid);
                send(sock, stringUmidade, tamanho, 0);
				send(sock, "%  ", 3, 0);
			}

            else if(len>=1 && (rx_buffer[0]=='D' || rx_buffer[0]=='d'))
			{
				xQueueReceive(bufferDistancia,&dist,pdMS_TO_TICKS(0));
	
				if(dist < 10)
					tamanho = 1;
				else if(dist >= 10 && dist < 100)
					tamanho = 2;
				else if(dist >= 100 && dist < 1000)
					tamanho = 3;
				
                sprintf(stringDistancia,"%d",dist);
                send(sock, stringDistancia, tamanho, 0);
				send(sock, "cm  ", 4, 0);
			}

        }
    } while (len > 0);
}

static void tcp_server_task(void *pvParameters)
{
    char addr_str[128];
    int addr_family;
    int ip_protocol;


#ifdef CONFIG_EXAMPLE_IPV4
    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(PORT);
    addr_family = AF_INET;
    ip_protocol = IPPROTO_IP;
    inet_ntoa_r(dest_addr.sin_addr, addr_str, sizeof(addr_str) - 1);
#else // IPV6
    struct sockaddr_in6 dest_addr;
    bzero(&dest_addr.sin6_addr.un, sizeof(dest_addr.sin6_addr.un));
    dest_addr.sin6_family = AF_INET6;
    dest_addr.sin6_port = htons(PORT);
    addr_family = AF_INET6;
    ip_protocol = IPPROTO_IPV6;
    inet6_ntoa_r(dest_addr.sin6_addr, addr_str, sizeof(addr_str) - 1);
#endif

    int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
    if (listen_sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        vTaskDelete(NULL);
        return;
    }
    ESP_LOGI(TAG, "Socket created");

    int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0) {
        ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        goto CLEAN_UP;
    }
    ESP_LOGI(TAG, "Socket bound, port %d", PORT);

    err = listen(listen_sock, 1);
    if (err != 0) {
        ESP_LOGE(TAG, "Error occurred during listen: errno %d", errno);
        goto CLEAN_UP;
    }

    while (1) {

        ESP_LOGI(TAG, "Socket listening");

        struct sockaddr_in6 source_addr; // Large enough for both IPv4 or IPv6
        uint addr_len = sizeof(source_addr);
        int sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
            break;
        }

        // Convert ip address to string
        if (source_addr.sin6_family == PF_INET) {
            inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr.s_addr, addr_str, sizeof(addr_str) - 1);
        } else if (source_addr.sin6_family == PF_INET6) {
            inet6_ntoa_r(source_addr.sin6_addr, addr_str, sizeof(addr_str) - 1);
        }
        ESP_LOGI(TAG, "Socket accepted ip address: %s", addr_str);

        do_retransmit(sock);

        shutdown(sock, 0);
        close(sock);
    }

CLEAN_UP:
    close(listen_sock);
    vTaskDelete(NULL);
}

void task_ultrasonic(void *pvParamters)
{
    ultrasonic_sensor_t sensor = {
        .trigger_pin = TRIGGER_GPIO,
        .echo_pin = ECHO_GPIO
    };
    ultrasonic_init(&sensor);

    while (true)
    {
        uint32_t distance;
        esp_err_t res = ultrasonic_measure_cm(&sensor, MAX_DISTANCE_CM, &distance);
        if (res != ESP_OK)
        {
            printf("Error: ");
            switch (res)
            {
                case ESP_ERR_ULTRASONIC_PING:
                    printf("Cannot ping (device is in invalid state)\n");
                    break;
                case ESP_ERR_ULTRASONIC_PING_TIMEOUT:
                    printf("Ping timeout (no device found)\n");
                    break;
                case ESP_ERR_ULTRASONIC_ECHO_TIMEOUT:
                    printf("Echo timeout (i.e. distance too big)\n");
                    break;
                default:
                    printf("%d\n", res);
            }
        }
        else{
            xQueueSend(bufferDistancia,&distance,pdMS_TO_TICKS(0));
            //printf("Distance: %d cm\n", distance);
            }

        vTaskDelay(3000 / portTICK_PERIOD_MS);
    }
}

void task_dht(void *pvParameters)
{
    int16_t temperature = 0;
    int16_t humidity = 0;    
    gpio_set_pull_mode(dht_gpio, GPIO_PULLUP_ONLY);
    while (1)
    {
        if (dht_read_data(sensor_type, dht_gpio, &humidity, &temperature) == ESP_OK)
        {
			humidity = humidity / 10;
			temperature = temperature / 10;
            xQueueSend(bufferUmidade,&humidity,pdMS_TO_TICKS(0));
            xQueueSend(bufferTemperatura,&temperature,pdMS_TO_TICKS(0)); 
            //printf("Humidity: %d%% Temp: %dC\n", humidity, temperature);
        }
        else
        printf("Could not read data from sensor\n");

        vTaskDelay(2000 / portTICK_PERIOD_MS);
        
    }
}

void app_main()
{
	/*ESP_LOGI(TAG, "[APP] Inicio..");
    ESP_LOGI(TAG, "[APP] Memória Livre: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] Versão IDF: %s", esp_get_idf_version());*/
		
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());

    bufferUmidade = xQueueCreate(3,sizeof(uint16_t));
    bufferTemperatura = xQueueCreate(3,sizeof(uint16_t));
    bufferDistancia = xQueueCreate(1,sizeof(uint16_t));

    xTaskCreate(tcp_server_task, "tcp_server", 4096, NULL, 5, NULL);
	xTaskCreate(task_dht, "task_dht", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);
    xTaskCreate(task_ultrasonic, "task_ultrasonic", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);

	//ESP_LOGI(TAG, "[APP] Memória Livre Após Criação das Tasnks: %d bytes", esp_get_free_heap_size());

}
