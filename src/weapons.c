/*
 *  QWProgs-DM
 *  Copyright (C) 2004  [sd] angel
 *
 *  This code is based on QuakeWorld DM mod code by Id Software, Inc.
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *
 *  $Id$
 */

#include "g_local.h"


void ReportMe();
void AdminImpBot();
void CaptainPickPlayer();

// called by SP_worldspawn
void W_Precache()
{
	trap_precache_sound( "weapons/r_exp3.wav" );	// new rocket explosion
	trap_precache_sound( "weapons/rocket1i.wav" );	// spike gun
	trap_precache_sound( "weapons/sgun1.wav" );
	trap_precache_sound( "weapons/guncock.wav" );	// player shotgun
	trap_precache_sound( "weapons/ric1.wav" );	// ricochet (used in c code)
	trap_precache_sound( "weapons/ric2.wav" );	// ricochet (used in c code)
	trap_precache_sound( "weapons/ric3.wav" );	// ricochet (used in c code)
	trap_precache_sound( "weapons/spike2.wav" );	// super spikes
	trap_precache_sound( "weapons/tink1.wav" );	// spikes tink (used in c code)
	trap_precache_sound( "weapons/grenade.wav" );	// grenade launcher
	trap_precache_sound( "weapons/bounce.wav" );	// grenade bounce
	trap_precache_sound( "weapons/shotgn2.wav" );	// super shotgun

	if ( cvar("k_instagib_custom_models") )
		trap_precache_sound( "weapons/coilgun.wav" );	// player railgun for instagib 
}


void            W_FireSpikes( float ox );
void            W_FireLightning();

/*
================
W_FireAxe
================
*/
void W_FireAxe()
{
	vec3_t          source, dest;
	vec3_t          org;

	self->ps.wpn[wpAXE].attacks++;

	trap_makevectors( self->s.v.v_angle );

	VectorCopy( self->s.v.origin, source );
	source[2] += 16;
	VectorScale( g_globalvars.v_forward, 64, dest );
	VectorAdd( dest, source, dest )
	//source = self->s.v.origin + '0 0 16';
	
	traceline( PASSVEC3( source ), PASSVEC3( dest ), false, self );
	if ( g_globalvars.trace_fraction == 1.0 )
		return;

	VectorScale( g_globalvars.v_forward, 4, org );
	VectorSubtract( g_globalvars.trace_endpos, org, org );
// org = trace_endpos - v_forward*4;

	if ( PROG_TO_EDICT( g_globalvars.trace_ent )->s.v.takedamage )
	{
		int damage = 20; // default damage is 20

		if ( PROG_TO_EDICT( g_globalvars.trace_ent )->ct == ctPlayer )
			self->ps.wpn[wpAXE].hits++;

		if ( deathmatch > 3 )
			damage = 75;
		else if ( deathmatch == 3 )
			damage = k_jawnmode ? 50 : 20; // Jawnmode: 50 axe dmg in dmm3

		PROG_TO_EDICT( g_globalvars.trace_ent )->axhitme = 1;
		SpawnBlood( org, damage );
		PROG_TO_EDICT( g_globalvars.trace_ent )->deathtype = dtAXE;

		T_Damage( PROG_TO_EDICT( g_globalvars.trace_ent ), self, self, damage );
	} else
	{	// hit wall

		//crt - get rid of axe sound for spec
		if ( !isRA() || ( isWinner( self ) || isLoser( self ) ) )
			sound( self, CHAN_WEAPON, "player/axhit2.wav", 1, ATTN_NORM );

		WriteByte( MSG_MULTICAST, SVC_TEMPENTITY );
		WriteByte( MSG_MULTICAST, TE_GUNSHOT );
		WriteByte( MSG_MULTICAST, 3 );
		WriteCoord( MSG_MULTICAST, org[0] );
		WriteCoord( MSG_MULTICAST, org[1] );
		WriteCoord( MSG_MULTICAST, org[2] );
		trap_multicast( PASSVEC3( org ), MULTICAST_PVS );
	}
}


//============================================================================


//============================================================================


void wall_velocity( vec3_t v )
{
	vec3_t          vel;

	normalize( self->s.v.velocity, vel );

	vel[0] += g_globalvars.v_up[0] * ( g_random() - 0.5 ) +
	          g_globalvars.v_right[0] * ( g_random() - 0.5 );

	vel[1] += g_globalvars.v_up[1] * ( g_random() - 0.5 ) +
	          g_globalvars.v_right[1] * ( g_random() - 0.5 );
	vel[2] += g_globalvars.v_up[2] * ( g_random() - 0.5 ) +
	          g_globalvars.v_right[2] * ( g_random() - 0.5 );
	
	VectorNormalize( vel );

	vel[0] += 2 * g_globalvars.trace_plane_normal[0];
	vel[1] += 2 * g_globalvars.trace_plane_normal[1];
	vel[2] += 2 * g_globalvars.trace_plane_normal[2];

	VectorScale( vel, 200, v );

	//return vel;
}


/*
================
SpawnMeatSpray
================
*/
void SpawnMeatSpray( vec3_t org, vec3_t vel )
{
	gedict_t       *missile;

	//vec3_t  org;

	missile = spawn();
	missile->s.v.owner = EDICT_TO_PROG( self );
	missile->s.v.movetype = MOVETYPE_BOUNCE;
	missile->s.v.solid = SOLID_NOT;

	trap_makevectors( self->s.v.angles );

	VectorCopy( vel, missile->s.v.velocity );
// missile->s.v.velocity = vel;
	missile->s.v.velocity[2] = missile->s.v.velocity[2] + 250 + 50 * g_random();

	SetVector( missile->s.v.avelocity, 3000, 1000, 2000 );
// missile.avelocity = '3000 1000 2000';

// set missile duration
	missile->s.v.nextthink = g_globalvars.time + 1;
	missile->s.v.think = ( func_t ) SUB_Remove;

	setmodel( missile, "progs/zom_gib.mdl" );
	setsize( missile, 0, 0, 0, 0, 0, 0 );
	setorigin( missile, PASSVEC3( org ) );
}

/*
================
SpawnBlood
================
*/
void SpawnBlood( vec3_t org, float damage )
{
	if ( damage < 1 )
		return;

	WriteByte( MSG_MULTICAST, SVC_TEMPENTITY );
	WriteByte( MSG_MULTICAST, TE_BLOOD );
	WriteByte( MSG_MULTICAST, damage );
	WriteCoord( MSG_MULTICAST, org[0] );
	WriteCoord( MSG_MULTICAST, org[1] );
	WriteCoord( MSG_MULTICAST, org[2] );
	trap_multicast( PASSVEC3( org ), MULTICAST_PVS );
}

/*
================
spawn_touchblood
================
*/
void spawn_touchblood( float damage )
{
	vec3_t          vel;

	wall_velocity( vel );
	VectorScale( vel, 0.2 * 0.01, vel );
	VectorAdd( self->s.v.origin, vel, vel );
	SpawnBlood( vel, damage );
}

/*
==============================================================================

MULTI-DAMAGE

Collects multiple small damages into a single damage

==============================================================================
*/
gedict_t       *multi_ent;
float           multi_damage;
deathType_t		multi_damage_type;

vec3_t          blood_org;
float           blood_count;

vec3_t          puff_org;
float           puff_count;
void ClearMultiDamage()
{
	multi_ent = world;
	multi_damage = 0;
	blood_count = 0;
	puff_count = 0;
	multi_damage_type = dtNONE;
}

void ApplyMultiDamage()
{
	if ( multi_ent == world )
		return;

	multi_ent->deathtype = multi_damage_type;
	T_Damage( multi_ent, self, self, multi_damage );
}

void AddMultiDamage( gedict_t * hit, float damage )
{
	if ( !hit )
		return;

	if ( hit != multi_ent )
	{
		ApplyMultiDamage();
		multi_damage = damage;
		multi_ent = hit;
	} else
		multi_damage = multi_damage + damage;
}

void Multi_Finish()
{
	if ( puff_count )
	{
		WriteByte( MSG_MULTICAST, SVC_TEMPENTITY );
		WriteByte( MSG_MULTICAST, TE_GUNSHOT );
		WriteByte( MSG_MULTICAST, puff_count );
		WriteCoord( MSG_MULTICAST, puff_org[0] );
		WriteCoord( MSG_MULTICAST, puff_org[1] );
		WriteCoord( MSG_MULTICAST, puff_org[2] );
		trap_multicast( PASSVEC3( puff_org ), MULTICAST_PVS );
	}

	SpawnBlood( puff_org, blood_count );
}

