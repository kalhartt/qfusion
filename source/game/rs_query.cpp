#include "g_local.h"

static stat_query_api_t *rs_sqapi;

cvar_t *rs_statsEnabled;
cvar_t *rs_statsId;
cvar_t *rs_statsKey;

void RS_InitQuery( void )
{
	rs_statsEnabled = trap_Cvar_Get( "rs_statsEnabled", "0", CVAR_ARCHIVE );
	rs_statsId = trap_Cvar_Get( "rs_statsId", "", CVAR_ARCHIVE );
	rs_statsKey = trap_Cvar_Get( "rs_statsKey", "", CVAR_ARCHIVE );
	rs_sqapi = trap_GetStatQueryAPI();
	if( !rs_sqapi )
		trap_Cvar_ForceSet( rs_statsEnabled->name, "0" );
}

void RS_ShutdownQuery( void )
{
}

/**
 * RS_GenToken
 * Generate the server token for the given string
 * @param dst The token string to append to
 * @param str The message to generate a token for
 * @return void
 */
static void RS_GenToken( char *token, const char *str )
{
	unsigned char digest[SHA256_DIGEST_SIZE];
	char *digest64,
		*message = va( "%s|%s", str, rs_statsKey->string );
	size_t outlen;

	sha256( (const unsigned char*)message, strlen( message ), digest );
	digest64 = (char*)base64_encode( digest, (size_t)SHA256_DIGEST_SIZE, &outlen );

	Q_strncatz( token, digest64, MAX_STRING_CHARS );
	free( digest64 );
}

/**
 * Sign a query with the generated server token
 * @param query The query to sign
 * @param uTime The time to sign the query with
 */
static void RS_SignQuery( stat_query_t *query, int uTime )
{
	char *token;

	token = (char*)G_Malloc( MAX_STRING_CHARS );
	Q_strncpyz( token, rs_statsId->string, sizeof( token ) );
	Q_strncatz( token, ".", sizeof( token ) );
	RS_GenToken( token, va( "%d", uTime ) );

	rs_sqapi->SetField( query, "sid", rs_statsId->string );
	rs_sqapi->SetField( query, "uTime", va( "%d", uTime ) );
	rs_sqapi->SetField( query, "sToken", token );
	G_Free( token );
}

/**
 * AuthPlayer callback function
 * @param query Query calling this function
 * @param success True on any response
 * @param customp gclient_t of the client being queried
 * @return void
 */
void RS_AuthRegister_Done( stat_query_t *query, qboolean success, void *customp )
{
}

/**
 * Register a player
 * @param client The client being authenticated
 * @param name The player's auth name
 * @param pass The player's pre-hashed password
 * @param email The player's email address
 * @param nick The player's nick to protect
 * @return void
 */
void RS_AuthRegister( gclient_t *client, const char *name, const char *pass, const char *email, const char *nick )
{
	stat_query_t *query;
	char url[MAX_STRING_CHARS], *b64name, *b64email, *b64nick;

	if( !name || !strlen( name ) ||
		!pass || !strlen( pass ) ||
		!email || !strlen( email ) ||
		!nick || !strlen( nick ) )
		return;

	// Make the URL
	b64name = (char*)base64_encode( (unsigned char *)name, strlen( name ), NULL );
	Q_strncpyz( url, "api/player/", sizeof( url ) - 1 );
	Q_strncatz( url, b64name, sizeof( url ) - 1 );
	free( b64name );

	// Form the query and query parameters
	b64nick = (char*)base64_encode( (unsigned char *)nick, strlen( nick ), NULL );
	b64email = (char*)base64_encode( (unsigned char *)email, strlen( email ), NULL );
	query = rs_sqapi->CreateQuery( url, qfalse );
	rs_sqapi->SetField( query, "email", b64email );
	rs_sqapi->SetField( query, "nick", b64nick );
	rs_sqapi->SetField( query, "cToken", pass );

	RS_SignQuery( query, (int)time( NULL ) );
	rs_sqapi->SetCallback( query, RS_AuthRegister_Done, (void*)client );
	rs_sqapi->Send( query );
	free( b64email );
	free( b64nick );
	query = NULL;
}

