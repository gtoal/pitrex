/*
 * debugger.h: debugger for Atari Vector game simulator
 *
 * Copyright 1991, 1992, 1993, 1996 Hedley Rainnie and Eric Smith
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id: debugger.h,v 1.1 2018/07/31 01:19:44 pi Exp $
 */


#define INTBREAK 0x01
#define BREAKPT  0x02
#define BREAK	 0x03
#define STEP	 0x04
#define ERRORBRK 0x05
#define INSTTAG  0x06  /* instruction fetch from other than normal memory */

int debugger (int type);

int reload (char *s);
