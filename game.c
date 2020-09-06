/* game.c - xmain, prntr */

#include <conf.h>
#include <kernel.h>
#include <io.h>
#include <bios.h>
#include <dos.h>
#include <conio.h>
#include <stdio.h> 
#include <stdlib.h> 
#include <time.h>
#include <string.h>
//Keys for player 1 : a w d x
#define LEFT_PLAYER1 1
#define RIGHT_PLAYER1 2
#define UP_PLAYER1 3
#define DOWN_PLAYER1 4
#define RELEAS_PLAYER1 5
//Keys for player 2 : up down left right
#define LEFT_PLAYER2 6
#define RIGHT_PLAYER2 7
#define UP_PLAYER2 8
#define DOWN_PLAYER2 9
#define RELEAS_PLAYER2 10
//game screens:
#define START_SCREEN 11
#define GAME_SCREEN 12
#define END_SCREEN 13

#define INTERRUPT70H	0x70
#define latch_0x70 248
//Life time for game: 
#define LEVEL1SECONDS 240
#define LEVEL2SECONDS 180
#define LEVEL3SECONDS 120

INTPROC Int0x70Handler(int mdevno);
extern SYSCALL  sleept(int);
extern struct intmap far *sys_imp;

/*------------------------------------------------------------------------
 *  xmain  --  example of 2 processes executing the same code concurrently
 *------------------------------------------------------------------------
 */
 
//-------------------------------------------------

//function definitions
void initialGameBoard();
void initializer();
void start_game();
void change_only_color_on_screen(int row,int col,int color);
void change_screen(int row,int col, char c,int color);
int get_rand_color(int numOfColors);
void display_next(int positionX,int positionY,int nextColors[],char orientation);
void delete_previou_location(int positionX,int positionY, char orientation);
void fill_next_colors(int player,int nextColors[]);
void clearScreen();	
void generate_welcome_screen();
void changeColors(int);
void readyNextColors(int player);
void clearBoard();
void displayTime();
void duplicateEnamy(int player);
void deleteFromBoard(int player, int row, int col);

//-------------------------------------------------
//structures:

typedef struct player
{
	int board[19][6];                      //the board of the player. contain 19 x 6 places.
	int score;                             //score of the player.
	int speed;                             //speed of the player.
	int oldSpeed;                          //old speed of the player.
	int curPiece[3];                       //this is the 3 colore pieces of the shape which currently fall.
	char curOrientation;                   //orientation of the current shape currently fall.
	int nextPiece[3];                      //the 3 colore pieces of the next shape to come.
	int curPiece_x;                        //the x coordinate of the top left on the shape regard to the all the screan.
	int curPiece_y;                        //the y coordinate of the top left on the shape regard to the all the screan.
	int placeOnBoard_x;                    //the x coordinate of the top left on the shape regard to the board of the player (19 x 6).
	int placeOnBoard_y;                    //the y coordinate of the top left on the shape regard to the board of the player (19 x 6)
	int oldLocation_x;                     //save the x previus location (for press on down key) before release the keyboard kay
	int oldLocation_y;                     //save the y previus location (for press on down key) before release the keyboard kay
	int sequence;
}PLAYER;

typedef struct game_info
{
	char fireworks[12][54];
	int selectedDifficultyLevel;           //tell which Level player choose. default is 1 (easy level).
	int screen;                            //tell which screan: START_SCREEN \ GAME_SCREEN
	int curLevel;                          //Current level
	int speed;                             //time left for the game.
	int secondsToLevelEnd;                 //speed for each level
	int numOfColors;                       //tell how many colores in game according to the chosen level.
	char timeArray[5];                     //save the remaining time in string for printing to screen
	int playerWon;                         //which player won
	int fireworksIndex;                    //index for the fireworks in the end of the game screen.
}GAME;

//-------------------------------------------------

//global variables

volatile int lastKey=RELEAS_PLAYER1;        //the last key pressed in keyboard.
volatile int sequenceFlagFor6_2=0;
volatile int sequenceFlagFor6_1=0;
volatile unsigned long count0x70 = 0;
int receiver_pid;                           //save the pid of receiver proccess
int is_long_down_press=0;
int colors[6]={0x19,0x2a,0x3b,0x4c,0x5d,0x6e};//array of colors, Lottery of colors by using get_rand_color() function which return the index in this array.
int input_arr[2048];                        //array for the input from the keyboard.
int front = -1;
int rear = -1;                              //like an index for the input_arr array
int isStopped1=1;                           //flag for player1 which tells if the shape stoped by hit the floor or hit other shapes 
int isStopped2=1;                           //flag for player2 which tells if the shape stoped by hit the floor or hit other shapes
int point_in_cycle;							
int gcycle_length;                          
int gno_of_pids;
char game_board[25][80];                    //screan matrix.
int update_screen[25][80];                  //update the screen colors for the players.
char msg[3][8][69];                         //msg to the winner (to print it);
char cur_firework[12][54];                  //array saving for the firework at the end.
GAME game;

PLAYER player1;
PLAYER player2;

//-------------------------------------------------

//keyboard inteurupt
INTPROC new_int9(int mdevno)
{
	int result;
	int temp;
	int scan = 0;
	int ascii = 0;
	int i,j;

	asm {
		MOV AH,1
		INT 16h
		JZ Skip1
		MOV AH,0
		INT 16h
		MOV BYTE PTR scan,AH
		MOV BYTE PTR ascii,AL
	} //asm
	if(scan<=127)
	{	
	if(game.screen==GAME_SCREEN)
	{
		if(scan==75)//a key
		{
			//change_screen(1,3,'1',0x0A);
			result=LEFT_PLAYER2;
		}
		else if(scan==72)//w key
		{
			//change_screen(1,3,'2',0x0A);
			result=UP_PLAYER2;
		}
		else if(scan==77)//d key
		{
			//change_screen(1,3,'3',0x0A);
			result=RIGHT_PLAYER2;
		}
		else if(scan==30)//left key
		{
			//change_screen(1,3,'4',0x0A);
			result=LEFT_PLAYER1;
		}
		else if(scan==32)//right key
		{
			//change_screen(1,3,'5',0x0A);
			result=RIGHT_PLAYER1;
		}
		else if(scan==17)//up key
		{
			//change_screen(1,3,'6',0x0A);
			result=UP_PLAYER1;
		}
		else if(scan==45 && lastKey==RELEAS_PLAYER1) //s key pressed (no release)
		{
			lastKey=DOWN_PLAYER1;
			result=DOWN_PLAYER1;
		}
		else if(scan==173 ) //s key released  סקאן קוד שחרור
		{
			lastKey=RELEAS_PLAYER1;
			result=RELEAS_PLAYER1;
		}
	/*	else if(scan==48)
		{
			
		}
	
		else if(scan==83)
		{
			
		}*/
			
		else if ((scan == 46)&& (ascii == 3)) // Ctrl-C?
			asm INT 27; // terminate xinu
		temp=result;
		send(receiver_pid, temp);
	}
	else if(game.screen==START_SCREEN)
	{
		for(i=19;i<22;i++)
			for(j=5;j<43;j++)
				change_only_color_on_screen(i,j,0x0d);

		if(scan==2)
		{
			game.selectedDifficultyLevel=1;
			change_only_color_on_screen(0,0,0x0C);
			game.numOfColors = 4;
			game.speed=1;
		}
		else if(scan==3)
		{
			game.selectedDifficultyLevel=2;
			change_only_color_on_screen(0,0,0x0C);
			game.numOfColors = 5;
			game.speed=2;
		}
		else if(scan==4)
		{
			game.selectedDifficultyLevel=3;
			game.numOfColors = 6;
			game.speed=3;
		}
			
		else if (scan==28)
		{
			player1.speed = player2.speed = game.speed;
			start_game();
			
		}			
		else if ((scan == 46)&& (ascii == 3)) // Ctrl-C?
			asm INT 27; // terminate xinu
		else if (scan==72)
			i=0;
	}
 
//עוד לא עשיתי פה את החץ למטה (לחיצה קצרה וארוכה) ואת המקש x (לחיצה קצרה וארוכה עבור השחקן השני)
  

Skip1:

	}
} // new_int9


void set_new_int9_newisr()
{
  int i;
  for(i=0; i < 32; i++)
    if (sys_imp[i].ivec == 9)
    {
     sys_imp[i].newisr = new_int9;
     return;
    }

} // set_new_int9_newisr


