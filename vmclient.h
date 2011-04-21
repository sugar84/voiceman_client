/*
	Copyright (c) 2000-2010 Michael Pozhidaev<msp@altlinux.org>
   This file is part of the VoiceMan speech service.

   VoiceMan speech service is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   VoiceMan speech service is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.
*/

#ifndef __VOICEMAN_VMCLIENT_H__
#define __VOICEMAN_VMCLIENT_H__

/*#ifdef __cplusplus 
#define EXTC extern "C"
#else
#define EXTC extern
#endif * __cplusplus*
*/

#define VOICEMAN_LANG_NONE 0
#define VOICEMAN_LANG_ENG 1
#define VOICEMAN_LANG_RUS 2

#define VOICEMAN_PROCMODE_ALL 0
#define VOICEMAN_PROCMODE_SOME 1
#define VOICEMAN_PROCMODE_NONE 2

typedef int vm_connection_t;
#define VOICEMAN_BAD_CONNECTION (vm_connection_t)-1

typedef int vm_result_t;
#define VOICEMAN_ERROR (vm_result_t)-1
#define VOICEMAN_OK 0

#define VOICEMAN_DEFAULT_SOCKET  '/var/run/voiceman.socket'
#define VOICEMAN_DEFAULT_PORT 5511

vm_connection_t vm_connect();
vm_connection_t vm_connect_unix(char* path);
vm_connection_t vm_connect_inet(char* host, unsigned long port);
void vm_close(vm_connection_t con);
vm_result_t vm_text(vm_connection_t con, char* text); 
vm_result_t vm_stop(vm_connection_t con);
vm_result_t vm_letter(vm_connection_t con, char* letter);
vm_result_t vm_tone(vm_connection_t con, unsigned long freq, unsigned long lengthms);
vm_result_t vm_pitch(vm_connection_t con, unsigned char value);
vm_result_t vm_rate(vm_connection_t con, unsigned char value);
vm_result_t vm_volume(vm_connection_t con, unsigned char value);
vm_result_t vm_procmode(vm_connection_t con, unsigned char procmode);
vm_result_t vm_family(vm_connection_t con, unsigned char lang, char* family);

/*
EXTC vm_connection_t vm_connect();
EXTC vm_connection_t vm_connect_unix(char* path);
EXTC vm_connection_t vm_connect_inet(char* host, size_t port);
EXTC void vm_close(vm_connection_t con);
EXTC vm_result_t vm_text(vm_connection_t con, char* text); 
EXTC vm_result_t vm_stop(vm_connection_t con);
EXTC vm_result_t vm_letter(vm_connection_t con, char* letter);
EXTC vm_result_t vm_tone(vm_connection_t con, size_t freq, size_t lengthms);
EXTC vm_result_t vm_pitch(vm_connection_t con, unsigned char value);
EXTC vm_result_t vm_rate(vm_connection_t con, unsigned char value);
EXTC vm_result_t vm_volume(vm_connection_t con, unsigned char value);
EXTC vm_result_t vm_procmode(vm_connection_t con, unsigned char procmode);
EXTC vm_result_t vm_family(vm_connection_t con, unsigned char lang, char* family);
*/

#endif /*__VOICEMAN_VMCLIENT_H__*/
