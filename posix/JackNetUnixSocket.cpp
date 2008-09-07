/*
Copyright (C) 2008 Romain Moret at Grame

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include "JackNetUnixSocket.h"

namespace Jack
{
    //utility *********************************************************************************************************
    int GetHostName ( char * name, int size )
    {
        if ( gethostname ( name, size ) == SOCKET_ERROR )
        {
            jack_error ( "Can't get 'hostname' : %s", strerror ( NET_ERROR_CODE ) );
            strcpy ( name, "default" );
            return SOCKET_ERROR;
        }
        return 0;
    }

    //construct/destruct***********************************************************************************************
    JackNetUnixSocket::JackNetUnixSocket()
    {
        fSockfd = 0;
        fPort = 0;
        fSendAddr.sin_family = AF_INET;
        fSendAddr.sin_addr.s_addr = htonl ( INADDR_ANY );
        memset ( &fSendAddr.sin_zero, 0, 8 );
        fRecvAddr.sin_family = AF_INET;
        fRecvAddr.sin_addr.s_addr = htonl ( INADDR_ANY );
        memset ( &fRecvAddr.sin_zero, 0, 8 );
    }

    JackNetUnixSocket::JackNetUnixSocket ( const char* ip, int port )
    {
        fSockfd = 0;
        fPort = port;
        fSendAddr.sin_family = AF_INET;
        fSendAddr.sin_port = htons ( port );
        inet_aton ( ip, &fSendAddr.sin_addr );
        memset ( &fSendAddr.sin_zero, 0, 8 );
        fRecvAddr.sin_family = AF_INET;
        fRecvAddr.sin_port = htons ( port );
        fRecvAddr.sin_addr.s_addr = htonl ( INADDR_ANY );
        memset ( &fRecvAddr.sin_zero, 0, 8 );
    }

    JackNetUnixSocket::JackNetUnixSocket ( const JackNetUnixSocket& socket )
    {
        fSockfd = 0;
        fPort = socket.fPort;
        fSendAddr = socket.fSendAddr;
        fRecvAddr = socket.fRecvAddr;
    }

    JackNetUnixSocket::~JackNetUnixSocket()
    {
        Close();
    }

    JackNetUnixSocket& JackNetUnixSocket::operator= ( const JackNetUnixSocket& socket )
    {
        if ( this != &socket )
        {
            fSockfd = 0;
            fPort = socket.fPort;
            fSendAddr = socket.fSendAddr;
            fRecvAddr = socket.fRecvAddr;
        }
        return *this;
    }

    //socket***********************************************************************************************************
    int JackNetUnixSocket::NewSocket()
    {
        if ( fSockfd )
        {
            Close();
            Reset();
        }
        fSockfd = socket ( AF_INET, SOCK_DGRAM, 0 );
        return fSockfd;
    }

    int JackNetUnixSocket::Bind()
    {
        return bind ( fSockfd, reinterpret_cast<socket_address_t*> ( &fRecvAddr ), sizeof ( socket_address_t ) );
    }

    int JackNetUnixSocket::BindWith ( const char* ip )
    {
        int addr_conv = inet_aton ( ip, &fRecvAddr.sin_addr );
        if ( addr_conv < 0 )
            return addr_conv;
        return Bind();
    }

    int JackNetUnixSocket::BindWith ( int port )
    {
        fRecvAddr.sin_port = htons ( port );
        return Bind();
    }

    int JackNetUnixSocket::Connect()
    {
        return connect ( fSockfd, reinterpret_cast<socket_address_t*> ( &fSendAddr ), sizeof ( socket_address_t ) );
    }

    int JackNetUnixSocket::ConnectTo ( const char* ip )
    {
        int addr_conv = inet_aton ( ip, &fSendAddr.sin_addr );
        if ( addr_conv < 0 )
            return addr_conv;
        return Connect();
    }

    void JackNetUnixSocket::Close()
    {
        if ( fSockfd )
            close ( fSockfd );
        fSockfd = 0;
    }

    void JackNetUnixSocket::Reset()
    {
        fSendAddr.sin_family = AF_INET;
        fSendAddr.sin_port = htons ( fPort );
        fSendAddr.sin_addr.s_addr = htonl ( INADDR_ANY );
        memset ( &fSendAddr.sin_zero, 0, 8 );
        fRecvAddr.sin_family = AF_INET;
        fRecvAddr.sin_port = htons ( fPort );
        fRecvAddr.sin_addr.s_addr = htonl ( INADDR_ANY );
        memset ( &fRecvAddr.sin_zero, 0, 8 );
    }

    bool JackNetUnixSocket::IsSocket()
    {
        return ( fSockfd ) ? true : false;
    }

    //IP/PORT***********************************************************************************************************
    void JackNetUnixSocket::SetPort ( int port )
    {
        fPort = port;
        fSendAddr.sin_port = htons ( port );
        fRecvAddr.sin_port = htons ( port );
    }

    int JackNetUnixSocket::GetPort()
    {
        return fPort;
    }

    //address***********************************************************************************************************
    int JackNetUnixSocket::SetAddress ( const char* ip, int port )
    {
        int addr_conv = inet_aton ( ip, &fSendAddr.sin_addr );
        if ( addr_conv < 0 )
            return addr_conv;
        fSendAddr.sin_port = htons ( port );
        return 0;
    }

    char* JackNetUnixSocket::GetSendIP()
    {
        return inet_ntoa ( fSendAddr.sin_addr );
    }

    char* JackNetUnixSocket::GetRecvIP()
    {
        return inet_ntoa ( fRecvAddr.sin_addr );
    }

    //utility************************************************************************************************************
    int JackNetUnixSocket::GetName ( char* name )
    {
        return gethostname ( name, 255 );
    }

    int JackNetUnixSocket::JoinMCastGroup ( const char* ip )
    {
        struct ip_mreq multicast_req;
        inet_aton ( ip, &multicast_req.imr_multiaddr );
        multicast_req.imr_interface.s_addr = htonl ( INADDR_ANY );
        return SetOption ( IPPROTO_IP, IP_ADD_MEMBERSHIP, &multicast_req, sizeof ( multicast_req ) );
    }

    //options************************************************************************************************************
    int JackNetUnixSocket::SetOption ( int level, int optname, const void* optval, socklen_t optlen )
    {
        return setsockopt ( fSockfd, level, optname, optval, optlen );
    }

    int JackNetUnixSocket::GetOption ( int level, int optname, void* optval, socklen_t* optlen )
    {
        return getsockopt ( fSockfd, level, optname, optval, optlen );
    }

    //timeout************************************************************************************************************
    int JackNetUnixSocket::SetTimeOut ( int us )
    {
        jack_log ( "JackNetUnixSocket::SetTimeout %d usecs", us );

        //negative timeout, or exceding 10s, return
        if ( ( us < 0 ) || ( us > 10000000 ) )
            return SOCKET_ERROR;
        struct timeval timeout;

        //less than 1sec
        if ( us < 1000000 )
        {
            timeout.tv_sec = 0;
            timeout.tv_usec = us;
        }
        //more than 1sec
        else
        {
            float sec = static_cast<float> ( us ) / 1000000.f;
            timeout.tv_sec = ( int ) sec;
            float usec = ( sec - static_cast<float> ( timeout.tv_sec ) ) * 1000000;
            timeout.tv_usec = ( int ) usec;
        }
        return SetOption ( SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof ( timeout ) );
    }

    //local loop**********************************************************************************************************
    int JackNetUnixSocket::SetLocalLoop()
    {
        char disable = 0;
        return SetOption ( IPPROTO_IP, IP_MULTICAST_LOOP, &disable, sizeof ( disable ) );
    }

    //network operations**************************************************************************************************
    int JackNetUnixSocket::SendTo ( const void* buffer, size_t nbytes, int flags )
    {
        return sendto ( fSockfd, buffer, nbytes, flags, reinterpret_cast<socket_address_t*> ( &fSendAddr ), sizeof ( socket_address_t ) );
    }

    int JackNetUnixSocket::SendTo ( const void* buffer, size_t nbytes, int flags, const char* ip )
    {
        int addr_conv = inet_aton ( ip, &fSendAddr.sin_addr );
        if ( addr_conv < 1 )
            return addr_conv;
        return SendTo ( buffer, nbytes, flags );
    }

    int JackNetUnixSocket::Send ( const void* buffer, size_t nbytes, int flags )
    {
        return send ( fSockfd, buffer, nbytes, flags );
    }

    int JackNetUnixSocket::RecvFrom ( void* buffer, size_t nbytes, int flags )
    {
        socklen_t addr_len = sizeof ( socket_address_t );
        return recvfrom ( fSockfd, buffer, nbytes, flags, reinterpret_cast<socket_address_t*> ( &fRecvAddr ), &addr_len );
    }

    int JackNetUnixSocket::Recv ( void* buffer, size_t nbytes, int flags )
    {
        return recv ( fSockfd, buffer, nbytes, flags );
    }

    int JackNetUnixSocket::CatchHost ( void* buffer, size_t nbytes, int flags )
    {
        socklen_t addr_len = sizeof ( socket_address_t );
        return recvfrom ( fSockfd, buffer, nbytes, flags, reinterpret_cast<socket_address_t*> ( &fSendAddr ), &addr_len );
    }

    net_error_t JackNetUnixSocket::GetError()
    {
        switch ( errno )
        {
            case EAGAIN:
                return NET_NO_DATA;
            case ECONNABORTED:
                return NET_CONN_ERROR;
            case EINVAL:
                return NET_CONN_ERROR;
            case ECONNREFUSED:
                return NET_CONN_ERROR;
            case ECONNRESET:
                return NET_CONN_ERROR;
            case EHOSTDOWN:
                return NET_CONN_ERROR;
            case EHOSTUNREACH:
                return NET_CONN_ERROR;
            default:
                return NET_OP_ERROR;
        }
    }
}