/*
==============================================================================
BULLETS
==============================================================================
*/
/*
================
TraceAttack
================
*/
void TraceAttack( float damage, vec3_t dir )
{

	vec3_t          vel, org, tmp;

	VectorScale( g_globalvars.v_up, crandom(), tmp );
	VectorAdd( dir, tmp, vel );
	VectorScale( g_globalvars.v_right, crandom(), tmp );
	VectorAdd( vel, tmp, vel );
	VectorNormalize( vel );
	VectorScale( g_globalvars.trace_plane_normal, 2, tmp );
	VectorAdd( vel, tmp, vel );
	VectorScale( vel, 200, vel );
	/*vel = normalize(dir + v_up*crandom() + v_right*crandom());
	   vel = vel + 2*trace_plane_normal;
	   vel = vel * 200; */
	VectorScale( dir, 4, tmp );
	VectorSubtract( g_globalvars.trace_endpos, tmp, org );
// org = trace_endpos - dir*4;

	if ( PROG_TO_EDICT( g_globalvars.trace_ent )->s.v.takedamage )
	{
		if ( PROG_TO_EDICT( g_globalvars.trace_ent )->ct == ctPlayer ) {
			if ((int)self->s.v.weapon == IT_SHOTGUN)
				self->ps.wpn[wpSG].hits++;
			else if ((int)self->s.v.weapon == IT_SUPER_SHOTGUN)
				self->ps.wpn[wpSSG].hits++;
			else
			 ; // hmmm, make error?
		}
		blood_count = blood_count + 1;
		VectorCopy( org, blood_org );	//  blood_org = org;
		AddMultiDamage( PROG_TO_EDICT( g_globalvars.trace_ent ), damage );
	}
	else
	{
		puff_count = puff_count + 1;
	}
}

/*
================
FireInstaBullet

Used by coilgun for Instagib mode, bullet doesn't stop at first lpayer, it goes through till it hits wall
================
*/

/*
void FireInstaBullet( vec3_t org, vec3_t dir, deathType_t deathtype )
{
        traceline(org, org + dir * 8096)
 
        if (trace_ent != player)
                return;
 
        dodamage(trace_ent)
        FireInstaBullet( trace_end, dir, deathType_t deathtype )
} 
*/
void FireInstaBullet( vec3_t dir, deathType_t deathtype )
{
        vec3_t          direction;
        vec3_t          src, tmp;

        trap_makevectors( self->s.v.v_angle );
        VectorScale( g_globalvars.v_forward, 10, tmp );
        VectorAdd( self->s.v.origin, tmp, src );
        src[2] = self->s.v.absmin[2] + self->s.v.size[2] * 0.7;

        ClearMultiDamage();
        multi_damage_type = deathtype;

        traceline( PASSVEC3( src ), src[0] + dir[0] * 8192,
                src[1] + dir[1] * 8192,
                src[2] + dir[2] * 8192, false, self );
        
	VectorScale( dir, 4, tmp );
        VectorSubtract( g_globalvars.trace_endpos, tmp, puff_org );     // puff_org = trace_endpos - dir*4;

        VectorScale( g_globalvars.v_right, 0, tmp );
        VectorAdd( dir, tmp, direction );
        VectorScale( g_globalvars.v_up, 0, tmp );
     	VectorAdd( direction, tmp, direction );

       	VectorScale( direction, 8192, tmp );

        VectorAdd( src, tmp, tmp );
        traceline( PASSVEC3( src ), PASSVEC3( tmp ), false, self );
        if ( g_globalvars.trace_fraction != 1.0 )
                TraceAttack( 4, direction );
	        
        ApplyMultiDamage();
        Multi_Finish();
}



/*
================
FireBullets

Used by shotgun, super shotgun, and enemy soldier firing
Go to the trouble of combining multiple pellets into a single damage call.
================
*/
void FireBullets( float shotcount, vec3_t dir, float spread_x, float spread_y, float spread_z, deathType_t deathtype )
{
	vec3_t          direction;
	vec3_t          src, tmp, tmp2;

	trap_makevectors( self->s.v.v_angle );
	VectorScale( g_globalvars.v_forward, 10, tmp );
	VectorAdd( self->s.v.origin, tmp, src );
	//src = self->s.v.origin + v_forward*10;
	src[2] = self->s.v.absmin[2] + self->s.v.size[2] * 0.7;

	ClearMultiDamage();
	multi_damage_type = deathtype;

	if ( cvar("k_instagib") )
		traceline( PASSVEC3( src ), src[0] + dir[0] * 8192, 
		src[1] + dir[1] * 8192, 
		src[2] + dir[2] * 8192, false, self );
	else
		traceline( PASSVEC3( src ), src[0] + dir[0] * 2048, 
		src[1] + dir[1] * 2048, 
		src[2] + dir[2] * 2048, false, self );
	VectorScale( dir, 4, tmp );
	VectorSubtract( g_globalvars.trace_endpos, tmp, puff_org );	// puff_org = trace_endpos - dir*4;

	while ( shotcount > 0 )
	{
		if ( k_jawnmode )
		{
			if ( shotcount == 1 )
			{
				VectorClear( tmp );
				VectorClear( tmp2 );
			}
			else if ( shotcount == 2 )
			{
				VectorClear( tmp );
				VectorClear( tmp2 );
			}
			else if ( shotcount == 3 )
			{
				VectorClear( tmp );
				VectorScale( g_globalvars.v_up,     1.00 * spread_y, tmp2 );
			}
			else if ( shotcount == 4 )
			{
				VectorClear( tmp );
				VectorScale( g_globalvars.v_up,    -1.00 * spread_y, tmp2 );
			}
			else if ( shotcount == 5 )
			{
				VectorScale( g_globalvars.v_right,  1.00 * spread_x, tmp );
				VectorClear( tmp2 );
			}
			else if (shotcount == 6)
			{
				VectorScale( g_globalvars.v_right, -1.00 * spread_x, tmp );
				VectorClear( tmp2 );
			}
			else if ( shotcount == 7 )
			{
				VectorClear( tmp );
				VectorScale( g_globalvars.v_up,     0.50 * spread_y, tmp2 );
			}
			else if ( shotcount == 8 )
			{
				VectorClear( tmp );
				VectorScale( g_globalvars.v_up,    -0.50 * spread_y, tmp2 );
			}
			else if ( shotcount == 9 )
			{
				VectorScale( g_globalvars.v_right,  0.50 * spread_x, tmp );
				VectorClear( tmp2 );
			}
			else if ( shotcount == 10 )
			{
				VectorScale( g_globalvars.v_right, -0.50 * spread_x, tmp );
				VectorClear( tmp2 );
			}
			else if ( shotcount == 11 )
			{
				VectorScale( g_globalvars.v_right,  0.50 * spread_x, tmp );
				VectorScale( g_globalvars.v_up,     0.50 * spread_y, tmp2 );
			}
			else if ( shotcount == 12 )
			{
				VectorScale( g_globalvars.v_right,  0.50 * spread_x, tmp );
				VectorScale( g_globalvars.v_up,    -0.50 * spread_y, tmp2 );
			}
			else if ( shotcount == 13 )
			{
				VectorScale( g_globalvars.v_right, -0.50 * spread_x, tmp );
				VectorScale( g_globalvars.v_up,    -0.50 * spread_y, tmp2 );
			}
			else if ( shotcount == 14 )
			{
				VectorScale( g_globalvars.v_right, -0.50 * spread_x, tmp );
				VectorScale( g_globalvars.v_up,     0.50 * spread_y, tmp2 );
			}
			else if ( shotcount == 15 )
			{
				VectorScale( g_globalvars.v_right,  0.25 * spread_x, tmp );
				VectorScale( g_globalvars.v_up,     0.25 * spread_y, tmp2 );
			}
			else if ( shotcount == 16 )
			{
				VectorScale( g_globalvars.v_right,  0.25 * spread_x, tmp );
				VectorScale( g_globalvars.v_up,    -0.25 * spread_y, tmp2 );
			}
			else if ( shotcount == 17 )
			{
				VectorScale( g_globalvars.v_right, -0.25 * spread_x, tmp );
				VectorScale( g_globalvars.v_up,    -0.25 * spread_y, tmp2 );
			}
			else if ( shotcount == 18 )
			{
				VectorScale( g_globalvars.v_right, -0.25 * spread_x, tmp );
				VectorScale( g_globalvars.v_up,     0.25 * spread_y, tmp2 );
			}
			else if ( shotcount == 19 )
			{
				VectorScale( g_globalvars.v_right, -1.00 * spread_x, tmp );
				VectorClear( tmp2 );
			}
			else if ( shotcount == 20 )
			{
				VectorClear( tmp );
				VectorClear( tmp2 );
			}
			else if ( shotcount == 21 )
			{
				VectorScale( g_globalvars.v_right,  1.00 * spread_x, tmp );
				VectorClear( tmp2 );
			}
			else
			{
				// No design for this bullet, place it in the middle
				VectorClear( tmp );
				VectorClear( tmp2 );
			}
			VectorAdd( dir, tmp, direction );
			VectorAdd( direction, tmp2, direction );
		}
		else
		{
			VectorScale( g_globalvars.v_right, crandom() * spread_x, tmp );
			VectorAdd( dir, tmp, direction );
			VectorScale( g_globalvars.v_up, crandom() * spread_y, tmp );
			VectorAdd( direction, tmp, direction );
		}

//  direction = dir + crandom()*spread[0]*v_right + crandom()*spread[1]*v_up;
		if ( cvar("k_instagib") )
			VectorScale( direction, 8192, tmp );
		else
			VectorScale( direction, 2048, tmp );

		VectorAdd( src, tmp, tmp );
		traceline( PASSVEC3( src ), PASSVEC3( tmp ), false, self );
		if ( g_globalvars.trace_fraction != 1.0 )
			TraceAttack( 4, direction );

		shotcount = shotcount - 1;
	}
	ApplyMultiDamage();
	Multi_Finish();
}

