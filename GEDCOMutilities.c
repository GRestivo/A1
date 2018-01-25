#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "GEDCOMparser.h"
#include "GEDCOMutilities.h"
#include "LinkedListAPI.h"

/*will also have to create an array or list to collect all the xref_ID's to check for them on
another run through*/

//when creating individual lists and fam lists, set individual  as strong and fam as week

/*skip blank lines*/
/*for printGEDCOM you must truncate printing the version in certain amount of decimals?*/

Line *createLine(int level, char *xref_ID, char *tag, char *lineVal, int lineNum) {
  Line *l = malloc(sizeof(Line));
  l->level = level;
  l->xref_ID = xref_ID;
  l->tag = tag;
  l->lineVal = lineVal;
  l->lineNum = lineNum;
  return l;
}

int parser (int argc, char **argv) { /*must pass data structure*/
  char tempLine[1000];
  int lineNum = 0,i,j;
  FILE *fPtr;
  Header *h = NULL;
  char *submID = malloc(sizeof(char) * 100);
  char *tempVersion = malloc(sizeof(char) * 100);
  Line *l;
  Node *curr;
  Submitter *s = NULL;

  //while loop to ensure all xrefID's point to something real and exist

  /*Error test to see if there are the correct number of command line arguments, if not end program*/
  if (argc != 2) {
    printf("Error! Incorrect amount of command line arguments!\n");
    return -1;
  }

  /*Opens given file for reading*/
  fPtr = fopen(argv[1], "r");

  /*Sees if file opening was successful, if not end program*/
  if (fPtr == NULL) {
    printf("Error! Could not find designated file.\n");
    return -1;
  }

  //Initialize Lists
  List headerList = initializeList(&printLine,&dummyDelete,&compareFields);
  List wholeFile = initializeList(&printLine, &deleteLine, &compareFields);
  List submitterList = initializeList(&printLine, &dummyDelete, &compareFields);
  List familyList = initializeList(&printFamily, &deleteFamily, &compareFamilies);
  List individualList = initializeList(&printIndividual, &deleteIndividual, &compareIndividuals);


  //While loop that reads whole file into list of lines
  while (fgets(tempLine, 999, fPtr) != NULL) {
    lineNum++;
    if (tempLine[0] == '0' && isdigit(tempLine[1]) != 0) {
      printf("Level Num error\n");
      //error MSB 0 before level number
      //any other basic error checks to do here
    }
    l = parseLine(tempLine);
    l->lineNum = lineNum;
    insertBack(&wholeFile, l);
  }

  //printf("||||||||||||||||||||||||||||||||WholeFile:\n%s\n||||||||||||||||||||||||||||||\n", toString(wholeFile));

  //Loop to remove /'s from any name field
  curr = wholeFile.head;
  for (i = 0; i < wholeFile.length; i++) {
    l = (Line*)curr->data;
    if (strcmp(l->tag,"NAME") == 0) {
      for (j = 0; j < strlen(l->lineVal); j++) {
        if (l->lineVal[j] == '/') {
          while (l->lineVal[j + 1] != '/') {
            l->lineVal[j] = l->lineVal[j + 1];
            j++;
          }
        break;
        }
      }
      l->lineVal[j] = '\0';
    }
    curr = curr->next;
  }

  //Loop that takes header portion and parses it
  curr = wholeFile.head;
  l = (Line*)curr->data;
  while (curr != NULL) {
    l = (Line*)curr->data;
    if (strcmp(l->tag, "HEAD") == 0) {
       curr = curr->next;
    } else if (l->level == 0) {
       break;
    } else {
      insertBack(&headerList, l);
      curr = curr->next;
    }
  }
  //printf("||||||||||||||HeaderList: \n%s\n|||||||||||||||||\n", toString(headerList));

  /*
  sprintf(string,"%%",5);
  
  */


  curr = headerList.head;

  float *gedcVersion = NULL;
  float tempFVersion;
  CharSet encoding;
  char *source = malloc(sizeof(char) * 200);
  //tempVersion = malloc(sizeof(char) * 20);
  List headerOtherFields = initializeList(&printField, &deleteField, &compareFields);
  GEDCOMerror returnErr = parseHeader(&headerList, h, submID, source, tempVersion, &encoding, &headerOtherFields);
  tempFVersion = atof(tempVersion);
  gedcVersion = &tempFVersion;
  h = createHeader(source, *gedcVersion, encoding, NULL, &headerOtherFields);
  free(source);
  free(tempVersion);

  //Loop that finds submitter and parses it
  curr = wholeFile.head;
  while (curr != NULL) {
    l = (Line*)curr->data;
    if (strcmp(l->xref_ID, submID) == 0) {
      curr = curr->next;
      l = (Line*)curr->data;
      while (l->level != 0) {
        insertBack(&submitterList, l);
        curr = curr->next;
        l = (Line*)curr->data;
      }
      break;
    }
    curr = curr->next;
 }

  //statements to create submitter
  List otherFields = initializeList(&printField,&deleteField,&compareFields);
  char *submitterName = malloc(sizeof(char) * 200);
  char *addr = malloc(sizeof(char) * 250);
  s = createSubmitter(submitterName, &otherFields, addr);
  //check for return error
  h->submitter = s;

  //printf("Submitter: \n!!!%s\n||||||||||||||||||||||||||||||\n", toString(submitterList));

  printf("1\n");

  returnErr = parseIndividuals (&wholeFile, &individualList, &familyList);
  //check for error return

  printf("Before free's\n");

  //Clear and free pointers
  //ALL OF THIS WILL HAPPEN IN DELETE GEDCOM
  clearList(&headerList);
  clearList(&wholeFile);
  clearList(&submitterList);
  clearList(&individualList);
  free(submID);
  printf("End\n");

return 1;
}

