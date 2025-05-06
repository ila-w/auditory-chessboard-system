#include <SoftwareSerial.h> // used for monitoring the speaker hardware
#include <DFRobotDFPlayerMini.h> // used for .mp3 player hardware

const int row_pins[8] = {2, 3, 4, 5, 6, 7, 8, 9}; // contains the pin numbers for the rows
const int column_mux_sig_pins[4] = {A0, A1, A2, A3}; // contains the pin numbers for the columns (each mux provides a single output outcome for 16 inputs)
const int column_mux_selector_pins[4] = {A4, A5, 12, 13}; // contains the pin numbers for the s0, s1, s2, and s3 values used in the mux to store an outcome 

const char* rank_1 = "RNBQKBNR"; // white starting board at A1 to A8 row
const char* rank_8 = "rnbkqbnr"; // black starting board at H1 to H8 row

char chess_board[8][8]; // initializes blank chessboard
bool previous_state[8][8]; // save previous state of the chessboard

int previous_check_row = -1; // holds previous check of row value
int previous_check_column = -1; // holds previous check of column value

bool is_white_player_turn = true; // white player goes first, so first state is true
bool white_castle = true; // determines if white is able to castle king and rook
bool black_castle = true; // determines if black is able to castle king and rook

SoftwareSerial dfSerial(11, 10); // assigns the RX and TX pins for audio player to pins 11 and 10 
DFRobotDFPlayerMini player; // initializes the .mp3 audio player 

/**
* Selects a new MUX channel from the available 4 muxes.
*/
void select_MUX_channel(int channel) {
  for(int i = 0; i < 4; i++) {
    digitalWrite(column_mux_selector_pins[i], bitRead(channel, i));
  }
}

/**
* Calculates the current reed location in order to play
* the correct audio file related to the location.
*/
int get_reed_location(int row, int column) {
  return row * 8 + column + 1;
}

/**
* Returns the stringified label of the current 
* reed location for testing purposes in the 
* serial monitor in the Arduino IDE.
*/
String get_reed_label(int row, int column) {
  return String((char)('A' + row)) + String(column + 1);
}

/**
* Returns the track number of a given player_type in 
* order to play the correct audio file. 0075_white.mp3 
* contains "white" and 0076_black.mp3 contains "black".
*/
int get_player_color_audio(char player_type) {
  if(isupper(player_type)) {
    return 75; // "White" player audio file number
  }
  return 76; // "Black" player audio file number
}

/**
* Returns the track number of a given chess piece in 
* order to play the correct audio file.
*/
int get_chess_piece_audio(char piece_type) {
  switch(tolower(piece_type)) {
    case 'p': return 65; break; // "Pawn" audio file number
    case 'r': return 66; break; // "Rook" audio file number
    case 'n': return 67; break; // "Knight" audio file number
    case 'b': return 68; break; // "Bishop" audio file number
    case 'q': return 69; break; // "Queen" audio file number
    case 'k': return 70; break; // "King" audio file number
    default: return 0; // the given chess piece is not a valid type
  }
}

/**
* Determines which audio files will be played if a player 
* has moved their piece to a new location. The player can 
* move to an empty location or take another piece's place.
*/
void play_move_audio(int row, int column) {
  char moving_piece = chess_board[previous_check_row][previous_check_column];
  char captured_piece = chess_board[row][column];

  player.playMp3Folder(get_player_color_audio(moving_piece));
  delay(500); // delay to allow audio to play

  player.playMp3Folder(get_chess_piece_audio(moving_piece));
  delay(500); // delay to allow audio to play

  player.playMp3Folder(73); // 
  delay(300); // delay to allow audio to play

  player.playMp3Folder(get_reed_location(previous_check_row, previous_check_column));
  delay(500); // delay to allow audio to play

  if(captured_piece != '.') { // there was a piece to take
    player.playMp3Folder(74); 
    delay(400); // delay to allow audio to play

    player.playMp3Folder(get_player_color_audio(captured_piece));
    delay(500); // delay to allow audio to play

    player.playMp3Folder(get_chess_piece_audio(captured_piece));
    delay(500); // delay to allow audio to play

    player.playMp3Folder(73); 
    delay(300); // delay to allow audio to play

    player.playMp3Folder(get_reed_location(row, column));
    delay(600);// delay to allow audio to play
  } else { // there was no piece to take
    player.playMp3Folder(71);
    delay(300); // delay to allow audio to play

    player.playMp3Folder(get_reed_location(row, column));
    delay(500); // delay to allow audio to play

  }

  // Debugging statements for the serial monitor to make sure the program is detecting moves correctly
  Serial.print((is_white_player_turn ? "White" : "Black"));
  Serial.print(": ");
  Serial.print(get_reed_label(previous_check_row, previous_check_column));
  Serial.print(" to ");
  Serial.println(get_reed_label(row, column));
}