/*
================
W_FireShotgun
================
*/
void W_FireShotgun()
{
	vec3_t          dir;
	int				bullets = 6;
	
	self->ps.wpn[wpSG].attacks += bullets;

	if ( cvar("k_instagib_custom_models") && cvar("k_instagib") == 1 )
		sound( self, CHAN_WEAPON, "weapons/coilgun.wav", 1, ATTN_NORM );
	else
		sound( self, CHAN_WEAPON, "weapons/guncock.wav", 1, ATTN_NORM );

	g_globalvars.msg_entity = EDICT_TO_PROG( self );

	WriteByte( MSG_ONE, SVC_SMALLKICK );

    if ( match_in_progress == 2 )
		if ( deathmatch != 4 )
			self->s.v.currentammo = --( self->s.v.ammo_shells );

	//dir = aim (self, 100000);
	aim( dir );
	if ( cvar("k_instagib") == 1 )
		FireInstaBullet( dir, dtSG );
	else
		FireBullets( bullets, dir, 0.04, 0.04, 0, dtSG );
}

/*
================
W_FireSuperShotgun
================
*/
void W_FireSuperShotgun()
{
	vec3_t          dir;
	int				bullets = ( k_jawnmode ? 21 : 14 );

	if ( self->s.v.currentammo == 1 )
	{
		W_FireShotgun();
		return;
	}
	self->ps.wpn[wpSSG].attacks += bullets;

	if ( cvar("k_instagib_custom_models") && cvar("k_instagib") == 2 )
		sound( self, CHAN_WEAPON, "weapons/coilgun.wav", 1, ATTN_NORM );
	else
		sound( self, CHAN_WEAPON, "weapons/shotgn2.wav", 1, ATTN_NORM );

	g_globalvars.msg_entity = EDICT_TO_PROG( self );

	WriteByte( MSG_ONE, SVC_BIGKICK );

    if ( match_in_progress == 2 )
		if ( deathmatch != 4 )
			self->s.v.currentammo = self->s.v.ammo_shells = self->s.v.ammo_shells - 2;

	//dir = aim (self, 100000);
	aim( dir );
	if ( cvar("k_instagib") == 2 )
		FireInstaBullet( dir, dtSSG );
	else if ( k_jawnmode )
	{
	        // Jawnmode: larger SSG spread, higher reload time, more damage
	        // - Molgrum
	        FireBullets( bullets, dir, 0.18, 0.12, 0, dtSSG );
	}
	else
		FireBullets( bullets, dir, 0.14, 0.08, 0, dtSSG );
}

/*
==============================================================================

ROCKETS

==============================================================================
*/

void FixQuad(gedict_t *owner)
{
	// well, quaded rocket is no more quaded after owner is dead,
	// at the same time that still allow apply full quad damage to nearby players in case of "quad bore"
	if ( owner->ct == ctPlayer && ISDEAD( owner ) )
		owner->super_damage_finished = 0;
}

void T_MissileTouch()
{
	float           damg;
	vec3_t          tmp;

	if ( other == PROG_TO_EDICT( self->s.v.owner ) )
		return;		// don't explode on owner

	if ( self->voided )
	{
		return;
	}
	self->voided = 1;
	if ( trap_pointcontents( PASSVEC3( self->s.v.origin ) ) == CONTENT_SKY )
	{
		ent_remove( self );
		return;
	}

	FixQuad(PROG_TO_EDICT( self->s.v.owner ));

	// Jawnmode: always 110 dmg on direct hits
	// - Molgrum
	damg = 100 + (k_jawnmode ? 10 : g_random() * 20);

	if ( other->s.v.takedamage ) {
		if ( other->ct == ctPlayer )
			PROG_TO_EDICT( self->s.v.owner )->ps.wpn[wpRL].hits++;
	}

	if ( ISLIVE( other ) )
	{
		other->deathtype = dtRL;
		T_Damage( other, self, PROG_TO_EDICT( self->s.v.owner ), damg );
	}
	// don't do radius damage to the other, because all the damage
	// was done in the impact


	T_RadiusDamage( self, PROG_TO_EDICT( self->s.v.owner ), 120, other, dtRL );

//  sound (self, CHAN_WEAPON, "weapons/r_exp3.wav", 1, ATTN_NORM);
	normalize( self->s.v.velocity, tmp );
	VectorScale( tmp, -8, tmp );
	VectorAdd( self->s.v.origin, tmp, self->s.v.origin )
// self->s.v.origin = self->s.v.origin - 8 * normalize(self->s.v.velocity);
	WriteByte( MSG_MULTICAST, SVC_TEMPENTITY );
	WriteByte( MSG_MULTICAST, TE_EXPLOSION );
	WriteCoord( MSG_MULTICAST, self->s.v.origin[0] );
	WriteCoord( MSG_MULTICAST, self->s.v.origin[1] );
	WriteCoord( MSG_MULTICAST, self->s.v.origin[2] );
	trap_multicast( PASSVEC3( self->s.v.origin ), MULTICAST_PHS );

	ent_remove( self );
}

/*
================
W_FireRocket
================
*/

void W_FireRocket()
{
	self->ps.wpn[wpRL].attacks++;

    if ( match_in_progress == 2 )
		if ( deathmatch != 4 )
			self->s.v.currentammo = self->s.v.ammo_rockets = self->s.v.ammo_rockets - 1;

	sound( self, CHAN_WEAPON, "weapons/sgun1.wav", 1, ATTN_NORM );

	g_globalvars.msg_entity = EDICT_TO_PROG( self );
	WriteByte( MSG_ONE, SVC_SMALLKICK );

	newmis = spawn();
	g_globalvars.newmis = EDICT_TO_PROG( newmis );
	newmis->s.v.owner = EDICT_TO_PROG( self );
	newmis->s.v.movetype = MOVETYPE_FLYMISSILE;
	newmis->s.v.solid = SOLID_BBOX;

// set newmis speed     
	trap_makevectors( self->s.v.v_angle );
	aim( newmis->s.v.velocity );	// = aim(self, 1000);
	if ( cvar("k_midair") && self->super_damage_finished > g_globalvars.time ) 
	{
		VectorScale ( newmis->s.v.velocity, 2000, newmis->s.v.velocity );
		newmis->s.v.effects = EF_BLUE;
	}
	else
		VectorScale( newmis->s.v.velocity, 1000, newmis->s.v.velocity );

	vectoangles( newmis->s.v.velocity, newmis->s.v.angles );
	
	newmis->s.v.touch = ( func_t ) T_MissileTouch;
	newmis->voided = 0;

// set newmis duration
	newmis->s.v.nextthink = g_globalvars.time + 5;
	newmis->s.v.think = ( func_t ) SUB_Remove;
	newmis->s.v.classname = "rocket";

	setmodel( newmis, "progs/missile.mdl" );
	setsize( newmis, 0, 0, 0, 0, 0, 0 );

// setorigin (newmis, self->s.v.origin + v_forward*8 + '0 0 16');
	setorigin( newmis, self->s.v.origin[0] + g_globalvars.v_forward[0] * 8,
			self->s.v.origin[1] + g_globalvars.v_forward[1] * 8,
			self->s.v.origin[2] + g_globalvars.v_forward[2] * 8 + 16 );

	// midair 
	VectorCopy( self->s.v.origin, newmis->s.v.oldorigin );
}

/*
===============================================================================
LIGHTNING
===============================================================================
*/

