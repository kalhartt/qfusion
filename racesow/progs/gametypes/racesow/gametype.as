/**
 * Racesow Gametype interface
 * Base class that all race gametypes should inherit from.
 *
 * @package Racesow
 * @version 1.1.0
 */
class RS_Gametype
{
    /**
     * InitGametype
     * Setup the gametype
     * @return void
     */
    void InitGametype()
    {
        G_Print( "Init Gametype" );
        gametype.title = "Racesow";
        gametype.version = "1.1.0";
        gametype.author = "inc.mgxrace.net";

        gametype.spawnableItemsMask = ( IT_WEAPON | IT_AMMO | IT_ARMOR | IT_POWERUP | IT_HEALTH );
        if ( gametype.isInstagib )
          gametype.spawnableItemsMask &= ~uint(G_INSTAGIB_NEGATE_ITEMMASK);

        gametype.respawnableItemsMask = gametype.spawnableItemsMask;
        gametype.dropableItemsMask = 0;
        gametype.pickableItemsMask = 0;

        gametype.isRace = true;

        gametype.ammoRespawn = 0;
        gametype.armorRespawn = 0;
        gametype.weaponRespawn = 0;
        gametype.healthRespawn = 0;
        gametype.powerupRespawn = 0;
        gametype.megahealthRespawn = 0;
        gametype.ultrahealthRespawn = 0;

        gametype.readyAnnouncementEnabled = false;
        gametype.scoreAnnouncementEnabled = false;
        gametype.countdownEnabled = false;
        gametype.mathAbortDisabled = true;
        gametype.shootingDisabled = false;
        gametype.infiniteAmmo = true;
        gametype.canForceModels = true;
        gametype.canShowMinimap = false;
        gametype.teamOnlyMinimap = true;

        // set spawnsystem type
        for ( int team = TEAM_PLAYERS; team < GS_MAX_TEAMS; team++ )
          gametype.setTeamSpawnsystem( team, SPAWNSYSTEM_INSTANT, 0, 0, false );
    }

    void SpawnGametype()
    {
    }

    void Shutdown()
    {
    }

    bool MatchStateFinished( int incomingMathState )
    {
        return true;
    }

    void MatchStateStarted()
    {
    }

    void ThinkRules()
    {
        RS_Player @player;

        for( int i = 0; i < maxClients; i++ )
        {
            @player = @players[i];
            if( @player is null )
                continue;

            player.setHUD();
        }
    }

    void PlayerRespawn( Entity @ent, int old_team, int new_team )
    {
        if( ent.isGhosting() )
            return;

        RS_Player @player = RS_getPlayer( @ent );
        if( @player !is null )
            player.cancelRace();
    }

    /**
     * ScoreEvent
     * Called by the game when one of the following events occurs (taken from
     * the warsow wiki page)
     * TODO: check this is up to date with 1.1
     * TODO: gather the arguments associated with the event
     *  - "enterGame" - A client has finished connecting and enters the level
     *  - "connect" - A client just connected
     *  - "disconnect" - A client just disconnected
     *  - "dmg" - A client has inflicted some damage
     *  - "kill" - A client has killed some other entity
     *  - "award" - A client receives an award
     *  - "pickup" - A client picked up an item (use args.getToken( 0 ) to get
     *    the item's classname)
     *  - "projectilehit" - A client is hit by a projectile
     *
     * @param Client client The client associated with the event
     * @param String score_event The name of the event
     * @param String args Arguments associated with the event
     */
    void ScoreEvent( Client @client, const String &score_event, const String &args )
    {
        if( @client is null )
            return;

        if( score_event == "enterGame" )
        {
            // Make a RS_Player for the client.
            @players[client.get_playerNum()] = @RS_Player( @client );
        }
        else if( score_event == "disconnect" )
        {
            // Release the associated RS_Player object
            @players[client.get_playerNum()] = null;
        }
    }

    String @ScoreboardMessage( uint maxlen )
    {
        return null;
    }

    Entity @SelectSpawnPoint( Entity @self )
    {
        return null;
    }

    bool UpdateBotStatus( Entity @self )
    {
        return false;
    }

    bool Command( Client @client, const String @cmdString, const String @argsString, int argc )
    {
        return false;
    }
}