Line *parseLine (char *lineToParse) {
  char tempLine[1000], tempLvl[10];
  char *token, *tempToken, *tag, *xref_ID, *lineVal;
  const char s[2] = " ";
  int i = 0, j, level = 0, lineNum = 0;
  bool termCheck, levelCheck, tagCheck;
  Line *l;

  xref_ID = NULL;
  lineVal = NULL;
  strcpy(tempLine, lineToParse);
  termCheck = false;
  levelCheck = true;
  tagCheck = false;
  lineNum++;
  i = 0;
  while (lineToParse[i] != ' ') {
    tempLvl[i] = lineToParse[i];
    i++;
    if (isdigit(lineToParse[i]) == 0) {
      levelCheck = false;
    }
  }
  tempLvl[i] = '\0';
  tag = malloc(sizeof(char) * 10);
  xref_ID = malloc(sizeof(char) * 50);
  lineVal = malloc(sizeof(char) * 300);
  strcpy(tag,"");
  strcpy(xref_ID,"");
  strcpy(lineVal,"");

  if (strlen(lineToParse) > 250) {
    printf("LINE TOO LONG ERROR!\n");
  }

  /*maybe make sure there cant be a most sig 0, like '02'*/
  level = atoi(tempLvl);
  token = strtok (lineToParse, s);
  i = 0;
  tagCheck = false;
  while (token != NULL) {
    if (i > 3) {
      printf("Error ocurred\n");
    }
    switch (i) {
      case 0:
        break;
      case 1:
        if (token[0] == '@') {
          j = 1;
          while (token[j] != '@') {
            xref_ID[j - 1] = token[j];
            j++;
          }
        } else {
          strcpy(tag, token);
          tagCheck = true;
        }
        break;
      case 2:
        if (tagCheck == true) {
          tempToken = strtok(tempLine, s);
          tempToken = strtok(NULL, s);
          tempToken = strtok(NULL,s);
          if (tempToken == NULL) {
            i = 3;
            printf("TERMINATOR CHECK\n");
            /*do actual terminator check...MAYBE NOT CAUSE ITS ALWAYS A '\n'??*/
            termCheck = true;
          } else {
            do {
              strcat(lineVal, token);
              token = strtok(NULL, s);
              if (token != NULL) {
                strcat(lineVal, " ");
              }
              /*check here to see if the value is a pointer*/
            } while (token != NULL); /*love u*/
          }
        } else {
          strcpy(tag, token);
          tagCheck = true;
        }
        break;
      case 3:
        do {
          strcat(lineVal, token);
          token = strtok(NULL, s);
        } while (token != NULL);
        break;
      default:
        printf("Parser Switch Error\n");
    }
    i++;
    token = strtok(NULL, s);
  }

  if (levelCheck == false || termCheck == false || tagCheck == false) {
    /*return some kind of error*/
  }

  //For loops to remove new line characters
  for (i = 0; i < strlen(tag); i++) {
    if (tag[i] == '\n' || tag[i] == '\r')
      tag[i] = '\0';
  }
  for (i = 0; i < strlen(lineVal); i++) {
    if (lineVal[i] == '\n' || lineVal[i] == '\r')
      lineVal[i] = '\0';
  }

  //Allocates and create line
  l = createLine(level, xref_ID, tag, lineVal, 0);

return l;
}