int findSequence(int player)
{
	int R=19;// Rows 
	int C=6;//columns
	int row,col,k,color,flag,rd,cd,dir,maxSeq=0,seqArr[19][6],i,j;
	// For searching in all 8 direction
	int x[] = { -1, -1, -1, 0, 0, 1, 1, 1 }; 
	int y[] = { -1, 0, 1, -1, 1, -1, 0, 1 };
	//seqArr simulate board array but includes only the colors that have sequence. in places that there is no sequence fill with -1.
	//initialize seqArr with -1:
	for(i=0;i<19;i++)
		for(j=0;j<6;j++)
			seqArr[i][j]=-1;
	if(player==1)//first player
	{
		for(row=0;row<19;row++)
			for(col=0;col<6;col++)//search over all squares in board array
			{
				if(player1.board[row][col]!=0x00)// not black?
				{
					color=player1.board[row][col];//save the color
				
					for(dir=0;dir<8;dir++)// search over all directions
					{
						rd=row+x[dir];//initialize row direction
						cd=col+y[dir];//initialize column direction
						for(k=1;;k++)
						{
							// If out of bound break 
							if(rd >= R || rd < 0 || cd >= C || cd < 0)
								break;
							// If not matched,  break 
							if (player1.board[rd][cd] != color) 
								break; 		
							//  Moving in particular direction 
							if(k==2)//sequence of 3
							{
								seqArr[row][col]=player1.board[row][col];//The current color in place on board
								seqArr[row+x[dir]][col+y[dir]]= player1.board[row+x[dir]][col+y[dir]];
								seqArr[row+2*x[dir]][col+2*y[dir]]= player1.board[row+2*x[dir]][col+2*y[dir]];
							}
							else if(k>2)// 4 or more sequence
								seqArr[rd][cd]=player1.board[rd][cd];//put the colore in seqArr from board array
							rd += x[dir], cd += y[dir];//update in the same diraction
						}//end for
						if(maxSeq<k)
							maxSeq=k;
					}
				}
			}
//		change_screen(2,5,'0'+maxSeq,0x0e);
		for(i=0;i<19;i++)
			for(j=0;j<6;j++)
			{
				if(seqArr[i][j]!= -1)
					deleteFromBoard(player,i,j);
			}
		return maxSeq;
	}
	else if(player==2)//same as player 1
	{
		for(row=0;row<19;row++)
			for(col=0;col<6;col++)
			{
				if(player2.board[row][col]!=0x00)
				{
					color=player2.board[row][col];
				
					for(dir=0;dir<8;dir++)
					{
						rd=row+x[dir];
						cd=col+y[dir];
						for(k=1;;k++)
						{
							// If out of bound break 
							if(rd >= R || rd < 0 || cd >= C || cd < 0)
								break;
							// If not matched,  break 
							if (player2.board[rd][cd] != color) 
								break; 		
							//  Moving in particular direction 
							if(k==2)
							{
								seqArr[row][col]=player2.board[row][col];
								seqArr[row+x[dir]][col+y[dir]]=player2.board[row+x[dir]][col+y[dir]];
								seqArr[row+2*x[dir]][col+2*y[dir]]=player2.board[row+2*x[dir]][col+2*y[dir]];
							}
							else if(k>2)
								seqArr[rd][cd]=player2.board[rd][cd];
							rd += x[dir], cd += y[dir];
						}
						if(maxSeq<k)
							maxSeq=k;
					}
				}
			}
//		change_screen(2,50,'0'+maxSeq,0x0e);
		for(i=0;i<19;i++)
			for(j=0;j<6;j++)
			{
				if(seqArr[i][j]!= -1)
					deleteFromBoard(player,i,j);
			}
			
		return maxSeq;
	}
	
}

