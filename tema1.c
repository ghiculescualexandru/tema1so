#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

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

#define EQ_DEL "="
#define SPACE_DEL " "
#define ALL_DEL "[\t []{}<>=+-*/%!&|^.,:;()\"]"

#define NPTRS 2

typedef struct HashItem {
	char key[KEY_SIZE];
	char *value;
}
HashItem;

typedef struct HashMap {
	int currentSize;
	int maxSize;
	HashItem *items;
}
HashMap;

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
int replace(char *buffer, char *toReplace, char *replacement);
int handleInputFile(FILE *input_file, FILE *output_file, HashMap *map,
	char **dir_paths, int dir_pathsCounter);
int includeHandler(HashMap *map, FILE *output_file, char **dir_paths,
	int dir_pathsCounter);

HashMap mapInit(void)
{
	HashMap map;

	map.maxSize = MAP_MAX_SIZE;
	map.currentSize = 0;
	map.items = (HashItem *) calloc(MAP_INITIAL_SIZE, sizeof(HashItem));

	return map;
}

int hashItemInit(HashItem *item, char key[KEY_SIZE], char *value)
{
	item->value = (char *)calloc(sizeof(char), strlen(value) + 1);
	if (item->value == NULL)
		return -ENOMEM;

	strcpy(item->key, key);
	strcpy(item->value, value);

	return 1;
}

void insert(HashMap *map, HashItem item)
{
	int itemExists = 0, i;

	for (i = 0; i < map->currentSize; i++) {
		HashItem currItem = map->items[i];

		if (strcmp(currItem.key, item.key) == 0)
			itemExists = 1;
	}

	if (!itemExists) {
		map->items[map->currentSize] = item;
		map->currentSize++;
	}
}

void put(HashMap *map, char *key, char *val)
{
	int i;

	for (i = 0; i < map->currentSize; i++) {
		HashItem currItem = map->items[i];

		if (strcmp(currItem.key, key) == 0)
			strcpy(currItem.value, val);
	}
}

char *search(HashMap *map, char key[KEY_SIZE])
{
	int i;

	for (i = 0; i < map->currentSize; i++) {
		HashItem currItem = map->items[i];

		if (strcmp(currItem.key, key) == 0)
			return currItem.value;
	}

	return NULL;
}

void delete(HashMap *map, char key[KEY_SIZE])
{
	int i, j;
	HashItem currItem;

	for (i = 0; i < map->currentSize; i++) {
		currItem = map->items[i];
		if (strcmp(currItem.key, key) == 0) {
			for (j = i; j < map->currentSize; j++)
				map->items[j] = map->items[j + 1];

			map->currentSize--;
		}
		free(currItem.value);
	}
}

int handleDInput(HashMap *map, char *DInput)
{
	char  *key = strtok(DInput, EQ_DEL);
	char *val = strtok(NULL, EQ_DEL);

	if (val == NULL) {
		val = (char *)calloc(1, sizeof(char));
		if (val == NULL)
			return -ENOMEM;

		strcpy(val, "");
	}

	if (search(map, key) == NULL) {
		HashItem newItem;

		if (hashItemInit(&newItem, key, val) == -ENOMEM)
			return -ENOMEM;

		insert(map, newItem);
	}

	if (strcmp(val, "") == 0)
		free(val);

	return 1;
}

/********************************/
/*DIRECTIVE HELPERS/AUXILIARY */
/******************************/

int defineWithValue(HashMap *map, char *key, char *val)
{
	char *tok, *replacement;
	// for define in define
	char *valBuffer = (char *)calloc(1, BUF_LEN);

	if (valBuffer == NULL)
		return -ENOMEM;

	strcpy(valBuffer, val);

	// replace all values in define with
	// existing values if any
	tok = strtok(valBuffer, ALL_DEL);
	while (tok != NULL) {
		replacement = search(map, tok);
		if (replacement != NULL) {
			if (replace(val, tok, replacement) == -ENOMEM)
				return -ENOMEM;
		}
		tok = strtok(NULL, ALL_DEL);
	}

	// insert or update key in map
	// using the computed val (replaced with
	// other map values if any)
	if (search(map, key) == NULL) {
		HashItem newItem;

		if (hashItemInit(&newItem, key, val) == -ENOMEM)
			return -ENOMEM;

		insert(map, newItem);
	} else {
		put(map, key, val);
	}

	free(valBuffer);
	return 1;
}

