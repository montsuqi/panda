/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2006 Ogochan.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

/*
#define	DEBUG
#define	TRACE
*/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif
#include	<stdio.h>
#include	<string.h>
#include	<stdlib.h>
#include	<ctype.h>
#include	<iconv.h>
#include	"types.h"
#include	"libmondai.h"
#include	"xxml.h"
#include	"debug.h"

extern	void
PrintUTF(
	const xmlChar	*xstr)
{
	char	*ob
		,	*istr;
	char	obb[3];
	size_t	sib
	,		sob;
	iconv_t		cd;

	istr = (char *)xstr;

	if		(  istr  ==  NULL  )	return;
	if		(  ( cd = iconv_open("iso-2022-jp","utf-8") )  <  0  ) {
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

extern	Bool
IsAllSpaces(
	char	*text)
{
	Bool	ret;

	ret = FALSE;
	while	(  *text  !=  0  ) {
		if		(  !isspace(*text)  ) {
			ret = TRUE;
			break;
		}
		text ++;
	}
	return	(!ret);
}

extern	Bool
LastNode(
	xmlNodePtr		node)
{
	Bool	ret;
	xmlNodePtr	next;

	if		(  ( next = XMLNodeNext(node) )  ==  NULL  ) {
		ret = TRUE;
	} else {
		switch	(XMLNodeType(next)) {
		  case	XML_ELEMENT_NODE:
			ret = FALSE;
			break;
		  case	XML_TEXT_NODE:
			if		(	(  IsAllSpaces(XMLNodeContent(next))  )
					&&	(  XMLNodeNext(next)  ==  NULL        ) )	{
				ret = TRUE;
			} else {
				ret = FALSE;
			}
			break;
		  default:
			ret = TRUE;
			break;
		}
	}
	return	(ret);
}

extern	xmlChar	*
MakePureText(
	xmlChar	*str)
{
	xmlChar	*qq
		,	*ret
		,	*p;
	size_t	len;

ENTER_FUNC;
	while	(	(  *str  !=  0   )
			&&	(  isspace(*str) ) )	str ++;
	qq = str + strlen(str);
	while	(	(  qq  !=  str    )
			&&	(  isspace(*qq)   ) )	qq --;
	len = qq - str + 1;

	ret = (xmlChar *)xmalloc((len+1)*sizeof(xmlChar));
	p = ret;
	while	(  len  >  0  ) {
		if		(  !isspace(*str)  ) {
			*p ++ = *str;
		}
		str ++;
		len --;
	}
	*p = 0;
LEAVE_FUNC;
	return	(ret);
}

extern	xmlChar	*
XMLGetText(
	xmlNodePtr	tree)
{
	xmlChar	*q
		,	*ret;
	xmlNodePtr	sub;

ENTER_FUNC;
	if		(	(  ( sub = XMLNodeChildren(tree) )  !=  NULL  )
			&&	(  XMLNodeType(sub)    ==  XML_TEXT_NODE      )
			&&	(  ( q = XMLNodeContent(sub) )  !=  NULL      ) ) {
		ret = q;
	} else {
		ret = NULL;
	}
LEAVE_FUNC;
	return	(ret);
}

extern	xmlChar	*
XMLGetPureText(
	xmlNodePtr	tree)
{
	xmlChar	*q
		,	*ret;

ENTER_FUNC;
	if		(  ( q = XMLGetText(tree) )  !=  NULL  ) {
		ret = MakePureText(q);
	} else {
		ret = NULL;
	}
LEAVE_FUNC;
	return	(ret);
}

extern	void
OtherXMLType(
	xmlNodePtr	tree)
{
#ifdef	DEBUG
	switch	(XMLNodeType(tree)) {
     case	XML_COMMENT_NODE:
		printf("<!-- ");
		PrintUTF(tree->content);
		printf(" -->");
		break;
      case	XML_ATTRIBUTE_NODE:
		printf("ATTRIBUTE_NODE\n");
		break;
      case	XML_CDATA_SECTION_NODE:
		printf("CDATA_SECTION_NODE\n");
		break;
      case	XML_ENTITY_REF_NODE:
		printf("ENTITY_REF_NODE\n");
		break;
      case	XML_ENTITY_NODE:
		printf("ENTITY_NODE\n");
		break;
      case	XML_PI_NODE:
		printf("PI_NODE\n");
		break;
      case	XML_DOCUMENT_NODE:
		printf("DOCUMENT_NODE\n");
		break;
      case	XML_DOCUMENT_TYPE_NODE:
		printf("DOCUMENT_TYPE_NODE\n");
		break;
      case	XML_DOCUMENT_FRAG_NODE:
		printf("DOCUMENT_FRAG_NODE\n");
		break;
      case	XML_NOTATION_NODE:
		printf("NOTATION_NODE\n");
		break;
      case	XML_HTML_DOCUMENT_NODE:
		printf("HTML_DOCUMENT_NODE\n");
		break;
      case	XML_DTD_NODE:
		printf("DTD_NODE\n");
		break;
      case	XML_ELEMENT_DECL:
		printf("ELEMENT_DECL\n");
		break;
      case	XML_ATTRIBUTE_DECL:
		printf("ATTRIBUTE_DECL\n");
		break;
      case	XML_ENTITY_DECL:
		printf("ENTITY_DECL\n");
		break;
      case	XML_NAMESPACE_DECL:
		printf("NAMESPACE_DECL\n");
		break;
      case	XML_XINCLUDE_START:
		printf("XINCLUDE_START\n");
		break;
      case	XML_XINCLUDE_END:
		printf("XINCLUDE_END\n");
		break;
	  default:
		break;
	}
#endif
}

extern	Bool
InTag(
	xmlNodePtr tree,
	char		*ns,
	char		*tag)
{
	Bool	ret;
	char	*thisns;

	ret = FALSE;
	while	(  tree  !=  NULL  ) {
		switch	(XMLNodeType(tree)) {
		  case	XML_ELEMENT_NODE:
			if		(  XMLNs(tree)  !=  NULL  ) {
				thisns = XMLNsBody(tree);
			} else {
				thisns = "";
			}
			if		(	(  !strcmp(thisns,ns)  )
					&&	(  !strcmp(XMLName(tree),tag)  ) ) {
				ret = TRUE;
				goto	ex;
			}
			break;
		  default:
			break;
		}
		tree = XMLNodeParent(tree);
	}
  ex:
	return	(ret);
}

extern	xmlNodePtr
SearchNode(
	xmlNodePtr	tree,
	char		*ns,
	char		*tag,
	char		*prop,
	char		*pvalue)
{
	char	*thisns;
	char	*thisnn;
	xmlNodePtr	ret;

ENTER_FUNC;
	ret = NULL;
	while	(  tree  !=  NULL  ) {
		switch	(XMLNodeType(tree)) {
		  case	XML_ELEMENT_NODE:
			if		(  XMLNs(tree)  !=  NULL  ) {
				thisns = XMLNsBody(tree);
			} else {
				thisns = "";
			}
			if		(	(  tag  ==  NULL  )
					&&	(  ns   ==  NULL  )	) {
				if		(	(  ( thisnn = XMLGetProp(tree,prop) )  !=  NULL  )
						&&	(  !strcmp(thisnn,pvalue)  ) ) {
					ret = tree;
				} else {
					ret = SearchNode(XMLNodeChildren(tree),ns,tag,prop,pvalue);
				}
			} else
			if		(	(  pvalue  ==  NULL  )
					&&	(  !strcmp(thisns,ns)  )
					&&	(  !strcmp(XMLName(tree),tag)  ) ) {
				ret = tree;
			} else
			if		(	(  !strcmp(thisns,ns)  )
					&&	(  !strcmp(XMLName(tree),tag)  )
					&&	(  ( thisnn = XMLGetProp(tree,prop) )  !=  NULL  )
					&&	(  !strcmp(thisnn,pvalue)  ) ) {
				ret = tree;
			} else
			if		(  XMLNodeChildren(tree)  !=  NULL  ) {
				ret = SearchNode(XMLNodeChildren(tree),ns,tag,prop,pvalue);
			}
			break;
		  case	XML_TEXT_NODE:
			break;
		  default:
			OtherXMLType(tree);
			break;
		}
		if		(  ret  !=  NULL  )	break;
		tree = XMLNodeNext(tree);
	}
LEAVE_FUNC;
	return	(ret);
}

extern	char	*
SearchParentProp(
	xmlNodePtr	tree,
	char		*prop)
{
	char	*ret;

ENTER_FUNC;
	ret = NULL;
	if		(  tree  !=  NULL  ) {
		switch	(XMLNodeType(tree)) {
		  case	XML_ELEMENT_NODE:
			if		(  ( ret = XMLGetProp(tree,prop) )  ==  NULL  ) {
				if		(  XMLNodeParent(tree)  !=  NULL  ) {
					ret = SearchParentProp(XMLNodeParent(tree),prop);
				} else {
					ret = NULL;
				}
			}
			break;
		  case	XML_TEXT_NODE:
			break;
		  default:
			OtherXMLType(tree);
			break;
		}
	}
LEAVE_FUNC;
	return	(ret);
}