/**
* Checks whether a player can castle their rook and king. If they can and 
* this movement is selected, the move will occur for the rook as the king 
* is taken care of in the main loop.
*/
void check_castle(int row, int column) {
  char temp_piece = chess_board[previous_check_row][previous_check_column]; // stores current piece

  // checks for black castle conditions
  if(black_castle && temp_piece == 'k' && previous_check_row == 7 && previous_check_column == 4) {
    if(column == 6) {
      chess_board[7][5] = 'r';
      chess_board[7][7] = '.';
      black_castle = false; // black player can no longer castle their king and rook
    }
  }

  // checks for white castle conditions
  if(white_castle && temp_piece == 'K' && previous_check_row == 0 && previous_check_column == 4) {
    if(column == 2) {
      chess_board[0][3] = 'r';
      chess_board[0][0] = '.';
      white_castle = false; // white player can no longer castle their king and rook
    }
  }
}

/**
* Checks if a pawn is able to be promoted (i.e. reached the end of the 
* chessboard opposite to itself), and if so, promotes the piece to a 
* queen by default.
*/
void promote_pawn(int row, int column) {
  if(chess_board[row][column] == 'P' && row == 7) { // checks if white's pawn reached the other side
    chess_board[row][column] = 'Q';
  }
  if(chess_board[row][column] == 'p' && row == 0) { //checks if black's pawn reached the other side
    chess_board[row][column] = 'q';
  }
}

/**
* Checks if the given king type (white or black) 
* is currently in check or not.
*/
bool is_check(char king_type) {
  int k_row = -1; // default row value for king
  int k_column = -1; // default column value for king

  // Searches for the current location of the king on chessboard
  for(int i = 0; i < 8; i++) {
    for(int j = 0; j < 8; j++) {
      if(chess_board[i][j] == king_type) {
        k_row = i;
        k_column = j;
        break; // stop as the king was found
      }
    }
  }

  if(k_row == -1) return false; // king was never found

  // Check all pieces on the board that may be threatening the king
  for(int i = 0; i < 8; i++) {
    for(int j = 0; j < 8; j++) {
      char attacker = chess_board[i][j]; // stores the attacker's piece type at the current location

      if(attacker == '.' || attacker == king_type) continue; // no attacker here

      int dr = abs(k_row - i); // calculating the distance between the current row and king's row
      int dc = abs(k_column - j); // calculating the distance between the current column and king's column

      // Checks the piece type and if it is a threat to the king
      switch(toLowerCase(attacker)) {
        case 'p': if(king_type == 'K') {
                    if(i + 1 == k_row && abs(j - k_column) == 1) {
                      return true;
                    }
                  }
                  if(king_type == 'k') {
                    if(i - 1 == k_row && abs(j - k_column) == 1) {
                      return true;
                    }
                  } break;

        case 'n': if((dr == 2 && dc == 1) || (dr == 1 && dc == 2)) {
                    return true;
                  } break;

        case 'b': if(dr == dc) {
                    int sr = (k_row - i) / dr;
                    int sc = (k_column - j) / dc;
                    bool attacking = true;

                    for(int k = 0; k < dr; k++) {
                      if(chess_board[i + k * sr][j + k * sc] != '.') {
                        attacking = false;
                      }
                    }

                    if(attacking) {
                      return true;
                    }
                  } break;

        case 'r': if((dr == 0 || dc == 0)) {
                    int sr = (dr == 0) ? 0 : (k_row - i) / dr;
                    int sc = (dc == 0) ? 0 : (k_column - j) / dc;
                    bool attacking = true;

                    for(int k = 1; k < max(dr, dc); k++) {
                      if(chess_board[i + k * sr][j + k * sc] != '.') {
                        attacking = false;
                      }
                    }

                    if(attacking) {
                      return true;
                    }
                  } break;

        case 'q': if((dr == dc || dr == 0) || dc == 0) {
                    int sr = (dr == 0) ? 0 : (k_row - i) / max(dr, 1);
                    int sc = (dc == 0) ? 0 : (k_column - j) / max(dc, 1);
                    bool attacking = true;

                    for(int k = 1; k < max(dr, dc); k++) {
                      if(chess_board[i + k * sr][j + k * sc] != '.') {
                        attacking = false;
                      }
                    }

                    if(attacking) {
                      return true;
                    }
                  } break;

        case 'k':  if(dr <= 1 && dr <= 1) return true; break;
      }
    }
  }

  return false; // the king is not in check (no danger)
}

/**
* Checks for if checkmate has occurred between the 
* two players, and thus, ending the game. Uses the 
* supporter function is_check() in order to determine 
* if check has occurred first.
*/
void check_checkmate() {
  if(is_check('K')) { // checking if white is in check
    player.playMp3Folder(77); // "check" is played
    delay(5000); // delay to allow audio to play

    player.playMp3Folder(78); // "checkmate" is played if no move occurs
    delay(1000); // delay to allow audio to play

    player.playMp3Folder(76); // "black" is played
    delay(500); // delay to allow audio to play

    player.playMp3Folder(80); // "wins" is played
    delay(500); // delay to allow audio to play
  }

  if(is_check('k')) { // checking if black is in check
    player.playMp3Folder(77); // "check" is played
    delay(5000); // delay to allow audio to play

    player.playMp3Folder(78); // "checkmate" is played if no move occurs
    delay(1000); // delay to allow audio to play

    player.playMp3Folder(75); // "white" is played
    delay(500); // delay to allow audio to play

    player.playMp3Folder(80); // "wins" is played
    delay(500); // delay to allow audio to play
  }
} 