void init_fireworks(int state)
{
	int i,j,k;

		for(j=0;j<12;j++)
			for(k=0;k<54;k++)
				cur_firework[j][k]=' ';
			
	
	switch (state)
	{
		case 0:
			strcpy(cur_firework[ 0],"");
			strcpy(cur_firework[ 1],"");
			strcpy(cur_firework[ 2],"");
			strcpy(cur_firework[ 3],"");
			strcpy(cur_firework[ 4],"                       .|");
			strcpy(cur_firework[ 5],"                       | |");
			strcpy(cur_firework[ 6],"                       |'|");
			strcpy(cur_firework[ 7],"               ___    |  |            |.   |' .----|");
			strcpy(cur_firework[ 8],"       _    .-'   '-. |  |     .--'|  ||   | _|    |");
			strcpy(cur_firework[ 9],"    .-'|  _.|  |    ||   '-__  |   |  |    ||      |");
			strcpy(cur_firework[10],"    |' | |.    |    ||       | |   |  |    ||      |");
			strcpy(cur_firework[11],"                                                    ");
			for(i=0;i<12;i++)
				for(j=0;j<54;j++)
					change_screen(i+10,j+10,cur_firework[i][j],0x0A);
			break;
		case 1:
			strcpy(cur_firework[ 0],"");
			strcpy(cur_firework[ 1],"");
			strcpy(cur_firework[ 2],"");
			strcpy(cur_firework[ 3],"");
			strcpy(cur_firework[ 4],"                      .|");
			strcpy(cur_firework[ 5],"                       | |");
			strcpy(cur_firework[ 6],"                       |'|  '");
			strcpy(cur_firework[ 7],"               ___    |  | .          |.   |' .----|");
			strcpy(cur_firework[ 8],"       _    .-'   '-. |  | .   .--'|  ||   | _|    |");
			strcpy(cur_firework[ 9],"    .-'|  _.|  |    ||   '-__  |   |  |    ||      |");
			strcpy(cur_firework[10],"    |' | |.    |    ||       | |   |  |    ||      |");
			strcpy(cur_firework[11],"                                                    ");
			break;
		case 2:
			strcpy(cur_firework[ 0],"");
			strcpy(cur_firework[ 1],"");
			strcpy(cur_firework[ 2],"");
			strcpy(cur_firework[ 3],"");
			strcpy(cur_firework[ 4],"                       .|   _\\/_");
			strcpy(cur_firework[ 5],"                       | |   /\\ ");
			strcpy(cur_firework[ 6],"                       |'|  '         ._____");
			strcpy(cur_firework[ 7],"               ___    |  | .          |.   |' .---\"|");
			strcpy(cur_firework[ 8],"       _    .-'   '-. |  | .   .--'|  ||   | _|    |");
			strcpy(cur_firework[ 9],"    .-'|  _.|  |    ||   '-__  |   |  |    ||      |");
			strcpy(cur_firework[10],"    |' | |.    |    ||       | |   |  |    ||      |");
			strcpy(cur_firework[11],"");
			break;
		case 3:
			strcpy(cur_firework[ 0],"");
			strcpy(cur_firework[ 1],"");
			strcpy(cur_firework[ 2],"");
			strcpy(cur_firework[ 3],"                            *  *");
			strcpy(cur_firework[ 4],"                       .|  *_\\/_*");
			strcpy(cur_firework[ 5],"                       | | * /\\ *");
			strcpy(cur_firework[ 6],"                       |'|  *  *      ._____");
			strcpy(cur_firework[ 7],"               ___    |  |            |.   |' .---\"|");
			strcpy(cur_firework[ 8],"       _    .-'   '-. |  |     .--'|  ||   | _|    |");
			strcpy(cur_firework[ 9],"    .-'|  _.|  |    ||   '-__  |   |  |    ||      |");
			strcpy(cur_firework[10],"    |' | |.    |    ||       | |   |  |    ||      |");
			strcpy(cur_firework[11],"                                                    ");
			break;
		case 4:
			strcpy(cur_firework[ 0],"");
			strcpy(cur_firework[ 1],"");
			strcpy(cur_firework[ 2],"");
			strcpy(cur_firework[ 3],"                            *  *");
			strcpy(cur_firework[ 4],"                       .|  *    * ");
			strcpy(cur_firework[ 5],"                       | | *    * _\\/_");
			strcpy(cur_firework[ 6],"          _\\/_         |'|  *  *   /\\ ._____");
			strcpy(cur_firework[ 7],"           /\\  ___    |  |            |.   |' .---\"|");
			strcpy(cur_firework[ 8],"       _    .-'   '-. |  |     .--'|  ||   | _|    |");
			strcpy(cur_firework[ 9],"    .-'|  _.|  |    ||   '-__  |   |  |    ||      |");
			strcpy(cur_firework[10],"    |' | |.    |    ||       | |   |  |    ||      |");
			strcpy(cur_firework[11],"                                                    ");
			break;
		case 5:
			strcpy(cur_firework[ 0],"");
			strcpy(cur_firework[ 1],"");
			strcpy(cur_firework[ 2],"");
			strcpy(cur_firework[ 3],"                            *  *");
			strcpy(cur_firework[ 4],"               _\\/_    .|  *    * .::.");
			strcpy(cur_firework[ 5],"          .''.  /\\     | | *     :_\\/_:");
			strcpy(cur_firework[ 6],"         :_\\/_:        |'|  *  * : /\\ :_____");
			strcpy(cur_firework[ 7],"         : /\\ :___    |  |   o    '::'|.   |' .---\"|");
			strcpy(cur_firework[ 8],"       _  '..-'   '-. |  |     .--'|  ||   | _|    |");
			strcpy(cur_firework[ 9],"    .-'|  _.|  |    ||   '-__  |   |  |    ||      |");
			strcpy(cur_firework[10],"    |' | |.    |    ||       | |   |  |    ||      |");
			strcpy(cur_firework[11],"                                                    ");
			break;
		case 6:
			strcpy(cur_firework[ 0],"                                                    ");
			strcpy(cur_firework[ 1],"                                                    ");
			strcpy(cur_firework[ 2],"                                                    ");
			strcpy(cur_firework[ 3],"               .''.          *  ");
			strcpy(cur_firework[ 4],"              :_\\/_:   .|         .::.");
			strcpy(cur_firework[ 5],"          .''.: /\\ :   | |       :    :");
			strcpy(cur_firework[ 6],"         :    :'..'    |'|  \\'/  :    :_____");
			strcpy(cur_firework[ 7],"         :    :___    |  | = o =  '::'|.   |' .---\"|");
			strcpy(cur_firework[ 8],"       _  '..-'   '-. |  |  /.\\.--'|  ||   | _|    |");
			strcpy(cur_firework[ 9],"    .-'|  _.|  |    ||   '-__  |   |  |    ||      |");
			strcpy(cur_firework[10],"    |' | |.    |    ||       | |   |  |    ||      |");
			strcpy(cur_firework[11],"                                                    ");
			break;
		case 7:
			strcpy(cur_firework[ 0],"                                                    ");
			strcpy(cur_firework[ 1],"                                                    ");
			strcpy(cur_firework[ 2],"                           _\\)/_");
			strcpy(cur_firework[ 3],"               .''.         /(\\  ");
			strcpy(cur_firework[ 4],"              :    :   .|         _\\/_");
			strcpy(cur_firework[ 5],"          .''.:    :   | |   :     /\\  ");
			strcpy(cur_firework[ 6],"         :    :'..'    |'|'.\\'/.'     ._____");
			strcpy(cur_firework[ 7],"         :    :___    |  |-= o =-     |.   |' .---\"|");
			strcpy(cur_firework[ 8],"       _  '..-'   '-. |  |.'/.\\:--'|  ||   | _|    |");
			strcpy(cur_firework[ 9],"    .-'|  _.|  |    ||   '-__: |   |  |    ||      |");
			strcpy(cur_firework[10],"    |' | |.    |    ||       | |   |  |    ||      |");
			strcpy(cur_firework[11],"                                                    ");
			break;
		case 8:
			strcpy(cur_firework[ 0],"                                                    ");
			strcpy(cur_firework[ 1],"                             .");
			strcpy(cur_firework[ 2],"                           _\\)/_");
			strcpy(cur_firework[ 3],"               .''.         /(\\   .''.");
			strcpy(cur_firework[ 4],"              :    :   .|    '   :_\\/_:");
			strcpy(cur_firework[ 5],"              :    :   | |   :   : /\\ :");
			strcpy(cur_firework[ 6],"               '..'    |'|'. ' .' '..'._____");
			strcpy(cur_firework[ 7],"               ___    |  |-=   =-     |.   |' .---\"|");
			strcpy(cur_firework[ 8],"       _    .-'   '-. |  |.' . :--'|  ||   | _|    |");
			strcpy(cur_firework[ 9],"    .-'|  _.|  |    ||   '-__: |   |  |    ||      |");
			strcpy(cur_firework[10],"    |' | |.    |    ||       | |   |  |    ||      |");
			strcpy(cur_firework[11],"                                                    ");
			break;
		case 9:
			strcpy(cur_firework[ 0],"                                                    ");
			strcpy(cur_firework[ 1],"                                                    ");
			strcpy(cur_firework[ 2],"                                                    ");
			strcpy(cur_firework[ 3],"                _\\/_              .''.");
			strcpy(cur_firework[ 4],"                 /\\    .|        :    :");
			strcpy(cur_firework[ 5],"                       | |       :    :");
			strcpy(cur_firework[ 6],"                       |'|        '..'._____");
			strcpy(cur_firework[ 7],"               ___    |  |            |.   |' .---\"|");
			strcpy(cur_firework[ 8],"       _    .-'   '-. |  |     :--'|  ||   | _|    |");
			strcpy(cur_firework[ 9],"    .-'|  _.|  |    ||   '-__: |   |  |    ||      |");
			strcpy(cur_firework[10],"    |' | |.    |    ||       | |   |  |    ||      |");
			strcpy(cur_firework[11],"                                                    ");
			break;
		case 10:
			strcpy(cur_firework[ 0],"                                                    ");
			strcpy(cur_firework[ 1],"                              \\'/");
			strcpy(cur_firework[ 2],"                *  *         = o =");
			strcpy(cur_firework[ 3],"               *_\\/_*         /.\\ .''.");
			strcpy(cur_firework[ 4],"               * /\\ *  .|        :    :");
			strcpy(cur_firework[ 5],"                *  *   | |       :    :");
			strcpy(cur_firework[ 6],"                       |'|        '..'._____");
			strcpy(cur_firework[ 7],"               ___    |  |            |.   |' .---\"|");
			strcpy(cur_firework[ 8],"       _    .-'   '-. |  |     :--'|  ||   | _|    |");
			strcpy(cur_firework[ 9],"    .-'|  _.|  |    ||   '-__: |   |  |    ||      |");
			strcpy(cur_firework[10],"    |' | |.    |    ||       | |   |  |    ||      |");
			strcpy(cur_firework[11],"                                                    ");
			break;
		case 11:
			strcpy(cur_firework[ 0],"                                                    ");
			strcpy(cur_firework[ 1],"                            '.\\'/.'");
			strcpy(cur_firework[ 2],"                *  *        -= o =-");
			strcpy(cur_firework[ 3],"               *    *       .'/.\\'.");
			strcpy(cur_firework[ 4],"               *    *  .|      :");
			strcpy(cur_firework[ 5],"                *  *   | | ");
			strcpy(cur_firework[ 6],"                       |'|            ._____");
			strcpy(cur_firework[ 7],"               ___    |  |            |.   |' .---\"|");
			strcpy(cur_firework[ 8],"       _    .-'   '-. |  |     :--'|  ||   | _|    |");
			strcpy(cur_firework[ 9],"    .-'|  _.|  |    ||   '-__: |   |  |    ||      |");
			strcpy(cur_firework[10],"    |' | |.    |    ||       | |   |  |    ||      |");
			strcpy(cur_firework[11],"                                                    ");
			break;
		case 12:
			strcpy(cur_firework[ 0],"                                                    ");
			strcpy(cur_firework[ 1],"                            '.\\'/.'");
			strcpy(cur_firework[ 2],"                            -=   =-");
			strcpy(cur_firework[ 3],"                   o        .'/.\\'.");
			strcpy(cur_firework[ 4],"            o          .|      :");
			strcpy(cur_firework[ 5],"                       | |        .:.");
			strcpy(cur_firework[ 6],"                       |'|        ':' ._____");
			strcpy(cur_firework[ 7],"               ___    |  |            |.   |' .---\"|");
			strcpy(cur_firework[ 8],"       _    .-'   '-. |  |     :--'|  ||   | _|    |");
			strcpy(cur_firework[ 9],"    .-'|  _.|  |    ||   '-__: |   |  |    ||      |");
			strcpy(cur_firework[10],"    |' | |.    |    ||       | |   |  |    ||      |");
			strcpy(cur_firework[11],"                                                    ");
			break;
		case 13:
			strcpy(cur_firework[ 0],"                                                    ");
			strcpy(cur_firework[ 1],"                            '. ' .'");
			strcpy(cur_firework[ 2],"                  \\'/       -     -");
			strcpy(cur_firework[ 3],"           \\'/   = o =      .' . '.");
			strcpy(cur_firework[ 4],"          = o =   /.\\  .|      :  .:::.");
			strcpy(cur_firework[ 5],"           /.\\         | |       :::::::");
			strcpy(cur_firework[ 6],"                       |'|        ':::'_____");
			strcpy(cur_firework[ 7],"               ___    |  |            |.   |' .---\"|");
			strcpy(cur_firework[ 8],"       _    .-'   '-. |  |     :--'|  ||   | _|    |");
			strcpy(cur_firework[ 9],"    .-'|  _.|  |    ||   '-__: |   |  |    ||      |");
			strcpy(cur_firework[10],"    |' | |.    |    ||       | |   |  |    ||      |");
			strcpy(cur_firework[11],"                                                    ");
			break;
		case 14:
			strcpy(cur_firework[ 0],"                                                    ");
			strcpy(cur_firework[ 1],"                   :");
			strcpy(cur_firework[ 2],"            :   '.\\'/.'");
			strcpy(cur_firework[ 3],"         '.\\'/.'-= o =-           .:::.");
			strcpy(cur_firework[ 4],"         -= o =-.'/.\\'..|        :::::::");
			strcpy(cur_firework[ 5],"         .'/.\\'.   :   | |       :::::::");
			strcpy(cur_firework[ 6],"            :          |'|        ':::'_____");
			strcpy(cur_firework[ 7],"               ___    |  |            |.   |' .---\"|");
			strcpy(cur_firework[ 8],"       _    .-'   '-. |  |     :--'|  ||   | _|    |");
			strcpy(cur_firework[ 9],"    .-'|  _.|  |    ||   '-__: |   |  |    ||      |");
			strcpy(cur_firework[10],"    |' | |.    |    ||       | |   |  |    ||      |");
			strcpy(cur_firework[11],"                                                    ");
			break;
		case 15:
			strcpy(cur_firework[ 0],"                                                    ");
			strcpy(cur_firework[ 1],"                   :");
			strcpy(cur_firework[ 2],"            :   '.\\'/.'");
			strcpy(cur_firework[ 3],"         '.\\'/.'-=   =-       *   .:::.");
			strcpy(cur_firework[ 4],"         -=   =-.'/.\\'..|        ::' '::");
			strcpy(cur_firework[ 5],"         .'/.\\'.   :   | |       ::. .::");
			strcpy(cur_firework[ 6],"            :          |'|        ':::'_____");
			strcpy(cur_firework[ 7],"               ___    |  |            |.   |' .---\"|");
			strcpy(cur_firework[ 8],"       _    .-'   '-. |  |     :--'|  ||   | _|    |");
			strcpy(cur_firework[ 9],"    .-'|  _.|  |    ||   '-__: |   |  |    ||      |");
			strcpy(cur_firework[10],"    |' | |.    |    ||       | |   |  |    ||      |");
			strcpy(cur_firework[11],"                                                    ");
			break;
		case 16:
			strcpy(cur_firework[ 0],"	                                                  ");
			strcpy(cur_firework[ 1],"                   :          .");
			strcpy(cur_firework[ 2],"            :   '. ' .'     _\\)/_"); 
			strcpy(cur_firework[ 3],"         '. ' .'-     -      /(\\  .'''.");
			strcpy(cur_firework[ 4],"         -     -.' . '..|     '  :     :");
			strcpy(cur_firework[ 5],"         .' . '.   :   | |       :     :");
			strcpy(cur_firework[ 6],"            :          |'|        '...'_____");
			strcpy(cur_firework[ 7],"               ___    |  |            |.   |' .---\"|");
			strcpy(cur_firework[ 8],"       _    .-'   '-. |  |     :--'|  ||   | _|    |");
			strcpy(cur_firework[ 9],"    .-'|  _.|  |    ||   '-__: |   |  |    ||      |");
			strcpy(cur_firework[10],"    |' | |.    |    ||       | |   |  |    ||      |");
			strcpy(cur_firework[11],"                                                    ");
			break;
		case 17:
			strcpy(cur_firework[ 0],"	                                                  ");
			strcpy(cur_firework[ 1],"                              .");
			strcpy(cur_firework[ 2],"                            _\\)/_        _\\/_"); 
			strcpy(cur_firework[ 3],"                             /(\\   _\\/_   /\\");
			strcpy(cur_firework[ 4],"                       .|     '     /\\");
			strcpy(cur_firework[ 5],"                       | | ");
			strcpy(cur_firework[ 6],"                       |'|            ._____");
			strcpy(cur_firework[ 7],"               ___    |  |            |.   |' .---\"|");
			strcpy(cur_firework[ 8],"       _    .-'   '-. |  |     :--'|  ||   | _|    |");
			strcpy(cur_firework[ 9],"    .-'|  _.|  |    ||   '-__: |   |  |    ||      |");
			strcpy(cur_firework[10],"    |' | |.    |    ||       | |   |  |    ||      |");
			strcpy(cur_firework[11],"                                                    ");
			break;
		case 18:
			strcpy(cur_firework[ 0],"	                                                  ");
			strcpy(cur_firework[ 1],"                              .          .''.");
			strcpy(cur_firework[ 2],"                            _\\)/_  .''. :_\\/_:");
			strcpy(cur_firework[ 3],"                             /(\\  :_\\/_:: /\\ :");
			strcpy(cur_firework[ 4],"                       .|     '   : /\\ : '..'");
			strcpy(cur_firework[ 5],"               o       | |         '..'");
			strcpy(cur_firework[ 6],"                       |'|            ._____");
			strcpy(cur_firework[ 7],"               ___    |  |            |.   |' .---\"|");
			strcpy(cur_firework[ 8],"       _    .-'   '-. |  |     :--'|  ||   | _|    |");
			strcpy(cur_firework[ 9],"    .-'|  _.|  |    ||   '-__: |   |  |    ||      |");
			strcpy(cur_firework[10],"    |' | |.    |    ||       | |   |  |    ||      |");
			strcpy(cur_firework[11],"                                                    ");
			break;
		case 19:
			strcpy(cur_firework[ 0],"	                                                   ");
			strcpy(cur_firework[ 1],"                                         .''.");
			strcpy(cur_firework[ 2],"                                   .''. :    :");
			strcpy(cur_firework[ 3],"                           _\\/_   :    ::    :");
			strcpy(cur_firework[ 4],"              \\'/      .|   /\\    :    : '..'");
			strcpy(cur_firework[ 5],"             = o =     | |    _\\/_ '..'");
			strcpy(cur_firework[ 6],"              /.\\      |'|     /\\     ._____");
			strcpy(cur_firework[ 7],"               ___    |  |            |.   |' .---\"|");
			strcpy(cur_firework[ 8],"       _    .-'   '-. |  |     :--'|  ||   | _|    |");
			strcpy(cur_firework[ 9],"    .-'|  _.|  |    ||   '-__: |   |  |    ||      |");
			strcpy(cur_firework[10],"    |' | |.    |    ||       | |   |  |    ||      |");
			strcpy(cur_firework[11],"                                                    ");
			break;
		case 20:
			strcpy(cur_firework[ 0],"");
			strcpy(cur_firework[ 1],"");
			strcpy(cur_firework[ 2],"                           .''.");
			strcpy(cur_firework[ 3],"               :          :_\\/_:");
			strcpy(cur_firework[ 4],"            '.\\'/.'    .| : /\\.:'.");
			strcpy(cur_firework[ 5],"            -= o =-    | | '.:_\\/_:");
			strcpy(cur_firework[ 6],"            .'/.\\'.    |'|   : /\\ :   ._____");
			strcpy(cur_firework[ 7],"               :__    |  |    '..'    |.   |' .---\"|");
			strcpy(cur_firework[ 8],"       _    .-'   '-. |  |     :--'|  ||   | _|    |");
			strcpy(cur_firework[ 9],"    .-'|  _.|  |    ||   '-__: |   |  |    ||      |");
			strcpy(cur_firework[10],"    |' | |.    |    ||       | |   |  |    ||      |");
			strcpy(cur_firework[11],"");
			break;
		case 21:
			strcpy(cur_firework[ 0],"	                                                  ");
			strcpy(cur_firework[ 1],"	                                                  ");
			strcpy(cur_firework[ 2],"                           .''. ");
			strcpy(cur_firework[ 3],"               :          :    :");
			strcpy(cur_firework[ 4],"            '.\\'/.'    .| :   .:'.");
			strcpy(cur_firework[ 5],"            -=   =-    | | '.:    :");
			strcpy(cur_firework[ 6],"            .'/.\\'.    |'|   :    :   ._____");
			strcpy(cur_firework[ 7],"               :__    |  |    '..'    |.   |' .---\"");
			strcpy(cur_firework[ 8],"       _    .-'   '-. |  |     :--'|  ||   | _|    |");
			strcpy(cur_firework[ 9],"    .-'|  _.|  |    ||   '-__: |   |  |    ||      |");
			strcpy(cur_firework[10],"    |' | |.    |    ||       | |   |  |    ||      |");
			strcpy(cur_firework[11],"                                                    ");
			break;
		case 22:
			strcpy(cur_firework[ 0],"	                                                  ");
			strcpy(cur_firework[ 1],"	                                                  ");
			strcpy(cur_firework[ 2],"                           .''. ");
			strcpy(cur_firework[ 3],"               :          :    :");
			strcpy(cur_firework[ 4],"            '. ' .'    .| :   .:'.");
			strcpy(cur_firework[ 5],"            -     -    | | '.:    :");
			strcpy(cur_firework[ 6],"            .' . '.    |'|   :    :   ._____");
			strcpy(cur_firework[ 7],"               :__    |  |    '..'    |.   |' .---\"|");
			strcpy(cur_firework[ 8],"       _    .-'   '-. |  |     :--'|  ||   | _|    |");
			strcpy(cur_firework[ 9],"    .-'|  _.|  |    ||   '-__: |   |  |    ||      |");
			strcpy(cur_firework[10],"    |' | |.    |    ||       | |   |  |    ||      |");
			strcpy(cur_firework[11],"                                                    ");
			break;
			
	}	
	for(i=0;i<12;i++)
				for(j=0;j<54;j++)
					change_screen(i+10,j+10,cur_firework[i][j],0x0A);
}



