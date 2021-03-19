#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MAP_MAX_SIZE 30
#define MAP_INITIAL_SIZE 10

#define KEY_SIZE 10

#define D_ARGUMENT "-D"
#define I_ARGUMENT "-I"
#define O_ARGUMENT "-o"

#define BUF_LEN 256

#define DEFINE_STR "define"
#define INCLUDE_STR "include"
#define UNDEF_STR "undef"
#define IF_STR "if"
#define ELSE_STR "else"
#define ELIF_STR "elif"
#define ENDIF_STR "endif"
#define IFDEF_STR "ifdef"
#define IFNDEF_STR "ifndef"

#define HASH '#'

const char EQ_DEL[2] = "=";
const char SPACE_DEL[2] = " ";
const char ALL_DEL[28] = "[\t []{}<>=+-*/%!&|^.,:;()\"]";

#define NPTRS 2

typedef struct HashItem
{
	char key[KEY_SIZE];
	char *value;
} HashItem;

typedef struct HashMap
{
	int currentSize;
	int maxSize;
	HashItem *items;
} HashMap;

int initArgs(
		int argc,
		char *argv[],
		HashMap *map,
		FILE **input_file,
		FILE **output_file,
		int *isInputFile,
		int *isOutputFile,
		char **dir_paths,
		int *dir_pathsCounter);
void replace(char *buffer, char *toReplace, char *replacement);
int handleInputFile(FILE *input_file, FILE *output_file, HashMap *map, char **dir_paths, int dir_pathsCounter);
int includeHandler(HashMap *map, FILE *output_file, char **dir_paths, int dir_pathsCounter);

HashMap mapInit()
{
	HashMap map;

	map.maxSize = MAP_MAX_SIZE;
	map.currentSize = 0;
	map.items = (HashItem *)calloc(MAP_INITIAL_SIZE, sizeof(HashItem));
	if (map.items == NULL)
	{
		//fprintf(stdout, "Malloc err 80.\n");
		exit(12);
	}

	return map;
}

HashItem hashItemInit(char key[KEY_SIZE], char *value)
{
	HashItem item;

	item.value = (char *)calloc(sizeof(char), strlen(value) + 1);
	if (item.value == NULL)
	{
		//fprintf(stdout, "Malloc err 91\n");
		exit(12);
	}

	strcpy(item.key, key);
	strcpy(item.value, value);

	return item;
}

void insert(HashMap *map, HashItem item)
{
	int itemExists = 0;

	for (int i = 0; i < map->currentSize; i++)
	{
		HashItem currItem = map->items[i];
		if (strcmp(currItem.key, item.key) == 0)
		{
			itemExists = 1;
		}
	}

	if (!itemExists)
	{
		map->items[map->currentSize] = item;
		map->currentSize++;
	}
}

void put(HashMap *map, char *key, char *val)
{
	for (int i = 0; i < map->currentSize; i++)
	{
		HashItem currItem = map->items[i];
		if (strcmp(currItem.key, key) == 0)
		{
			strcpy(currItem.value, val);
		}
	}
}

char *search(HashMap *map, char key[KEY_SIZE])
{
	for (int i = 0; i < map->currentSize; i++)
	{
		HashItem currItem = map->items[i];
		if (strcmp(currItem.key, key) == 0)
		{
			return currItem.value;
		}
	}

	return NULL;
}

void delete (HashMap *map, char key[KEY_SIZE])
{
	for (int i = 0; i < map->currentSize; i++)
	{
		HashItem currItem = map->items[i];
		if (strcmp(currItem.key, key) == 0)
		{
			for (int j = i; j < map->currentSize; j++)
			{
				map->items[j] = map->items[j + 1];
			}
			map->currentSize--;
		}
		free(currItem.value);
	}
}

int handleDInput(HashMap *map, char *DInput)
{
	char *key = strtok(DInput, EQ_DEL);
	char *val = strtok(NULL, EQ_DEL);

	if (val == NULL)
	{
		val = (char *)calloc(1, sizeof(char));
		if (val == NULL)
		{
			//fprintf(stdout, "Malloc err 147.\n");
			return 12;
		}

		strcpy(val, "");
	}

	if (search(map, key) == NULL)
	{
		HashItem newItem = hashItemInit(key, val);
		insert(map, newItem);
	}

	if (strcmp(val, "") == 0)
	{
		free(val);
	}

	return 1;
}