/**
 * AuthPlayer callback function
 * @param query Query calling this function
 * @param success True on any response
 * @param customp gclient_t of the client being queried
 * @return void
 */
void RS_AuthPlayer_Done( stat_query_t *query, qboolean success, void *customp )
{
}

/**
 * Authenticate a player
 * @param client The client being authenticated
 * @param name The player's auth name
 * @param ctoken The client's signed token
 * @param uTime Unix timestamp tokens were generated with
 * @param mapId Id number of the map to return playerdata for
 * @return void
 */
void RS_AuthPlayer( gclient_t *client, const char *name, const char *ctoken, int uTime, int mapId )
{
	stat_query_t *query;
	char url[MAX_STRING_CHARS], *b64name;

	if( !name || !strlen( name ) || !ctoken || !strlen( ctoken ) )
		return;

	// Make the URL
	b64name = (char*)base64_encode( (unsigned char *)name, strlen( name ), NULL );
	Q_strncpyz( url, "api/player/", sizeof( url ) - 1 );
	Q_strncatz( url, b64name, sizeof( url ) - 1 );
	free( b64name );

	// Form the query and query parameters
	query = rs_sqapi->CreateQuery( url, qtrue );
	rs_sqapi->SetField( query, "mid",  va( "%d", mapId ) );
	rs_sqapi->SetField( query, "cToken", ctoken );

	RS_SignQuery( query, uTime );
	rs_sqapi->SetCallback( query, RS_AuthPlayer_Done, (void*)client );
	rs_sqapi->Send( query );
	query = NULL;
}


/**
 * AuthNick callback function
 * @param query Query calling this function
 * @param success True on any response
 * @param customp The gclient_t client to nick check
 * @return void
 */
void RS_AuthNick_Done( stat_query_t *query, qboolean success, void *customp )
{
}

/**
 * Validate a given nickname
 * @return void
 */
void RS_AuthNick( gclient_t *client, const char *nick )
{
	stat_query_t *query;
	char *b64name = (char*)base64_encode( (unsigned char *)nick, strlen( nick ), NULL );

	query = rs_sqapi->CreateQuery( va( "api/nick/%s", b64name ), qtrue );
	free( b64name );

	RS_SignQuery( query, (int)time( NULL ) );
	rs_sqapi->SetCallback( query, RS_AuthNick_Done, (void*)client );
	rs_sqapi->Send( query );
	query = NULL;
}

/**
 * AuthMap callback function
 * @param query Query calling this function
 * @param success True on any response
 * @param customp Extra parameters, should be NULL
 * @return void
 */
void RS_AuthMap_Done( stat_query_t *query, qboolean success, void *customp )
{
}

/**
 * Get auth data for the current map
 * @return void
 */
void RS_AuthMap()
{
	stat_query_t *query;
	char *b64name, url[MAX_STRING_CHARS];

	// Form the url
	b64name = (char*)base64_encode( (unsigned char *)level.mapname, strlen( level.mapname ), NULL );
	Q_strncpyz( url, "api/map/", sizeof( url ) - 1 );
	Q_strncatz( url, b64name, sizeof( url ) - 1 );
	free( b64name );

	// Form the query
	query = rs_sqapi->CreateQuery( url, qtrue );
	rs_sqapi->SetCallback( query, RS_AuthMap_Done, NULL );

	RS_SignQuery( query, (int)time( NULL ) );
	rs_sqapi->Send( query );
	query = NULL;
}

/**
 * Callback for report race
 * @return void
 */
void RS_ReportRace_Done( stat_query_t *query, qboolean success, void *customp )
{
}

/**
 * Report Race data
 */