void LightningHit( gedict_t *from, float damage )
{
	if ( PROG_TO_EDICT( g_globalvars.trace_ent )->ct == ctPlayer )
		self->ps.wpn[wpLG].hits++;

	WriteByte( MSG_MULTICAST, SVC_TEMPENTITY );
	WriteByte( MSG_MULTICAST, TE_LIGHTNINGBLOOD );
	WriteCoord( MSG_MULTICAST, g_globalvars.trace_endpos[0] );
	WriteCoord( MSG_MULTICAST, g_globalvars.trace_endpos[1] );
	WriteCoord( MSG_MULTICAST, g_globalvars.trace_endpos[2] );
	trap_multicast( PASSVEC3( g_globalvars.trace_endpos ), MULTICAST_PVS );

	PROG_TO_EDICT( g_globalvars.trace_ent )->deathtype = dtLG_BEAM;
	T_Damage( PROG_TO_EDICT( g_globalvars.trace_ent ), from, from, damage );
}

/*
=================
LightningDamage
=================
*/
void LightningDamage( vec3_t p1, vec3_t p2, gedict_t * from, float damage )
{
#if 0 /* qqshka - this code was in original qw from ID, but i make code more ktpro'ish :( */

	gedict_t       *e1, *e2;
	vec3_t          f;


	VectorSubtract( p2, p1, f );	// f = p2 - p1;
	VectorNormalize( f );	// normalize (f);

	f[0] = 0 - f[1];
	f[1] = f[0];
	f[2] = 0;
	VectorScale( f, 16, f );	//f = f*16;

	e1 = e2 = world;
#endif

	traceline( PASSVEC3( p1 ), PASSVEC3( p2 ), false, self );

	if ( PROG_TO_EDICT( g_globalvars.trace_ent )->s.v.takedamage )
	{
		LightningHit( from, damage );
		if ( self->ct == ctPlayer )
		{
// qqshka: this shit doesn't work in QVM
//			if ( other->ct == ctPlayer )
// 		   make my own
			gedict_t *gre = PROG_TO_EDICT ( self->s.v.groundentity );

			if (    gre 
				 && gre == PROG_TO_EDICT( g_globalvars.trace_ent )
				 && streq( gre->s.v.classname, "door" ) 
			   )
				PROG_TO_EDICT( g_globalvars.trace_ent )->s.v.velocity[2] += 400;
		}
	}

#if 0  /* qqshka - this code was in original qw from ID, but i make code more ktpro'ish :( */

	e1 = PROG_TO_EDICT( g_globalvars.trace_ent );

	//traceline (p1 + f, p2 + f, FALSE, self);
	traceline( p1[0] + f[0], p1[1] + f[1], p1[2] + f[2], p2[0] + f[0],
			p2[1] + f[1], p2[2] + f[2], false, self );
	if ( PROG_TO_EDICT( g_globalvars.trace_ent ) != e1
	     && PROG_TO_EDICT( g_globalvars.trace_ent )->s.v.takedamage )
	{
		LightningHit( from, damage );
	}
	e2 = PROG_TO_EDICT( g_globalvars.trace_ent );

	traceline( p1[0] - f[0], p1[1] - f[1], p1[2] - f[2], p2[0] - f[0],
			p2[1] - f[1], p2[2] - f[2], false, self );
	if ( PROG_TO_EDICT( g_globalvars.trace_ent ) != e1
	     && PROG_TO_EDICT( g_globalvars.trace_ent ) != e2
	     && PROG_TO_EDICT( g_globalvars.trace_ent )->s.v.takedamage )
	{
		LightningHit( from, damage );
	}

#endif
}

void W_FireLightning()
{
	vec3_t          org;
	float           cells;
	vec3_t          tmp;


    if ( self->s.v.ammo_cells < 1 || match_in_progress == 1 )
	{
		self->s.v.weapon = W_BestWeapon();
		W_SetCurrentAmmo();
		return;
	}

// explode if under water
    if ( self->s.v.waterlevel > 1 && match_in_progress == 2 )
	{
		if ( isRA() ) {
			self->s.v.ammo_cells = 0;
			W_SetCurrentAmmo();
			return;
		}

		if ( deathmatch > 3 )
		{
			// Jawnmode: this always kills the player that discharges in dmm4
			// - Molgrum
			if ( k_jawnmode || g_random() <= 0.5 )
			{
				self->deathtype = dtLG_DIS_SELF;
				T_Damage( self, self, self, 4000 );
				return;
			} else
			{
				cells = self->s.v.ammo_cells;
				self->s.v.ammo_cells = 0;
				W_SetCurrentAmmo();

                if ( !cvar( "k_dis" ) ) 
                    return;

				T_RadiusDamage( self, self, 35 * cells, world, dtLG_DIS );
				return;
			}
		} else
		{
			cells = self->s.v.ammo_cells;
			self->s.v.ammo_cells = 0;
			W_SetCurrentAmmo();

            if ( !cvar( "k_dis" ) )
                return;

			T_RadiusDamage( self, self, 35 * cells, world, dtLG_DIS );
			return;
		}
	}

	self->ps.wpn[wpLG].attacks++;

	if ( self->t_width < g_globalvars.time )
	{
		sound( self, CHAN_WEAPON, "weapons/lhit.wav", 1, ATTN_NORM );
		self->t_width = g_globalvars.time + 0.6;
	}
	g_globalvars.msg_entity = EDICT_TO_PROG( self );
	WriteByte( MSG_ONE, SVC_SMALLKICK );

    if ( match_in_progress == 2 )
		if ( deathmatch != 4 )
			self->s.v.currentammo = self->s.v.ammo_cells = self->s.v.ammo_cells - 1;

	VectorCopy( self->s.v.origin, org );	//org = self->s.v.origin + '0 0 16';
	org[2] += 16;


	traceline( PASSVEC3( org ), org[0] + g_globalvars.v_forward[0] * 600,
			org[1] + g_globalvars.v_forward[1] * 600,
			org[2] + g_globalvars.v_forward[2] * 600, true, self );

	WriteByte( MSG_MULTICAST, SVC_TEMPENTITY );
	WriteByte( MSG_MULTICAST, TE_LIGHTNING2 );
	WriteEntity( MSG_MULTICAST, self );
	WriteCoord( MSG_MULTICAST, org[0] );
	WriteCoord( MSG_MULTICAST, org[1] );
	WriteCoord( MSG_MULTICAST, org[2] );
	WriteCoord( MSG_MULTICAST, g_globalvars.trace_endpos[0] );
	WriteCoord( MSG_MULTICAST, g_globalvars.trace_endpos[1] );
	WriteCoord( MSG_MULTICAST, g_globalvars.trace_endpos[2] );

	trap_multicast( PASSVEC3( org ), MULTICAST_PHS );

	VectorScale( g_globalvars.v_forward, 4, tmp );
	VectorAdd( g_globalvars.trace_endpos, tmp, tmp );
// qqshka - not from 'self->s.v.origin' but from 'org'
//	LightningDamage( self->s.v.origin, tmp, self, 30 );
	LightningDamage( org, tmp, self, 30 );
}

//=============================================================================

void GrenadeExplode()
{
	if ( self->voided )
	{
		return;
	}
	self->voided = 1;

	FixQuad(PROG_TO_EDICT( self->s.v.owner ));

	T_RadiusDamage( self, PROG_TO_EDICT( self->s.v.owner ), 120, world, dtGL );

	WriteByte( MSG_MULTICAST, SVC_TEMPENTITY );
	WriteByte( MSG_MULTICAST, TE_EXPLOSION );
	WriteCoord( MSG_MULTICAST, self->s.v.origin[0] );
	WriteCoord( MSG_MULTICAST, self->s.v.origin[1] );
	WriteCoord( MSG_MULTICAST, self->s.v.origin[2] );
	trap_multicast( PASSVEC3( self->s.v.origin ), MULTICAST_PHS );

	ent_remove( self );
}

void GrenadeTouch()
{
	if ( other == PROG_TO_EDICT( self->s.v.owner ) )
		return;		// don't explode on owner
	
	if ( other->s.v.takedamage ) {
		if ( other->ct == ctPlayer )
			PROG_TO_EDICT( self->s.v.owner )->ps.wpn[wpGL].hits++;
	}

	if ( other->s.v.takedamage == DAMAGE_AIM )
	{
		GrenadeExplode();
		return;
	}
	sound( self, CHAN_WEAPON, "weapons/bounce.wav", 1, ATTN_NORM );	// bounce sound
	if ( self->s.v.velocity[0] == 0 && self->s.v.velocity[1] == 0 && self->s.v.velocity[2] == 0 )
		VectorClear( self->s.v.avelocity );
}