//------------MAIN SECTION OF CODE-----------------------------------
/**
* This function is run once to set up the entire 
* Arduino project so that it is ready to execute 
* the loop() function, which contains the actual 
* code and decisions towards other functions.
*/
void setup() {
  Serial.begin(9600); // begins the serial monitor for the speaker
  dfSerial.begin(9600); // begins the dfplayer monitor

  // Checking for connection between the Arduino and the audio system
  if(!player.begin(dfSerial)) {
    Serial.println("The DFPlayer was not found. Please reset the system.");
    while(1);
  }

  player.volume(25); // speaker max volume is 30
  player.playMp3Folder(79); // plays the audio file "game start"

  delay(1000); // delays before user action will be considered to allow for audio to play

  // Sets up the row pins on the Arduino for listening
  for(int i = 0; i < 8; i++) {
    pinMode(row_pins[i], OUTPUT);
    pinMode(column_mux_selector_pins[i], OUTPUT);
  }

  // Sets up the column (mux) pins on the Arduino for listening
  for(int i = 0; i < 4; i++) {
    pinMode(column_mux_sig_pins[i], INPUT_PULLUP);
    pinMode(column_mux_selector_pins[i], OUTPUT);
  }

  // Sets up a blank chessboard with nothing on the board previously
  for(int i = 0; i < 8; i++) {
    for(int j = 0; j < 8; j++) {
      previous_state[i][j] = false;
      chess_board[i][j] = '.';
    }
  }

  // Sets up a new default chessboard starting state for each piece
  for(int i = 0; i < 8; i++) {
    chess_board[0][i] = rank_1[i]; // white strong pieces set up to A1 to A8
    chess_board[1][i] = 'P'; // white pawns set up to B1 to B8
    chess_board[6][i] = 'p'; // black pawns set up to G1 to G8
    chess_board[7][i] = rank_8[i]; // black strong pieces set up to H1 to H8
  }
}

/**
* This function runs continuously for the Arduino project 
* so that it is ready for player interaction with the 
* auditory chessboard system and creates diverging paths 
* for move checking.
*/
void loop() {
  bool state_change = false; // state of the chessboard has not changed yet

  // Checks each row and column continuously for any changes in the game state
  for(int i = 0; i < 8; i++) { // rows
    digitalWrite(row_pins[i], HIGH); // sets row pins to high voltage to check for any changes
    delayMicroseconds(100); // delay to give the Arduino time to change all the row pins to high voltage

    for(int j = 0; j < 4; j++) { // columns (for each mux)
      for(int k = 0; k < 16; k++) { // mux input pins
        int current_index = j * 16 + k; // calculating the current (global) index of the column (mux)

        if(current_index >= 64) continue; // current index is out of the bounds of the chessboard locations

        int row = current_index / 8; // calculating the row number
        int column = current_index % 8; // calculating the column number

        if(row != i) continue; // current row doesn't match the current section for checking the row (i)
        
        select_MUX_channel(k); // select the current location within the saved values of the mux (column)
        delayMicroseconds(50); // delay to give the Arduino time to check the curret s0, s1, s2, s4 values

        bool current_state = !digitalRead(column_mux_sig_pins[j]); // checking for changes in the mux signal pins

        if(current_state != previous_state[i][column]) { // checking if current state has changed from previous state
          previous_state[i][column] = current_state; // save changed state into previous state matrix

          if(current_state) { // checking if current state had a move of a chess piece or pieces
            state_change = true; // state change occurred

            if(previous_check_row == -1) { // previous row check didn't save a value (-1)
              previous_check_row = i; // save current checking row
              previous_check_column = column; // save current column
            } else {
              player.playMp3Folder(is_white_player_turn ? 75 : 76); // if white player's turn, play "white", otherwise, play "black" audio file
              delay(300); // delay to allow audio to play of either player type

              play_move_audio(i, column); // plays move audio for previous state changes
              check_castle(i, column); // check if castling can occur and if it does
              
              chess_board[i][column] = chess_board[previous_check_row][previous_check_column]; // moving value of piece to new location
              chess_board[previous_check_row][previous_check_column] = '.'; // resetting previous location to default

              promote_pawn(i, column); // if a pawn has reached the end of the board, promote it to a queen (default)

              previous_check_row = -1; // reset previous check row value for future checks
              previous_check_column = -1; // reset previous check column value for future checks

              check_checkmate(); // checking whether a checkmate has occurred
              is_white_player_turn = !is_white_player_turn; // switching to the next player turn state (whose turn it is)

              while(!state_change) { // if no state changes occur
                delay(10); // delay before next run of check
              }
            }
          }
        }
      }
    }

    digitalWrite(row_pins[i], LOW); // reset current row pin to low voltage
  }

  if(!state_change) { // if no state changes occur
    delay(50); // delay before next run of check
  }
}
//-------------------------------------------------------------------
