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

#include<assert.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<unistd.h>
#include<errno.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<sys/socket.h>
#include<sys/un.h>
#include<arpa/inet.h>
#include<netdb.h>
#include<resolv.h>
#include"vmclient.h"

#define IO_BUF_SIZE 2048
#define INET_PREFIX "inet:"

static char* check_inet_prefix(char* p)
{
  unsigned long k;
  char* pr = NULL;
  if (strlen(p) < strlen(INET_PREFIX))
    return NULL;
  pr =INET_PREFIX;
  for(k = 0;pr[k] != '\0';k++)
    if (p[k] != pr[k])
      return NULL;
  return &p[k];
}

static int parse_inet_str(char* s, char* host, unsigned long* port)
{
  unsigned long k = 0;/*for colons counting;*/
  char* p=s;
  while(*p)
    {
      if (*p == ':')
	k++;
      p++;
    } /*while(*p);*/
  if (k == 0)
    {
      strcpy(host, s);
      *port = VOICEMAN_DEFAULT_PORT;
      return 1;
    }
  if (k != 1)
    return 0;
  for(k = 0;s[k] != ':';k++)
    host[k] = s[k];
  host[k] = '\0';
  k++;
  *port = 0;
  if (s[k]=='\0')
    {
      *port = VOICEMAN_DEFAULT_PORT;
      return 1;
    }
  for(;s[k]!='\0';k++)
    {
      if (s[k] < '0' || s[k] > '9')
	return 0;
      *port *= 10;
      *port += (s[k] - '0');
    }
  return 1;
}

vm_connection_t vm_connect()
{
  unsigned long port;
  char* host = NULL;
  char* inet = NULL;
  vm_connection_t con = VOICEMAN_BAD_CONNECTION;
  char* vm=getenv("VOICEMAN");
  if (vm == NULL || vm[0] == '\0')
    return vm_connect_unix(VOICEMAN_DEFAULT_SOCKET);
  inet=check_inet_prefix(vm);
  if (inet == NULL)
    return vm_connect_unix(vm);
  host = (char*)malloc(strlen(inet) + 1);
  if (host == NULL)
    return VOICEMAN_BAD_CONNECTION;
  if (!parse_inet_str(inet, host, &port))
    {
      free(host);
      return VOICEMAN_BAD_CONNECTION;
    }
  con = vm_connect_inet(host, port);
  free(host);
  return con;
}

vm_connection_t vm_connect_unix(char* path)
{
  struct sockaddr_un addr;
  int fd;
  assert(path);
  if (!path)
    return VOICEMAN_BAD_CONNECTION;
  fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd == -1)
    return VOICEMAN_BAD_CONNECTION;
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, path, sizeof(addr.sun_path));
  if (connect(fd, (struct sockaddr*)&addr, sizeof(struct sockaddr_un)) == -1)
    {
      close(fd);
      return VOICEMAN_BAD_CONNECTION;
    }
  return fd;
}

vm_connection_t vm_connect_inet(char* host, unsigned long port)
{
  struct hostent* host_addr = NULL;
  int fd;
  struct sockaddr_in saddr;
  assert(host);
  if (!host)
    return VOICEMAN_BAD_CONNECTION;
  fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd == -1)
    return VOICEMAN_BAD_CONNECTION;
  bzero(&saddr, sizeof(struct sockaddr_in));
  saddr.sin_family = AF_INET;
  saddr.sin_port = htons(port);
  host_addr = gethostbyname(host);
  if (host_addr == NULL)
    {
      close(fd);
      return VOICEMAN_BAD_CONNECTION;
    }
  memcpy(&saddr.sin_addr, host_addr->h_addr, 4);
  if (connect(fd, (struct sockaddr*)&saddr, sizeof(struct sockaddr_in)) == -1)
    {
      close(fd);
      return VOICEMAN_BAD_CONNECTION;
    }
  return fd;
}

void vm_close(vm_connection_t con)
{
  if (con == VOICEMAN_BAD_CONNECTION)
    return;
  close(con);
}

/*RWrites block of specified length and produces subsequent calls of write() in case of short write operation*/
static long writeblock(int fd, void* buf, unsigned long bufSize)
{
  char* b = (char*)buf;
  unsigned long c = 0;
  assert(buf);
  while(c < bufSize)
    {
      long res = write(fd, &b[c], bufSize - c);
      if (res == -1)
	return -1;
      assert(res >= 0);
      c += (unsigned long)res;
    } /*while();*/
  assert(c == bufSize);
  return (long)c;
}

/*Writes buffer of any length producing subsequent calls to write fixed length blocks*/
static long writebuf(int fd, void* buf, unsigned long bufSize)
{
  char* b = (char*)buf;
  unsigned long c = 0;
  assert(buf);
  while(c < bufSize)
    {
      unsigned long requiredSize = bufSize > c + IO_BUF_SIZE?IO_BUF_SIZE:(unsigned long)(bufSize - c);
      unsigned long res = writeblock(fd, &b[c], requiredSize);
      if (res == -1)
	return -1;
      assert(res == (long)requiredSize);
      c += (unsigned long)res;
    } /*while();*/
  assert(c == bufSize);
  return (long)c;
}

