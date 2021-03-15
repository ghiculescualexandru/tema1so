#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MAP_MAX_SIZE 30
#define MAP_INITIAL_SIZE 10

#define KEY_SIZE 10

#define D_ARGUMENT "-D"
#define I_ARGUMENT "-I"
#define O_ARGUMENT "-O"

#define BUF_LEN 256

#define DEFINE_STR "define"
#define INCLUDE_STR "include"
#define UNDEF_STR "undef"
#define IF_STR "if"
#define ELSE_STR "else"
#define ELIF_STR "elif"
#define ENDIF_STR "endif"
#define IFDEF_STR "ifdef"

const char EQ_DEL[2] = "=";
const char SPACE_DEL[2] = " ";
const char ALL_DEL[28] = "[\t []{}<>=+-*/%!&|^.,:;()\"]";

#define NPTRS 2

void replaceAll(
	char *buffer,
	char *toReplace,
	char *replacement
) {
	char *occurence = strstr(buffer, toReplace);

    for (; occurence != NULL; occurence = strstr(buffer, toReplace)) {
        char *aux = (char*)malloc(BUF_LEN);
        if (aux == NULL) {
        	fprintf(stdout, "Malloc err.\n");
        }

        strcpy(aux, buffer);

        int index = occurence - buffer;
        buffer[index] = '\0';

        strcat(buffer, replacement);
        strcat(buffer, aux + index + strlen(toReplace));

        occurence = strstr(buffer, toReplace);

        free(aux);
    }
}

typedef struct HashItem {
	char key[KEY_SIZE];
	char *value;
} HashItem;

typedef struct HashMap {
	int currentSize;
	int maxSize;
	HashItem* items;
} HashMap;

HashMap mapInit() {
	HashMap map;

	map.maxSize = MAP_MAX_SIZE;
	map.currentSize = 0;
	map.items = (HashItem*)malloc(MAP_INITIAL_SIZE * sizeof(HashItem));
	if (map.items == NULL) {
		fprintf(stdout, "Malloc err.\n");
	}

	return map;
}

HashItem hashItemInit(char key[KEY_SIZE], char *value) {
	HashItem item;

	item.value = (char*)malloc(sizeof(char) * strlen(value) + 1);
	if (item.value == NULL) {
		fprintf(stdout, "Malloc err\n");
	}

	strcpy(item.key, key);
	strcpy(item.value, value);

	return item;
}

void insert(HashMap *map, HashItem item) {
	int itemExists = 0;

	for (int i = 0; i < map->currentSize; i++) {
		HashItem currItem = map->items[i];
		if (strcmp(currItem.key, item.key) == 0) {
			itemExists = 1;
		}
	}

	if (!itemExists) {
		map->items[map->currentSize] = item;
		map->currentSize++;
	}
}

char* search(HashMap *map, char key[KEY_SIZE]) {
	for (int i = 0; i < map->currentSize; i++) {
		HashItem currItem = map->items[i];
		if (strcmp(currItem.key, key) == 0) {
			return currItem.value;
		}
	}

	return NULL;
}

void delete(HashMap *map, char key[KEY_SIZE]) {
	for (int i = 0; i < map->currentSize; i++) {
		HashItem currItem = map->items[i];
		if (strcmp(currItem.key, key) == 0) {
			for (int j = i; j < map->currentSize; j++) {
				map->items[j] = map->items[j + 1];
			}
			map->currentSize--;
		}
		free(currItem.value);
	}
}

void handleDInput(HashMap *map, char *DInput) {
	char *key = strtok(DInput, EQ_DEL);
	char *val = strtok(NULL, EQ_DEL);

	if (val == NULL) {
		val = (char*)malloc(sizeof(char));
		if (val == NULL) {
			fprintf(stdout, "Malloc err.\n");
		}

		strcpy(val, "");
	}

	if (search(map, key) == NULL) {
		HashItem newItem = hashItemInit(key, val);
		insert(map, newItem);
	}
}