GEDCOMerror parseHeader (List *headerList, Header *h, char *submID, char *source, char *tempVersion, CharSet *encoding, List *otherFields) {
  GEDCOMerror errorVal;
  char /*tempVersion[100],*/ parsedSubmID[100];
  //float tempFVersion;
  int i, j = 0, versDecimalCheck = 0;
  Node *currentNode = headerList->head;

  while (currentNode != NULL) {/*goes through all of tokenLine*/
    Line *currLine = (Line*)currentNode->data;
    if (strcmp(currLine->tag, "SOUR") == 0) {
    /*DO YOU COPY ALL OF THE SOURCE DATA?????? DOESNT MAKE SENSE CAUSE a Line cant be more than 250chars and there code be multiple lines in full source*/
      strcpy(source, currLine->lineVal);
    } else if (strcmp(currLine->tag, "GEDC") == 0) {/*Gets GEDCOM version, returns error if inccorect fomrat, formats version number to 1 decimal*/
      currentNode = currentNode->next;
      currLine = (Line *)currentNode->data;
      if (strcmp(currLine->tag, "VERS") != 0 || strcmp(currLine->lineVal, "") == 0) {
          printf("Version Error\n");
          break;
      }
      strcpy(tempVersion, currLine->lineVal);
      for (i = 0; i < strlen(tempVersion); i++) {/*Block for formatting version to 1 decimal*/
        if (tempVersion[i] != '.') {
          tempVersion[j] = tempVersion[i];
        } else {
          if (versDecimalCheck == 1) {
            i++;
            tempVersion[j] = tempVersion[i];
          } else {
            tempVersion[j] = tempVersion[i];
            versDecimalCheck = 1;
          }
        }
        j++;
      }
      tempVersion[j] = '\0';
    } else if (strcmp(currLine->tag, "CHAR") == 0) {/*Block that finds the CharSet for the header*/
      if (strcmp(currLine->lineVal, "ANSEL") == 0) {
        *encoding = 0;
      } else if (strcmp(currLine->lineVal, "ASCII") == 0) {
        *encoding = 3;
      } else if (strcmp(currLine->lineVal, "UNICODE") == 0) {
        *encoding = 2;
      } else if (strcmp(currLine->lineVal, "UTF-8") == 0 || strcmp(currLine->lineVal, "UTF8") == 0) {
        *encoding = 1;
      } else {
        printf("Charset error\n");
      }
    } else if (strcmp(currLine->tag, "SUBM") == 0) {
      if (strcmp(currLine->lineVal, "") != 0) {
        strcpy(submID, currLine->lineVal);
      } else {
        printf("Submitter id missing\n");
      }
    } else {
      Field *newField = createField(currLine->tag, currLine->lineVal);
      insertBack(otherFields, newField);
    }
    currentNode = currentNode->next;
  }

  j = 0;
  for (i = 0; i < strlen(submID) + 1; i++) {
    if (submID[i] != '@') {
      parsedSubmID[j] = submID[i];
      j++;
    }
  }
  strcpy(submID, parsedSubmID);

  return errorVal;/*if no error it will stay as 0*/
}

GEDCOMerror parseSubmitter( List submitterList, char *name, List *otherFields, char *addr) {
  GEDCOMerror cs;
  Node *curr;
  Line *l;
  char  *tag = NULL, *value = NULL;

  curr = submitterList.head;
  while (curr != NULL) {
    l = (Line*)curr->data;
    if (strcmp(l->tag,"NAME") == 0) {
      if (strlen(l->lineVal) > 61) {
        printf("NAME OF SUBMITTER TOO LONG ERROR\n");
      } else {
        strcpy(name,l->lineVal);
      }
    } else if (strcmp(l->tag,"ADDR") == 0) {
      if (l->lineVal != NULL)
        strcpy(addr, l->lineVal);
    } else if (strcmp(l->tag,"CONT") == 0) {
      //check previous
    } else {
      tag = malloc(sizeof(char) * strlen(l->tag) + 1);
      value = malloc(sizeof(char) * strlen(l->lineVal) + 1);
      strcpy(tag, l->tag);
      strcpy(value, l->lineVal);
      Field *newField = createField(tag, value);
      insertBack(otherFields, newField);
    }
    curr = curr->next;
  }

  return cs;
}