/*
================
W_FireGrenade
================
*/
void W_FireGrenade()
{
	self->ps.wpn[wpGL].attacks++;

    if ( match_in_progress == 2 )
		if ( deathmatch != 4 )
			self->s.v.currentammo = self->s.v.ammo_rockets = self->s.v.ammo_rockets - 1;

	sound( self, CHAN_WEAPON, "weapons/grenade.wav", 1, ATTN_NORM );

	g_globalvars.msg_entity = EDICT_TO_PROG( self );

	WriteByte( MSG_ONE, SVC_SMALLKICK );

	newmis = spawn();
	g_globalvars.newmis = EDICT_TO_PROG( newmis );
	newmis->voided = 0;
	newmis->s.v.owner = EDICT_TO_PROG( self );
	newmis->s.v.movetype = MOVETYPE_BOUNCE;
	newmis->s.v.solid = SOLID_BBOX;
	newmis->s.v.classname = "grenade";

// set newmis speed     

	trap_makevectors( self->s.v.v_angle );

	if ( self->s.v.v_angle[0] )
	{
		float r1 = crandom(), r2 = crandom();

		// Jawnmode: disable randomness in grenade aim
		// - Molgrum
		if ( k_jawnmode )
			r1 = r2 = 0;

		newmis->s.v.velocity[0] =
		    g_globalvars.v_forward[0] * 600 + g_globalvars.v_up[0] * 200 +
		    r1 * g_globalvars.v_right[0] * 10 +
		    r2 * g_globalvars.v_up[0]    * 10;
		newmis->s.v.velocity[1] =
		    g_globalvars.v_forward[1] * 600 + g_globalvars.v_up[1] * 200 +
		    r1 * g_globalvars.v_right[1] * 10 +
		    r2 * g_globalvars.v_up[1]    * 10;
		newmis->s.v.velocity[2] =
		    g_globalvars.v_forward[2] * 600 + g_globalvars.v_up[2] * 200 +
		    r1 * g_globalvars.v_right[2] * 10 +
		    r2 * g_globalvars.v_up[0]    * 10;
	} else
	{
		aim( newmis->s.v.velocity );	// = aim(self, 10000);
		VectorScale( newmis->s.v.velocity, 600, newmis->s.v.velocity );	// * 600;
		newmis->s.v.velocity[2] = 200;
	}
	SetVector( newmis->s.v.avelocity, 300, 300, 300 );
// newmis.avelocity = '300 300 300';

	vectoangles( newmis->s.v.velocity, newmis->s.v.angles );

	newmis->s.v.touch = ( func_t ) GrenadeTouch;
	newmis->s.v.nextthink = g_globalvars.time + 2.5;
	newmis->s.v.think = ( func_t ) GrenadeExplode;

	if ( deathmatch == 4 && cvar("k_dmm4_gren_mode") )
		newmis->s.v.think = ( func_t ) SUB_Remove;

	setmodel( newmis, "progs/grenade.mdl" );
	setsize( newmis, 0, 0, 0, 0, 0, 0 );
	setorigin( newmis, PASSVEC3( self->s.v.origin ) );
}

//=============================================================================

void            spike_touch();

/*
===============
launch_spike

Used for both the player and the traps
===============
*/

void launch_spike( vec3_t org, vec3_t dir )
{
	newmis = spawn();
	g_globalvars.newmis = EDICT_TO_PROG( newmis );
	newmis->voided = 0;
	newmis->s.v.owner = EDICT_TO_PROG( self );
	newmis->s.v.movetype = MOVETYPE_FLYMISSILE;
	newmis->s.v.solid = SOLID_BBOX;

	newmis->s.v.touch = ( func_t ) spike_touch;
	newmis->s.v.classname = "spike";
	newmis->s.v.think = ( func_t ) SUB_Remove;
	newmis->s.v.nextthink = g_globalvars.time + 6;
	setmodel( newmis, "progs/spike.mdl" );
	setsize( newmis, 0, 0, 0, 0, 0, 0 );
	setorigin( newmis, PASSVEC3( org ) );

	// Jawnmode: spikes velocity is 1800 instead of 1000
	// - Molgrum
	VectorScale( dir, (k_jawnmode ? 1800 : 1000), newmis->s.v.velocity );

	vectoangles( newmis->s.v.velocity, newmis->s.v.angles );
}

void spike_touch()
{
	if ( other == PROG_TO_EDICT( self->s.v.owner ) )
		return;

	if ( self->voided )
	{
		return;
	}
	self->voided = 1;

	if ( other->s.v.solid == SOLID_TRIGGER )
		return;		// trigger field, do nothing

	if ( trap_pointcontents( PASSVEC3( self->s.v.origin ) ) == CONTENT_SKY )
	{
		ent_remove( self );
		return;
	}

	FixQuad(PROG_TO_EDICT( self->s.v.owner ));

// hit something that bleeds
	if ( other->s.v.takedamage )
	{
		if ( other->ct == ctPlayer )
			PROG_TO_EDICT( self->s.v.owner )->ps.wpn[wpNG].hits++;

		spawn_touchblood( 1 );
		other->deathtype = dtNG;
		T_Damage( other, self, PROG_TO_EDICT( self->s.v.owner ), 9 );
	} else
	{
		WriteByte( MSG_MULTICAST, SVC_TEMPENTITY );
		if ( !strcmp( self->s.v.classname, "wizspike" ) )
			WriteByte( MSG_MULTICAST, TE_WIZSPIKE );
		else if ( !strcmp( self->s.v.classname, "knightspike" ) )
			WriteByte( MSG_MULTICAST, TE_KNIGHTSPIKE );
		else
			WriteByte( MSG_MULTICAST, TE_SPIKE );
		WriteCoord( MSG_MULTICAST, self->s.v.origin[0] );
		WriteCoord( MSG_MULTICAST, self->s.v.origin[1] );
		WriteCoord( MSG_MULTICAST, self->s.v.origin[2] );
		trap_multicast( PASSVEC3( self->s.v.origin ), MULTICAST_PHS );
	}

	ent_remove( self );
}

void superspike_touch()
{
	if ( other == PROG_TO_EDICT( self->s.v.owner ) )
		return;

	if ( self->voided )
	{
		return;
	}
	self->voided = 1;


	if ( other->s.v.solid == SOLID_TRIGGER )
		return;		// trigger field, do nothing

	if ( trap_pointcontents( PASSVEC3( self->s.v.origin ) ) == CONTENT_SKY )
	{
		ent_remove( self );
		return;
	}

	FixQuad(PROG_TO_EDICT( self->s.v.owner ));

// hit something that bleeds
	if ( other->s.v.takedamage )
	{
		if ( other->ct == ctPlayer )
			PROG_TO_EDICT( self->s.v.owner )->ps.wpn[wpSNG].hits++;

		spawn_touchblood( 2 );
		other->deathtype = dtSNG;
		T_Damage( other, self, PROG_TO_EDICT( self->s.v.owner ), ( k_jawnmode ? 16 : 18 ) );
	} else
	{
		WriteByte( MSG_MULTICAST, SVC_TEMPENTITY );
		WriteByte( MSG_MULTICAST, TE_SUPERSPIKE );
		WriteCoord( MSG_MULTICAST, self->s.v.origin[0] );
		WriteCoord( MSG_MULTICAST, self->s.v.origin[1] );
		WriteCoord( MSG_MULTICAST, self->s.v.origin[2] );
		trap_multicast( PASSVEC3( self->s.v.origin ), MULTICAST_PHS );
	}

	ent_remove( self );
}

void W_FireSuperSpikes()
{
	vec3_t          dir, tmp;

	self->ps.wpn[wpSNG].attacks++;

	sound( self, CHAN_WEAPON, "weapons/spike2.wav", 1, ATTN_NORM );
	self->attack_finished = g_globalvars.time + 0.2;

    if ( match_in_progress == 2 )
		if ( deathmatch != 4 )
			self->s.v.currentammo = self->s.v.ammo_nails = self->s.v.ammo_nails - 2;
	aim( dir );		//dir = aim (self, 1000);

	VectorCopy( self->s.v.origin, tmp );
	tmp[2] += 16;
	launch_spike( tmp, dir );
	newmis->s.v.touch = ( func_t ) superspike_touch;
	setmodel( newmis, "progs/s_spike.mdl" );
	setsize( newmis, 0, 0, 0, 0, 0, 0 );
	g_globalvars.msg_entity = EDICT_TO_PROG( self );
	WriteByte( MSG_ONE, SVC_SMALLKICK );
}

