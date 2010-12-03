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
  free(bc_server->connected_clients);
  free(bc_server->unconnected_clients);
  free(bc_server);
  exit(0);
  
}

void handle_messages(SERVER_OBJ *server, SERVER_QUEUE_OBJ *messages);

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
  WIN_OBJ *window;
  
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
	    int user = get_from_user_from_message(message);
	    HST_OBJ temp = server->clients[user]->user_data->history;
	    
	    server->clients[user]->user_data->history->line = get_text_from_message(message);
	    server->clients[user]->user_data->history->from = NULL;
	    server->clients[user]->user_data->history->next = temp;
	    //TODO update time
	    
	    //TODO broadcaast text to all users
	    
	    
	    break;
	  case TEXT_YELL:
	    //TODO Send message with server yells to client
	    break;
	  case TEXT_STATUS:
	    //The server should only get this if the client went into lurking mode
	    int user = get_from_user_from_message(message);
	    
	    server->clients[user]->user_data->lurk = 1;
	    
	    //TODO maybe send lurking command to server to update status or send and entirely new status to client
	    break;
	  case TEXT_IM:
	    int user = get_from_user_from_message(message);
	    int to_user = get_user_from_message(message);
	    HST_OBJ temp = server->clients[user]->user_data->im;
	    
	    server->clients[user]->user_data->im->line = get_text_from_message(message);
	    server->clients[user]->user_data->im->from = NULL;
	    server->clients[user]->user_data->im->next = temp;
	    //TODO update time
	    
	    //TODO broadcaast text to IM user
	    break;
	  default:
	    //TODO Send ERROR_UNKNOWN_MSG
	    break;
	  
	}
      break;
    case CMD_WINDOW:
      get_window_from_message(message, window);
      
      break;
    case CMD_VOTE:
      int uid_vote;
      
      vote_type = get_vote_type_from_message(message);
      
      switch(vote_type){
      
	case VOTE_ACCEPTED:
	  break;
	case VOTE_NOT_ACCEPTED:
	  break;
	default:
	  //TODO Send list of connected users to vote off to client
	  break;
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
}