/********************************/
/* DIRECTIVE HELPERS/AUXILIARY */
/******************************/

int defineWithValue(HashMap *map, char *key, char *val)
{
	// for define in define

	// using a buffer for strtok
	char *valBuffer = (char *)calloc(1, BUF_LEN);
	if (valBuffer == NULL)
	{
		return 12;
	}
	strcpy(valBuffer, val);

	// replace all values in define with
	// existing values if any
	char *tok = strtok(valBuffer, ALL_DEL);
	while (tok != NULL)
	{
		char *replacement = search(map, tok);
		if (replacement != NULL)
		{
			replace(val, tok, replacement);
		}
		tok = strtok(NULL, ALL_DEL);
	}

	// insert or update key in map
	// using the computed val (replaced with
	// other map values if any)
	if (search(map, key) == NULL)
	{
		HashItem newItem = hashItemInit(key, val);
		insert(map, newItem);
	}
	else
	{
		put(map, key, val);
	}

	free(valBuffer);
	return 1;
}

int defineWithoutValue(HashMap *map, char *key)
{
	char *val = (char *)calloc(1, 2);
	if (val == NULL)
	{
		//fprintf(stderr, "MALLLOC ERR 234\n");
		return 12;
	}
	strcpy(val, "");

	// insert the key, value pair only
	// if the key is not defined already
	if (search(map, key) == NULL)
	{
		HashItem newItem = hashItemInit(key, val);
		insert(map, newItem);
	}

	free(val);
	return 1;
}

/***********************/
/* DIRECTIVE HANDLERS */
/*********************/

int defineHandler(HashMap *map)
{
	char *key = strtok(NULL, SPACE_DEL);
	char *val = strtok(NULL, "\n");

	// check if define has a key or the item is just defined
	if (val)
	{
		if (defineWithValue(map, key, val) == 12)
		{
			return 12;
		}
	}
	else
	{
		if (defineWithoutValue(map, key) == 12)
		{
			return 12;
		}
	}

	return 1;
}

int removeWhiteSpaces(char *buffer)
{
	char *aux = (char *)calloc(1, strlen(buffer) + 1);
	strcpy(aux, buffer);
	int i = 0;
	for (; i < strlen(buffer); i++)
	{
		if (aux[i] != ' ')
			break;
	}

	strcpy(buffer, aux + i);
	free(aux);
	return 1;
}

int multiLineDefineHandler(HashMap *map, FILE *input_file, char *buffer)
{
	char *multiLineBuffer = (char *)calloc(1, BUF_LEN);
	strcpy(multiLineBuffer, buffer);

	while (1)
	{
		fgets(buffer, BUF_LEN - 1, input_file);
		buffer[strcspn(buffer, "\n")] = 0;

		strcat(multiLineBuffer, buffer);
		char *isMultiLine = strstr(buffer, "\\");
		if (!isMultiLine)
		{
			break;
		}
	}

	char *tok = strtok(multiLineBuffer, ALL_DEL);
	char *key = strtok(NULL, SPACE_DEL);
	char *val = (char *)calloc(1, 50);

	char *valBuffer = strtok(NULL, SPACE_DEL);
	while (valBuffer != NULL)
	{
		if (strcmp(valBuffer, "\\") == 0)
		{
			valBuffer = strtok(NULL, "\\");
			continue;
		}
		removeWhiteSpaces(valBuffer);

		strcat(val, valBuffer);
		strcat(val, " ");

		valBuffer = strtok(NULL, "\\");
	}
	defineWithValue(map, key, val);
	free(multiLineBuffer);
	free(val);

	return 1;
}

int undefineHandler(HashMap *map)
{
	char *key = strtok(NULL, SPACE_DEL);

	if (search(map, key))
	{
		delete (map, key);
	}

	return 1;
}

