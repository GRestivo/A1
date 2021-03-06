#ifndef GEDCOMUTILITIES_H
#define GEDCOMUTILITIES_H

#include "GEDCOMparser.h"

typedef struct GEDCOMline {
  int level;
  char *xref_ID;/*optional, you will know if it is surrounded by @@*/
  char *tag;
  char *lineVal;/*optional, has a delim/space before it, could be pointer or text*/
  /*some form of endline*/
  int lineNum;
}Line;

int parser(int argc, char **argv);

Line *parseLine (char *lineToParse);

GEDCOMerror parseHeader(List *headerList, Header *h, char *submID, char *source, char *tempVersion, CharSet *encoding, List *otherFields);

GEDCOMerror parseSubmitter(List submitterLst, char *name, List *otherFields, char *addr);

GEDCOMerror parseIndividuals (List *wholeFile, List *individuals, List *families);

GEDCOMerror parseFamilies (List *wholeFile, List *individuals, List *families);

Event *parseEvent(List *eventLines);

char *printError(GEDCOMerror err);

Line *createLine(int level, char *xref_ID, char *tag, char *lineVal, int lineNum);

Individual *findPerson (const GEDCOMobject *familyRecord, bool (*compare)(const void *first, const void *second),const void *person);

GEDCOMobject *createGEDCOMobj (Header *header, List *families, List *individuals, Submitter *submitter);

GEDCOMerror *createGEDerror (ErrorCode type, int line);

Event *createEvent (char *type, char *date, char *place, List *otherFields);/*others will always be of type Field*/

Field *createField (char *tag, char *value);/*neither can be NULL nor Empty*/

Submitter *createSubmitter (char *submitter, List *otherFields, char address[]);/*address is a flexible array, check slides*/

Header *createHeader (char *source, float gedcVersion, CharSet encoding, Submitter *submitter, List *otherFields);

Individual *createIndividual (char *givenName, char *surname, List *events, List *families, List *otherFields);

Family *createFamily (Individual *wife, Individual *husband, List *children, List *otherFields);

void deleteLine (void *toBeDeleted);

char *printLine (void *toBePrinted);

void dummyDelete (void *toBeDeleted);

#endif