void W_FireSpikes( float ox )
{
	vec3_t          dir, tmp;

	// Jawnmode: ignores alternating nails effect in nailgun
	// - Molgrum
	if ( k_jawnmode )
		ox = 0;

	trap_makevectors( self->s.v.v_angle );

    if( match_in_progress != 1 )
		if ( self->s.v.ammo_nails >= 2 && self->s.v.weapon == IT_SUPER_NAILGUN )
		{
			W_FireSuperSpikes();
			return;
		}


    if ( self->s.v.ammo_nails < 1 || match_in_progress == 1 )
	{
		self->s.v.weapon = W_BestWeapon();
		W_SetCurrentAmmo();
		return;
	}
	
	self->ps.wpn[wpNG].attacks++;

	sound( self, CHAN_WEAPON, "weapons/rocket1i.wav", 1, ATTN_NORM );
	self->attack_finished = g_globalvars.time + 0.2;

    if ( match_in_progress == 2 )
		if ( deathmatch != 4 )
			self->s.v.currentammo = self->s.v.ammo_nails = self->s.v.ammo_nails - 1;

	aim( dir );		// dir = aim (self, 1000);
	VectorScale( g_globalvars.v_right, ox, tmp );
	VectorAdd( tmp, self->s.v.origin, tmp );
	tmp[2] += 16;
	launch_spike( tmp, dir );

	g_globalvars.msg_entity = EDICT_TO_PROG( self );
	WriteByte( MSG_ONE, SVC_SMALLKICK );
}

/*
===============================================================================

PLAYER WEAPON USE

===============================================================================
*/

void W_SetCurrentAmmo()
{
	qboolean need_fix = false;
	int             items;
	float old_currentammo = self->s.v.currentammo;

// { get out of any weapon firing states

	if ( self->s.v.think == ( func_t )player_stand1 || self->s.v.think == ( func_t )player_run ) {
		if ( self->s.v.weapon == IT_AXE || self->s.v.weapon == IT_HOOK ) {
			if ( self->s.v.velocity[0] || self->s.v.velocity[1] ) { // run
				if ( self->s.v.frame < 0 || self->s.v.frame > 5 )
					need_fix = true; // wrong axe run frame
			}
			else { // stand
				if ( cvar("k_vweapons_models") )
				{
					if ( self->s.v.frame < 6 || self->s.v.frame > 17 )
						need_fix = true; // wrong axe stand frame
				} else {
					if ( self->s.v.frame < 17 || self->s.v.frame > 28 )
						need_fix = true; // wrong axe stand frame
				}
			}
		}
		else {
			if ( self->s.v.velocity[0] || self->s.v.velocity[1] ) { // run
				if ( cvar("k_vweapons_models") )
				{
					if ( self->s.v.frame < 0 || self->s.v.frame > 5 )
						need_fix = true; // wrong non axe run frame
				} else {
					if ( self->s.v.frame < 6 || self->s.v.frame > 11 )
						need_fix = true; // wrong non axe run frame
				}
			}
			else { // stand
				if ( cvar("k_vweapons_models") )
				{
					if ( self->s.v.frame < 6 || self->s.v.frame > 10)
						need_fix = true; // wrong non axe stand frame
				} else {
					if ( self->s.v.frame < 12 || self->s.v.frame > 16)
						need_fix = true; // wrong non axe stand frame
				}
			}
		}
	}
	else {
		need_fix = true; // hm, set proper ->s.v.think
	}

	if ( need_fix ) {
		// may change ->s.v.frame
		self->walkframe = 0;
		player_run();
	}
	else {
		// does't change ->s.v.frame and ->walkframe, so we does't break current animation sequence on weapon change
		self->s.v.nextthink = g_globalvars.time + 0.1;
		self->s.v.weaponframe = 0;
	}

// }

	items = self->s.v.items;
	items -= items & ( IT_SHELLS | IT_NAILS | IT_ROCKETS | IT_CELLS );
	switch ( ( int ) self->s.v.weapon )
	{
	case IT_AXE:
		self->s.v.currentammo = 0;
		self->s.v.weaponmodel = "progs/v_axe.mdl";
		self->s.v.weaponframe = 0;
#ifdef VWEP_TEST
        if(vw_available > 0)
            self->vw_index = 1;
#endif
		break;

	case IT_SHOTGUN:
		self->s.v.currentammo = self->s.v.ammo_shells;
		if ( cvar("k_instagib_custom_models") && cvar("k_instagib") == 1 )
			self->s.v.weaponmodel = "progs/v_coil.mdl";
		else
			self->s.v.weaponmodel = "progs/v_shot.mdl";
		self->s.v.weaponframe = 0;
		items |= IT_SHELLS;
#ifdef VWEP_TEST
        if(vw_available > 0)
            self->vw_index = 2;
#endif
		break;

	case IT_SUPER_SHOTGUN:
		self->s.v.currentammo = self->s.v.ammo_shells;
		if ( cvar("k_instagib_custom_models") && cvar("k_instagib") == 2 )
			self->s.v.weaponmodel = "progs/v_coil.mdl";
		else
			self->s.v.weaponmodel = "progs/v_shot2.mdl";
		self->s.v.weaponframe = 0;
		items |= IT_SHELLS;
#ifdef VWEP_TEST
        if(vw_available > 0)
            self->vw_index = 3;
#endif
		break;

	case IT_NAILGUN:
		self->s.v.currentammo = self->s.v.ammo_nails;
		self->s.v.weaponmodel = "progs/v_nail.mdl";
		self->s.v.weaponframe = 0;
		items |= IT_NAILS;
#ifdef VWEP_TEST
        if(vw_available > 0)
            self->vw_index = 4;
#endif
		break;

	case IT_SUPER_NAILGUN:
		self->s.v.currentammo = self->s.v.ammo_nails;
		self->s.v.weaponmodel = "progs/v_nail2.mdl";
		self->s.v.weaponframe = 0;
		items |= IT_NAILS;
#ifdef VWEP_TEST
        if(vw_available > 0)
            self->vw_index = 5;
#endif
		break;

	case IT_GRENADE_LAUNCHER:
		self->s.v.currentammo = self->s.v.ammo_rockets;
		self->s.v.weaponmodel = "progs/v_rock.mdl";
		self->s.v.weaponframe = 0;
		items |= IT_ROCKETS;
#ifdef VWEP_TEST
        if(vw_available > 0)
            self->vw_index = 6;
#endif
		break;

	case IT_ROCKET_LAUNCHER:
		self->s.v.currentammo = self->s.v.ammo_rockets;
		self->s.v.weaponmodel = "progs/v_rock2.mdl";
		self->s.v.weaponframe = 0;
		items |= IT_ROCKETS;
#ifdef VWEP_TEST
        if(vw_available > 0)
            self->vw_index = 7;
#endif
		break;

	case IT_LIGHTNING:
		self->s.v.currentammo = self->s.v.ammo_cells;
		self->s.v.weaponmodel = "progs/v_light.mdl";
		self->s.v.weaponframe = 0;
		items |= IT_CELLS;
#ifdef VWEP_TEST
        if(vw_available > 0)
            self->vw_index = 8;
#endif
		break;

	case IT_HOOK:
		self->s.v.currentammo = 0;

		if ( k_ctf_custom_models )
			self->s.v.weaponmodel = "progs/v_star.mdl";
		else
			self->s.v.weaponmodel = "progs/v_axe.mdl";

		self->s.v.weaponframe = 0;
#ifdef VWEP_TEST
        if(vw_available > 0)
            self->vw_index = 1;
#endif
		break;

	default:
		self->s.v.currentammo = 0;
		self->s.v.weaponmodel = "";
		self->s.v.weaponframe = 0;
#ifdef VWEP_TEST
        if(vw_available > 0)
            self->vw_index = 0;
#endif
		break;
	}

	if ( match_in_progress != 2 )
	{
		if ( old_currentammo )
			self->s.v.currentammo = old_currentammo;
		else
			self->s.v.currentammo = 1000;
	}
	self->s.v.items = items;
}


float W_BestWeapon()
{
	int             it;

	it = self->s.v.items;

	if ( self->s.v.waterlevel <= 1 && self->s.v.ammo_cells >= 1
	     && ( it & IT_LIGHTNING ) )
		return IT_LIGHTNING;

	else if ( self->s.v.ammo_nails >= 2 && ( it & IT_SUPER_NAILGUN ) )
		return IT_SUPER_NAILGUN;

	else if ( self->s.v.ammo_shells >= 2 && ( it & IT_SUPER_SHOTGUN ) )
		return IT_SUPER_SHOTGUN;

	else if ( self->s.v.ammo_nails >= 1 && ( it & IT_NAILGUN ) )
		return IT_NAILGUN;

	else if ( self->s.v.ammo_shells >= 1 && ( it & IT_SHOTGUN ) )
		return IT_SHOTGUN;

/*
 if(self.ammo_rockets >= 1 && (it & IT_ROCKET_LAUNCHER) )
  return IT_ROCKET_LAUNCHER;
 else if(self.ammo_rockets >= 1 && (it & IT_GRENADE_LAUNCHER) )
  return IT_GRENADE_LAUNCHER;

*/

	return (it & IT_AXE ? IT_AXE : 0 );
}