GEDCOMerror parseIndividuals (List *wholeFile, List *individuals, List *families) {
  GEDCOMerror ge;
  List otherFields;//list for other fields of a specific individual
  List eventList;//list for each individual, holding their events
  List currEventLines;//Lines to be parsed by event parser, holds the lines of only one event
  Node *curr;
  Individual *ind;
  Line *l;
  Event *ev;
  char *fName, *lName, *tag, *value;
  //char famReference[20];

  printf("I1\n");

  curr = wholeFile->head;
  while (curr != NULL) {
    printf("I2");
    l = (Line*)curr->data;
    if (strcmp(l->tag, "INDI") == 0) {
      fName = malloc(sizeof(char) * 50);
      lName = malloc(sizeof(char) * 50);
      strcpy(fName, "");
      strcpy(lName, "");

      printf("I3\n");

      eventList = initializeList(&printEvent,&deleteEvent,&compareEvents);//Initialize new event list for current individual
      otherFields = initializeList(&printField, &deleteField, &compareFields);//initialize otherFields list for current individual
      //get person xref_ID here??
      curr = curr->next;
      while (l->level != 0 && curr != NULL) {
        printf("i4\n");
        l = (Line*)curr->data;
        if (strcmp(l->tag, "GIVN") == 0) {//Person's given name
          printf("i5 : %s\n", l->lineVal);
          strcpy(fName, l->lineVal);
        } else if (strcmp(l->tag, "SURN") == 0) {//Person's surname
          strcpy(lName, l->lineVal);
          printf("i6 : %s\n", l->lineVal);
        } else if (strcmp(l->tag, "DEAT") == 0 || strcmp(l->tag, "BURI") == 0 || strcmp(l->tag, "CREM") == 0 || strcmp(l->tag, "BAPM") == 0 || strcmp(l->tag, "BARM") == 0 || strcmp(l->tag, "BASM") == 0 || strcmp(l->tag, "BLES") == 0 || strcmp(l->tag, "CHRA") == 0 || strcmp(l->tag, "CONF") == 0 || strcmp(l->tag, "FCOM") == 0 || strcmp(l->tag, "ORDN") == 0 || strcmp(l->tag, "NATU") == 0 || strcmp(l->tag, "EMIG") == 0 || strcmp(l->tag, "IMMI") == 0 || strcmp(l->tag, "CENS") == 0 || strcmp(l->tag, "PROB") == 0 || strcmp(l->tag, "WILL") == 0 || strcmp(l->tag, "GRAD") == 0 || strcmp(l->tag, "RETI") == 0 || strcmp(l->tag, "EVEN") == 0 || strcmp(l->tag, "ADOP") == 0 || strcmp(l->tag, "BIRT") == 0 || strcmp(l->tag, "CHR") == 0) {//events here
          printf("i7 This is an event %s.\n", l->tag);
          currEventLines = initializeList(&printLine, &dummyDelete, &compareFields);
          insertBack(&currEventLines, l);
          curr = curr->next;
          l = (Line*)curr->data;
          puts("i8");
          while (l->level != 1 && curr != NULL) {//go until line level is 1 again,
            puts("i9");
            insertBack(&currEventLines, l);
            curr = curr->next;
            l = (Line*)curr->data;
          }
          puts("i10");
          ev = parseEvent(&currEventLines); //call parse event func with given list of lines
          insertBack(&eventList, ev);
          puts("i11");
        }else {
          puts("i12");
          tag = malloc(sizeof(char) * 50);
          value= malloc(sizeof(char) * 200);
          strcpy(tag, l->tag);
          strcpy(value, l->lineVal);
          Field *newField = createField(tag,value);
          insertBack(&otherFields, newField);
        }
        puts("i13");
        curr = curr->next;
      }
      puts("i14");
      ind = createIndividual(fName,lName, &eventList, families, &otherFields);
      insertBack(individuals, ind);
      curr = curr->previous;
      l = (Line*)curr->data;//might be an unneeded line
    }
    curr = curr->next;
  }

  //Should this function call parse fam??
  //GEDCOMerror returnError = parseFamilies (wholeFile, individuals, families);
  //after getting the list of fams, add them to every individual

  return ge;
}