void generate_ending_screen()
{
	int i,j;
	
	game.screen=END_SCREEN;//change the flage to the end screen.
	//reset update_screen array
	for(i=0;i<25;i++)
		for(j=0;j<80;j++)
			update_screen[i][j]=-1;
	for(i=0;i<12;i++)
				for(j=0;j<54;j++)
				change_screen(i+10,j+10,' ',0x0A);
	
			
	init_fireworks(game.fireworksIndex%23);
	game.fireworksIndex++;
	
	for(i=0;i<8;i++)
		for(j=0;j<68;j++)
			change_screen(i+1,j+8,msg[game.playerWon-1][i][j],0x03);	
}


void set_Int0x70Handler()
{/* setting interupt 0x70 to act periodically*/
	asm{
		CLI
		PUSH AX
		IN AL,0A1h
		AND AL,0FEh
		OUT 0A1h,AL
		IN AL,70h
		MOV AL,0Ah
		OUT 70h,AL
		MOV AL,8Ah
		OUT 70h,AL
		IN AL,71h
		AND AL,10000000b
		OR AL,01010001b //248 interrupts per second.
		OUT 71h,AL
		IN AL,71h
		IN AL,70h
		MOV AL,0Bh
		OUT 70h,AL
		MOV AL,8Bh
		OUT 70h,AL
		IN AL,71h
		OR AL,40h
		OUT 71h,AL
		IN AL,71h
		IN AL, 021h
		AND AL, 0FBh
		OUT 021h, AL
		IN AL, 70h
		MOV AL, 0Ch
		OUT 70h, AL
		IN AL, 70h
		MOV AL, 8Ch
		OUT 70h, AL
		IN AL, 71h
		IN AL, 70h
		MOV AL, 0Dh
		OUT 70h, AL
		IN AL, 70h
		MOV AL, 8Dh
		OUT 70h, AL
		IN AL, 71h
		POP AX
		STI
	}
	
}
int Int0x70Handler(int mdevno)
{ 
	int i,j;
	int maxSequence1=0;
	int maxSequence2=0;
	asm{
		/* basic settings & protocols of the original 0x70 inerrupt. */
		PUSH AX
		PUSH BX
		IN AL,70h
		MOV BX,AX
		MOV AL,0Ch
		OUT 70h,AL
		MOV AL,8Ch
		OUT 70h,AL
		IN AL,71h
		MOV AX,BX
		OUT 70h,AL
		MOV AL,20h
		OUT 0A0h,AL
		OUT 020h,AL
		POP BX
		POP AX
	}
	/* additions for the game. */
	if (count0x70 < 4294967295) // makes sure that the counter is within range.
	{	
		count0x70++;
	}
	else
		count0x70 = 0; // before the counter is about to reach unsigned long limits (max unsinged long value), resets it (to 0).
					   //move the tron from top to lower floor
					   
	if(count0x70 % latch_0x70 == 0 && game.screen == GAME_SCREEN)
	{
		game.secondsToLevelEnd--;
		displayTime();
		if(game.secondsToLevelEnd==0)
		{
			game.curLevel++;
			if(game.curLevel>3)//only 3 levels in game. show end screen.
			{
				game.screen=END_SCREEN;
				if(player1.score> player2.score)
					game.playerWon=1;
				else if(player1.score<player2.score)
					game.playerWon=2;
				else game.playerWon=3;//tie
				for(i=0;i<25;i++)
					for(j=0;j<80;j++)
						change_screen(i,j,' ',0x00);//black screen
				generate_ending_screen();//show end screen
			}
			else
			{
				if(game.curLevel==2)//level 2
				{
					game.secondsToLevelEnd=LEVEL2SECONDS;
					player1.speed=player2.speed=++game.speed;
				}
				else if(game.curLevel==3)//level 3
				{
					game.secondsToLevelEnd=LEVEL3SECONDS;
					player1.speed=player2.speed=++game.speed;
				}
				clearBoard();//clear board of the screan and of board[][] array (for the players).
						//initialize the players shape to top:
				player1.curPiece_x=13;
				player1.curPiece_y=4;
				
				player1.placeOnBoard_x=2;
				player1.placeOnBoard_y=0;
				
				player2.curPiece_x=61;
				player2.curPiece_y=4;
					
				player2.placeOnBoard_x=2;
				player2.placeOnBoard_y=0;
			}
		}
			
	}
	
	if (count0x70 % (latch_0x70/player1.speed)== 0 && game.screen == GAME_SCREEN) //(int) (100 / player1.speed)
	{
		if(isStopped1==1)//if the shape stoped moving?
		{
			readyNextColors(1);
			fill_next_colors(1,player1.nextPiece);
			//update location on screen:
			player1.curPiece_x=13;
			player1.curPiece_y=4;
			
			display_next(player1.curPiece_x,player1.curPiece_y,player1.curPiece,'v');
			isStopped1=0;
			//update location on Board array:
			player1.placeOnBoard_x=2;
			player1.placeOnBoard_y=0;
			sequenceFlagFor6_1=0;
		}
		if(player1.curPiece_y<20 && player1.board[player1.placeOnBoard_y+3][player1.placeOnBoard_x]==0x00)
		{
			delete_previou_location(player1.curPiece_x,player1.curPiece_y,'v');//delete previous location
			if(count0x70 % 124 ==0 )//if there is sequence of 6 , changh colors every half the time
				{
					//sequenceFlagFor6_1=1;
					if( sequenceFlagFor6_1==1)
					{
					player1.curPiece[0]= colors[get_rand_color(game.numOfColors)];
					player1.curPiece[1]= colors[get_rand_color(game.numOfColors)];
					player1.curPiece[2]= colors[get_rand_color(game.numOfColors)];
					}
				}
			player1.curPiece_y++;//update the new y place.
			display_next(player1.curPiece_x,player1.curPiece_y,player1.curPiece,'v');
			player1.placeOnBoard_y++; //update the new y place in the board array.
			findSequence(1);
		}
		else // not posible to down the shape?
		{
			isStopped1=1;
			//update the board of  player1 with the shape where it stoped.
			player1.board[player1.placeOnBoard_y][player1.placeOnBoard_x]=player1.curPiece[0]; //**********************
			player1.board[player1.placeOnBoard_y+1][player1.placeOnBoard_x]=player1.curPiece[1]; //**********************
			player1.board[player1.placeOnBoard_y+2][player1.placeOnBoard_x]=player1.curPiece[2]; //**********************

			maxSequence1= findSequence(1);//search for Sequence in vertical / horizontal/ diagonal1 /diagonal2 and return the biggest Sequence.
				
			if(maxSequence1==4)
				duplicateEnamy(2);//if the biggest Sequence was 4 then duplicate the Enamy last line.
			else if(maxSequence1==6)
				sequenceFlagFor6_2=1;
			else if(maxSequence1==5)
			{
				player1.speed--;
				player2.speed++;
			}
						
			
				
			if(player1.board[player1.placeOnBoard_y+3][player1.placeOnBoard_x]!=0x00  
					&& player1.curPiece_x>6 && player1.curPiece_x<22 && player1.curPiece_y<=5) //full shapes on screen
			{

				clearBoard();//clear board of the screan and of board[][] array (for the players).
				player2.score++;//update score to the winner
				change_screen(7,42,player2.score+'0',0xEE);//print score2 to screen
				change_screen(7,38,player1.score+'0',0x0E);//print score1 to screen
				game.curLevel++;//update current level
				if(game.curLevel>3)//end of the game (only 3 levels)
				{
					game.screen=END_SCREEN;//flage to END_SCREEN
					if(player1.score> player2.score)//player1 won
						game.playerWon=1;
					else if(player1.score<player2.score)//player2 won
						game.playerWon=2;
					else game.playerWon=3;//tie
					game.screen=END_SCREEN;
					//initialize screen to black:
					for(i=0;i<25;i++)
						for(j=0;j<80;j++)
							change_screen(i,j,' ',0x00);
					generate_ending_screen();//call func to show ending_screen.
				}

				else if(game.curLevel==2)//Level2
				{
					game.secondsToLevelEnd=LEVEL2SECONDS;
					player1.speed=player2.speed=++game.speed;
				}
				else if(game.curLevel==3)//Level3
				{
					game.secondsToLevelEnd=LEVEL3SECONDS;
					player1.speed=player2.speed=++game.speed;
				}
				
				player2.curPiece_x=61;
				player2.curPiece_y=4;
				
				player2.placeOnBoard_x=2;
				player2.placeOnBoard_y=0;
			}
		}
	}//end if (count0x70 % (latch_0x70/player1.speed)== 0 && game.screen == GAME_SCREEN)
	if (count0x70 % (latch_0x70/player2.speed)== 0 && game.screen == GAME_SCREEN) //(int) (1024 / player1.speed)
	{
	
		if(isStopped2==1)//if the shape stoped moving?
		{
			readyNextColors(2);
			fill_next_colors(2,player2.nextPiece);
			//update location on screen:
			player2.curPiece_x=61;
			player2.curPiece_y=4;
			
			display_next(player2.curPiece_x,player2.curPiece_y,player2.curPiece,'v');
			isStopped2=0;
			//update location on Board array:
			player2.placeOnBoard_x=2;
			player2.placeOnBoard_y=0;
			sequenceFlagFor6_2=0;
		}
		if(player2.curPiece_y<20 && player2.board[player2.placeOnBoard_y+3][player2.placeOnBoard_x]==0x00)
		{
			delete_previou_location(player2.curPiece_x,player2.curPiece_y,'v');
			if(count0x70 % 124 ==0 )
			{
				if( sequenceFlagFor6_2==1)
				{
					player2.curPiece[0]= colors[get_rand_color(game.numOfColors)];
					player2.curPiece[1]= colors[get_rand_color(game.numOfColors)];
					player2.curPiece[2]= colors[get_rand_color(game.numOfColors)];
				}
			}
			player2.curPiece_y++;//update the new y place.
			display_next(player2.curPiece_x,player2.curPiece_y,player2.curPiece,'v');
			player2.placeOnBoard_y++; //update the new y place in the board array.
			findSequence(2);
		}
		else// not posible to down the shape?
		{
			isStopped2=1;
			//update the board of  player2 with the shape where it stoped.
			player2.board[player2.placeOnBoard_y][player2.placeOnBoard_x]=player2.curPiece[0]; //**********************
			player2.board[player2.placeOnBoard_y+1][player2.placeOnBoard_x]=player2.curPiece[1]; //**********************
			player2.board[player2.placeOnBoard_y+2][player2.placeOnBoard_x]=player2.curPiece[2]; //**********************
			
		
			maxSequence2= findSequence(2);//search for Sequence in vertical / horizontal/ diagonal1 /diagonal2 and return the biggest Sequence.
			if(maxSequence2==4)
				duplicateEnamy(1);//if the biggest Sequence was 4 then duplicate the Enamy last line.
			else if(maxSequence2==6)
				sequenceFlagFor6_1=1;
			else if(maxSequence2==5)
			{
				player2.speed--;
				player1.speed++;
			}

			if(player2.board[player2.placeOnBoard_y+3][player2.placeOnBoard_x]!=0x00 
					&& player2.curPiece_x>54 && player2.curPiece_x<70 && player2.curPiece_y<=5)//screen is full of shapes?
			{

				clearBoard();//clear board of the screan and of board[][] array (for the players).
				player1.score++;//update score to the winner
				change_screen(7,42,player2.score+'0',0xEE);
				change_screen(7,38,player1.score+'0',0x0E);
				game.curLevel++;
				if(game.curLevel>3)
				{
					game.screen=END_SCREEN;
					if(player1.score> player2.score)
						game.playerWon=1;
					else if(player1.score<player2.score)
						game.playerWon=2;
					else game.playerWon=3;
					for(i=0;i<25;i++)
						for(j=0;j<80;j++)
							change_screen(i,j,' ',0x00);
					generate_ending_screen();
				}

				else if(game.curLevel==2)
				{
					game.secondsToLevelEnd=LEVEL2SECONDS;
					player1.speed=player2.speed= ++game.speed;
				}
				else if(game.curLevel==3)
				{
					game.secondsToLevelEnd=LEVEL3SECONDS;
					player1.speed=player2.speed=++game.speed;
				}
				
				player1.curPiece_x=13;
				player1.curPiece_y=4;
				
				player1.placeOnBoard_x=2;
				player1.placeOnBoard_y=0;
			}
		}
	}
}

