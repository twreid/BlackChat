
/*protocol defines */
/* basic protocol CMD_<define>:however the CMD differs */
/* the command is in the first byte */
#define	CMD_TEXT		0
#define	CMD_WINDOW		1
#define	CMD_VOTE		2
#define CMD_USERLIST		3
#define CMD_ERROR		4

/* text defines can be combined in the following ways
	TEXT_MAIN_CHAT & TEXT_YELL
	TEXT_IM & TEXT_YELL*/
#define TEXT_MAIN_CHAT	1	// TEXT DEFINE<0-99>:UID<0-256>:TEXT LEN<2048>:TEXT 
#define TEXT_YELL	2
#define TEXT_STATUS	4	// TEXT DEFINE:UID:TEXT LEN:TEXT 
#define TEXT_IM		8	// TEXT DEFINE:FROM UID:TO UID:TEXT LEN:TEXT 


/* window defines */
#define TYP_STATUS	0
#define TYP_HELP	1
#define TYP_MAIN	2
#define TYP_IM		3
#define TYP_INPUT	4
#define TYP_DISPLAY	5

#define VOTE_ACCEPTED		1
#define VOTE_NOT_ACCEPTED	2

#define USER_LIST_REQUEST	0
#define USER_LIST_CURRENT	2
#define USER_LIST_SIGN_OFF	3

#define USER_NAME_LEN	20	//this is code points not chars
				/* which means any char that uses this will need
				to times this by 4 for the max size */

typedef struct protocol_command {

} PRO_CMD;

typedef struct window_obj {
	int wid;
	int type;
	int x, y, z, w, h;
} *WIN_OBJ;

typedef struct history_obj {
	char* line;
	int time;
	struct user_obj *from;
	struct history_obj *next;
} *HST_OBJ;

typedef struct user_obj {
	int lurk;
	int vote; //id of user to vote off
	HST_OBJ *history;
	int uid;
	char name[USER_NAME_LEN*4];	//alpha num + foreign chars
	HST_OBJ *im;
} *UR_OBJ;
			
int get_type_from_message(const char *message);
/* returns CMD_* */

//text
int get_text_type_from_message(const char *message);
void get_text_from_message(char *message, char *result);

//window
int get_window_type_from_message(const char *message);
int get_window_x_from_message(const char *message);
int get_window_y_from_message(const char *message);
int get_window_z_from_message(const char *message);
int get_window_w_from_message(const char *message);
int get_window_h_from_message(const char *message);
int get_window_id_from_message(const char *message);
void get_window_from_message(const char *message, WIN_OBJ window);

int get_userlist_type_from_message(const char *message);

int get_vote_type_from_message(const char *message);
int get_voted_for_uid_from_message(const char *message);

int get_user_from_message(const char *message);
int get_from_user_from_message(const char *message);

int get_error_type_from_message(const char *message);

int get_next_user(int offset, const char *message, UR_OBJ user);
int get_first_user(const char *message, UR_OBJ user);