GEDCOMerror parseFamily(List *wholeFile, List *individuals, List *families) {
  GEDCOMerror ge;


  return ge;
}


Event *parseEvent(List *eventLines) {
  Event *ev;
  char type[10];
  char *date, *place, *tag, *value;
  Line *l;
  List otherFields = initializeList(&printField, &deleteField, &compareFields);
  Node *curr = eventLines->head;
  date = malloc(sizeof(char) * 200);
  place = malloc(sizeof(char) * 100);
  strcpy(date, "");
  strcpy(place, "");


  while (curr != NULL) {
    l = (Line*)curr;
    if (strcmp(l->tag, "DEAT") == 0 || strcmp(l->tag, "BURI") == 0 || strcmp(l->tag, "CREM") == 0 || strcmp(l->tag, "BAPM") == 0 || strcmp(l->tag, "BARM") == 0 || strcmp(l->tag, "BASM") == 0 || strcmp(l->tag, "BLES") == 0 || strcmp(l->tag, "CHRA") == 0 || strcmp(l->tag, "CONF") == 0 || strcmp(l->tag, "FCOM") == 0 || strcmp(l->tag, "ORDN") == 0 || strcmp(l->tag, "NATU") == 0 || strcmp(l->tag, "EMIG") == 0 || strcmp(l->tag, "IMMI") == 0 || strcmp(l->tag, "CENS") == 0 || strcmp(l->tag, "PROB") == 0 || strcmp(l->tag, "WILL") == 0 || strcmp(l->tag, "GRAD") == 0 || strcmp(l->tag, "RETI") == 0 || strcmp(l->tag, "EVEN") == 0 || strcmp(l->tag, "ADOP") == 0 || strcmp(l->tag, "BIRT") == 0 || strcmp(l->tag, "CHR") == 0) {
      strcpy(type, l->tag);
    } else if (strcmp(l->tag, "DATE") == 0) {
      strcpy(date, l->lineVal);
    } else  if (strcmp(l->tag, "PLAC") == 0) {
      strcpy(place, l->lineVal);
    } else {
      tag = malloc(sizeof(char) * 50);
      value = malloc(sizeof(char) * 150);
      strcpy(tag, l->tag);
      strcpy(value, l->lineVal);
      Field *newField = createField(tag, value);
      insertBack(&otherFields, newField);
    }
    curr = curr->next;
  }
  ev = createEvent(type, date, place, &otherFields);
  return ev;
}


char *printError(GEDCOMerror err) {
  char *errorReturn;
  char lineNumber[10];

  errorReturn = malloc(sizeof(char) * 50);

  if (err.line != -1) {
    sprintf(lineNumber, "%d", err.line);
  }

  /*WHY DO YOU THINK IT HAS TO BE DYNAMICALLY ALLOCATED*/
  switch (err.type) {
    case 0:
      strcat(errorReturn,"OK");
      /*line number = -1*/
      break;
    case 1:
      strcat(errorReturn,"Invaid File");
      break;
    case 2:
      strcat(errorReturn,"Invalid GEDCOM Object");
      break;
    case 3:
      strcat(errorReturn,"Invalid Header");
      break;
    case 4:
      strcat(errorReturn,"Invalid Record");
      break;
    case 5:
      strcat(errorReturn,"Other Error");
      break;
    default:
      /*Do something here??*/
      break;
  }
  strcat(errorReturn," (line: ");
  if (err.type == 0 || err.type == 1) {
    strcat(errorReturn,"-1");
  } else {
    strcat(errorReturn, lineNumber);
  }
  strcat(errorReturn,")");


  return errorReturn;
}

Individual *findPerson (const GEDCOMobject *familyRecord, bool (*compare)(const void *first, const void *second), const void *person) {
  /*this custom compare func must be defined to compare a person within the node data*/
  if (familyRecord == NULL || compare == NULL || person == NULL) {
    return NULL;
  }

  Node *toSearch = familyRecord->individuals.head;
  while (toSearch != NULL) {
    if (compare(toSearch->data, person) == true) { 
      return toSearch->data;
    }
    toSearch = toSearch->next;
  }

  return NULL;
}


/********************************************************************************************/