void RS_ReportRace( gclient_t *client, int playerId, int mapId, int rtime, CScriptArrayInterface *checkpoints )
{
	stat_query_t *query;
	int numCheckpoints = checkpoints->GetSize();

	// Use cJSON to format the checkpoint array
	cJSON *arr = cJSON_CreateArray();
	for( int i = 0; i < numCheckpoints; i++ )
		cJSON_AddItemToArray( arr, cJSON_CreateNumber( *((int*)checkpoints->At( i )) ) );

	// Form the query
	query = rs_sqapi->CreateQuery( "api/race/", qfalse );
	rs_sqapi->SetField( query, "pid", va( "%d", playerId ) );
	rs_sqapi->SetField( query, "mid", va( "%d", mapId ) );
	rs_sqapi->SetField( query, "time", va( "%d", rtime ) );
	rs_sqapi->SetField( query, "checkpoints", cJSON_Print( arr ) );

	RS_SignQuery( query, (int)time( NULL ) );
	rs_sqapi->SetCallback( query, RS_ReportRace_Done, (void*)client );
	rs_sqapi->Send( query );
	query = NULL;
}

/**
 * Report Map data
 * @param playTime Time in milliseconds played on the map
 * @param races Number of races completed on the map
 * @return void
 */
void RS_ReportMap( int playTime, int races )
{
	stat_query_t *query;
	char *b64name, url[MAX_STRING_CHARS];

	// Form the url
	b64name = (char*)base64_encode( (unsigned char *)level.mapname, strlen( level.mapname ), NULL );
	Q_strncpyz( url, "api/map/", sizeof( url ) - 1 );
	Q_strncatz( url, b64name, sizeof( url ) - 1 );
	free( b64name );

	// Form the query
	query = rs_sqapi->CreateQuery( url, qfalse );
	rs_sqapi->SetField( query, "playTime", va( "%d", playTime ) );
	rs_sqapi->SetField( query, "races", va( "%d", races ) );

	RS_SignQuery( query, (int)time( NULL ) );
	rs_sqapi->Send( query );
	query = NULL;
}

/**
 * Report Player data
 * @param name Auth name of the player to report
 * @param mapId ID number of the map reporting on
 * @param playTime
 * @param races
 * @param client
 */
void RS_ReportPlayer( const char *name, int mapId, int playTime, int races )
{
	stat_query_t *query;
	char *b64name, url[MAX_STRING_CHARS];

	// Make the URL
	b64name = (char*)base64_encode( (unsigned char *)name, strlen( name ), NULL );
	Q_strncpyz( url, "api/player/", sizeof( url ) - 1 );
	Q_strncatz( url, b64name, sizeof( url ) - 1 );
	free( b64name );

	// Form the query
	query = rs_sqapi->CreateQuery( url, qfalse );
	rs_sqapi->SetField( query, "mid", va( "%d", mapId ) );
	rs_sqapi->SetField( query, "playTime", va( "%d", playTime ) );
	rs_sqapi->SetField( query, "races", va( "%d", races ) );

	RS_SignQuery( query, (int)time( NULL ) );
	rs_sqapi->Send( query );
	query = NULL;
}

/**
 * Callback for top
 * @return void
 */