int W_CheckNoAmmo()
{
	if ( self->s.v.currentammo > 0 )
		return true;

	if ( self->s.v.weapon == IT_AXE || self->s.v.weapon == IT_HOOK )
		return true;

	self->s.v.weapon = W_BestWeapon();

	W_SetCurrentAmmo();

// drop the weapon down
	return false;
}

/*
============
W_Attack

An attack impulse can be triggered now
============
*/
void W_Attack()
{
	float           r;

	if ( !W_CheckNoAmmo() )
		return;

	self->lastwepfired = self->s.v.weapon;
	if (self->k_666 == 1 && !(self->s.v.weapon == IT_AXE || self->s.v.weapon == IT_SHOTGUN)) {
		self->s.v.items = self->s.v.items - ((int)self->s.v.items & IT_INVULNERABILITY);
		self->invincible_time = 0;
		self->invincible_finished = 0;
		self->k_666 = 0;
	}

	trap_makevectors( self->s.v.v_angle );	// calculate forward angle for velocity
	self->show_hostile = g_globalvars.time + 1;	// wake monsters up

	switch ( ( int ) self->s.v.weapon )
	{
	case IT_AXE:
        if ( self->ctf_flag & CTF_RUNE_HST )
		{
			self->attack_finished = g_globalvars.time + 0.3;
			HasteSound( self );
		}
	    else
			self->attack_finished = g_globalvars.time + 0.5;

		// crt - no axe sound for spec
		if ( !isRA() || ( isWinner( self ) || isLoser( self ) ) )
			sound( self, CHAN_WEAPON, "weapons/ax1.wav", 1, ATTN_NORM );

		r = g_random();
		if ( r < 0.25 )
			player_axe1();
		else if ( r < 0.5 )
			player_axeb1();
		else if ( r < 0.75 )
			player_axec1();
		else
			player_axed1();
		break;

	case IT_SHOTGUN:
		player_shot1();

		if ( self->ctf_flag & CTF_RUNE_HST )
		{
			self->attack_finished = g_globalvars.time + 0.3;
			HasteSound( self );
		}
		else
			self->attack_finished = g_globalvars.time + 0.5;

		W_FireShotgun();
		break;

	case IT_SUPER_SHOTGUN:
		player_shot1();

        if ( self->ctf_flag & CTF_RUNE_HST )
		{
			self->attack_finished = g_globalvars.time + 0.4;
			HasteSound( self );
		}
		else
			self->attack_finished = g_globalvars.time + 0.7;

		W_FireSuperShotgun();
		break;

	case IT_NAILGUN:
		self->s.v.ltime = g_globalvars.time;
		player_nail1();
		break;

	case IT_SUPER_NAILGUN:
		self->s.v.ltime = g_globalvars.time;
		player_nail1();
		break;

	case IT_GRENADE_LAUNCHER:
		player_rocket1();

		if ( self->ctf_flag & CTF_RUNE_HST )
		{
			self->attack_finished = g_globalvars.time + 0.3;
			HasteSound( self );
		}
		else
			self->attack_finished = g_globalvars.time + 0.6;

		W_FireGrenade();
		break;

	case IT_ROCKET_LAUNCHER:
		player_rocket1();

		if ( self->ctf_flag & CTF_RUNE_HST )
		{
			self->attack_finished = g_globalvars.time + 0.4;
			HasteSound( self );
		}
		else
			self->attack_finished = g_globalvars.time + 0.8;

		W_FireRocket();
		break;

	case IT_LIGHTNING:
		self->attack_finished = g_globalvars.time + 0.1;
		sound( self, CHAN_AUTO, "weapons/lstart.wav", 1, ATTN_NORM );
		self->s.v.ltime = g_globalvars.time;
		player_light1();
		break;

	case IT_HOOK:
		if ( self->hook_out )
			player_chain3();
		else
			player_chain1();

		self->attack_finished = g_globalvars.time + 0.1;
		break;
	}
}

qboolean W_CanSwitch( int wp, qboolean warn )
{
	int             it, am, fl = 0;

	it = self->s.v.items;
	am = 0;

	switch ( wp )
	{
	case 1:
		fl = IT_AXE;
		break;
	case 2:
		fl = IT_SHOTGUN;
		if ( self->s.v.ammo_shells < 1 )
			am = 1;
		break;
	case 3:
		fl = IT_SUPER_SHOTGUN;
		if ( self->s.v.ammo_shells < 2 )
			am = 1;
		break;
	case 4:
		fl = IT_NAILGUN;
		if ( self->s.v.ammo_nails < 1 )
			am = 1;
		break;
	case 5:
		fl = IT_SUPER_NAILGUN;
		if ( self->s.v.ammo_nails < 2 )
			am = 1;
		break;
	case 6:
		fl = IT_GRENADE_LAUNCHER;
		if ( self->s.v.ammo_rockets < 1 )
			am = 1;
		break;
	case 7:
		fl = IT_ROCKET_LAUNCHER;
		if ( self->s.v.ammo_rockets < 1 )
			am = 1;
		break;
	case 8:
		fl = IT_LIGHTNING;
		if ( self->s.v.ammo_cells < 1 )
			am = 1;
		break;

	case 22:
		fl = IT_HOOK;
		break;

	default:
		break;
	}

	if ( !( it & fl ) )
	{			// don't have the weapon or the ammo
		if ( warn )
			G_sprint( self, PRINT_HIGH, "no weapon\n" );
		return false;
	}

	if ( am )
	{			// don't have the ammo
		if ( warn )
			G_sprint( self, PRINT_HIGH, "not enough ammo\n" );
		return false;
	}

	return true;
}

/*
============
W_ChangeWeapon

============
*/
qboolean W_ChangeWeapon( int wp )
{
	int             it, am, fl = 0;

	if ( g_globalvars.time < self->attack_finished )
		return false;

	it = self->s.v.items;
	am = 0;

	switch ( wp )
	{
	case 1:
		fl = IT_AXE;
		break;
	case 2:
		fl = IT_SHOTGUN;
		if ( self->s.v.ammo_shells < 1 )
			am = 1;
		break;
	case 3:
		fl = IT_SUPER_SHOTGUN;
		if ( self->s.v.ammo_shells < 2 )
			am = 1;
		break;
	case 4:
		fl = IT_NAILGUN;
		if ( self->s.v.ammo_nails < 1 )
			am = 1;
		break;
	case 5:
		fl = IT_SUPER_NAILGUN;
		if ( self->s.v.ammo_nails < 2 )
			am = 1;
		break;
	case 6:
		fl = IT_GRENADE_LAUNCHER;
		if ( self->s.v.ammo_rockets < 1 )
			am = 1;
		break;
	case 7:
		fl = IT_ROCKET_LAUNCHER;
		if ( self->s.v.ammo_rockets < 1 )
			am = 1;
		break;
	case 8:
		fl = IT_LIGHTNING;
		if ( self->s.v.ammo_cells < 1 )
			am = 1;
		break;

	case 22:
		fl = IT_HOOK;
		if ( self->s.v.weapon != IT_HOOK )
		{
			if ( self->hook_out )
				GrappleReset( self->hook );
			self->hook_out = false;
			self->on_hook = false;
		}
		break;

	default:
		break;
	}

	if ( !( it & fl ) ) // don't have the weapon or the ammo
		G_sprint( self, PRINT_HIGH, "no weapon\n" );
	else if ( am ) // don't have the ammo
		G_sprint( self, PRINT_HIGH, "not enough ammo\n" );
	else {
	//
	// set weapon, set ammo
	//
		self->s.v.weapon = fl;
		W_SetCurrentAmmo();
	}

	return true;
}

/*
============
CheatCommand
============
*/
void CheatCommand()
{
	if ( !iKey( world, "*cheats" ) ) // FIXME: is this working?
		return;

//      if (deathmatch || coop)
	return;
/*
	self->s.v.ammo_rockets = 100;
	self->s.v.ammo_nails = 200;
	self->s.v.ammo_shells = 100;
	self->s.v.items = ( int ) self->s.v.items |
	    IT_AXE |
	    IT_SHOTGUN |
	    IT_SUPER_SHOTGUN |
	    IT_NAILGUN |
	    IT_SUPER_NAILGUN |
	    IT_GRENADE_LAUNCHER | IT_ROCKET_LAUNCHER | IT_KEY1 | IT_KEY2;

	self->s.v.ammo_cells = 200;
	self->s.v.items = ( int ) self->s.v.items | IT_LIGHTNING;

	self->s.v.weapon = IT_ROCKET_LAUNCHER;
	W_SetCurrentAmmo();
*/
}