/*Create functions for dynamically creating structs to follow*/

/*
GEDCOMerror createGEDCOM (char *fileName, GEDCOMobject **obj) {
  GEDCOMobject *o = createGEDCOMobj();

  *obj = o;
}
*/

GEDCOMobject *createGEDCOMobj (Header *header, List *families, List *individuals, Submitter *submitter) {
  /*double pointer???*/
  GEDCOMobject *go = malloc(sizeof(GEDCOMobject));
  go->header = header;
  go->families = *families; /* TODO: is this the right level of pointer?*/
  go->individuals = *individuals; /* TODO: See above*/
  go->submitter = submitter;
  return go;
}

GEDCOMerror *createGEDerror (ErrorCode type, int line) {
  GEDCOMerror *ge = malloc(sizeof(GEDCOMerror));
  ge->type = type;
  ge->line = line;
  return ge;
}

Event *createEvent (char *type, char *date, char *place, List *otherFields) { /*others will always be of type Field*/
  Event *e = malloc(sizeof(Event));
  strcpy(e->type, type); /* TODO: is type the same size as e->type */
  e->date = date;
  e->place = place;
  e->otherFields = *otherFields; /* TODO: right level of pointer? */
  return e;
}

Field *createField (char *tag, char *value) { /*neither can be NULL nor Empty*/
  Field *f = malloc(sizeof(Field));
  f->tag = tag;
  f->value = value;
  return f;
}

Submitter *createSubmitter (char *submitterName, List *otherFields, char address[]) { /*address is a flexible array, check slides*/
  int length = strlen(address) + 1;
  Submitter *s = malloc(sizeof(Submitter) + length * sizeof(char));
  strcpy(s->submitterName, submitterName);
  s->otherFields = *otherFields;
  strcpy(s->address, address);
  return s;
}

Header *createHeader (char *source, float gedcVersion, CharSet encoding, Submitter *submitter, List *otherFields) {
  Header *h = malloc(sizeof(Header));
  strcpy(h->source, source);  /*maybe check to make sure its not longer than 250*/
  h->gedcVersion = gedcVersion;
  h->encoding = encoding;
  h->submitter = submitter;
  h->otherFields = *otherFields;
  return h;
}

Individual *createIndividual (char *givenName, char *surname, List *events, List *families, List *otherFields) {
  Individual *i = malloc(sizeof(Individual));
  i->givenName = givenName; /* TODO: use strcpy? */
  i->surname = surname;
  i->events = *events;
  i->families = *families;
  i->otherFields = *otherFields;
  return i;
}

Family *createFamily (Individual *wife, Individual *husband, List *children, List *otherFields) {
  Family *fa = malloc(sizeof(Family));
  fa->wife = wife;
  fa->husband = husband;
  fa->children = *children;
  fa->otherFields = *otherFields;
  return fa;
}

/*******************************************************************************************/
/*print, delete and compare functions to follow*/

void deleteEvent (void *toBeDeleted) {
  Event *tmpEvent;

  if (toBeDeleted == NULL)
    return;

  tmpEvent = (Event*)toBeDeleted;

  List *otherFields = &tmpEvent->otherFields;

  free(tmpEvent->date);
  free(tmpEvent->place);
  clearList(otherFields);
  free(tmpEvent);
}

int compareEvents (const void *first, const void *second) {

  Event *firstEvent;
  Event *secEvent;

  if (first == NULL || second == NULL)
    return 0;

  firstEvent = (Event*)first;
  secEvent = (Event*)second;

  return strcmp((char*)firstEvent->type,(char*)secEvent->type);
  /*Must also check the place, maybe tag?? */
}