int defineWithoutValue(HashMap *map, char *key)
{
	char *val = (char *)calloc(1, 2);

	if (val == NULL)
		return -ENOMEM;

	strcpy(val, "");

	// insert the key, value pair only
	// if the key is not defined already
	if (search(map, key) == NULL) {
		HashItem newItem;

		if (hashItemInit(&newItem, key, val) == -ENOMEM)
			return -ENOMEM;

		insert(map, newItem);
	}

	free(val);
	return 1;
}

/***********************/
/*DIRECTIVE HANDLERS */
/*********************/

int defineHandler(HashMap *map)
{
	char *key = strtok(NULL, SPACE_DEL);
	char *val = strtok(NULL, "\n");

	// check if define has a key or the item is just defined
	if (val) {
		if (defineWithValue(map, key, val) == -ENOMEM)
			return -ENOMEM;
	} else {
		if (defineWithoutValue(map, key) == -ENOMEM)
			return -ENOMEM;
	}

	return 1;
}

int removeWhiteSpaces(char *buffer)
{
	int i = 0;
	char *aux;

	aux = (char *) calloc(1, strlen(buffer) + 1);
	if (aux == NULL)
		return -ENOMEM;

	strcpy(aux, buffer);
	for (; i < strlen(buffer); i++) {
		if (aux[i] != ' ')
			break;
	}

	strcpy(buffer, aux + i);
	free(aux);
	return 1;
}

int multiLineDefineHandler(HashMap *map, FILE *input_file, char *buffer)
{
	char *tok, *key, *val, *valBuffer, *multiLineBuffer;

	multiLineBuffer = (char *)calloc(1, BUF_LEN);
	if (multiLineBuffer == NULL)
		return -ENOMEM;

	strcpy(multiLineBuffer, buffer);

	while (1) {
		fgets(buffer, BUF_LEN - 1, input_file);
		buffer[strcspn(buffer, "\n")] = 0;

		strcat(multiLineBuffer, buffer);
		if (buffer[strlen(buffer) - 1] != '\\')
			break;
	}

	tok = strtok(multiLineBuffer, ALL_DEL);
	key = strtok(NULL, SPACE_DEL);
	val = (char *) calloc(1, 50);
	if (val == NULL)
		return -ENOMEM;

	valBuffer = strtok(NULL, SPACE_DEL);
	while (valBuffer != NULL) {
		if (strcmp(valBuffer, "\\") == 0) {
			valBuffer = strtok(NULL, "\\");
			continue;
		}
		if (removeWhiteSpaces(valBuffer) == -ENOMEM)
			return -ENOMEM;

		strcat(val, valBuffer);
		strcat(val, " ");

		valBuffer = strtok(NULL, "\\");
	}
	val[strlen(val) - 1] = '\0';
	if (defineWithValue(map, key, val) == -ENOMEM)
		return -ENOMEM;

	free(multiLineBuffer);
	free(val);

	return 1;
}

int undefineHandler(HashMap *map)
{
	char *key = strtok(NULL, SPACE_DEL);

	if (search(map, key))
		delete(map, key);

	return 1;
}

int includeHandler(HashMap *map, FILE *output_file, char **dir_paths,
	int dir_pathsCounter)
{
	int foundHeader = 0, len; // Found the include file.
	char *fileNameBuffer, *end, *fileName;
	FILE *header_file;

	fileNameBuffer = strtok(NULL, SPACE_DEL); // text after #include
	fileNameBuffer++; // ignore first " or first <

	// compute the length of the file name
	end = strstr(fileNameBuffer, "\"");
	len = end - fileNameBuffer;

	fileName = (char *)calloc(len + 1, sizeof(char));
	if (fileName == NULL)
		return -ENOMEM;

	memcpy(fileName, fileNameBuffer, len);

	// if header file exists with the path after #include
	header_file = fopen(fileName, "r");
	if (header_file != NULL) {
		foundHeader = 1;
		// write the content of the header
		if (handleInputFile(header_file, output_file, map, dir_paths,
			dir_pathsCounter) == -ENOMEM)
			return -ENOMEM;

		fclose(header_file);
	}
	// else try to append it to all paths received as
	// parameters or to the current path
	else {
		int j;
		char *path;

		for (j = 0; j <= dir_pathsCounter; j++) {
			path = (char *)calloc(1, 50);
			if (path == NULL)
				return -ENOMEM;

			strcpy(path, dir_paths[j]);
			strcat(path, fileName);

			header_file = fopen(path, "r");
			if (header_file != NULL) {
				foundHeader = 1;
				handleInputFile(header_file, output_file,
					map, dir_paths, dir_pathsCounter);
				fclose(header_file);
			}

			free(path);
			if (foundHeader)
				break;
		}
	}

	if (!foundHeader)
		return 2;

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
		strcpy(cond, res);

	// check if condition is valid
	if (strcmp(cond, "0"))
		return 1;
	else
		return 0;
}

