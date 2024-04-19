#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define INITIAL_MOVES_SIZE 1024

int count_moves(const char *moves) {
    int count = 0;
    const char *ptr = moves;
    while (*ptr) {
        if (*ptr == ' ') {
            count++;
        }
        ptr++;
    }
    return count / 2;
}

void remove_move_numbers(char *moves) {
    char *src = moves;
    char *dst = moves;
    while (*src) {
        if (isdigit(*src)) {
            char *peek = src;
            while (isdigit(*peek)) {
                peek++;
            }
            if (*peek == '.' && *(peek + 1) == ' ') {
                src = peek + 2;
                continue;
            }
        }
        *dst++ = *src++;
    }
    *dst = '\0';
}

int main() {
    FILE *pgn = fopen("PGN_Files/lichess_db_1GB.pgn", "r");
    if (!pgn) {
        perror("Failed to open PGN file");
        return EXIT_FAILURE;
    }

    FILE *csv_white_wins = fopen("CSV_Files/1GB_white_wins.csv", "w");
    FILE *csv_black_wins = fopen("CSV_Files/1GB_black_wins.csv", "w");
    if (!csv_white_wins || !csv_black_wins) {
        perror("Failed to open CSV file(s)");
        fclose(pgn);
        if (csv_white_wins) fclose(csv_white_wins);
        if (csv_black_wins) fclose(csv_black_wins);
        return EXIT_FAILURE;
    }

    fprintf(csv_white_wins, "White,Black,Result,WhiteElo,BlackElo,TimeControl,Termination,Moves\n");
    fprintf(csv_black_wins, "White,Black,Result,WhiteElo,BlackElo,TimeControl,Termination,Moves\n");

    char line[1024];
    char *moves = malloc(INITIAL_MOVES_SIZE);
    if (moves == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory for moves.\n");
        fclose(pgn);
        fclose(csv_white_wins);
        fclose(csv_black_wins);
        return EXIT_FAILURE;
    }
    size_t movesSize = INITIAL_MOVES_SIZE;

    char white[100], black[100], result[10], whiteElo[10], blackElo[10], timeControl[20], termination[50];
    int skip_game = 0;

    while (fgets(line, sizeof(line), pgn) != NULL) {
        if (strstr(line, "[Event ") != NULL) {
            skip_game = 0;
            moves[0] = '\0';
            movesSize = INITIAL_MOVES_SIZE;
            strcpy(white, "");
            strcpy(black, "");
            strcpy(result, "");
            strcpy(whiteElo, "");
            strcpy(blackElo, "");
            strcpy(timeControl, "");
            strcpy(termination, "");

            do {
                if (strstr(line, "[White ") != NULL) sscanf(line, "[White \"%99[^\"]\"]", white);
                else if (strstr(line, "[Black ") != NULL) sscanf(line, "[Black \"%99[^\"]\"]", black);
                else if (strstr(line, "[Result ") != NULL) sscanf(line, "[Result \"%9[^\"]\"]", result);
                else if (strstr(line, "[WhiteElo ") != NULL) sscanf(line, "[WhiteElo \"%9[^\"]\"]", whiteElo);
                else if (strstr(line, "[BlackElo ") != NULL) sscanf(line, "[BlackElo \"%9[^\"]\"]", blackElo);
                else if (strstr(line, "[TimeControl ") != NULL) sscanf(line, "[TimeControl \"%19[^\"]\"]", timeControl);
                else if (strstr(line, "[Termination ") != NULL) sscanf(line, "[Termination \"%49[^\"]\"]", termination);
            } while (fgets(line, sizeof(line), pgn) && line[0] == '[');

            do {
                if (strstr(line, "%eval") != NULL) {
                    skip_game = 1; // Mark to skip this game
                }
                if (line[0] != '[' && strlen(line) > 2 && !skip_game) {
                    size_t lineLen = strlen(line);
                    if (movesSize < strlen(moves) + lineLen + 1) {
                        size_t newSize = movesSize * 2;
                        char *newMoves = realloc(moves, newSize);
                        if (newMoves == NULL) {
                            fprintf(stderr, "Error: Failed to allocate memory for moves.\n");
                            free(moves);
                            fclose(pgn);
                            fclose(csv_white_wins);
                            fclose(csv_black_wins);
                            return EXIT_FAILURE;
                        }
                        moves = newMoves;
                        movesSize = newSize;
                    }
                    strcat(moves, line);
                }
            } while (fgets(line, sizeof(line), pgn) && line[0] != '\n' && line[0] != '[');

            if (!skip_game) {
                remove_move_numbers(moves);
                size_t len = strlen(moves);
                if (len > 0 && moves[len - 1] == '\n') moves[len - 1] = '\0';

                int wElo = atoi(whiteElo);
                int bElo = atoi(blackElo);

                if (wElo >= 1700 && bElo >= 1700 && strcmp(result, "1/2-1/2") != 0 && count_moves(moves) >= 20) {
                    FILE *csv_file = (strcmp(result, "1-0") == 0) ? csv_white_wins : csv_black_wins;
                    fprintf(csv_file, "\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\"\n",
                            white, black, result, whiteElo, blackElo, timeControl, termination, moves);
                }
            }

            if (line[0] == '[') fseek(pgn, -strlen(line), SEEK_CUR);
        }
    }

    free(moves);
    fclose(pgn);
    fclose(csv_white_wins);
    fclose(csv_black_wins);
    return EXIT_SUCCESS;
}