char *printEvent(void *toBePrinted) {
  char *tmpStr, *otherFieldsInfo;
  Event *tmpEvent;
  int len;

  tmpEvent = (Event*)toBePrinted;

  if (toBePrinted == NULL) {
    return NULL;
  }

  int size = 200;
  otherFieldsInfo = malloc(sizeof(char) * size);
  strcpy(otherFieldsInfo, "");

  Node *temp = tmpEvent->otherFields.head;
  Field *f;
  while (temp != NULL) {
    f = (Field*)temp->data;
    if (strlen(f->tag) + strlen(f->value) + strlen(otherFieldsInfo) + 4 > size) {
      size = size*2;
      otherFieldsInfo = (char*)realloc(otherFieldsInfo, size);
    }
    strcat(otherFieldsInfo, f->tag);
    strcat(otherFieldsInfo," ");
    strcat(otherFieldsInfo, f->value);
    strcat(otherFieldsInfo," | ");
    temp = temp->next;
  }

  len = strlen(tmpEvent->date) + strlen(tmpEvent->place) + strlen(otherFieldsInfo) + 100;
  tmpStr = (char*)malloc(sizeof(char) * len);

  if (strcmp(otherFieldsInfo, "") == 0) {
    sprintf(tmpStr, "Type: %s | Date: %s at location %s.\nExtra Fields: none.\n",tmpEvent->type, tmpEvent->date, tmpEvent->place);
  } else {
    sprintf(tmpStr, "Type %s | Date %s at location %s.\nExtra Fields: %s\n", tmpEvent->type, tmpEvent->date, tmpEvent->place, otherFieldsInfo);
  }
  free(otherFieldsInfo);
  return tmpStr;
}


void deleteIndividual (void *toBeDeleted) {
  Individual *tmpInd;

  if (toBeDeleted == NULL)
    return;

  tmpInd = (Individual*)toBeDeleted;

  List *events = &tmpInd->events;
  List *families = &tmpInd->families;
  List *otherFields = &tmpInd->otherFields;


  free(tmpInd->givenName);
  free(tmpInd->surname);
  clearList(events);
  clearList(families);
  clearList(otherFields);
  free(tmpInd);
}

int compareIndividuals (const void *first, const void *second) {
  Individual *firstInd;
  Individual *secondInd;

  if (first == NULL || second == NULL)
    return 0;

  firstInd = (Individual*)first;
  secondInd = (Individual*)second;

  if (strcmp((char*)firstInd->givenName,(char*)secondInd->givenName) == 0)
    return strcmp((char*)firstInd->surname,(char*)secondInd->surname);
  return -1;
}

char *printIndividual (void *toBePrinted) {
  char *tmpStr = NULL;
  /*char *tmpStr, *otherFieldsInfo;
  Individual *tmpInd;
  int len;

  tmpInd = (Individual*)toBePrinted;

  if (toBePrinted == NULL) {
    return NULL;
  }


  int size = 200;
  otherFieldsInfo = malloc(sizeof(char) * size);
  strcpy(otherFieldsInfo, "");

  Node *temp = tmpEvent->otherFields.head;
  Individual *i;
  while (temp != NULL) {
    i = (Individual*)temp->data;
    if (strlen(i->tag) + strlen(i->value) + strlen(otherFieldsInfo) + 4 > size) {
      size = size*2;
      otherFieldsInfo = (char*)realloc(otherFieldsInfo, size);
    }
    strcat(otherFieldsInfo, i->tag);
    strcat(otherFieldsInfo," ");
    strcat(otherFieldsInfo, i->value);
    strcat(otherFieldsInfo," | ");
    temp = temp->next;
  }*/

  /*still need to add event list size and family list size*/
  /*len = strlen(tmpInd->givenName) + strlen(tmpInd->surname) + strlen(otherFieldsInfo) + 100;
  tmpStr = (char*)malloc(sizeof(char) * len);

  if (strcmp(otherFieldsInfo, "") == 0) {
    sprintf(tmpStr, "Name: %s %s",tmpInd->givenName, tmpInd->surname);
  } else {
    sprintf(tmpStr, "Type %s | Date %s at location %s.\nExtra Fields: %s\n", tmpEvent->type, tmpEvent->date, tmpEvent->place, otherFieldsInfo);
  }
  free(otherFieldsInfo);*/
  return tmpStr;
}


void deleteFamily (void *toBeDeleted) {
  Family *tmpFamily;

  if (toBeDeleted == NULL)
    return;

  tmpFamily = (Family*)toBeDeleted;

  List *otherFields1 = &tmpFamily->otherFields;
  List *children2 = &tmpFamily->children;

  free(tmpFamily->wife);
  free(tmpFamily->husband);
  clearList(otherFields1);
  clearList(children2);
  free(tmpFamily);
}

int compareFamilies (const void *first, const void *second) {
  Family *firstFam;
  Family *secondFam;

  if (first == NULL || second == NULL)
    return 0;

  firstFam = (Family*)first;
  secondFam = (Family*)second;

  int wife = compareIndividuals(firstFam->wife, secondFam->wife);
  int husb = compareIndividuals(firstFam->husband, secondFam->husband);
  /*include list children???*/

  if (wife == 0 && husb == 0)
    return 0;
  return -1;
}