int elseHandler(int conditionValid)
{
	if (conditionValid)
		return 0;
	else
		return 1;
}

int elifHandler(HashMap *map, int conditionValid)
{
	char *cond = strtok(NULL, SPACE_DEL);
	char *res = search(map, cond);

	if (res)
		strcpy(cond, res);

	if (strcmp(cond, "0"))
		return 1;
	else
		return 0;
}

int ifdefHandler(HashMap *map)
{
	char *cond = strtok(NULL, SPACE_DEL);

	if (search(map, cond))
		return 1;
	else
		return 0;
}

int ifndefHandler(HashMap *map)
{
	char *cond = strtok(NULL, SPACE_DEL);

	if (search(map, cond) == NULL)
		return 1;
	else
		return 0;
}

int quoteHandler(HashMap *map, char *buffer, char *startPos, char **endPos,
	char *startBuffer, char *endBuffer, char **res)
{
	memcpy(startBuffer, buffer, startPos - buffer);
	startBuffer[startPos - buffer] = '\0';

	// search for ",
	*endPos = strstr(buffer, "\",");

	if (*endPos == NULL)
		*endPos = strstr(buffer, "\")");

	if (*endPos) {
		char *endendAux, *endtok;

		*res = (char *)calloc(sizeof(char), strlen(startPos) + 1);
		if (res == NULL)
			return -ENOMEM;

		// save in res the content between " "
		memcpy(*res, startPos + 1, *endPos - (startPos + 1));
		// //fprintf(stderr, "res: %s\n", res);

		// save in endBuffer the content after ""
		strcpy(endBuffer, *endPos);

		// replace in the content after "" all occurences
		// with hashmap values
		endendAux = (char *) calloc(1, strlen(endBuffer) + 1);
		if (endendAux == NULL)
			return -ENOMEM;

		strcpy(endendAux, endBuffer);
		endtok = strtok(endendAux, ALL_DEL);
		while (endtok != NULL) {
			char *endres;

			endres = search(map, endtok);
			if (endres != NULL) {
				if (replace(endBuffer, endtok, endres)
					== -ENOMEM)
					return -ENOMEM;
			}

			endtok = strtok(NULL, ALL_DEL);
		}

		free(endendAux);
	}
}

int defineAux(HashMap *map, FILE *input_file, char *buffer)
{
	if (buffer[strlen(buffer) - 1] == '\\') {
		if (multiLineDefineHandler(map, input_file, buffer) == -ENOMEM)
			return -ENOMEM;

		return 2;
	}

	return 1;
}