void RS_QueryTop_Done( stat_query_t *query, qboolean success, void *customp )
{
	int count, playerNum, i;
	rs_racetime_t top, racetime, timediff;
	cJSON *data, *node;
	char *mapname;

	gclient_t *client = (gclient_t *)customp;
	playerNum = (int)( client - game.clients );

	if( playerNum < 0 || playerNum >= gs.maxclients )
		return;

	if( rs_sqapi->GetStatus( query ) != 200 )
	{
		G_PrintMsg( &game.edicts[ playerNum + 1 ], "%sError:%s Top query failed\n", 
					S_COLOR_RED, S_COLOR_WHITE );
		return;
	}

	// We assume the response is properly formed
	data = (cJSON*)rs_sqapi->GetRoot( query );
	count = cJSON_GetObjectItem( data, "count" )->valueint;
	mapname = cJSON_GetObjectItem( data, "map" )->valuestring;

	G_PrintMsg( &game.edicts[ playerNum + 1 ], "%sTop %d times on map %s\n",
	 			S_COLOR_ORANGE, count, mapname );

	node = cJSON_GetObjectItem( data, "races" )->child;
	if( node )
		RS_Racetime( cJSON_GetObjectItem( node, "time" )->valueint, &top );

	for( i = 0; node != NULL; i++, node=node->next )
	{
		// Calculate the racetime and difftime from top for each record
		RS_Racetime( cJSON_GetObjectItem( node, "time" )->valueint, &racetime );
		RS_Racetime( racetime.timedelta - top.timedelta, &timediff );

		// Print the row
		G_PrintMsg( &game.edicts[ playerNum + 1 ], "%s%d. %s%02d:%02d.%02d %s+[%02d:%02d.%02d] %s%s %s%s %s\n",
			S_COLOR_WHITE, i + 1,
			S_COLOR_GREEN, ( racetime.hour * 60 ) + racetime.min, racetime.sec, racetime.milli / 10,
			( top.timedelta == racetime.timedelta ? S_COLOR_YELLOW : S_COLOR_RED ), 
			( timediff.hour * 60 ) + timediff.min, timediff.sec, timediff.milli / 10,
			S_COLOR_WHITE, cJSON_GetObjectItem( node, "playerName" )->valuestring,
			S_COLOR_WHITE, cJSON_GetObjectItem( node, "created" )->valuestring,
			( i == 0 ? cJSON_GetObjectItem( data, "oneliner" )->valuestring : "" ) );
	}
}


void RS_QueryTop( gclient_t *client, const char* mapname, int limit )
{
	stat_query_t *query;
	char *b64name = (char*)base64_encode( (unsigned char *)mapname, strlen( mapname ), NULL );

	// Form the query
	query = rs_sqapi->CreateQuery( "api/race/", qtrue );
	rs_sqapi->SetField( query, "map", b64name );
	rs_sqapi->SetField( query, "limit", va( "%d", limit ) );

	RS_SignQuery( query, (int)time( NULL ) );
	rs_sqapi->SetCallback( query, RS_QueryTop_Done, (void*)client );
	rs_sqapi->Send( query );
	free( b64name );
	query = NULL;
}

void RS_QueryMaps_Done( stat_query_t *query, qboolean success, void *customp )
{
	edict_t *ent;
	int playerNum, start, i, j;
	cJSON *data, *node, *tag;

	gclient_t *client = (gclient_t *)customp;
	playerNum = (int)( client - game.clients );

	if( playerNum < 0 || playerNum >= gs.maxclients )
		return;

	ent = &game.edicts[ playerNum + 1 ];

	if( rs_sqapi->GetStatus( query ) != 200 )
	{
		G_PrintMsg( ent, "%sError:%s Maplist query failed\n", 
					S_COLOR_RED, S_COLOR_WHITE );
		return;
	}

	// We assume the response is properly formed
	data = (cJSON*)rs_sqapi->GetRoot( query );
	start = cJSON_GetObjectItem( data, "start" )->valueint;

	node = cJSON_GetObjectItem( data, "maps" )->child;
	for( i = 0; node != NULL; i++, node=node->next )
	{
		// Print the row
		G_PrintMsg( ent, "%s# %d%s: %-25s ",
			S_COLOR_ORANGE, start + i + 1, S_COLOR_WHITE,
			cJSON_GetObjectItem( node, "name" )->valuestring );

		tag = cJSON_GetObjectItem( node, "tags" )->child;
		for( j = 0; tag != NULL; j++, tag=tag->next )
		{
			G_PrintMsg( ent, "%s%s", ( j == 0 ? "" : ", " ), tag->valuestring );
		}
		G_PrintMsg( ent, "\n" );
	}
}