void deleteFromBoard(int player, int row, int col)
{
	int i,j;
	int k;
	if(player==1) //player1
	{
		//changes on screen:
		for(k=0;(row-k-1)>=0 ;k++)
		{
			for(j=0;j<3;j++)
			{
				change_screen(row+4-k,col*3+j+7, 176,player1.board[row-k-1][col]);

			}

		}
		//changes on board array
		for(i=row-1;i>=0;i--)
		{
			player1.board[i+1][col]=player1.board[i][col];
		}
		

	}
	else if(player==2)//player2
	{
		//changes on screen:
		for(k=0;(row-k-1)>=0 ;k++)
		{
			for(j=0;j<3;j++)
			{
				change_screen(row+4-k,col*3+j+55, 176,player2.board[row-k-1][col]);

			}

		}
		//changes on board array
		for(i=row-1;i>=0;i--)
		{
			player2.board[i+1][col]=player2.board[i][col];
		}
	}
}



//-------------------------------------------------

//initialize the game - welcome to columns screan.
void initializer()
{
	int i,j;
	game.speed=1;
	game.numOfColors=4;
	game.screen=START_SCREEN;
	game.curLevel=0;
	//initialize update_screen array with no colors in it.
	for(i=0; i<25; i++){
		for(j=0; j<80; j++){
			update_screen[i][j]=-1;
		}
	}
	//initialize board array with black colors
	for(i=0;i<19;i++){
		for(j=0;j<6;j++){
			player1.board[i][j]=0x00;
			player2.board[i][j]=0x00;
		}
	}		
	//initialize for level1 4 miniuts life time game.
	strcpy(game.timeArray,"4:00");

	player1.score=0;
	player2.score=0;
	player1.speed=1;
	player2.speed=1;
	
	
}
//start game board
void start_game()
{
//	change_screen(0,0,'%',0xEE);
	game.screen=GAME_SCREEN;//change the flage to the game screen.
	game.curLevel=1;
	game.secondsToLevelEnd=LEVEL1SECONDS;
	initialGameBoard();
}
//Gives color to a square on the screen by column and row
void change_only_color_on_screen(int row,int col,int color)
{
	
	int place=(row*80+col)*2;

	asm{
		
		MOV              AX,0B800h     
		MOV              ES,AX         
		MOV              DI,place
		MOV              BX,color
		MOV              AH,BL
		INC              DI
		MOV              BYTE PTR ES:[DI],AH
	}
}





