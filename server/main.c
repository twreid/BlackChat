#include "bc_client.h"
#include "bc_server_queue.h"
#include "bc_network.h"
#include <unistd.h>

SERVER_OBJ *bc_server;

void update_time(int sig){
    pthread_mutex_lock(&mutex);
    int num_clients = bc_server->num_users_connected;
    pthread_mutex_unlock(&mutex);

    for(int i = 0; i < num_clients; i++){
      
      if(bc_server->clients[i]->is_connected){
	//TODO update the time 
      }
		     
    }

  //TODO send time updates to each client
}

void cleanup(int sig){

  //TODO add cleanup code
  close(bc_server->server_socket);
  free(bc_server->clients);
//  free(bc_server->connected_clients);
//  free(bc_server->unconnected_clients);
  free(bc_server);
  exit(0);
  
}

void handle_messages(SERVER_OBJ *server, SERVER_QUEUE_OBJ *messages);
int save_user_window(WIN_OBJ window, char *user);
int get_user_window(WIN_OBJ window, char *user);
void disconnect_user(int uid);

int main(int argc, char **argv) {
  signal(SIGTERM, cleanup);
  signal(SIGPIPE, SIG_IGN);
  signal(SIGALRM, update_time); 
  
  bc_server = (SERVER_OBJ *)malloc(sizeof(SERVER_OBJ));
  memset(bc_server, 0, sizeof(SERVER_OBJ));
  
  SERVER_QUEUE_OBJ *messages = (SERVER_QUEUE_OBJ *)malloc(sizeof(SERVER_QUEUE_OBJ));
  memset(messages, 0, sizeof(SERVER_QUEUE_OBJ));
  
  messages = init_queue(-1);
  
  bc_server = init_network(messages);
  
  if(bc_server == NULL){
  
    perror("Unable to init server");
    exit(-1);
    
  }
  
  //Once here I need to start handling messages
  
  handle_messages(bc_server, messages);
  
    
}