void RS_QueryMaps( gclient_t *client, const char *pattern, const char *tags, int page )
{
	stat_query_t *query;
	char tagset[1024], *token, *b64tags, *b64pattern = (char*)base64_encode( (unsigned char *)pattern, strlen( pattern ), NULL );
	cJSON *arr = cJSON_CreateArray();

	// Make the taglist
	Q_strncpyz( tagset, tags, sizeof( tagset ) );
	token = strtok( tagset, " " );
	while( token != NULL )
	{
		cJSON_AddItemToArray( arr, cJSON_CreateString( token ) );
		token = strtok( NULL, " " );
	}
	token = cJSON_Print( arr );
	b64tags = (char*)base64_encode( (unsigned char *)token, strlen( token ), NULL );

	// which page to display?
	page = page == 0 ? 0 : page - 1;

	// Form the query
	query = rs_sqapi->CreateQuery( "api/map/", qtrue );
	rs_sqapi->SetField( query, "pattern", b64pattern );
	rs_sqapi->SetField( query, "tags", b64tags );
	rs_sqapi->SetField( query, "start", va( "%d", page * RS_MAPLIST_ITEMS ) );
	rs_sqapi->SetField( query, "limit", va( "%d", RS_MAPLIST_ITEMS ) );

	RS_SignQuery( query, (int)time( NULL ) );
	rs_sqapi->SetCallback( query, RS_QueryMaps_Done, (void*)client );
	rs_sqapi->Send( query );
	free( b64pattern );
	free( b64tags );
	query = NULL;
}

void RS_QueryRandmap_Done( stat_query_t *query, qboolean success, void *customp )
{
	char **mapname = (char**)customp;
	cJSON *data = (cJSON*)rs_sqapi->GetRoot( query );

	// invalid response?
	if( !data || rs_sqapi->GetStatus( query ) != 200 )
	{
		G_PrintMsg( NULL, "Invalid map picked, vote canceled\n" );
		G_CallVotes_Reset();
		return;
	}

	// Did we get any results?
	int count = cJSON_GetObjectItem( data, "count" )->valueint;
	if( !count )
	{
		G_PrintMsg( NULL, "Invalid map picked, vote canceled\n" );
		G_CallVotes_Reset();
		return;
	}

	// copy the mapname
	cJSON *map = cJSON_GetObjectItem( cJSON_GetObjectItem( data, "maps" )->child, "name" );
	Q_strncpyz( *mapname, map->valuestring, MAX_STRING_CHARS );

	// do we have the map or is it the current map?
	if( !trap_ML_FilenameExists( *mapname ) || !Q_stricmp( *mapname, level.mapname ) )
	{
		G_PrintMsg( NULL, "Invalid map picked, vote canceled\n" );
		G_CallVotes_Reset();
		return;
	}

	G_PrintMsg( NULL, "Randmap picked: %s\n", *mapname );
}

void RS_QueryRandmap( char* tags[], void *data )
{
	stat_query_t *query;
	char *b64tags;
	cJSON *arr = cJSON_CreateArray();

	// Format the tags
	while( *tags )
	{
		cJSON_AddItemToArray( arr, cJSON_CreateString( *tags++ ) );
		G_Printf( "%s\n", *tags++ );
	}
	b64tags = cJSON_Print( arr );
	b64tags = (char*)base64_encode( (unsigned char *)b64tags, strlen( b64tags ), NULL );

	// Form the query
	query = rs_sqapi->CreateQuery( "api/map/", qtrue );
	rs_sqapi->SetField( query, "pattern", "" );
	rs_sqapi->SetField( query, "tags", b64tags );
	rs_sqapi->SetField( query, "rand", "1" );
	
	RS_SignQuery( query, (int)time( NULL ) );
	rs_sqapi->SetCallback( query, RS_QueryRandmap_Done, data );
	rs_sqapi->Send( query );
	query = NULL;
	free( b64tags );
}