/*------------------------------------------------------------------------
 *  prntr  --  print a character indefinitely
 *------------------------------------------------------------------------
 */

void displayTime( )
 {
	char minute, seconds[2];
	int sec, i;
	
	game.timeArray[0] = (game.secondsToLevelEnd /60)+'0';
	sec = game.secondsToLevelEnd % 60;
	
	seconds[1]= sec%10 + '0';
	sec= sec/10;
	seconds[0]= sec + '0';
	 
	game.timeArray[1] = ':';
	game.timeArray[2] = seconds[0];
	game.timeArray[3] = seconds[1];
	
	for(i=0;i<4;i++)
		change_screen(3,38+i,game.timeArray[i],0x0E);
 }

//************************************
/**void changeColors(int num)
{
	int temp;
	if(num==UP_PLAYER1)
	{
		temp= player1.curPiece[2];
		player1.curPiece[2]=player1.curPiece[1];
		player1.curPiece[1]=player1.curPiece[0];
		player1.curPiece[0]=temp;
	}
	else if(num==UP_PLAYER2)
	{
		temp= player2.curPiece[2];
		player2.curPiece[2]=player2.curPiece[1];
		player2.curPiece[1]=player2.curPiece[0];
		player2.curPiece[0]=temp;
	}
	
}**/
//************************************

void displayer( void )
{
	int i,j;
	while (1)
    {
               receive();
			  for(i=0;i<25;i++)
				  for(j=0;j<80;j++)
				  {
					  if(update_screen[i][j]!=-1)
					  {
						  if(update_screen[i][j]==0x00)
							  change_screen(i,j,' ',0x00);
						  else change_screen(i,j,176,update_screen[i][j]);
					  }
					  update_screen[i][j]=-1;
				  }
				  
              
    } //while
} // prntr

void receiver()
{
  while(1)
  {
	  
    int temp;
    temp = receive();
	rear++;
	input_arr[rear] = temp;
    if (front == -1)
       front = 0;
//    change_screen(3,20,'0'+rear,0x0e);
  } // while

} //  receiver

void change_screen(int row,int col, char c,int color)
//משנה את כרטיס המסך במיקום הנתון ושם את תו c ואטריביוט color
{
	
	int place=(row*80+col)*2;

	asm{
		
		MOV              AX,0B800h     
		MOV              ES,AX         
		MOV              DI,place
		MOV              AL, c
		MOV              BX,color
		MOV              AH,BL
		MOV              WORD PTR ES:[DI],AX
	}
}

int get_rand_color(int numOfColors)//מגריל מספר רנדומלי בין 0-5 
{
	
	return rand() % (numOfColors);
	
}
	
void display_next(int positionX,int positionY,int nextColors[],char orientation)//מציג את החלק שיורד במיקום הנתון ועם הצבעים הנתונים, בכיוון orientation
{
	if(orientation=='v')
	{
		update_screen[positionY][positionX]=nextColors[0];
		update_screen[positionY][positionX+1]=nextColors[0];
		update_screen[positionY][positionX+2]=nextColors[0];
		
		update_screen[positionY+1][positionX]=nextColors[1];
		update_screen[positionY+1][positionX+1]=nextColors[1];
		update_screen[positionY+1][positionX+2]=nextColors[1];
		
		update_screen[positionY+2][positionX]=nextColors[2];
		update_screen[positionY+2][positionX+1]=nextColors[2];
		update_screen[positionY+2][positionX+2]=nextColors[2];
		
	}
	if(orientation=='h')
	{
		update_screen[positionY][positionX]=nextColors[0];
		update_screen[positionY][positionX+1]=nextColors[0];
		update_screen[positionY][positionX+2]=nextColors[0];
		
		update_screen[positionY][positionX+4]=nextColors[1];
		update_screen[positionY][positionX+5]=nextColors[1];
		update_screen[positionY][positionX+6]=nextColors[1];
		
		update_screen[positionY][positionX+8]=nextColors[2];
		update_screen[positionY][positionX+9]=nextColors[2];
		update_screen[positionY][positionX+10]=nextColors[2];
	}
}

void delete_previou_location(int positionX,int positionY, char orientation)//מוחק את החלק שירד מהלוח כדי שנוכל להדפיס אותו מחדש במקום החדשץ מקבל מיקום של הנקודה בה הוא מתחיל וכיוון (אופקי או אנכי)
{
	if(orientation=='v')
	{
		update_screen[positionY][positionX]=0x00;
		update_screen[positionY][positionX+1]=0x00;
		update_screen[positionY][positionX+2]=0x00;
		
		update_screen[positionY+1][positionX]=0x00;
		update_screen[positionY+1][positionX+1]=0x00;
		update_screen[positionY+1][positionX+2]=0x00;
		
		update_screen[positionY+2][positionX]=0x00;
		update_screen[positionY+2][positionX+1]=0x00;
		update_screen[positionY+2][positionX+2]=0x00;

	}
	if(orientation=='h')
	{
		update_screen[positionY][positionX]=0x00;
		update_screen[positionY][positionX+1]=0x00;
		update_screen[positionY][positionX+2]=0x00;
		
		update_screen[positionY][positionX+4]=0x00;
		update_screen[positionY][positionX+5]=0x00;
		update_screen[positionY][positionX+6]=0x00;
		
		update_screen[positionY][positionX+8]=0x00;
		update_screen[positionY][positionX+9]=0x00;
		update_screen[positionY][positionX+10]=0x00;
		
	}
}

void fill_next_colors(int player,int nextColors[])//עדכון המקום בצד שמציג את הצבעים הבאים שצריכים לרדתת מקבל מספר שחקן (1 או 2) ומערך המכיל את 3 הצבעים הבאים
{
	int i;
	if(player==1)
	{
		for(i=0;i<3;i++)
		{
			change_screen(12+i,33,176,nextColors[i]);
			change_screen(12+i,34,176,nextColors[i]);
			change_screen(12+i,35,176,nextColors[i]);
		}
	}
	if(player==2)
	{
		for(i=0;i<3;i++)
		{
			change_screen(12+i,44,176,nextColors[i]);
			change_screen(12+i,45,176,nextColors[i]);
			change_screen(12+i,46,176,nextColors[i]);
		}
	}
}	

void clearScreen()
{
	asm{
		MOV              AH,0      
		MOV              AL,3          
		INT              10h           

		MOV              AX,0B800h     

		MOV              ES,AX         
		MOV              DI,0          
		MOV              AL,' '        
		MOV              AH,0Eh        
		MOV              CX,1000       
		CLD                            
		REP              STOSW         

	}
}

