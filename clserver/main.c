#define	DEBUG
#define	TRACE

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include	<stdio.h>
#include	<string.h>
#include	<stdlib.h>
#include	<iconv.h>
#include	<libxml/parser.h>
#include	"types.h"
#include	"value.h"
#include	"debug.h"

static	void
PrintUTF(
	char	*istr)
{
	char	*ob;
	char	obb[3];
	size_t	sib
	,		sob;
	iconv_t		cd;

	if		(  istr  ==  NULL  )	return;
	if		(  ( cd = iconv_open("EUC-JP","utf-8") )  <  0  ) {
		printf("code set error\n");
		exit(1);
	}
	
	sib = strlen(istr);
	
	do {
		ob = obb;
		sob = 3;
		if		(  iconv(cd,&istr,&sib,&ob,&sob)  <  0  ) {
			printf("error\n");
			exit(1);
		}
		*ob = 0;
		printf("%s",obb);
	}	while	(  sib  >  0  );
	iconv_close(cd);
}

static	void
PrintContent(
	xmlNodePtr	tree)
{
	printf("<%s>",tree->name);
	//		PrintUTF(xmlNodeGetContent(tree));
	if		(  tree->content  !=  NULL  ) {
		printf("[");
		PrintUTF(tree->content);
		printf("]\n");
	}
	tree = tree->xmlChildrenNode;
	while	(  tree  !=  NULL  ) {
		PrintContent(tree);
		tree = tree->next;
	}
}

extern	int
main(
	int		argc,
	char	**argv)
{
	xmlDocPtr	doc;
	xmlNodePtr	tree;

	if		(  ( doc = xmlParseFile(argv[1]) )  ==  NULL  ) {
		printf("error\n");
		exit(0);
	}

	if		(  ( tree = xmlDocGetRootElement(doc) )  ==  NULL  ) {
		xmlFreeDoc(doc);
		printf("error empty\n");
		exit(1);
	}
    tree = tree->xmlChildrenNode;
	while	(  tree && xmlIsBlankNode(tree)  ) {
dbgmsg("+");
		tree = tree->next;
	}
dbgmsg("*");
    tree = tree->xmlChildrenNode;
dbgmsg("**");
	while	(  tree  !=  NULL  ) {
		PrintContent(tree);
		tree = tree->next;
	}
	//xmlSaveFormatFileEnc("-",doc,"euc-jp",atoi(argv[2]));
	return	(0);
}
