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

Comando para dados de temperatura: T

Comando para dados de umidade: U 

Comando para dados de distância: D
  


           
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