char *printFamily(void *toBePrinted) {
  char *tmpStr = NULL;
  /*char *tmpStr, *otherFieldsInfo;
  Family *tmpEvent;
  int len;

  tmpEvent = (Event*)toBePrinted;

  if (toBePrinted == NULL) {
    return NULL;
  }


  int size = 200;
  otherFieldsInfo = malloc(sizeof(char) * size);
  strcpy(otherFieldsInfo, "");

  Node *temp = tmpEvent->otherFields.head;
  Field *f;
  while (temp != NULL) {
    f = (Field*)temp->data;
    if (strlen(f->tag) + strlen(f->value) + strlen(otherFieldsInfo) + 4 > size) {
      size = size*2;
      otherFieldsInfo = (char*)realloc(otherFieldsInfo, size);
    }
    strcat(otherFieldsInfo, f->tag);
    strcat(otherFieldsInfo," ");
    strcat(otherFieldsInfo, f->value);
    strcat(otherFieldsInfo," | ");
    temp = temp->next;
  }

  len = strlen(tmpEvent->date) + strlen(tmpEvent->place) + strlen(otherFieldsInfo) + 100;
  tmpStr = (char*)malloc(sizeof(char) * len);

  //check for NULL

  if (strcmp(otherFieldsInfo, "") == 0) {
    sprintf(tmpStr, "Type: %s | Date: %s at location %s.\nExtra Fields: none.\n",tmpEvent->type, tmpEvent->date, tmpEvent->place);
  } else {
    sprintf(tmpStr, "Type %s | Date %s at location %s.\nExtra Fields: %s\n", tmpEvent->type, tmpEvent->date, tmpEvent->place, otherFieldsInfo);
  }
  free(otherFieldsInfo);*/
  return tmpStr;
}


void deleteField (void *toBeDeleted) {
  Field *tmpField;

  if (toBeDeleted == NULL)
    return;

  tmpField = (Field*)toBeDeleted;

  free(tmpField->tag);
  free(tmpField->value);
  free(tmpField);
}

int compareFields (const void *first, const void *second) {
  Field *firstField;
  Field *secondField;

  if (first == NULL || second == NULL)
    return 0;

  firstField = (Field*)first;
  secondField = (Field*)second;

  int tagVal = strcmp((char*)firstField->tag,(char*)secondField->tag);
  int valueVal = strcmp((char*)firstField->value,(char*)secondField->value);

  if (tagVal == 0 && valueVal == 0)
    return 0;
  return -1;
}

char *printField (void *toBePrinted) {
  char *tmpStr;
  Field *tmpField;
  int len;

  tmpField = (Field*)toBePrinted;

  if (toBePrinted == NULL || tmpField->tag == NULL || tmpField->value == NULL) {
    return NULL;
  }

  len = strlen(tmpField->tag) + strlen(tmpField->value) + 50;
  tmpStr = (char*)malloc(sizeof(char) * len);

  sprintf(tmpStr, "Tag: %s, Value: %s\n", tmpField->tag, tmpField->value);

  return tmpStr;
}

void deleteLine (void *toBeDeleted) {
  Line *tempLine;

  if (toBeDeleted == NULL) {
    return;
  }

  tempLine = (Line*)toBeDeleted;

  free(tempLine->xref_ID);
  free(tempLine->tag);
  free(tempLine->lineVal);
  free(tempLine);
}

char *printLine(void *toBePrinted) {
  Line *tempLine;
  char *tmpStr;
  int len;

  if(toBePrinted == NULL) {
    return NULL;
  }

  tempLine = (Line*)toBePrinted;
  len = strlen(tempLine->xref_ID) + strlen(tempLine->tag) + strlen(tempLine->lineVal) + 8 + 120;
  tmpStr = (char*)malloc(sizeof(char) * len);

  sprintf(tmpStr,"Level: %d\nXref_ID (opt): %s\nTag: %s\nLine Value (opt): %s\nLine Num: %d\n", tempLine->level,tempLine->xref_ID,tempLine->tag,tempLine->lineVal,tempLine->lineNum);

  return tmpStr;
}

void dummyDelete (void *toBeDeleted) {
}