int handleInputFile(FILE *input_file, FILE *output_file, HashMap *map,
	char **dir_paths, int dir_pathsCounter)
{
	char buffer[BUF_LEN];
	int conditionValid = 1;

	while (fgets(buffer, BUF_LEN - 1, input_file)) {
		int isDirective = 1, skipLine = 0,
		directiveInConditionValid, i; // To remember if there is a directive in this line.
		char *startPos, *startBuffer, *endBuffer, *res,
		*endPos, *bufferAux, *tok;
		// Skip the print of the line.

		buffer[strcspn(buffer, "\n")] = 0;

		directiveInConditionValid = conditionValid;

		for (i = 0; i < strlen(buffer); i++) {
			char currChar;

			currChar = buffer[i];
			if (currChar == HASH && isDirective) {
				char *bufferAux, *afterHashtag, *type;

				skipLine = 1; // This line won't be printed.

				bufferAux = (char *) calloc(1,
					strlen(buffer) + 1);
				if (bufferAux == NULL)
					return -ENOMEM;

				strcpy(bufferAux, buffer);
				afterHashtag = bufferAux + 1;

				type = strtok(afterHashtag, SPACE_DEL);
				if (strcmp(type, DEFINE_STR) == 0) {
					int res;

					if (!directiveInConditionValid)
						continue;

					res = defineAux(map, input_file,
					 buffer);

					if (res == -ENOMEM)
						return -ENOMEM;
					else if (res == 2) {
						free(bufferAux);
						continue;
					}

					if (defineHandler(map) == -ENOMEM)
						return -ENOMEM;
				} else if (strcmp(type, UNDEF_STR) == 0) {
					if (!directiveInConditionValid)
						continue;

					if (undefineHandler(map) == -ENOMEM)
						return -ENOMEM;
				} else if (strcmp(type, INCLUDE_STR) == 0) {
					int res;

					if (!directiveInConditionValid)
						continue;

					res = includeHandler(map,
						output_file, dir_paths,
						dir_pathsCounter);
					if (res == -ENOMEM)
						return -ENOMEM;
					else if (res == 2)
						return 2;
				} else if (strcmp(type, IF_STR) == 0) {
					if (!directiveInConditionValid)
						continue;

					conditionValid = ifHandler(map);
				} else if (strcmp(type, ELSE_STR) == 0) {
					conditionValid = elseHandler(
						conditionValid);
					directiveInConditionValid =
						conditionValid;
				} else if (strcmp(type, ELIF_STR) == 0) {
					conditionValid = elifHandler(map,
						conditionValid);
					directiveInConditionValid =
						conditionValid;
				} else if (strcmp(type, ENDIF_STR) == 0) {
					conditionValid = 1;
					directiveInConditionValid =
						conditionValid;
				} else if (strcmp(type, IFDEF_STR) == 0) {
					if (!directiveInConditionValid)
						continue;

					conditionValid = ifdefHandler(map);
				} else if (strcmp(type, IFNDEF_STR) == 0) {
					if (!directiveInConditionValid)
						continue;

				conditionValid = ifndefHandler(map);
				}

				free(bufferAux);
			} else {
				isDirective = 0;
			}
		}

		if (skipLine || strcmp(buffer, "") == 0 ||
			conditionValid == 0)
			continue;
		startPos = strstr(buffer, "\"");

		startBuffer = (char *) calloc(1, 50);
		if (startBuffer == NULL)
			return -ENOMEM;

		endBuffer = (char *) calloc(1, 50);
		if (endBuffer == NULL)
			return -ENOMEM;

		if (startPos) {
			if (quoteHandler(map, buffer, startPos, &endPos,
				startBuffer, endBuffer, &res) == -ENOMEM)
				return -ENOMEM;
		}

		bufferAux = (char *) calloc(1, strlen(buffer) + 1);
		if (bufferAux == NULL)
			return -ENOMEM;
		strcpy(bufferAux, buffer);
		tok = strtok(bufferAux, ALL_DEL);
		while (tok != NULL) {
			char *res;

			res = search(map, tok);
			if (res != NULL) {
				if (replace(buffer, tok, res) == -ENOMEM)
					return -ENOMEM;
			}
			tok = strtok(NULL, ALL_DEL);
		}

		if (startPos) {
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
	int foundInputFile = 0;
	int foundOutputFile = 0;
	int i;

	for (i = 1; i < argc; i++) {
		char *current = argv[i];

		if (strcmp(current, D_ARGUMENT) == 0) {
			char *DInput = argv[i + 1];

			if (handleDInput(map, DInput) == -ENOMEM)
				return -ENOMEM;
			i++;
		} else if (strncmp(current, D_ARGUMENT, 2) == 0) {
			char *DInput = current + 2;

			if (handleDInput(map, DInput) == -ENOMEM)
				return -ENOMEM;
		} else if (strcmp(current, I_ARGUMENT) == 0) {
			char *dir_path = (char *)calloc(1, 50);

			if (dir_path == NULL)
				return -ENOMEM;
			(*dir_pathsCounter)++;
			strcpy(dir_path, argv[i + 1]);
			strcat(dir_path, "/");
			strcpy(dir_paths[*dir_pathsCounter], dir_path);
			i++;
			free(dir_path);
		} else if (strncmp(current, I_ARGUMENT, 2) == 0) {
			char *dir_path = (char *)calloc(1, 50);

			if (dir_path == NULL)
				return -ENOMEM;
			(*dir_pathsCounter)++;
			strcpy(dir_path, current + 2);
			strcat(dir_path, "/");
			strcpy(dir_paths[*dir_pathsCounter], dir_path);
			i++;
		} else if (strcmp(current, O_ARGUMENT) == 0) {
			*output_file = fopen(argv[i + 1], "w");
				return 2;
			*isOutputFile = 1;
			i++;
		} else if (strncmp(current, O_ARGUMENT, 2) == 0) {
			*output_file = fopen(current + 2, "w");
				return 2;
		} else if (*isInputFile == 0) {
			foundInputFile = 1;
			*input_file = fopen(current, "r");
			if (*input_file == NULL)
				return 2;

			*isInputFile = 1;

		} else if (*isOutputFile == 0) {
			foundOutputFile = 1;
			*output_file = fopen(current, "w");
			if (*output_file == NULL)
				return 2;

			*isOutputFile = 1;
		} else
			return 2;

	}

	return 1;
}

int main(int argc, char *argv[])
{
	FILE *input_file, *output_file;
	HashMap map = mapInit();
	int isInputFile, isOutputFile, res, i, dir_pathsCounter;
	char **dir_paths;

	if (map.items == NULL)
		return -ENOMEM;

	input_file = stdin;
	output_file = stdout;

	isInputFile = 0;
	isOutputFile = 0;

	dir_paths = (char **) calloc(50, sizeof(char *));
	if (dir_paths == NULL)
		return -ENOMEM;

	for (i = 0; i < 50; i++) {
		dir_paths[i] = (char *) calloc(50, sizeof(char));
		if (dir_paths[i] == NULL)
			return -ENOMEM;
	}

	strcpy(dir_paths[0], "./_test/inputs/");
	dir_pathsCounter = 0;

	res = initArgs(argc, argv, &map, &input_file, &output_file,
		&isInputFile, &isOutputFile, dir_paths, &dir_pathsCounter);
	if (res == -ENOMEM)
		return -ENOMEM;

	if (res == 2)
		return 2;

	if (isInputFile) {
		if (isOutputFile) {
			int res = handleInputFile(input_file, output_file,
				&map, dir_paths, dir_pathsCounter);

			if (res == -ENOMEM)
				return -ENOMEM;
			else if (res == 2)
				return 2;
		} else {
			int res = handleInputFile(input_file, stdout, &map,
				dir_paths, dir_pathsCounter);

			if (res == -ENOMEM)
				return -ENOMEM;
			else if (res == 2)
				return 2;
		}
	} else {
		if (isOutputFile) {
			int res = handleInputFile(stdin, output_file, &map,
				dir_paths, dir_pathsCounter);

			if (res == -ENOMEM)
				return -ENOMEM;
			else if (res == 2)
				return 2;

		} else {
			int res = handleInputFile(stdin, stdout, &map,
				dir_paths, dir_pathsCounter);

			if (res == -ENOMEM)
				return -ENOMEM;
			else if (res == 2)
				return 2;

		}
	}

	fclose(input_file);
	fclose(output_file);

	for (i = 0; i < map.currentSize; i++)
		free(map.items[i].value);

	free(map.items);

	for (i = 0; i < 50; i++)
		free(dir_paths[i]);

	free(dir_paths);

	return 0;
}

int replace(
	char *buffer,
	char *toReplace,
	char *replacement)
{
	char *occurence, *aux;
	int index;

	occurence = strstr(buffer, toReplace);

	aux = (char *) calloc(1, BUF_LEN + 1);
	if (aux == NULL)
		return -ENOMEM;

	strcpy(aux, buffer);

	index = occurence - buffer;
	buffer[index] = '\0';

	strcat(buffer, replacement);
	strcat(buffer, aux + index + strlen(toReplace));

	free(aux);
	return 1;
}
