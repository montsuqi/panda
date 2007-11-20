/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2006-2007 Ogochan.
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

#ifndef	_INC_XXML_H
#define	_INC_XXML_H
#include	<libxml/parser.h>

#undef	GLOBAL
#ifdef	MAIN
#define	GLOBAL		/*	*/
#else
#define	GLOBAL		extern
#endif
#undef	GLOBAL

#define	XMLNodeType(cur)			((cur)->type)
#define	XMLNodeNext(cur)			((cur)->next)
#define	XMLNs(cur)					((cur)->ns)
#define	XMLNsBody(cur)				(char *)((cur)->ns->href)
#define	XMLNsPrefix(cur)			((cur)->ns->prefix)
#define	XMLName(cur)				(char *)((cur)->name)
#define	XMLNodeProperties(cur)		((cur)->properties)
#define	XMLPropertyName(attr)		((attr)->name)
#define	XMLPropertyValue(attr)		((attr)->children->content)
#define	XMLPropertyNext(attr)		((attr)->next)
#define	XMLNodeChildren(cur)		((cur)->children)
#define	XMLNodeParent(cur)			((cur)->parent)
#define	XMLNodeContent(cur)			((cur)->content)
#define	XMLGetProp(tree,name)		xmlGetProp((tree),(const char *)(name))
#define	XMLSetProp(tree,name,val)	xmlSetProp((tree),(name),(val))
#define	XMLUnsetProp(tree,name)		xmlUnsetProp((tree),(name))
#define	XMLAddChild(par,cur)		xmlAddChild((par),(cur))
#define	XMLDoc(cur)					((cur)->doc)

extern	void	PrintUTF(const xmlChar *xstr);
extern	void	OtherXMLType(xmlNodePtr tree);
extern	xmlChar	*MakePureText(xmlChar *str);
extern	xmlChar	*XMLGetText(xmlNodePtr tree);
extern	xmlChar	*XMLGetPureText(xmlNodePtr tree);
extern	Bool	IsAllSpaces(char *text);
extern	Bool	LastNode(xmlNodePtr node);

extern	Bool		InTag(xmlNodePtr tree, char *ns, char *tag);
extern	xmlNodePtr	SearchNode(xmlNodePtr tree, char *ns, char *tag,
							   char *prop, char *pvalue);
extern	char	*SearchParentProp(xmlNodePtr tree, char *prop);

#endif