void initialGameBoard()
{
	 int i,j;
  time_t t;
		//אתחול המערך המכיל את הטיוטה עבור הדפסה לכרטיס המסך של הלוח הבסיסי של המשחק
	for(i=0;i<25;i++)
		for(j=0;j<80;j++)
		{
			game_board[i][j]=' ';
			change_screen(i,j,' ',0x00);
		}
	
	
	//מפה מתחילה ההדפסה הראשונית של כל הקווים למסך
	for(i=4;i<24;i++)
	{
		game_board[i][5]='*';
		game_board[i][6]='*';
		game_board[i][25]='*';
		game_board[i][26]='*';
		game_board[i][53]='*';
		game_board[i][54]='*';
		game_board[i][73]='*';
		game_board[i][74]='*';
	}
	
	for(i=5;i<27;i++)
	{
		game_board[23][i]='*';
		
	}
	
	
	for(i=53;i<75;i++)
	{
		game_board[23][i]='*';
	}
	
	for(i=0;i<10;i++)
	{
		game_board[i][32]='*';
		game_board[i][31]='*';
		game_board[i][32]='*';
		game_board[i][48]='*';
		game_board[i][47]='*';
	}
	
	for(i=10;i<25;i++)
	{
		game_board[i][39]='*';
		game_board[i][40]='*';
	}
	
	for(i=32;i<48;i++)
	{
		game_board[9][i]='*';
		
	}
	
	for(i=12;i<15;i++)
	{
		game_board[i][32]='*';
		game_board[i][36]='*';
		game_board[i][43]='*';
		game_board[i][47]='*';
	}
	
	for(i=32;i<37;i++)
	{
		game_board[11][i]='*';
		game_board[15][i]='*';
	}
	
	for(i=43;i<48;i++)
	{
		game_board[11][i]='*';
		game_board[15][i]='*';
	}
	
	game_board[1][35]='T';
	game_board[1][36]='i';
	game_board[1][37]='m';
	game_board[1][38]='e';
	game_board[1][40]='l';
	game_board[1][41]='e';
	game_board[1][42]='f';
	game_board[1][43]='t';
	game_board[1][44]=':';
	
	game_board[1][10]='P';
	game_board[1][11]='L';
	game_board[1][12]='A';
	game_board[1][13]='Y';
	game_board[1][14]='E';
	game_board[1][15]='R';
	game_board[1][17]='1';
	
	game_board[1][58]='P';
	game_board[1][59]='L';
	game_board[1][60]='A';
	game_board[1][61]='Y';
	game_board[1][62]='E';
	game_board[1][63]='R';
	game_board[1][65]='2';
	
	game_board[5][38]='W';
	game_board[5][39]='i';
	game_board[5][40]='n';
	game_board[5][41]='s';
	game_board[5][42]=':';
	
	game_board[7][38]='0';
	game_board[7][39]=' ';
	game_board[7][40]=':';
	game_board[7][41]=' ';
	game_board[7][42]='0';
	
	
	game_board[21][30]='Q';
	game_board[21][31]='-';
	game_board[21][32]='Q';
	game_board[21][33]='u';
	game_board[21][34]='i';
	game_board[21][35]='t';
	
	
	game_board[21][43]='D';
	game_board[21][44]='e';
	game_board[21][45]='l';
	game_board[21][46]='-';
	game_board[21][47]='Q';
	game_board[21][48]='u';
	game_board[21][49]='i';
	game_board[21][50]='t';
	
		srand((unsigned) time(&t));
	
	for(i=0;i<25;i++)
		for(j=0;j<80;j++)
			if(game_board[i][j]=='*')
				change_screen(i,j,' ',0x0FF);
			else if((game_board[i][j]>='a' && game_board[i][j]<='z')||game_board[i][j]==':'||(game_board[i][j]>='A' && game_board[i][j]<='Z')||(game_board[i][j]>='0' && game_board[i][j]<='9')||game_board[i][j]=='-')
				change_screen(i,j,game_board[i][j],0x0E);

	player1.curPiece[0]=colors[get_rand_color(game.numOfColors)];
    player1.curPiece[1]=colors[get_rand_color(game.numOfColors)];
    player1.curPiece[2]=colors[get_rand_color(game.numOfColors)];
    player2.curPiece[0]=colors[get_rand_color(game.numOfColors)];
    player2.curPiece[1]=colors[get_rand_color(game.numOfColors)];
    player2.curPiece[2]=colors[get_rand_color(game.numOfColors)];
	
	player1.nextPiece[0]=colors[get_rand_color(game.numOfColors)];
    player1.nextPiece[1]=colors[get_rand_color(game.numOfColors)];
    player1.nextPiece[2]=colors[get_rand_color(game.numOfColors)];
    player2.nextPiece[0]=colors[get_rand_color(game.numOfColors)];
    player2.nextPiece[1]=colors[get_rand_color(game.numOfColors)];
    player2.nextPiece[2]=colors[get_rand_color(game.numOfColors)];
	
	fill_next_colors(1,player1.nextPiece);
	fill_next_colors(2,player2.nextPiece);
	
	player1.placeOnBoard_x=2;
	player1.placeOnBoard_y=0;
	
	player2.placeOnBoard_x=2;
	player2.placeOnBoard_y=0;
}

void generate_welcome_screen()
{
	int i,j,k=0;
	char welcomeMsg[320];
	char colomnsMsg[538];
	char difficultyMsg[34];
	char line[34];
	char difficultyMsgL1[37];
	char difficultyMsgL2[39];
	char difficultyMsgL3[37];
	char startMsg[21];
	strcpy(colomnsMsg," _______    _______    ___        __   __    __   __    __    _    _______ |       |  |       |  |   |      |  | |  |  |  |_|  |  |  |  | |  |       ||       |  |   _   |  |   |      |  | |  |  |       |  |   |_| |  |  _____||       |  |  | |  |  |   |      |  |_|  |  |       |  |       |  | |_____ |      _|  |  |_|  |  |   |___   |       |  |       |  |  _    |  |_____  ||     |_   |       |  |       |  |       |  | ||_|| |  | | |   |   _____| ||_______|  |_______|  |_______|  |_______|  |_|   |_|  |_|  |__|  |_______|");
	strcpy(welcomeMsg,"               _                            _                      | |                          | |        __      _____| | ___ ___  _ __ ___   ___  | |_ ___   \\ \\ /\\ / / _ \\ |/ __/ _ \\| '_ ` _ \\ / _ \\ | __/ _ \\   \\ V  V /  __/ | (_| (_) | | | | | |  __/ | || (_) |   \\_/\\_/ \\___|_|\\___\\___/|_| |_| |_|\\___|  \\__\\___/  ");
	strcpy(difficultyMsg,"Please select a difficulty level:");
	strcpy(line,"=================================");
	strcpy(difficultyMsgL1,"For easy level with 4 colors press 1");
	strcpy(difficultyMsgL2,"For medium level with 5 colors press 2");
	strcpy(difficultyMsgL3," For hard level with 6 colors press 3");
	strcpy(startMsg,"Press Enter to start ");
	
	for(i=0;i<25;i++)
		for(j=0;j<80;j++)
			game_board[i][j]=' ';
	
	for(i=0;i<7;i++)
		for(j=0;j<75;j++)
		{
			game_board[7+i][3+j]=colomnsMsg[k++];
			change_screen(7+i,3+j,game_board[7+i][3+j],0x0E);
		}
	k=0;
	for(i=0;i<6;i++)
		for(j=0;j<53;j++)
		{
			game_board[1+i][3+j]=welcomeMsg[k++];
			change_screen(1+i,3+j,game_board[1+i][3+j],0x0A);
		}

	for(i=0;i<33;i++)
	{
		game_board[17][5+i]=difficultyMsg[i];
		change_screen(17,5+i,game_board[17][5+i],0x0C);
	}	

	for(i=0;i<33;i++)
	{
		game_board[18][5+i]=line[i];
		change_screen(18,5+i,game_board[18][5+i],0x0C);
	}

	for(i=0;i<36;i++)
	{
		game_board[19][5+i]=difficultyMsgL1[i];
		change_screen(19,5+i,game_board[19][5+i],0x0D);
	}	

	for(i=0;i<38;i++)
	{
		game_board[20][5+i]=difficultyMsgL2[i];
		change_screen(20,5+i,game_board[20][5+i],0x0D);
	}

	for(i=0;i<37;i++)
	{
		game_board[21][4+i]=difficultyMsgL3[i];
		change_screen(21,4+i,game_board[21][4+i],0x0D);
	}

	for(i=0;i<21;i++)
	{
		game_board[20][50+i]=startMsg[i];
		change_screen(20,50+i,game_board[20][50+i],0x0C);
	}
}

void readyNextColors(int player)
{
	if(player==1)
	{
		player1.curPiece[0]= player1.nextPiece[0];
		player1.curPiece[1]= player1.nextPiece[1];
		player1.curPiece[2]= player1.nextPiece[2];
		
		player1.nextPiece[0]= colors[get_rand_color(game.numOfColors)];
		player1.nextPiece[1]= colors[get_rand_color(game.numOfColors)];
		player1.nextPiece[2]= colors[get_rand_color(game.numOfColors)];
	}

	else if(player==2)
	{
		player2.curPiece[0]= player2.nextPiece[0];
		player2.curPiece[1]= player2.nextPiece[1];
		player2.curPiece[2]= player2.nextPiece[2];
		
		player2.nextPiece[0]= colors[get_rand_color(game.numOfColors)];
		player2.nextPiece[1]= colors[get_rand_color(game.numOfColors)];
		player2.nextPiece[2]= colors[get_rand_color(game.numOfColors)];
	}
}

