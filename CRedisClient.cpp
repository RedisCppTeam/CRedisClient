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
    // fix bug #13 .call close() in advance to ensure the success of the connection.
    _socket.close();
    connect();
}

void CRedisClient::closeConnect()
{
    _socket.close();
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
std::cout<<sdData<<std::endl;
    size_t sded = 0;
    int sd = 0;
    do{
        sd = _socket.sendBytes( sdData, sdLen-sded );
        if ( sd <= 0 )
        {
            throw ConnectErr("sendByte exception!");
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



bool CRedisClient::_replyBulk(CResult& result , const std::string &len )
{
    // get the number of CResult received .
    int64_t protoLen = _valueFromString<int64_t>(len .substr(1));

    if ( protoLen == -1 )
    {
        result = "";
        result.setType(REDIS_REPLY_NIL);
        return false;
    }

    _socket.readN( protoLen, result );
    string tmp;
    _socket.readN( 2, tmp );

    result.setType( REDIS_REPLY_STRING );
    return true;
}

uint64_t CRedisClient::_replyMultiBulk(CResult& result, const std::string &line )
{
    // get the number of CResult received .
   int64_t replyNum = _valueFromString<int64_t>( line.substr(1) );
   //The concept of Null Array exists as well
   if ( -1 == replyNum )
   {
        result.setType( REDIS_REPLY_NIL );
        return 0;
   }

   CResult ele;
   for ( int i = 0; i< replyNum; i++ )
   {
       _getReply( ele );
       result.addElement( ele );
   }
   
   return result.getArry().size();
}

void CRedisClient::_getStringVecFromArry(const CResult::ListCResult &arry, CRedisClient::VecString &values )
{
    CResult::ListCResult::const_iterator it = arry.begin();
    CResult::ListCResult::const_iterator end = arry.end();

    for ( ; it != end; ++it )
    {
        values.push_back( static_cast<string>(*it) );
    }
}

void CRedisClient::_getStringMapFromArry(const CResult::ListCResult &arry, CRedisClient::MapString &pairs)
{
    CResult::ListCResult::const_iterator it = arry.begin();
    CResult::ListCResult::const_iterator it2 = it;
    CResult::ListCResult::const_iterator end = arry.end();

    for ( ; it != end; ++it )
    {
        it2 = it++;		// the next element is value.
        pairs.insert( MapString::value_type( *it2, *it ) );
    }
}



bool CRedisClient::_getStatus(  Command& cmd , string& status )
{
    CResult result;
    _socket.clearBuffer();
    _sendCommand( cmd );
    _getReply( result );

    ReplyType type = result.getType();
    if ( REDIS_REPLY_NIL ==  type )
    {
        return false;
    }
    if ( REDIS_REPLY_ERROR == type )
    {
        throw ReplyErr( result.getErrorString() );
    }
    if ( REDIS_REPLY_STATUS != type )
    {
       throw ProtocolErr( cmd.getCommand() + ": data recved is not status" );
    }
    status = result.getStatus();
    return true;
}



bool CRedisClient::_getInt(  Command& cmd , int64_t& number )
{
    number = 0;
    CResult result;
    _socket.clearBuffer();
    _sendCommand( cmd );
    _getReply( result );

    ReplyType type = result.getType();
    if ( REDIS_REPLY_NIL ==  type )
    {
        return false;
    }
    if ( REDIS_REPLY_ERROR == type )
    {
        throw ReplyErr( result.getErrorString() );
    }
    if ( REDIS_REPLY_INTEGERER != type )
    {
       throw ProtocolErr( cmd.getCommand() + ": data recved is not iintergerer" );
    }
    number  = result.getInt();
    return true;
}

bool CRedisClient::_getString(  Command& cmd , string& value  )
{
    CResult result;
    _socket.clearBuffer();
    _sendCommand( cmd );
    _getReply( result );

    ReplyType type = result.getType();
    if ( REDIS_REPLY_NIL ==  type )
    {
        return false;
    }
    if ( REDIS_REPLY_ERROR == type )
    {
        throw ReplyErr( result.getErrorString() );
    }
    if ( REDIS_REPLY_STRING != type )
    {
       throw ProtocolErr( cmd.getCommand() + ": data recved is not string" );
    }
    value = result.getString();
    return true;
}

bool CRedisClient::_getArry(Command &cmd, CResult &result)
{
    _socket.clearBuffer();
    _sendCommand( cmd );
    _getReply( result );

    ReplyType type = result.getType();

    if ( REDIS_REPLY_NIL == type )
    {
        return false;
    }
    if ( REDIS_REPLY_ERROR == type )
    {
        throw ReplyErr( result.getErrorString() );
    }
    if ( REDIS_REPLY_ARRAY != type )
    {
       throw ProtocolErr( cmd.getCommand() + ": data recved is not arry" );
    }

    return true;
}


uint64_t CRedisClient::_getArry(Command &cmd, VecString &values )
{
    _socket.clearBuffer();
    _sendCommand( cmd );
    CResult result;
    _getReply( result );

    ReplyType type = result.getType();

    if ( REDIS_REPLY_ERROR == type )
    {
        throw ReplyErr( result.getErrorString() );
    }
    if ( REDIS_REPLY_ARRAY != type )
    {
       throw ProtocolErr( cmd.getCommand() + ": data recved is not arry" );
    }

    _getStringVecFromArry( result.getArry(), values );
    return values.size();
}

uint64_t CRedisClient::_getArry(Command &cmd, CRedisClient::MapString &pairs )
{
    _socket.clearBuffer();
    _sendCommand( cmd );
    CResult result;
    _getReply( result );

    ReplyType type = result.getType();

    if ( REDIS_REPLY_ERROR == type )
    {
        throw ReplyErr( result.getErrorString() );
    }
    if ( REDIS_REPLY_ARRAY != type )
    {
       throw ProtocolErr( cmd.getCommand() + ": data recved is not arry" );
    }

    _getStringMapFromArry( result.getArry(), pairs );
    return pairs.size();
}