int includeHandler(HashMap *map, FILE *output_file, char **dir_paths, int dir_pathsCounter)
{
	int foundHeader = 0; // Found the include file.

	char *fileNameBuffer = strtok(NULL, SPACE_DEL); // text after #include
	fileNameBuffer++;																// ignore first " or first <

	// compute the length of the file name
	char *end = strstr(fileNameBuffer, "\"");
	int len = end - fileNameBuffer;

	char *fileName = (char *)calloc(len + 1, sizeof(char));
	if (fileName == NULL)
	{
		return 12;
	}
	memcpy(fileName, fileNameBuffer, len);

	// if header file exists with the path after #include
	FILE *header_file = fopen(fileName, "r");
	if (header_file != NULL)
	{
		foundHeader = 1;
		// write the content of the header
		if (handleInputFile(header_file, output_file, map, dir_paths, dir_pathsCounter) == 12)
		{
			return 12;
		}
		fclose(header_file);
	}
	// else try to append it to all paths received as
	// parameters or to the current path
	else
	{
		for (int j = 0; j <= dir_pathsCounter; j++)
		{
			char *path = (char *)calloc(1, 50);
			strcpy(path, dir_paths[j]);
			strcat(path, fileName);

			header_file = fopen(path, "r");
			if (header_file != NULL)
			{
				foundHeader = 1;
				handleInputFile(header_file, output_file, map, dir_paths, dir_pathsCounter);
				fclose(header_file);
			}
			free(path);
			if (foundHeader)
				break;
		}
	}

	if (!foundHeader)
	{
		return 2;
	}

	free(fileName);
	return 1;
}