/*
============
CycleWeaponCommand

Go to the next weapon with ammo
============
*/
qboolean CycleWeaponCommand()
{
	int             i, it, am;

	if ( g_globalvars.time < self->attack_finished )
		return false;

	it = self->s.v.items;

	for ( i = 0; i < 20; i++ ) // qqshka, 20 is just from head, but prevent infinite loop
	{
		am = 0;
		switch ( ( int ) self->s.v.weapon )
		{
		case IT_LIGHTNING:
			self->s.v.weapon = IT_AXE;
			break;

		case IT_AXE:
			self->s.v.weapon = IT_SHOTGUN;
			if ( self->s.v.ammo_shells < 1 )
				am = 1;
			break;

		case IT_SHOTGUN:
			self->s.v.weapon = IT_SUPER_SHOTGUN;
			if ( self->s.v.ammo_shells < 2 )
				am = 1;
			break;

		case IT_SUPER_SHOTGUN:
			self->s.v.weapon = IT_NAILGUN;
			if ( self->s.v.ammo_nails < 1 )
				am = 1;
			break;

		case IT_NAILGUN:
			self->s.v.weapon = IT_SUPER_NAILGUN;
			if ( self->s.v.ammo_nails < 2 )
				am = 1;
			break;

		case IT_SUPER_NAILGUN:
			self->s.v.weapon = IT_GRENADE_LAUNCHER;
			if ( self->s.v.ammo_rockets < 1 )
				am = 1;
			break;

		case IT_GRENADE_LAUNCHER:
			self->s.v.weapon = IT_ROCKET_LAUNCHER;
			if ( self->s.v.ammo_rockets < 1 )
				am = 1;
			break;

		case IT_ROCKET_LAUNCHER:
			self->s.v.weapon = IT_LIGHTNING;
			if ( self->s.v.ammo_cells < 1 )
				am = 1;
			break;
		}

		if ( ( it & ( int ) self->s.v.weapon ) && am == 0 )
		{
			W_SetCurrentAmmo();
			return true;
		}
	}

	return true;
}


/*
============
CycleWeaponReverseCommand

Go to the prev weapon with ammo
============
*/
qboolean CycleWeaponReverseCommand()
{
	int             i, it, am;

	if ( g_globalvars.time < self->attack_finished )
		return false;

	it = self->s.v.items;

	for ( i = 0; i < 20; i++ ) // qqshka, 20 is just from head, but prevent infinite loop
	{
		am = 0;
		switch ( ( int ) self->s.v.weapon )
		{
		case IT_LIGHTNING:
			self->s.v.weapon = IT_ROCKET_LAUNCHER;
			if ( self->s.v.ammo_rockets < 1 )
				am = 1;
			break;

		case IT_ROCKET_LAUNCHER:
			self->s.v.weapon = IT_GRENADE_LAUNCHER;
			if ( self->s.v.ammo_rockets < 1 )
				am = 1;
			break;

		case IT_GRENADE_LAUNCHER:
			self->s.v.weapon = IT_SUPER_NAILGUN;
			if ( self->s.v.ammo_nails < 2 )
				am = 1;
			break;

		case IT_SUPER_NAILGUN:
			self->s.v.weapon = IT_NAILGUN;
			if ( self->s.v.ammo_nails < 1 )
				am = 1;
			break;

		case IT_NAILGUN:
			self->s.v.weapon = IT_SUPER_SHOTGUN;
			if ( self->s.v.ammo_shells < 2 )
				am = 1;
			break;

		case IT_SUPER_SHOTGUN:
			self->s.v.weapon = IT_SHOTGUN;
			if ( self->s.v.ammo_shells < 1 )
				am = 1;
			break;

		case IT_SHOTGUN:
			self->s.v.weapon = IT_AXE;
			break;

		case IT_AXE:
			self->s.v.weapon = IT_LIGHTNING;
			if ( self->s.v.ammo_cells < 1 )
				am = 1;
			break;
		}
		if ( ( it & ( int ) self->s.v.weapon ) && am == 0 )
		{
			W_SetCurrentAmmo();
			return true;
		}
	}

	return true;
}


/*
============
ServerflagsCommand

Just for development
============
*/
void ServerflagsCommand()
{
	if ( !iKey( world, "*cheats" ) ) // FIXME: is this working?
		return;

//	g_globalvars.serverflags = ( int ) ( g_globalvars.serverflags ) * 2 + 1;
}


void kfjump ();
void krjump ();

int CaptainImpulses()
{
	if( k_captains != 2 )
		return 2;// s: return 2 if nothing interesting

	// ok - below possible captain stuff

	if( self->s.v.impulse > MAX_CLIENTS || !capt_num( self ) )
		return 0;// s: return 0 if captain mode is set and no captain things were entered

	return 1;// s: return 1 if it's a player picker impulse
}

/*
============
ImpulseCommands

============
*/

void ImpulseCommands()
{
	qboolean clear = true;
    int capt, impulse = self->s.v.impulse;

	if ( self->ct != ctPlayer )
		self->s.v.impulse = impulse = 0;

    if ( !impulse )
        return;

    capt = CaptainImpulses();

    if( !capt ) {
		; // empty
	}
    else if( capt == 1 )
        CaptainPickPlayer();

    else if( self->k_adminc && impulse >= 1 && impulse <= 9 )
        AdminImpBot();

    else if ( (impulse >= 1 && impulse <= 8) || impulse == 22 )
		clear = W_ChangeWeapon( impulse );

	else if ( impulse == 9 )
		CheatCommand();

	else if ( impulse == 10 )
		clear = CycleWeaponCommand();

	else if ( impulse == 11 )
		ServerflagsCommand();

	else if ( impulse == 12 )
		clear = CycleWeaponReverseCommand();

	else if ( impulse == 156 )
		kfjump ();

	else if ( impulse == 164 )
		krjump ();

	if ( clear )
		self->s.v.impulse = 0;
}

void can_prewar_msg( char *msg )
{
	if( g_globalvars.time > self->k_msgcount ) {
		self->k_msgcount = g_globalvars.time + 1;

		stuffcmd( self, "bf\n" );
		G_sprint( self, 2, "%s\n", msg);
	}
}


// if fire == false then can_prewar is called for jump
qboolean can_prewar ( qboolean fire )
{
	int k_prewar;

	if ( match_in_progress == 2 )
		return true;  // u can fire/jump

	k_prewar = cvar( "k_prewar" );

	switch ( k_prewar ) {
		case  1: goto captains; // probably u can fire/jump

		case  2: if ( self->ready )
					goto captains; // probably u can jump/fire if ready

				 if ( fire )
					 can_prewar_msg(redtext("type ready to enable fire"));
				 else
					 can_prewar_msg(redtext("type ready to enable jumps"));

				 return false; // u can't fire/jump if _not_ ready

		case  0:
		default: if ( fire ) {  // u can't fire
					 can_prewar_msg(redtext("can't fire due to prewar is off"));
					 return false;
				 }

				 goto captains;  // probably u can jump
	}

captains:

	if ( k_captains != 2 )
		return true; // u can fire/jump

	if ( fire ) {  // u can't fire
		can_prewar_msg(redtext("can't fire untill in captains mode"));
		return false;
	}

	return true; // u can jump
}

/*
============
W_WeaponFrame

Called every frame so impulse events can be handled as well as possible
============
*/
void W_WeaponFrame()
{
	if ( self->spawn_time + 0.05 > g_globalvars.time )
		return; // discard +attack till 50 ms after respawn, like ktpro 

	if ( self->wreg_attack ) // client simulate +attack via "cmd wreg" feature
		self->s.v.button0 = true;

	ImpulseCommands();

	if ( g_globalvars.time < self->attack_finished )
		return;

// check for attack

    if ( self->s.v.button0 && !intermission_running )
    {
		if ( !readytostart() )
			return;	

		if ( match_in_progress == 1 || !can_prewar( true ) )
			return;

		SuperDamageSound();
		W_Attack();
	}
}


/*
========
SuperDamageSound

Plays sound if needed
========
*/
void SuperDamageSound()
{
	if ( self->super_damage_finished > g_globalvars.time )
	{
		if ( self->super_sound < g_globalvars.time )
		{
			self->super_sound = g_globalvars.time + 1;
			// Play 8x sound if quad + strength rune
            if ( self->ctf_flag & CTF_RUNE_STR )
				sound( self, CHAN_AUTO, "rune/rune22.wav", 1, ATTN_NORM );
			else
				sound( self, CHAN_AUTO, "items/damage3.wav", 1, ATTN_NORM );
		}
	}
	else if ( self->ctf_flag & CTF_RUNE_STR )
	{
		if ( self->super_sound < g_globalvars.time )
		{
			self->super_sound = g_globalvars.time + 1;
			sound( self, CHAN_AUTO, "rune/rune2.wav", 1, ATTN_NORM );
		}
	}
	return;
}