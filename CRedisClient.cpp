/**
 *
 * @file	CRedisClient.cpp
 * @brief CRedisClient 类的公共　方法。
 *
 * @author: 		yuhaiyang
 * @date: 		2015年6月14日
 *
 */


#include "CRedisClient.h"
#include "Poco/Types.h"


const char CRedisClient:: PREFIX_REPLY_STATUS = '+';
const char CRedisClient:: PREFIX_REPLY_ERR = '-';
const char CRedisClient:: PREFIX_REPLY_INT = ':';
const char CRedisClient:: PREFIX_BULK_REPLY = '$';
const char CRedisClient:: PREFIX_MULTI_BULK_REPLY = '*';

//==============================based method====================================
CRedisClient::CRedisClient()
{
    Timespan timeout( 5 ,0 );
    _timeout = timeout;
}

CRedisClient::~CRedisClient()
{

}

void CRedisClient::setAddress(const string &ip, UInt16 port)
{
    Net::SocketAddress addr( ip, port );
    _addr =  addr;
    return;
}

string CRedisClient::getAddrip()
{
    return _addr.host().toString();
}

string CRedisClient::getAddr()
{
    return _addr.toString();
}



void CRedisClient::setTimeout(long seconds, long microseconds)
{
    Timespan timeout( seconds, microseconds );
    _timeout =  timeout;
}


void CRedisClient::connect( const string &ip, UInt16 port )
{
    setAddress( ip, port );
    _socket.connect( _addr, _timeout );
    _socket.setSendTimeout( _timeout );
    _socket.setReceiveTimeout( _timeout );
}

void CRedisClient::connect()
{
    _socket.connect( _addr,_timeout );
}

void CRedisClient::reconnect()
{
    _socket.shutdown();
    connect();
}

void CRedisClient::closeConnect()
{
	_socket.shutdown();
}

//bool CRedisClient::ping()
//{
//    try
//    {
//        _socket.clearBuffer();
//
//        Command cmd( "PING" );
//        _sendCommand( cmd );
//        DEBUGOUT( "send", string( cmd ) );
//
//        string value = _replyStatus();
//        if ( value != "PONG" )
//        {
//            return false;
//        }else
//        {
//            return true;
//        }
//    }catch ( std::exception& e )
//    {
//        DEBUGOUT( "Ping catch exception:", e.what() );
//       return false;
//    }
//}
//


//==============================method offen used====================================

void CRedisClient::_sendCommand( const string &cmd )
{
    const char* sdData = cmd.data();
    size_t sdLen = cmd.length();

    size_t sded = 0;
    int sd = 0;
    do{
        sd = _socket.sendBytes( sdData, sdLen-sded );
        if ( sd < 0 )
        {
            throw ConnectErr("sendByte failed!");
        }
        sded += sd;
        sdData += sd;
    }while( sded < sdLen );
    return ;
}

bool CRedisClient::_getReply( CResult &result )
{
    result.clear();
    std::string line;
    _socket.readLine( line );
    DEBUGOUT( "row data",line )
    switch ( line[0] )
    {
    case PREFIX_REPLY_INT:
        result.setType( REDIS_REPLY_INTEGERER );
        result = line.substr( 1 );
        break;
    case PREFIX_REPLY_STATUS:
        result.setType( REDIS_REPLY_STATUS );
        result = line.substr( 1 );
        break;
    case PREFIX_REPLY_ERR:
        result.setType( REDIS_REPLY_ERROR );
        result = line.substr( 1 );
        break;
    case PREFIX_BULK_REPLY:
        result.setType( REDIS_REPLY_STRING );
        _replyBulk( result,line );
        break;
    case PREFIX_MULTI_BULK_REPLY:
        result.setType( REDIS_REPLY_ARRAY );
        _replyMultiBulk( result,line );
        break;
    default:
        throw ProtocolErr( "unknow type" );
        break;
    }
    return true;
}

uint8_t CRedisClient::_replyBulk(CResult& value , const std::string &line)
{
    // get the number of CResult received .
    int64_t len = _valueFromString<int64_t>( line.substr(1) );

    if ( len == -1 )
    {
        value = "";
        value.setType( REDIS_REPLY_NIL );
        return 0;
    }

    _socket.readLine( value );

    if ( value.length() == ( uint32_t)len )
    {
        value.setType( REDIS_REPLY_STRING );
        return 1;
    }else
    {
        value.clear();
        throw ProtocolErr( "invalid bulk reply data; length of data is unexpected" );
    }
}


uint64_t CRedisClient::_replyMultiBulk(CResult& result, const std::string &line )
{
    // get the number of CResult received .
   int64_t replyNum = _valueFromString<int64_t>( line.substr(1) );
   CResult ele;
   for ( int i = 0; i< replyNum; i++ )
   {
       _getReply( ele );
       result.addElement( ele );
   }
   
   return result.getArry().size();
}

