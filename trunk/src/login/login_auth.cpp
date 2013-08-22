/*
===========================================================================

  Copyright (c) 2010-2012 Darkstar Dev Teams

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see http://www.gnu.org/licenses/

  This file is part of DarkStar-server source code.

===========================================================================
*/

#include "../common/malloc.h"
#include "../common/showmsg.h"
#include "../common/socket.h"

#include "account.h"
#include "login.h"
#include "login_auth.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
int32 login_fd;					//main fd(socket) of server

/*
*	
*		LOGIN SECTION
*
*/
int32 connect_client_login(int32 listenfd)
{
	int32 fd = 0;
	struct sockaddr_in client_address;
	if( ( fd = connect_client(listenfd,client_address) ) != -1 )
	{
		int32 code = create_session(fd, recv_to_fifo, send_from_fifo, login_parse);
		session[fd]->client_addr = ntohl(client_address.sin_addr.s_addr);
		session[fd]->client_port = ntohs(client_address.sin_port);
		return fd;
	}
	return -1;
}



int32 login_parse(int32 fd)
{
	login_session_data_t* sd = (login_session_data_t*)session[fd]->session_data;

	//check if sd will not defined
	if( sd == NULL )
	{
		CREATE(session[fd]->session_data,login_session_data_t,1);
		sd = (login_session_data_t*)session[fd]->session_data;
		sd->serviced = 0;
		login_sd_list.push_back(sd);
		sd->client_addr = session[fd]->client_addr;
		sd->client_port = session[fd]->client_port;
		sd->login_fd	= fd;
	}

	if( session[fd]->flag.eof != 0 )
	{
		do_close_login(sd,fd);
		return 0;
	}
	

	//all auth packets have one structure: 
	// [login][passwords][code] => summary assign 33 bytes
	if( session[fd]->rdata_size == 33 )
	{
		unsigned char* buff = session[fd]->rdata;
		int8 code = RBUFB(buff,32);

		char login[17];
		char password[17];

		memset(login,0,sizeof(login));
		memset(sd->login,0,sizeof(sd->login));
		memset(password,0,sizeof(password));

		memcpy(login,buff,16);
		memcpy(sd->login,login,16);
		memcpy(password,buff+16,16);

		//data check
		if( login_datacheck(login,3,sizeof(login)) == -1 || login_datacheck(password,6,sizeof(password)) == -1 )
		{
			
			WBUFB(session[fd]->wdata,0) = LOGIN_ERROR;
			WFIFOSET(fd,1);
			do_close_login(sd,fd);
			return -1;
		}

		switch(code)
		{
		case LOGIN_ATTEMPT:
			{
			const int8* fmtQuery = "SELECT accounts.id,accounts.status \
									FROM accounts \
									WHERE accounts.login = '%s' AND accounts.password = PASSWORD('%s')";
			int32 ret = Sql_Query(SqlHandle,fmtQuery,login,password);	
			if( ret != SQL_ERROR  && Sql_NumRows(SqlHandle) != 0)
			{
				ret = Sql_NextRow(SqlHandle);

				sd->accid   = (uint32)Sql_GetUIntData(SqlHandle,0);
				uint8 status = (uint8)Sql_GetUIntData(SqlHandle,1);

				if( status & ACCST_NORMAL )
				{
					
					fmtQuery = "UPDATE accounts SET accounts.timelastmodify = NULL WHERE accounts.id = %d";
					Sql_Query(SqlHandle,fmtQuery,sd->accid);
					memset(session[fd]->wdata,0,33);
					WBUFB(session[fd]->wdata,0) = LOGIN_SUCCESS;
					WBUFL(session[fd]->wdata,1) = sd->accid;
					WFIFOSET(fd,33);
					flush_fifo(fd);
					do_close_tcp(fd);
					return 0;
				}
				else if( status & ACCST_BANNED)
				{
					memset(session[fd]->wdata,0,33);
				
					WFIFOSET(fd,33);
					do_close_login(sd,fd);
					return 0;
				}

				
				int numCons = 0;
				for(login_sd_list_t::iterator i = login_sd_list.begin(); i != login_sd_list.end(); i++ ){
					if( (*i)->accid == sd->accid )
					{
						numCons++;
					}
				}

				if(numCons>1){
					
					for(int j=0; j<(numCons-1); j++){
						for(login_sd_list_t::iterator i = login_sd_list.begin(); i != login_sd_list.end(); i++ ){
							if( (*i)->accid == sd->accid )
							{
								
								login_sd_list.erase(i);
								break;
							}
						}
					} 
				}
				
				return 0;
			}else
			{
				WBUFB(session[fd]->wdata,0) = LOGIN_ERROR;
				WFIFOSET(fd,1);
				
				do_close_login(sd,fd);
			}
			}
			break;
		case LOGIN_CREATE:
			WBUFB(session[fd]->wdata,0) = LOGIN_ERROR_CREATE ;
				WFIFOSET(fd,1);
				
				do_close_login(sd,fd);
			
			break;
		default:
			
			do_close_login(sd,fd);
			break;
		};
		
	}else{
		do_close_login(sd,fd);
	}
	return 0;
};


int32 do_close_login(login_session_data_t* loginsd,int32 fd)
{
	
	
	if(session[fd]->session_data)
	{
		
	do_close_tcp(fd);
	
	}
	return 0;
}

int8 login_datacheck(const char *buf,size_t MinSize, size_t MaxSize)
{
	if(buf !=NULL)
	{
	//ShowDebug(CL_BG_RED"LOGIN CHECK DATA BUF %s MINSIZE %u MAXSIZE %u\n"CL_RESET,buf,MinSize,MaxSize);
	size_t str_size = 0;
	
	str_size = strnlen(buf,MaxSize);
	if( str_size < MinSize )
	{
		//ShowDebug(CL_BG_RED"LOGIN CHECK DATA BUF %u MINSIZE %u \n"CL_RESET,str_size,MinSize);
		return -1;
	}
	if( str_size > MaxSize )
	{
		//ShowDebug(CL_BG_RED"LOGIN CHECK DATA BUF %u MAXSIZE %u \n"CL_RESET,str_size,MaxSize);
		return -1;
	}

	for( size_t i = 0; i < str_size; ++i )
	{
		if( !isalpha(buf[i]) && !isdigit(buf[i]) )
		{
			return -1;
		}
	}
	return 0;
	}
	else
	{
    ShowDebug(CL_BG_RED"ELSE LOGIN CHECK DATA BUF %s MINSIZE %u MAXSIZE %u\n"CL_RESET,buf,MinSize,MaxSize);
	return false;
	}
	return false;
}