void handleInputFile(FILE *input_file, HashMap *map) {
	char buffer[BUF_LEN];
    int conditionValid = 1;

    while (fgets(buffer, BUF_LEN - 1, input_file))
    {
        buffer[strcspn(buffer, "\n")] = 0;
        // printf("%s\n", buffer);

        int isMacro = 1;
        int skip = 0;

        for (int i = 0; i < strlen(buffer); i++) {
        	char currChar = buffer[i];
        	if (currChar == '#' && isMacro) {
        		skip = 1;

        		char *afterHashtag = buffer + 1;
        		
        		char *type = strtok(afterHashtag, SPACE_DEL);
        		if (strcmp(type, DEFINE_STR) == 0) {
        			char *key = strtok(NULL, SPACE_DEL);
					char *val = strtok(NULL, "\n");

					// for define in define
					for (int j = 0; j < map->currentSize; j++) {
						replaceAll(val, map->items[i].key, map->items[i].value);
					}

					if (search(map, key) == NULL) {
						HashItem newItem = hashItemInit(key, val);
						insert(map, newItem);
					}
				} else if (strcmp(type, UNDEF_STR) == 0) {
					char *key = strtok(NULL, SPACE_DEL);

					if (search(map, key)) {
						delete(map, key);
					}

        		} else if (strcmp(type, INCLUDE_STR) == 0) {

        		} else if (strcmp(type, IF_STR) == 0) {
        			char *cond = strtok(NULL, SPACE_DEL);

        			char *res = search(map, cond);
        			// fprintf(stdout, "%s\n", res);
        			if (res) {
        				strcpy(cond, res);
        			}

        			if (strcmp(cond, "0")) {
        				conditionValid = 1;
        			} else {
        				conditionValid = 0;
        			}
        		} else if (strcmp(type, ELSE_STR) == 0) {
        			if (conditionValid) {
        				conditionValid = 0;
        			} else {
        				conditionValid = 1;
        			}
        		} else if (strcmp(type, ELIF_STR) == 0) {
        			char *cond = strtok(NULL, SPACE_DEL);

        			char *res = search(map, cond);
        			// fprintf(stdout, "%s\n", res);
        			if (res) {
        				strcpy(cond, res);
        			}

        			if (strcmp(cond, "0")) {
        				conditionValid = 1;
        			} else {
        				conditionValid = 0;
        			}
        		} else if (strcmp(type, ENDIF_STR) == 0) {
        			conditionValid = 1;
        		} else if (strcmp(type, IFDEF_STR) == 0) {
        			char *cond = strtok(NULL, SPACE_DEL);

        			if (search(map, cond)) {
        				conditionValid = 1;
        			} else {
        				conditionValid = 0;
        			}
        		}

        	} else {
        		isMacro = 0;
        	}
        }

        if (skip || strcmp(buffer, "") == 0 || conditionValid == 0) {
        	continue;
        }

		// char *start = strstr(buffer, "\"");
		// fprintf(stderr, "start: %s\n", start);
		// char *res, *end;
		// int closingType = 1; // ",
		// if (start) {
		// 	end = strstr(buffer, "\",");
		// 	fprintf(stderr, "end1: %s\n", end);

		// 	if (end == NULL) {
		// 		end = strstr(buffer, "\")");
		// 		closingType = 2; // ")
		// 	}

		// 	if (end) {
		// 		res = (char*)malloc(strlen(start) + 1);
		// 		memcpy(res, start + 1, end - (start + 1));
		// 		fprintf(stderr, "res: %s\n", res);
		// 	}
		// }
		
        char *bufferAux = (char*)malloc(strlen(buffer) + 1);
        strcpy(bufferAux, buffer);
        char *tok = strtok(bufferAux, ALL_DEL);
        while (tok != NULL) {
        	// fprintf(stdout, "tok: %s\n", tok);
        	char *res = search(map, tok);
        	// fprintf(stderr, "res: %s\n", res);
        	if (res != NULL) {
        		replaceAll(buffer, tok, res);
        	}
        	tok = strtok(NULL, ALL_DEL);
        }

        // if (start) {
	       //  fprintf(stdout, "buffer: %s\n", buffer);
        // 	memcpy(buffer + strlen(buffer) - strlen(start) + 1, res, strlen(res));
        // } 
        free(bufferAux);
        fprintf(stdout, "%s\n", buffer);
    }
}

void initArgs(
	int argc,
	char *argv[],
	HashMap *map,
	FILE **input_file,
	/* TODO:
	output,
	include*/
	int *isInputFile
) {
	for (int i = 1; i < argc; i++) {
		char *current = argv[i];
		// fprintf(stdout, "%s ", current);
		if (strcmp(current, D_ARGUMENT) == 0) {
			char *DInput = argv[i + 1];
			handleDInput(map, DInput);
			i++;
		} else if (strncmp(current, D_ARGUMENT, 1) == 0) {
			char *DInput = current + 2;
			handleDInput(map, DInput);
		} else if (strcmp(current, I_ARGUMENT) == 0) {
			// TODO
			i++;
		} else if (strncmp(current, I_ARGUMENT, 1) == 0) {
			// TODO
		} else if (strcmp(current, O_ARGUMENT) == 0) {
			// TODO
			i++;
		} else if (strncmp(current, O_ARGUMENT, 1) == 0) {
			// TODO
		} else {
			// input_file
			*input_file = fopen(current, "r");
			if (input_file == NULL) {
				// fprintf(stdout, "Input file <%s> failed to open.\n", current);
				exit(0);
			}
			*isInputFile = 1;
		}
	}
}

int main(int argc, char *argv[]) {
	HashMap map = mapInit();
	FILE *input_file;

	int isInputFile = 0;
	initArgs(argc, argv, &map, &input_file, &isInputFile);

	if (isInputFile) {
		handleInputFile(input_file, &map);
	} else {
		handleInputFile(stdin, &map);
	}
	
	fclose(input_file);

	for (int i = 0; i < map.currentSize; i++) {
		free(map.items[i].value);
	}
	free(map.items);

	return 0;
}