vm_result_t vm_text(vm_connection_t con, char* text)
{
  unsigned long k, i;
  char* t;
  assert(con != VOICEMAN_BAD_CONNECTION);
  if (con == VOICEMAN_BAD_CONNECTION)
    return VOICEMAN_ERROR;
  assert(text);
  if (!text)
    return VOICEMAN_ERROR;
  k = strlen(text);
  if (k == 0)
    return VOICEMAN_OK;
  t = (char*)malloc(k + 1);
  if (t == NULL)
    return VOICEMAN_ERROR;
  strcpy(t, text);
  for(i = 0;i < k;i++)/*all \n must be replaced with spaces;*/
    if (t[i] == '\n')
      t[i] = ' ';
  if (writeblock(con, "T:", 2) == -1)
    {
      free(t);
      return VOICEMAN_ERROR;
    }
  if (writebuf(con, t, k) == -1)
    {
      free(t);
      return VOICEMAN_ERROR;
    }
  free(t);
  if (writeblock(con, "\n", 1) == -1)
    return VOICEMAN_ERROR;
  return VOICEMAN_OK;
}

vm_result_t vm_stop(vm_connection_t con)
{
  assert(con != VOICEMAN_BAD_CONNECTION);
  if (con == VOICEMAN_BAD_CONNECTION)
    return VOICEMAN_ERROR;
  if (writeblock(con, "S:\n", 3) == -1)
    return VOICEMAN_ERROR;
  return VOICEMAN_OK;
}

vm_result_t vm_letter(vm_connection_t con, char* letter)
{
  assert(con != VOICEMAN_BAD_CONNECTION);
  if (con == VOICEMAN_BAD_CONNECTION)
    return VOICEMAN_ERROR;
  assert(letter);
  if (!letter)
    return VOICEMAN_ERROR;
  if (writeblock(con, "L:", 2) == -1)
    return VOICEMAN_ERROR;
  if (writebuf(con, letter, strlen(letter)) == -1)
    return VOICEMAN_ERROR;
  if (writeblock(con, "\n", 1) == -1)
    return VOICEMAN_ERROR;
  return VOICEMAN_OK;
}

vm_result_t vm_tone(vm_connection_t con, unsigned long freq, unsigned long lengthms)
{
  char buf[64];
  assert(con != VOICEMAN_BAD_CONNECTION);
  if (con == VOICEMAN_BAD_CONNECTION)
    return VOICEMAN_ERROR;
  sprintf(buf, "B:%lu:%lu\n", freq, lengthms);
  if (writebuf(con, buf, strlen(buf)) == -1)
    return VOICEMAN_ERROR;
  return VOICEMAN_OK;
}

vm_result_t vm_pitch(vm_connection_t con, unsigned char value)
{
  char buf[64];
  assert(con != VOICEMAN_BAD_CONNECTION);
  if (con == VOICEMAN_BAD_CONNECTION)
    return VOICEMAN_ERROR;
  sprintf(buf, "P:%u\n", value);
  if (writebuf(con, buf, strlen(buf)) == -1)
    return VOICEMAN_ERROR;
  return VOICEMAN_OK;
}

vm_result_t vm_rate(vm_connection_t con, unsigned char value)
{
  char buf[64];
  assert(con != VOICEMAN_BAD_CONNECTION);
  if (con == VOICEMAN_BAD_CONNECTION)
    return VOICEMAN_ERROR;
  sprintf(buf, "R:%u\n", value);
  if (writebuf(con, buf, strlen(buf)) == -1)
    return VOICEMAN_ERROR;
  return VOICEMAN_OK;
}

vm_result_t vm_volume(vm_connection_t con, unsigned char value)
{
  char buf[64];
  assert(con != VOICEMAN_BAD_CONNECTION);
  if (con == VOICEMAN_BAD_CONNECTION)
    return VOICEMAN_ERROR;
  sprintf(buf, "V:%u\n", value);
  if (writebuf(con, buf, strlen(buf)) == -1)
    return VOICEMAN_ERROR;
  return VOICEMAN_OK;
}

vm_result_t vm_procmode(vm_connection_t con, unsigned char procmode)
{
  char buf[32];
  assert(con != VOICEMAN_BAD_CONNECTION);
  if (con == VOICEMAN_BAD_CONNECTION)
    return VOICEMAN_ERROR;
  if (procmode == VOICEMAN_PROCMODE_ALL)
    strcpy(buf, "M:all\n"); else
  if (procmode == VOICEMAN_PROCMODE_NONE)
    strcpy(buf, "M:none\n"); else
  if (procmode == VOICEMAN_PROCMODE_SOME)
    strcpy(buf, "M:some\n"); else
    return VOICEMAN_ERROR;
  if (writebuf(con, buf, strlen(buf)) == -1)
    return VOICEMAN_ERROR;
  return VOICEMAN_OK;
}

vm_result_t vm_family(vm_connection_t con, unsigned char lang, char* family)
{
  unsigned long i;
  char* buf = NULL;
  assert(con != VOICEMAN_BAD_CONNECTION);
  if (con == VOICEMAN_BAD_CONNECTION)
    return VOICEMAN_ERROR;
  assert(family);
  if (!family)
    return VOICEMAN_ERROR;
  for(i = 0;family[i] != '\0';i++)
    if (family[i] == ':' || family[i] == '\n')
      return VOICEMAN_ERROR;
  buf = (char*)malloc(strlen(family) + 32);
  if (buf == NULL)
    return VOICEMAN_ERROR;
  if (lang == VOICEMAN_LANG_NONE)
    sprintf(buf, "F:%s\n", family); else
  if (lang == VOICEMAN_LANG_ENG)
    sprintf(buf, "F:%s:eng\n", family); else
  if (lang == VOICEMAN_LANG_RUS)
    sprintf(buf, "F:%s:rus\n", family); else
    {
      free(buf);
      return VOICEMAN_ERROR;
    }
  if (writebuf(con, buf, strlen(buf)) == -1)
    {
      free(buf);
      return VOICEMAN_ERROR;
    }
  free(buf);
  return VOICEMAN_OK;
}
