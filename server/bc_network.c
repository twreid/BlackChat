/* Copyright (C) 2010  BlackChat Group
This file is part of BlackChat.

Ashes is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Ashes is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with BlackChat.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "include/bc_network.h"
#include <sys/stat.h>
#include <sys/types.h>

pthread_mutex_t mutex;
sem_t messages_sem;

void fill_queue(SERVER_OBJ *server, SERVER_QUEUE_OBJ *messages);
void *listen_thread(void *args);
void *client_thread(void *args);

SERVER_OBJ *init_network(SERVER_QUEUE_OBJ *messages) {

  struct sockaddr_in server_addr;

  SERVER_OBJ *server = (SERVER_OBJ *)malloc(sizeof(SERVER_OBJ));
  memset(server, 0, sizeof(SERVER_OBJ));

  server->server_socket = socket(AF_INET, SOCK_STREAM, 0);

  if (pthread_mutex_init(&mutex, NULL) != 0)
    return NULL; // Couldn't create mutex

  if (sem_init(&messages_sem, 0, 0) == -1)
    return NULL; // Couldn't init semaphore

  if (server->server_socket == -1)
    return NULL; // Couldn't create socket

  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
  server_addr.sin_port = htons(SERVER_PORT);

  if (bind(server->server_socket, (struct sockaddr *)&server_addr,
           sizeof(server_addr)) == -1)
    return NULL; // Couldn't bind addrress

  if (listen(server->server_socket, LISTEN_QUEUE) == -1)
    return NULL; // Could't Listen for incoming connections

  server->num_users_connected = 0;
  server->user_idle_time_max = 1800; // 30 mins
  // server->clients = (CLIENT_OBJ *)malloc(sizeof(CLIENT_OBJ) * 11); //Make
  // room for 10 clients
  memset(server->clients, 0, sizeof(CLIENT_OBJ));

  pthread_mutex_lock(&mutex);
  fill_queue(server, messages);
  pthread_mutex_unlock(&mutex);

  if (pthread_create(&server->listen_thread_id, NULL, listen_thread, server) ==
      -1)
    return NULL; // Unable to spawn the thread to accept clients

  return server;
}

void fill_queue(SERVER_OBJ *server, SERVER_QUEUE_OBJ *messages) {

  for (int i = 1; i < 11; i++) {

    server->clients[i] = (CLIENT_OBJ *)calloc(1, sizeof(CLIENT_OBJ));

    server->clients[i]->bytes_from = 0;
    server->clients[i]->bytes_to = 0;
    server->clients[i]->user_data = (UR_OBJ)calloc(1, sizeof(struct user_obj));
    server->clients[i]->user_data->history =
        (HST_OBJ)malloc(sizeof(struct history_obj));
    server->clients[i]->user_data->im =
        (HST_OBJ)malloc(sizeof(struct history_obj));
    server->clients[i]->user_data->history->next = NULL;
    server->clients[i]->user_data->im->next = NULL;
    server->clients[i]->user_data->lurk = 0;
    server->clients[i]->user_data->uid = 1;
    server->clients[i]->user_data->vote = -1;
    snprintf(server->clients[i]->user_data->name,
             sizeof(server->clients[i]->user_data->name), "User%d", i);
    server->clients[i]->seconds_connected = 0;
    server->clients[i]->messages = messages;
    server->clients[i]->is_connected = false;
  }
}
void *listen_thread(void *args) {
  printf("Listen thread spawned\n");
  SERVER_OBJ *server = (SERVER_OBJ *)args;

  for (;;) {
    struct sockaddr_in client_addr;
    socklen_t client_len;
    int temp_client;
    int num_connections;
    CLIENT_OBJ *temp_client_obj = NULL;

    client_len = sizeof(client_addr);

    printf("Before client accept\n");
    temp_client = accept(server->server_socket, (struct sockaddr *)&client_addr,
                         &client_len);
    printf("Client accepted with socket: %d\n", temp_client);

    num_connections = server->num_users_connected;
    printf("num users online %d\n", num_connections);

    if (num_connections < MAX_CONNECTIONS) {
      for (int i = 1; i <= MAX_CONNECTIONS; i++) {
        if (true != server->clients[i]->is_connected) {
          temp_client_obj = server->clients[i];
          break;
        }
      }

      temp_client_obj->client_socket = temp_client;
      temp_client_obj->is_connected = true;
      temp_client_obj->time_connected = time(NULL);
      temp_client_obj->server = server;

      if (pthread_create(&temp_client_obj->client_thread_id, NULL,
                         client_thread, temp_client_obj) != 0)
        perror("Could not spawn client thread\n");

      server->num_users_connected += 1;
    } else {
      char *buff = (char *)calloc(1024, sizeof(char));
      // Uncomment once function is implemented
      // create_error_message(ERROR_CHAT_FULL, buff);

      write(temp_client, buff, 1024);
      close(temp_client);
    }
  }
}

void *client_thread(void *args) {

  printf("Client connected\n");

  CLIENT_OBJ *client = (CLIENT_OBJ *)args;
  char *buff = (char *)malloc(
      1024 * 8); // Read in 8k at a time should be more than enough
  int bytes_read;
  bool messages_empty;

  char *buff2 = (char *)malloc(1024 * 8);
  char *path = (char *)malloc(1024);
  char *temp_buff = (char *)malloc(1024 * 8);
  memset(temp_buff, '\0', 1024 * 8);
  char *temp_buff2 = (char *)malloc(1024 * 8);
  memset(temp_buff2, '\0', 1024 * 8);

  read(client->client_socket, buff2, USER_NAME_LEN * 4);
  printf("user name connected %s on socket %d\n", buff2, client->client_socket);
  memset(client->user_data->name, '\0', USER_NAME_LEN * 4);
  get_user_name_message(buff2, client->user_data->name);
  memset(path, '\0', 1024);
  buff2[0] = '\0';
  create_uid_message(client->user_data->uid, buff2);

  printf("UID MSG: %s", buff2);

  write(client->client_socket, buff2, strlen(buff2));

  free(buff2);

  sprintf(path, "./%s", client->user_data->name);

  // mkdir(path, 777);
  free(path);

  for (;;) {
    printf("Begin client loop\n");

    pthread_testcancel();

    pthread_mutex_lock(&mutex);
    messages_empty = isEmpty(client->messages);
    pthread_mutex_unlock(&mutex);

    if (!messages_empty) {

      memset(buff, '\0', 1024 * 8);
      bytes_read = read(client->client_socket, buff, 1024 * 8);
      client->bytes_from += bytes_read;

      if (bytes_read == 0) {

        sprintf(temp_buff2, "%s has logged off.", client->user_data->name);

        create_main_chat_message(client->user_data->uid, temp_buff2, temp_buff);

        for (int i = 1; i <= 10; i++) {

          if (client->user_data->uid != 1)
            broadcast_client(client->server->clients[i], temp_buff);
        }

        break;
      }

      // printf("Adding To message queue: %s\n", buff);
      pthread_mutex_lock(&mutex);
      enqueue(client->messages, buff);
      pthread_mutex_unlock(&mutex);

      sem_post(&messages_sem);
    } else {

      memset(buff, '\0', 1024 * 8);

      bytes_read = read(client->client_socket, buff, 1024 * 8);
      client->bytes_from += bytes_read;

      if (bytes_read == 0)
        break;

      // printf("Adding to message queue in else block: %s\n", buff);
      pthread_mutex_lock(&mutex);
      enqueue(client->messages, buff);
      pthread_mutex_unlock(&mutex);

      sem_post(&messages_sem); // Lets the main thread know there are messages
                               // to process
    }
  }

  free(temp_buff);
  free(temp_buff2);

  printf("Closing Client begin\n");
  pthread_mutex_lock(&mutex);
  client->bytes_from = 0;
  client->bytes_to = 0;
  client->is_connected = false;
  client->seconds_connected = 0;
  close(client->client_socket);
  client->server->num_users_connected -= 1;
  pthread_mutex_unlock(&mutex);
  printf("closing client end\n");

  pthread_exit(0);
}

void broadcast_all(CLIENT_OBJ *clients[], char *message) {

  int bytes_written;

  for (int i = 1; i <= MAX_CONNECTIONS; i++) {

    if (clients[i]->is_connected) {
      bytes_written =
          write(clients[i]->client_socket, message, strlen(message));

      pthread_mutex_lock(&mutex);
      clients[i]->bytes_to += bytes_written;
      pthread_mutex_unlock(&mutex);
    }
  }
}

void broadcast_client(CLIENT_OBJ *client, char *message) {

  int bytes_written;

  bytes_written = write(client->client_socket, message, 1024 * 8);

  pthread_mutex_lock(&mutex);
  client->bytes_to += bytes_written;
  pthread_mutex_unlock(&mutex);
}