int ifHandler(HashMap *map)
{
	char *cond = strtok(NULL, SPACE_DEL);

	// check if condition is something
	// defined in hashmap
	char *res = search(map, cond);
	if (res)
	{
		strcpy(cond, res);
	}

	// check if condition is valid
	if (strcmp(cond, "0"))
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

int elseHandler(int conditionValid)
{
	if (conditionValid)
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

int elifHandler(HashMap *map, int conditionValid)
{
	char *cond = strtok(NULL, SPACE_DEL);

	char *res = search(map, cond);
	if (res)
	{
		strcpy(cond, res);
	}

	if (strcmp(cond, "0"))
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

int ifdefHandler(HashMap *map)
{
	char *cond = strtok(NULL, SPACE_DEL);

	if (search(map, cond))
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

int ifndefHandler(HashMap *map)
{
	char *cond = strtok(NULL, SPACE_DEL);
	if (search(map, cond) == NULL)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

int quoteHandler(HashMap *map, char *buffer, char *startPos, char **endPos, char *startBuffer, char *endBuffer, char **res)
{
	memcpy(startBuffer, buffer, startPos - buffer);
	startBuffer[startPos - buffer] = '\0';

	// search for ",
	*endPos = strstr(buffer, "\",");

	if (*endPos == NULL)
	{
		// if not found, then search for ")
		*endPos = strstr(buffer, "\")");
		// closingType = 2; // ")
	}

	if (*endPos)
	{
		*res = (char *)calloc(sizeof(char), strlen(startPos) + 1);
		// save in res the content between " "
		memcpy(*res, startPos + 1, *endPos - (startPos + 1));
		// //fprintf(stderr, "res: %s\n", res);

		// save in endBuffer the content after ""
		strcpy(endBuffer, *endPos);

		// replace in the content after "" all occurences
		// with hashmap values
		char *endendAux = (char *)calloc(1, strlen(endBuffer) + 1);
		strcpy(endendAux, endBuffer);
		char *endtok = strtok(endendAux, ALL_DEL);
		while (endtok != NULL)
		{
			char *endres = search(map, endtok);
			if (endres != NULL)
			{
				replace(endBuffer, endtok, endres);
			}

			endtok = strtok(NULL, ALL_DEL);
		}

		free(endendAux);
	}
}

int handleInputFile(FILE *input_file, FILE *output_file, HashMap *map, char **dir_paths, int dir_pathsCounter)
{
	char buffer[BUF_LEN];
	int conditionValid = 1;

	while (fgets(buffer, BUF_LEN - 1, input_file))
	{
		buffer[strcspn(buffer, "\n")] = 0;

		int isDirective = 1; // To remember if there is a directive in this line.
		int skipLine = 0;		 // Skip the print of the line.
		// int foundHeader = 0; // Found an include directive.

		for (int i = 0; i < strlen(buffer); i++)
		{
			char currChar = buffer[i];
			if (currChar == HASH && isDirective)
			{
				skipLine = 1; // This line won't be printed.

				char *bufferAux = (char *)calloc(1, strlen(buffer) + 1);
				strcpy(bufferAux, buffer);
				char *afterHashtag = bufferAux + 1;

				char *type = strtok(afterHashtag, SPACE_DEL);
				if (strcmp(type, DEFINE_STR) == 0)
				{
					char *isMultiLine = strstr(buffer, "\\");
					if (isMultiLine)
					{
						multiLineDefineHandler(map, input_file, buffer);
						free(bufferAux);
						continue;
					}

					if (defineHandler(map) == 12)
					{
						return 12;
					}
				}
				else if (strcmp(type, UNDEF_STR) == 0)
				{
					if (undefineHandler(map) == 12)
					{
						return 12;
					}
				}
				else if (strcmp(type, INCLUDE_STR) == 0)
				{
					int res = includeHandler(map, output_file, dir_paths, dir_pathsCounter);
					if (res == 12)
					{
						return 12;
					}
					else if (res == 2)
					{
						return 2;
					}
				}
				else if (strcmp(type, IF_STR) == 0)
				{
					conditionValid = ifHandler(map);
				}
				else if (strcmp(type, ELSE_STR) == 0)
				{
					conditionValid = elseHandler(conditionValid);
				}
				else if (strcmp(type, ELIF_STR) == 0)
				{
					conditionValid = elifHandler(map, conditionValid);
				}
				else if (strcmp(type, ENDIF_STR) == 0)
				{
					conditionValid = 1;
				}
				else if (strcmp(type, IFDEF_STR) == 0)
				{
					conditionValid = ifdefHandler(map);
				}
				else if (strcmp(type, IFNDEF_STR) == 0)
				{
					conditionValid = ifndefHandler(map);
				}

				free(bufferAux);
			}
			else
			{
				isDirective = 0;
			}
		}

		if (skipLine || strcmp(buffer, "") == 0 || !conditionValid)
		{
			continue;
		}

		// pointer to first " occurence
		char *startPos = strstr(buffer, "\"");

		char *startBuffer = (char *)calloc(1, 50);
		char *endBuffer = (char *)calloc(1, 50);

		char *res, *endPos; // res - copy of content between " "
		// end - pointer to the second " occurence
		int closingType = 1; // ",

		if (startPos)
		{
			quoteHandler(map, buffer, startPos, &endPos, startBuffer, endBuffer, &res);
		}

		char *bufferAux = (char *)calloc(1, strlen(buffer) + 1);
		if (bufferAux == NULL)
		{
			//fprintf(stderr, "MALLOC ERR 387\n");

			exit(12);
		}
		strcpy(bufferAux, buffer);
		char *tok = strtok(bufferAux, ALL_DEL);
		while (tok != NULL)
		{
			char *res = search(map, tok);
			if (res != NULL)
			{
				replace(buffer, tok, res);
			}
			tok = strtok(NULL, ALL_DEL);
		}

		if (startPos)
		{
			strcpy(buffer, "");
			strcat(buffer, startBuffer);
			strcat(buffer, "\"");
			strcat(buffer, res);
			strcat(buffer, endBuffer);
			free(res);
		}
		free(bufferAux);
		free(startBuffer);
		free(endBuffer);
		fprintf(output_file, "%s\n", buffer);
	}

	return 1;
}

int initArgs(
		int argc,
		char *argv[],
		HashMap *map,
		FILE **input_file,
		FILE **output_file,
		int *isInputFile,
		int *isOutputFile,
		char **dir_paths,
		int *dir_pathsCounter)
{
	for (int i = 1; i < argc; i++)
	{
		char *current = argv[i];
		if (strcmp(current, D_ARGUMENT) == 0)
		{
			char *DInput = argv[i + 1];
			handleDInput(map, DInput);
			i++;
		}
		else if (strncmp(current, D_ARGUMENT, 2) == 0)
		{
			char *DInput = current + 2;
			handleDInput(map, DInput);
		}
		else if (strcmp(current, I_ARGUMENT) == 0)
		{
			char *dir_path = (char *)calloc(1, 50);
			if (dir_path == NULL)
			{
				//fprintf(stderr, "Malloc err 450.\n");
				return 12;
			}
			(*dir_pathsCounter)++;
			strcpy(dir_path, argv[i + 1]);
			strcat(dir_path, "/");
			strcpy(dir_paths[*dir_pathsCounter], dir_path);
			i++;
			free(dir_path);
		}
		else if (strncmp(current, I_ARGUMENT, 2) == 0)
		{
			char *dir_path = (char *)calloc(1, 50);
			if (dir_path == NULL)
			{
				//fprintf(stderr, "Malloc err 462.\n");
				return 12;
			}
			(*dir_pathsCounter)++;
			strcpy(dir_path, current + 2);
			strcat(dir_path, "/");
			strcpy(dir_paths[*dir_pathsCounter], dir_path);
			i++;
		}
		else if (strcmp(current, O_ARGUMENT) == 0)
		{
			*output_file = fopen(argv[i + 1], "w");
			if (output_file == NULL)
			{
				//fprintf(stdout, "Input file <%s> failed to open.\n", current);
			}
			*isOutputFile = 1;
			i++;
		}
		else if (strncmp(current, O_ARGUMENT, 2) == 0)
		{
			*output_file = fopen(current + 2, "w");
			if (output_file == NULL)
			{
				//fprintf(stdout, "Input file <%s> failed to open.\n", current);
			}
		}
		else if (*isInputFile == 0)
		{
			*input_file = fopen(current, "r");
			if (input_file == NULL)
			{
				//fprintf(stdout, "Input file <%s> failed to open.\n", current);
				exit(0);
			} 

			*isInputFile = 1;

		} 
		else if (*isOutputFile == 0)
		{
			*output_file = fopen(current, "w");
			if (output_file == NULL)
			{
				//fprintf(stdout, "Input file <%s> failed to open.\n", current);
				exit(0);
			} 
			
			*isOutputFile = 1;
		}
		else
		{
			return 2;
		}
	}

	return 1;
}

int main(int argc, char *argv[])
{
	HashMap map = mapInit();
	FILE *input_file = stdin;
	FILE *output_file = stdout;

	int isInputFile = 0;
	int isOutputFile = 0;

	char **dir_paths = (char **)calloc(50, sizeof(char *));
	if (dir_paths == NULL)
	{
		//fprintf(stderr, "Malloc err 509.\n");
		return 12;
	}

	for (int i = 0; i < 50; i++)
	{
		dir_paths[i] = (char *)calloc(50, sizeof(char));
		if (dir_paths[i] == NULL)
		{
			//fprintf(stderr, "Malloc err 515\n");
			return 12;
		}
	}

	strcpy(dir_paths[0], "./_test/inputs/");
	int dir_pathsCounter = 0;

	int res = initArgs(argc, argv, &map, &input_file, &output_file, &isInputFile, &isOutputFile, dir_paths, &dir_pathsCounter);
	if (res == 12)
	{
		return 12;
	}

	if (isInputFile && input_file == stdin) {
		return 2;
	}


	if (isInputFile)
	{
		if (isOutputFile)
		{
			int res = handleInputFile(input_file, output_file, &map, dir_paths, dir_pathsCounter);
			if (res == 12)
			{
				return 12;
			}
			else if (res == 2)
			{
				return 2;
			}
		}
		else
		{
			int res = handleInputFile(input_file, stdout, &map, dir_paths, dir_pathsCounter);
			if (res == 12)
			{
				return 12;
			}
			else if (res == 2)
			{
				return 2;
			}
		}
	}
	else
	{
		if (isOutputFile)
		{
			int res = handleInputFile(stdin, output_file, &map, dir_paths, dir_pathsCounter);
			if (res == 12)
			{
				return 12;
			}
			else if (res == 2)
			{
				return 2;
			}
		}
		else
		{
			int res = handleInputFile(stdin, stdout, &map, dir_paths, dir_pathsCounter);
			if (res == 12)
			{
				return 12;
			}
			else if (res == 2)
			{
				return 2;
			}
		}
	}

	fclose(input_file);
	fclose(output_file);

	for (int i = 0; i < map.currentSize; i++)
	{
		free(map.items[i].value);
	}
	free(map.items);

	for (int i = 0; i < 50; i++)
	{
		free(dir_paths[i]);
	}

	free(dir_paths);

	return 0;
}

void replace(
		char *buffer,
		char *toReplace,
		char *replacement)
{
	char *occurence = strstr(buffer, toReplace);

	char *aux = (char *)calloc(1, BUF_LEN + 1);
	if (aux == NULL)
	{
		//fprintf(stdout, "Malloc err 42.\n");
		exit(12);
	}

	strcpy(aux, buffer);

	int index = occurence - buffer;
	buffer[index] = '\0';

	strcat(buffer, replacement);
	strcat(buffer, aux + index + strlen(toReplace));

	free(aux);
}
