/**
 *
 * @file	RedisClientPSub.cpp		
 * @brief the Pub/Sub method of the CRedisClient
 * @author: 		yp
 * @date: 		Aug 6, 2015
 * Revision Description: initial version
 */
#include "Command.h"
#include "CRedisClient.h"









// uint64_t CRedisClient::psubscribe( VecString& pattern, VecString& value )
// {
//	Command cmd( "PSUBSCRIBE" );
//	VecString::const_iterator it = pattern.begin();
//	for ( ; it != pattern.end(); it++ )
//	{
//		cmd << *it;
//	}
//	_getArry( cmd, value );
//	return value.size();
// }



 uint64_t CRedisClient::publish( const string& channel, const string& message )
 {
	Command cmd( "PUBLISH" );
	cmd << channel << message;
	int64_t num = 0;
	_getInt( cmd, num );
	return num;
 }




 void CRedisClient::psubchannels( VecString& pattern, VecString& value )
 {
	Command cmd( "PUBSUB" );
	cmd << "CHANNELS";

	if ( pattern.size() != 0 )
	{
		VecString::const_iterator it = pattern.begin();
		for ( ; it != pattern.end(); it++ )
		{
			cmd << *it ;
		}
	}

	_getArry( cmd, value );
 }



 void CRedisClient::psubnumsub( VecString& channel, CRedisClient::MapString& value )
 {
	Command cmd( "PUBSUB" );
	cmd << "NUMSUB";
	if ( channel.size() != 0 )
	{
		VecString::const_iterator it = channel.begin();
		for ( ; it != channel.end(); it++ )
		{
			cmd << *it ;
		}
	}

	_getArry( cmd, value );
 }



 uint64_t CRedisClient::psubnumpat()
 {
	Command cmd( "PUBSUB" );
	cmd << "NUMPAT";
	int64_t num = 0;
	_getInt( cmd, num );
	return num;
 }




 void CRedisClient::punsubscribe( VecString& pattern, CResult& result )
 {
	_socket.clearBuffer();

	Command cmd( "PUNSUBSCRIBE" );
	if ( pattern.size() != 0 )
	{
		VecString::const_iterator it = pattern.begin();
		for ( ; it != pattern.end(); it++ )
		{
			cmd << *it ;
		}
	}
	_sendCommand( cmd );

	_getReply( result );
 }





// void CRedisClient::subscribe( VecString& channel, CResult& result )
// {
//	Command cmd( "SUBSCRIBE" );
//	VecString::const_iterator it = channel.begin();
//	for ( ; it != channel.end(); it++ )
//	{
//		cmd << *it ;
//	}
//	_getArry( cmd, result );
// }




 void CRedisClient::unsubscribe( VecString& channel, CResult& result )
 {
	_socket.clearBuffer();

	Command cmd( "UNSUBSCRIBE" );
	if ( channel.size() != 0 )
	{
		VecString::const_iterator it = channel.begin();
		for ( ; it != channel.end(); it++ )
		{
			cmd << *it ;
		}
	}
	_sendCommand( cmd );

	_getReply( result );
 }












