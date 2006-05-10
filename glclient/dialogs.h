/*
 * PANDA -- a simple transaction monitor
 * Copyright (C) 2004-2006 Kouji TAKAO
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

#ifndef	_INC_DIALOGS_H
#define	_INC_DIALOGS_H

GtkWidget* message_dialog( const char *message, gboolean message_type);
#ifdef USE_GNOME
GtkWidget* question_dialog(	const char *message, GtkSignalFunc clicked_handler,
							GtkWidget	*widget, GtkWindow	*window);

#endif

#endif