void clearBoard()
{
	int i,j;
	for(i=0;i<19;i++)
		for(j=0;j<6;j++)
		{
			player1.board[i][j]=0x00;
			player2.board[i][j]=0x00;
		}
	for(j=7;j<25;j++)	//col
	{
		for(i=4;i<=22;i++)//raw
			update_screen[i][j]=0x00;
	}
	
	for(j=55;j<73;j++)	//col
	{
		for(i=4;i<=22;i++)//raw
			update_screen[i][j]=0x00;
	}
	
	
}
void duplicateEnamy(int player)
{
	int i,j,k;
	if(player==2)
	{
		for(i=0;i<6;i++)
		{
			for(j=1;j<18 && player2.board[j][i] == 0x00;j++)
			{

				
			}
			if(player2.board[j][i]!=0x00)
			{
				player2.board[j-1][i]=player2.board[j][i];
				for(k=0;k<3;k++)
				{
					change_screen(j-1+4,i*3+k+55, 176,player2.board[j-1][i]);
				}	
			}
		}
	}
	else //player1
	{
		for(i=0;i<6;i++)
		{
			for(j=1;j<18 && player1.board[j][i] == 0x00;j++)
			{

				
			}
			if(player1.board[j][i]!=0x00)
			{
				player1.board[j-1][i]=player1.board[j][i];
				for(k=0;k<3;k++)
				{
					change_screen(j-1+4,i*3+k+7, 176,player1.board[j-1][i]);
				}	
			}
		}
		
	}
	//findSequence(player);
}


void updateter()
{
	int i,j;
	time_t t;
	char ch;
	int action;
	int temp;
	
	while(1)
	{

		receive();
   
		if(game.screen==START_SCREEN)
		{
			if(game.selectedDifficultyLevel==1)
				for(i=0;i<36;i++)
					change_only_color_on_screen(19,5+i,0x0EE);
			if(game.selectedDifficultyLevel==2)
				for(i=0;i<38;i++)
					change_only_color_on_screen(20,5+i,0x0EE);
			if(game.selectedDifficultyLevel==3)
				for(i=0;i<36;i++)
					change_only_color_on_screen(21,5+i,0x0EE);
			continue;
		}
		else if(game.screen==GAME_SCREEN)
		{
			while(front != -1)
			{
		   //Here updates are made following clicks on keyboard
				action = input_arr[front];
				if(front != rear)
					front++;
				else
					front = rear = -1;
	  
				if(action==LEFT_PLAYER1 || action==RIGHT_PLAYER1 || action==UP_PLAYER1|| 
							action==DOWN_PLAYER1 || action==RELEAS_PLAYER1)
				{
					if ( isStopped1 == 0) 
					{
			
						if(action == LEFT_PLAYER1)
						{				
							
							if (player1.curPiece_x>7 && player1.board[player1.placeOnBoard_y+2 ][player1.placeOnBoard_x-1]==0x00)
							{
								delete_previou_location(player1.curPiece_x,player1.curPiece_y,'v');
								player1.curPiece_x-=3;
								display_next(player1.curPiece_x,player1.curPiece_y,player1.curPiece,'v');
								player1.placeOnBoard_x--; //**********************
							}
						}
						else if ( action==RIGHT_PLAYER1 )
						{
							
							if (player1.curPiece_x<22 && player1.board[player1.placeOnBoard_y+2][player1.placeOnBoard_x+1]==0x00)
							{
								delete_previou_location(player1.curPiece_x,player1.curPiece_y,'v');
								player1.curPiece_x+=3;
								display_next(player1.curPiece_x,player1.curPiece_y,player1.curPiece,'v');	
								player1.placeOnBoard_x++; //**********************
							}
						}
						else if ( action == UP_PLAYER1)
						{
							
							temp= player1.curPiece[2];
							player1.curPiece[2]=player1.curPiece[1];
							player1.curPiece[1]=player1.curPiece[0];
							player1.curPiece[0]=temp;
						}
					
					}
				}
				else if(action==LEFT_PLAYER2 || action==RIGHT_PLAYER2 || action==UP_PLAYER2)	
				{
					if (isStopped2 == 0) 
					{
		
						if(action == LEFT_PLAYER2)
						{
						
							if (player2.curPiece_x>55 && player2.board[player2.placeOnBoard_y+2][player2.placeOnBoard_x-1]==0x00)
							{
								delete_previou_location(player2.curPiece_x,player2.curPiece_y,'v');
								player2.curPiece_x-=3;
								display_next(player2.curPiece_x,player2.curPiece_y,player2.curPiece,'v');
								player2.placeOnBoard_x--; //**********************
							}
			  
						}
						else if ( action==RIGHT_PLAYER2)
						{
							
							if (player2.curPiece_x<70 && player2.board[player2.placeOnBoard_y+2][player2.placeOnBoard_x+1]==0x00)
							{
								delete_previou_location(player2.curPiece_x,player2.curPiece_y,'v');
								player2.curPiece_x+=3;
								display_next(player2.curPiece_x,player2.curPiece_y,player2.curPiece,'v');
								player2.placeOnBoard_x++; //**********************
							}
						}
						else if ( action==UP_PLAYER2 )
						{
						
							temp= player2.curPiece[2];
							player2.curPiece[2]=player2.curPiece[1];
							player2.curPiece[1]=player2.curPiece[0];
							player2.curPiece[0]=temp;
						}
					}
				}
		
			} // while(front != -1)
		// לאחר שנגמרת הלולאה הפנימית מבצעים את השינויים בעקבות זמן שעובר(כגון ירידת החלק עוד שורה למטה)
			action= -1;
		
		
		}
		else if(game.screen==END_SCREEN)
			generate_ending_screen();
	}	

} // updater 



int sched_arr_pid[5] = {-1};
int sched_arr_int[5] = {-1};


SYSCALL schedule(int no_of_pids, int cycle_length, int pid1, ...)
{
  int i;
  int ps;
  int *iptr;

  disable(ps);

  gcycle_length = cycle_length;
  point_in_cycle = 0;
  gno_of_pids = no_of_pids;

  iptr = &pid1;
  for(i=0; i < no_of_pids; i++)
  {
    sched_arr_pid[i] = *iptr;
    iptr++;
    sched_arr_int[i] = *iptr;
    iptr++;
  } // for
  restore(ps);

} // schedule 

void playerWonMsg()
{
	
	int i,j,k;
	for(i=0;i<2;i++)
		for(j=0;j<8;j++)
			for(k=0;k<69;k++)
				msg[i][j][k]=' ';
	strcpy(msg[0][0],"        _                           __                         _ ");
	strcpy(msg[0][1],"       | |                         /_ |                       | |");
	strcpy(msg[0][2],"  _ __ | | __ _ _   _  ___ _ __     | |   __      _____  _ __ | |");
	strcpy(msg[0][3]," | '_ \\| |/ _` | | | |/ _ \\ '__|    | |   \\ \\ /\\ / / _ \\| '_ \\| |");
	strcpy(msg[0][4]," | |_) | | (_| | |_| |  __/ |       | |    \\ V  V / (_) | | | |_|");
	strcpy(msg[0][5]," | .__/|_|\\__,_|\\__, |\\___|_|       |_|     \\_/\\_/ \\___/|_| |_(_)");
	strcpy(msg[0][6]," | |             __/ |                                           ");
	strcpy(msg[0][7]," |_|            |___/                                            ");
	
	strcpy(msg[1][0],"        _                           ___                          _ ");
	strcpy(msg[1][1],"       | |                         |__ \\                        | |");
	strcpy(msg[1][2],"  _ __ | | __ _ _   _  ___ _ __       ) |   __      _____  _ __ | |");
	strcpy(msg[1][3]," | '_ \\| |/ _` | | | |/ _ \\ '__|     / /    \\ \\ /\\ / / _ \\| '_ \\| |");
	strcpy(msg[1][4]," | |_) | | (_| | |_| |  __/ |       / /_     \\ V  V / (_) | | | |_|");
	strcpy(msg[1][5]," | .__/|_|\\__,_|\\__, |\\___|_|      |____|     \\_/\\_/ \\___/|_| |_(_)");	
	strcpy(msg[1][6]," | |             __/ |                                             ");
	strcpy(msg[1][7]," |_|            |___/                                              ");
	
	strcpy(msg[2][0],"  _____ _______ _  _____                  _______ _____ ______ _ ");
	strcpy(msg[2][1]," |_   _|__   __( )/ ____|       /\\       |__   __|_   _|  ____| |");
	strcpy(msg[2][2],"   | |    | |  |/| (___        /  \\         | |    | | | |__  | |");
	strcpy(msg[2][3],"   | |    | |     \\___ \\      / /\\ \\        | |    | | |  __| | |");
	strcpy(msg[2][4],"  _| |_   | |     ____) |    / ____ \\       | |   _| |_| |____|_|");
	strcpy(msg[2][5]," |_____|  |_|    |_____/    /_/    \\_\\      |_|  |_____|______(_)");	
	strcpy(msg[2][6],"");
	strcpy(msg[2][7],"");
	
}


xmain()
{
	
    int uppid, dispid, recvpid;
	initializer();	
	clearScreen();
	playerWonMsg();
	asm{
	   PUSH AX
	   PUSH BX   
	   MOV AL,36h   //channel 0-00 11 011 b-0 
	   OUT 43h,AL
	   MOV BX, 4800
	   MOV AL,BL
	   OUT 40h,AL //port 40 channel 0
	   MOV AL,BH
	   OUT 40h,AL
	   POP BX
	   POP AX
	} // asm
	mapinit(INTERRUPT70H, Int0x70Handler, INTERRUPT70H); // sets a new entry at interrupts map, for interrupt 0x70.
	//initialGameBoard();
    resume( dispid = create(displayer, INITSTK, INITPRIO, "DISPLAYER", 0) );
    resume( recvpid = create(receiver, INITSTK, INITPRIO+3, "RECIVEVER", 0) );
    resume( uppid = create(updateter, INITSTK, INITPRIO, "UPDATER", 0) );
    receiver_pid =recvpid;  
	generate_welcome_screen();
    set_new_int9_newisr();
	set_Int0x70Handler();
    schedule(2,10, dispid, 0,  uppid, 3);
} // xmain
