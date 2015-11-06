# remoteDeviceControl

Para compilar o programa, é necessário alterar o arquivo Makefile:
  - Altere a variavel BROADCAST_IP para o ip broadcast da rede desejada.
  - Altere a variavel NAME para o nome desejado para o server.
  - Caso não tenha c++11 disponivel na máquina, comente a linha CFLAGS+=-std=c++11

Para executar o server:
  $ make
  $ make run

Para executar o client:
  $ make
  $ ./client <server_ip> <server_port> <msg>