void handle_messages(SERVER_OBJ* server, SERVER_QUEUE_OBJ* messages){
  
  int cmd_type;
  int text_type;
  int window_type;
  int vote_type;
  int user_type;
  int error_type;
  char *message;
  char *message_data;
  
  char *buff;
  char *result;
  
  if(isEmpty(messages)){
	sem_wait(&messages_sem);
  }
  pthread_mutex_lock(&mutex);
  message = dequeue(messages);	 
  pthread_mutex_unlock(&mutex);
  
  cmd_type = get_type_from_message(message);
  
  switch(cmd_type) {
  
    case CMD_TEXT:
	text_type = get_text_type_from_message(message);
	switch(text_type) {
	
	  case TEXT_MAIN_CHAT:
	  {
	    buff = (char *)malloc(1024 * 8);
	    int user = get_from_user_from_message(message);
	    HST_OBJ temp = server->clients[user]->user_data->history;
	    
	    
	    server->clients[user]->user_data->history->from = NULL;
	    server->clients[user]->user_data->history->next = temp;
	    //TODO update time
	    
	    create_text_message(TEXT_MAIN_CHAT, user, server->clients[user]->user_data->history->line, buff);
	    
	    broadcast_all(server->clients, buff);
	    
	    free(buff);
	    buff = NULL;
	  }    
	    break;
	  case TEXT_YELL:
	  {
	    int user = get_user_from_message(message);
	    buff = (char *)malloc(512);
	    result = (char *)malloc(512);
	    get_text_from_message(message,result);
	
	    create_yell_message(user, result, buff);
	    
	    broadcast_client(server->clients[user], buff);
	    
	    free(buff);
	    buff = NULL;
	  }
	    break;
	  case TEXT_STATUS:
	  {
	    //The server should only get this if the client went into lurking mode
	    int user = get_from_user_from_message(message);
	    
	    server->clients[user]->user_data->lurk = 1;
	    
	    //TODO maybe send lurking command to server to update status or send and entirely new status to client
	  }
	    break;
	  case TEXT_IM:
	  {
	    int user = get_from_user_from_message(message);
	    int to_user = get_user_from_message(message);
	    HST_OBJ temp = server->clients[user]->user_data->im;
	    buff = (char *)malloc(1024 * 8);
	    
	    get_text_from_message(message, server->clients[user]->user_data->im->line);
	    server->clients[user]->user_data->im->from = NULL;
	    server->clients[user]->user_data->im->next = temp;
	    //TODO update time
	    
	    create_im_message(user, to_user, server->clients[user]->user_data->im->line, buff);
	    
	    broadcast_client(server->clients[to_user], buff);
	    
	    free(buff);
	    buff = NULL;
	  }
	    break;
	  default:
	  {
//	    int user = get_from_user_from_message(message);
	//    char *temp = "Invalid Message Sent to Server!!\n";
	    //TODO send error
	    //create_text_message();
	  }
	    break;
	  
	}
      break;
    case CMD_WINDOW:
    {
      WIN_OBJ window = (WIN_OBJ)malloc(sizeof(struct window_obj));

      get_window_from_message(message, window);

      save_user_window(window, get_from_user_from_message(message));

	free(window);
    } 
      break;
    case CMD_VOTE:
    {
      int voted_user = get_voted_for_uid_from_message(message);
      int user = get_from_user_from_message(message);
      int num_votes = 0;
      bool is_vote_done = false;
      buff = (char *)malloc(1024);
      
      if(server->clients[user]->user_data->vote != voted_user){
	
	server->clients[user]->user_data->vote = voted_user;
	
	respond_vote_message(VOTE_ACCEPTED, user, voted_user, buff);
	
	broadcast_client(user, buff);
	
      }
      else{
      
	respond_vote_message(VOTE_NOT_ACCEPTED, user, voted_user, buff);
	broadcast_client(user, buff);
	
      }
      
      for(int i = 1; i < MAX_CONNECTIONS + 1; i++){
      
	pthread_mutex_lock(&mutex);
	if(server->clients[i]->is_connected && server->clients[i]->user_data->vote != -1)
	  num_votes++;
	
	pthread_mutex_unlock(&mutex);
	
      }
      
      if(num_votes == server->num_users_connected){
	int votes[] = {-1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	int user_voted_off = 0;
	
	for(int i = -1; i < MAX_CONNECTIONS + 1; i++){
	
	  pthread_mutex_lock(&mutex);
	  if(server->clients[i]->is_connected && server->clients[i]->user_data->vote != -1){	  
	    votes[server->clients[i]->user_data->vote]++;	    
	  }	  
	  pthread_mutex_lock(&mutex);
	  
	}
	
	for(int i = 0; i < 11; i++){
	
	  if(votes[user_voted_off] > votes[i])
	    user_voted_off = i;
	  
	}
	
	disconnect_user(user_voted_off);
	
	for(int i = 1; i < MAX_CONNECTIONS + 1; i++){
	
	  pthread_mutex_lock(&mutex);
	  server->clients[i]->user_data->vote = -1;
	  pthread_mutex_unlock(&mutex);
	  
	}
	
      }
    }
      
      break;
    case CMD_USERLIST:
      user_type = get_userlist_type_from_message(message);
      
      switch(user_type){
      
	case USER_LIST_REQUEST:
	  //TODO loop through user array and send to client
	  break;
	case USER_LIST_SIGN_OFF:
	  //Prefer not to use this and would rather detect a read of 0 bytes from client to disconnect
	  break;
	default:
	  //TODO send ERROR_UNKNOWN_MSG
	  break;
	
      }
      break;
    case CMD_ERROR:
	error_type = get_error_type_from_message(message);
	
	switch(error_type){
	
	  default:
	    //TODO send ERROR_UNKNOWN_MSG
	    break;
	  
	}
      break;
    default:
      //TODO send ERROR_UNKNOWN_MSG
    break;
  }

	free(result);
}


int save_user_window(WIN_OBJ window, int user){
  
  char *file_to_open = (char *)malloc(1024);
  FILE *file;
  int written;
  int *win[] = { window->h, window->w, window->x, window->y, window->z, window->type, window->wid};
  sprintf(file_to_open, "./saved_sessions/%d/%d/%d", user, window->type, window->wid);
  
  file = fopen(file_to_open, "wb");
  if(file == NULL)
    return -1;
    
  written = fwrite(win, sizeof(int), 7, file);
  
  if(written != 7)
    return -1;
  
  fclose(file);
  free(file_to_open);
  
  return 0;

}

int get_user_window(WIN_OBJ window, int user){  
    
  char *file_to_open = (char *)malloc(1024);
  FILE *file;
  int read;
  int *win[] = { 0, 0, 0, 0, 0, 0, 0};
  sprintf(file_to_open, "./saved_sessions/%d/%d/%d", user, window->type, window->wid);
  
  file = fopen(file_to_open, "rb");
  if(file == NULL)
    return -1;
    
  read = fread(win, sizeof(int), 7, file);
  
  if(read != 7)
    return -1;
  
  window->h = win[0];
  window->w = win[1];
  window->x = win[2];
  window->y = win[3];
  window->z = win[4];
  window->type = win[5];
  window->wid = win[6];
  
  fclose(file);
  free(file_to_open);
  
  return 0;

}


void disconnect_user(int uid){
  CLIENT_OBJ *temp = bc_server->clients[uid];
  pthread_mutex_lock(&mutex);
  
  pthread_cancel(temp->client_thread_id);
  temp->bytes_from = 0;
  temp->bytes_to = 0;
  temp->is_connected = false;
  close(temp->client_socket);
  pthread_mutex_unlock(&mutex);
}



