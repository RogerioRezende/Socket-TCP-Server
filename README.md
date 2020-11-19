O trabalho desenvolvido mostra a comunicaçao entre cliente e servidor através do ESP32.  O 

ESP32 foi configurado como um servidor socket TCP em uma porta específica e que um 

terminal de cliente possa conectar neste servidor e requisitar as informações de 

Temperatura, Umidade e Distância dos sensores utilizados.

O código de programação possui linhas de comando chamadas de "TAREFAS" para fazer a 

leitura dos dados obtidos através dos sensores ULTRASÔNICO e DHT.

Essas informações serão guardadas em " FILAS" para que quando o cliente faça a 

solicitação de informaçõs seja enviada.

O cliente irá fazer a soliçitação através dos comandos escrito em um programa de terminal 

chamado REALTERM que fará a comunição entre CLIENTE e SERVIDOR.
Comando para dados de temperatura: TEMP
Comando para dados de umidade: UMID 
Comando para dados de distância: DIST
  


            if(len>=3 && rx_buffer[0]=='T' && rx_buffer[1]=='E' && rx_buffer[2]=='M' && rx_buffer[3]=='P')
			{
				xQueueReceive(bufferTemperatura,&temp,pdMS_TO_TICKS(0));
                sprintf(stringTemperatura,"%d",temp);
                send(sock, stringTemperatura, len, 0);
			}

            else if(len>=3 && rx_buffer[0]=='U' && rx_buffer[1]=='M' && rx_buffer[2]=='I' && rx_buffer[3]=='D')
			{
				xQueueReceive(bufferUmidade,&umid,pdMS_TO_TICKS(0));
                sprintf(stringUmidade,"%d",umid);
                send(sock, stringUmidade, len, 0);
			}

            else if(len>=3 && rx_buffer[0]=='D' && rx_buffer[1]=='I' && rx_buffer[2]=='S' && rx_buffer[3]=='T')
			{
				xQueueReceive(bufferDistancia,&dist,pdMS_TO_TICKS(0));
                sprintf(stringDistancia,"%d",dist);
                send(sock, stringDistancia, len, 0);
			}


