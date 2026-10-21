#include "llc_galaxy_hell.h"

#include "llc_collision.h"

#include "llc_line3.h"

#include <Windows.h>
#include <mmsystem.h>
#include <llc_noise.h>

static	::llc::error_t	applyDamage
	( const int32_t	weaponDamage
	, int32_t		& healthPart
	, int32_t		& healthParent
	) {
	const uint32_t				finalDamage			= ::llc::min(weaponDamage, healthPart);
	healthPart				-= finalDamage;
	healthParent			-= finalDamage;
	return 0 >= healthPart;
}

static	::llc::error_t	collisionDetect		(::llc::SShots & shots, const ::llc::n3f2_t & modelPosition, ::llc::apod<::llc::n3f2_t> & collisionPoints)	{
	bool						detected			= false;
	collisionPoints.clear();
	for(uint32_t iShot = 0; iShot < shots.Particles.Position.size(); ++iShot) {
		const ::llc::line3f2_t		shotSegment			= {shots.PositionPrev[iShot], shots.Particles.Position[iShot]};
		float						t					= 0;
		::llc::n3f2_t				collisionPoint		= {};
		if( ::llc::intersectRaySphere(shotSegment.A, (shotSegment.B - shotSegment.A).Normalize(), {1.2, modelPosition}, t, collisionPoint)
			&& t < 1
		) {
			detected				= true;
			collisionPoints.push_back(collisionPoint);
			shots.Remove(iShot);
			--iShot;
		}
	}
	return 0;
}

static	::llc::error_t	handleCollisionPoint		(::ghg::SGalaxyHell & solarSystem, int32_t weaponDamage, ::llc::SSpaceshipScore & attackerScore, ::llc::SSpaceshipScore & damagedScore, ::llc::SSpaceshipOrbiter & damagedPart, int32_t iAttackedPart, ::llc::SSpaceshipCore & damagedShip, int32_t iAttackedShip, const ::llc::n3f2_t & sphereCenter, const ::llc::n3f2_t & collisionPoint)	{
	solarSystem.ShipState.SpaceshipManager.ShipOrbiterActionQueue[iAttackedShip]->push_back(::llc::SHIP_ACTION_Hit);
	const ::llc::n3f2_t			bounceVector				= (collisionPoint - sphereCenter).Normalize();
	solarSystem.DecoState.Debris.SpawnDirected(1 + weaponDamage / 10, 0.3, bounceVector, collisionPoint, 50, 1);
	attackerScore.Hits		+= 1;
	attackerScore.Score		+= weaponDamage;
	solarSystem.DecoState.ScoreParticles.Create(collisionPoint, bounceVector, 10.0f + weaponDamage * .001f, {weaponDamage, 1});
	attackerScore.DamageDone	+= weaponDamage;
	damagedScore.DamageReceived	+= weaponDamage;
	if(::applyDamage(weaponDamage, damagedPart.Health.Value, damagedShip.Health)) {	// returns true if health reaches zero
		attackerScore.Score			+= weaponDamage * 10;
		solarSystem.DecoState.ScoreParticles.Create(collisionPoint, bounceVector, 10.0f + weaponDamage * .001f, {weaponDamage * 10, 2});
		attackerScore.KilledOrbiters	+= 1;
		damagedScore.OrbitersLost		+= 1;
		const ::ghg::SGHEntityVisual		& entityGeometry			= solarSystem.ShipState.EntityVisuals[solarSystem.ShipState.GetShipPartEntity(iAttackedPart)];
		const int32_t				indexMesh					= entityGeometry.Geometry;
		const uint32_t				countTriangles				= solarSystem.ShipState.Scene.Geometry[indexMesh].Triangles.size();
		::ghg::decoExplosionAdd(solarSystem.DecoState.Explosions, indexMesh, entityGeometry.Image, countTriangles, collisionPoint, 60);
		solarSystem.DecoState.Debris.SpawnSpherical(20, collisionPoint, 60, 2);
		if(0 >= damagedShip.Health) {
			attackerScore.Score		+= weaponDamage * 50;
			solarSystem.DecoState.ScoreParticles.Create(collisionPoint, bounceVector, 10.0f + weaponDamage * .001f, {weaponDamage * 50, 3});
			attackerScore.KilledShips	+= 1;
			const ::llc::n3f2_t			& parentPosition			= solarSystem.ShipState.GetShipPosition(iAttackedShip);
			::ghg::decoExplosionAdd(solarSystem.DecoState.Explosions, indexMesh, entityGeometry.Image, countTriangles, parentPosition, 13);
			solarSystem.DecoState.Debris.SpawnSpherical(80, parentPosition, 13, 2.8f);
			solarSystem.PlayState.SimulatedTime.Step	= solarSystem.PlayState.Slowing;
			solarSystem.PlayState.SimulatedTime.Scale	= 1.0;
		}
		return 1;
	}
	else {
		if(0 < damagedShip.Health) 
			damagedScore.HitsSurvived	+= 1;
	}
	return 0;
}

static	::llc::error_t	updateEntityTransforms		(uint32_t iEntity, const ::llc::SVirtualEntityManager & entities, ::ghg::SShipScene & scene, ::llc::SRigidBodyIntegrator & bodies)	{
	const ::llc::SEntity			& entity					= entities[iEntity];
	if(::llc::EID_INVALID == entity.RigidBody) {
		if(::llc::EID_INVALID == entity.Parent)
			scene.Transforms[iEntity].Identity();
		else
			scene.Transforms[iEntity]	= scene.Transforms[entity.Parent];
	}
	else {
		::llc::m4f2_t				& matrixGlobal				= scene.Transforms[iEntity];
		bodies.GetTransform(entity.RigidBody, matrixGlobal);
		if(::llc::EID_INVALID != entity.Parent)
			matrixGlobal			*= scene.Transforms[entity.Parent];
	}

	const ::llc::vceid			entityChildren				= entities.GetChildren(iEntity);
	for(uint32_t iChild = 0; iChild < entityChildren.size(); ++iChild) {
		const uint32_t				iChildEntity				= entityChildren[iChild];
		::updateEntityTransforms(iChildEntity, entities, scene, bodies);
	}
	return 0;
}
static	::llc::error_t	updateShots				(::ghg::SGalaxyHell & solarSystem, double secondsLastFrame)	{
	for(uint32_t iShip = 0; iShip < solarSystem.ShipState.SpaceshipManager.ShipCores.size(); ++iShip) {
		::llc::au1_t					& shipParts				= solarSystem.ShipState.SpaceshipManager.ShipParts[iShip];
		for(uint32_t iPart = 0; iPart < shipParts.size(); ++iPart) {
			::std::lock_guard			lockUpdate			(solarSystem.LockUpdate);
			solarSystem.ShipState.WeaponManager.Shots[solarSystem.ShipState.SpaceshipManager.Orbiters[shipParts[iPart]].Weapon].Update((float)secondsLastFrame);
		}
	}

	::llc::apod<::llc::n3f2_t>	collisionPoints;
	for(uint32_t iShipAttacker = 0; iShipAttacker < solarSystem.ShipState.SpaceshipManager.ShipCores.size(); ++iShipAttacker) {
		::llc::SSpaceshipScore		& shipAttackerScore		= solarSystem.ShipState.SpaceshipManager.ShipScores	[iShipAttacker];
		::llc::SSpaceshipCore		& shipAttacker			= solarSystem.ShipState.SpaceshipManager.ShipCores	[iShipAttacker];
		::llc::au1_t					& shipAttackerParts		= solarSystem.ShipState.SpaceshipManager.ShipParts	[iShipAttacker];
		//::llc::apod<::llc::n3f2_t>	& shipDistancesToTarget	= solarSystem.ShipState.SpaceshipManager.ShipCoresDistanceToTargets	[iShipAttacker];
		for(uint32_t iPartAttacker = 0; iPartAttacker < shipAttackerParts.size(); ++iPartAttacker) {
			::llc::SSpaceshipOrbiter	& shipPartAttacker		= solarSystem.ShipState.SpaceshipManager.Orbiters[shipAttackerParts[iPartAttacker]];
			const ::llc::SWeapon		& weaponAttacker		= solarSystem.ShipState.WeaponManager.Weapons[shipPartAttacker.Weapon];
			::llc::SShots				& shotsAttacker			= solarSystem.ShipState.WeaponManager.Shots[shipPartAttacker.Weapon];

			for(uint32_t iShipAttacked = 0; iShipAttacked < solarSystem.ShipState.SpaceshipManager.ShipCores.size(); ++iShipAttacked) {
				::llc::SSpaceshipCore		& shipAttacked			= solarSystem.ShipState.SpaceshipManager.ShipCores	[iShipAttacked];
				if(shipAttacked.Health <= 0 || shipAttacker.Team == shipAttacked.Team) // avoid dead ships and ships of the same team
					continue;

				::llc::au1_t					& shipAttackedParts		= solarSystem.ShipState.SpaceshipManager.ShipParts	[iShipAttacked];
				::llc::SSpaceshipScore		& shipAttackedScore		= solarSystem.ShipState.SpaceshipManager.ShipScores	[iShipAttacked];

				for(uint32_t iPartAttacked = 0; iPartAttacked < shipAttackedParts.size(); ++iPartAttacked) {
					const int32_t				attackedPart			= shipAttackedParts[iPartAttacked];
					::llc::SSpaceshipOrbiter	& shipPartAttacked		= solarSystem.ShipState.SpaceshipManager.Orbiters[attackedPart];
					if(shipPartAttacked.Health.Value <= 0)
						continue;
					const int32_t				iAttackedPartEntity		= solarSystem.ShipState.ShipPartEntity[attackedPart];
					const ::ghg::SGHEntityVisual		& entity				= solarSystem.ShipState.EntityVisuals[iAttackedPartEntity];
					if(-1 != entity.Geometry) {
						const ::llc::n3f2_t			attackedPosition		= solarSystem.ShipState.Scene.Transforms[iAttackedPartEntity].GetTranslation();
						::std::lock_guard			lockUpdate				(solarSystem.LockUpdate);
						::collisionDetect(shotsAttacker, attackedPosition, collisionPoints);
						for(uint32_t iCollisionPoint = 0; iCollisionPoint < collisionPoints.size(); ++iCollisionPoint)
							if(::handleCollisionPoint(solarSystem, weaponAttacker.Shot.Damage, shipAttackerScore, shipAttackedScore, shipPartAttacked, attackedPart, shipAttacked, iShipAttacked, attackedPosition, collisionPoints[iCollisionPoint]))	// returns true if part health reaches zero.
								break;
					}
					const ::llc::vceid			  entityChildren		= solarSystem.ShipState.Engine.Entities.GetChildren(iAttackedPartEntity);
					for(uint32_t iEntity = 0; iEntity < entityChildren.size(); ++iEntity) {
						::ghg::SGHEntityVisual			& entityChild			= solarSystem.ShipState.EntityVisuals[entityChildren[iEntity]];
						if(::llc::EID_INVALID == solarSystem.ShipState.Engine.Entities[entityChildren[iEntity]].Parent)
							continue;

						if(-1 == entityChild.Geometry)
							continue;

						const ::llc::n3f2_t			attackedPosition		= solarSystem.ShipState.Scene.Transforms[entityChildren[iEntity]].GetTranslation();
						::std::lock_guard			lockUpdate				(solarSystem.LockUpdate);
						::collisionDetect(shotsAttacker, attackedPosition, collisionPoints);
						for(uint32_t iCollisionPoint = 0; iCollisionPoint < collisionPoints.size(); ++iCollisionPoint)
							if(::handleCollisionPoint(solarSystem, weaponAttacker.Shot.Damage, shipAttackerScore, shipAttackedScore, shipPartAttacked, attackedPart, shipAttacked, iShipAttacked, attackedPosition, collisionPoints[iCollisionPoint])) {	// returns true if part health reaches zero.
								break;
							}
					}
				}
			}

			//::llc::SWeapon	& weaponAttacker	= solarSystem.ShipState.WeaponManager.Weapons[shipPartAttacker.Weapon];
			if(weaponAttacker.Shot.Type == ::llc::WEAPON_LOAD_Rocket) {
				for(uint32_t iShot = 0; iShot < shotsAttacker.Particles.Position.size(); ++iShot) {
					const ::llc::apod<::llc::n3f2_t> & distances =  shotsAttacker.DistanceToTargets[iShot];
					if(0 == distances.size())
						continue;

					if(2.5f > shotsAttacker.Lifetime[iShot])
						continue;

					for(uint32_t iTarget = 0; iTarget < distances.size(); ++iTarget) {
						::llc::n3f2_t			target		= distances[iTarget];
						target.Normalize();
						const uint64_t			targetSeed	= solarSystem.PlayState.Constants.Seed + iPartAttacker + iShot;
						const ::llc::n3f2_t		& distance	= distances[::llc::noise1DBase32((uint32_t)targetSeed) % distances.size()];
						if(distance.LengthSquared() > 10000) 
							continue;

						if(shotsAttacker.Particles.Direction[iShot].Dot(target) > 0) {
							shotsAttacker.Particles.Direction[iShot] = ::llc::interpolate_linear(shotsAttacker.Particles.Direction[iShot], distance, ::llc::min(secondsLastFrame * 2, 1.0)).Normalize();
							break;
						}
					}
				}
			}
			else if(weaponAttacker.Shot.Type == ::llc::WEAPON_LOAD_Missile) {
				for(uint32_t iShot = 0; iShot < shotsAttacker.Particles.Position.size(); ++iShot) {
					const ::llc::apod<::llc::n3f2_t>	& distances =  shotsAttacker.DistanceToTargets[iShot];
					if(0 == distances.size())
						continue;

					if(2.5f > shotsAttacker.Lifetime[iShot])
						continue;

					const uint64_t					targetSeed	= solarSystem.PlayState.Constants.Seed + iPartAttacker + iShot;
					const ::llc::n3f2_t				& distance	= distances[::llc::noise1DBase32((uint32_t)targetSeed) % distances.size()];
					if(distance.LengthSquared() > 10000) 
						continue;

					shotsAttacker.Particles.Direction[iShot] = ::llc::interpolate_linear(shotsAttacker.Particles.Direction[iShot], distance, ::llc::min(secondsLastFrame * 1, 1.0)).Normalize();
				}
			}

		}
	}
	return 0;
}

static	::llc::error_t	updateDistancesToTargets	(::ghg::SGalaxyHell & solarSystem, int32_t team, int32_t iShipPart, ::llc::papod<::llc::n3f2_t> & orbiterDistancesToTargets, ::llc::SShots & shots)	{
	{
		::std::lock_guard			lockUpdate					(solarSystem.LockUpdate);
		orbiterDistancesToTargets.clear();
		for(uint32_t iShot = 0; iShot < shots.Particles.Position.size(); ++iShot)
			shots.DistanceToTargets[iShot].clear();
	}

	::llc::apod<::llc::SSpaceshipOrbiter>	& orbiters				= solarSystem.ShipState.SpaceshipManager.Orbiters				;
	::llc::apod<::llc::SSpaceshipCore>	& shipCores				= solarSystem.ShipState.SpaceshipManager.ShipCores				;
	::llc::aobj<::llc::au1_t>		& shipCoresParts		= solarSystem.ShipState.SpaceshipManager.ShipParts				;
	const ::llc::n3f2_t				weaponPosition			= solarSystem.ShipState.Scene.Transforms[solarSystem.ShipState.ShipPartEntity[iShipPart]].GetTranslation();
	for(uint32_t iShip = 0; iShip < shipCores.size(); ++iShip) {
		::llc::SSpaceshipCore				& ship					= shipCores[iShip];
		if(ship.Team == team || ship.Health <= 0)
			continue;

		for(uint32_t iPart = 0; iPart < shipCoresParts[iShip].size(); ++iPart) {
			::llc::SSpaceshipOrbiter					& shipPartTarget		= orbiters[shipCoresParts[iShip][iPart]];
			if(0 >= shipPartTarget.Health.Value)
				continue;

			const ::llc::n3f2_t				targetPosition			= solarSystem.ShipState.Scene.Transforms[solarSystem.ShipState.ShipPartEntity[shipCoresParts[iShip][iPart]]].GetTranslation();
			{
				::std::lock_guard				lockUpdate				(solarSystem.LockUpdate);
				orbiterDistancesToTargets->push_back(targetPosition - weaponPosition);
				for(uint32_t iShot = 0; iShot < shots.Particles.Position.size(); ++iShot) {
					shots.DistanceToTargets[iShot].push_back(targetPosition - shots.Particles.Position[iShot]);
				}
			}
		}
	}
	return 0;
}

static	::llc::error_t	updateShipOrbiter			(::ghg::SGalaxyHell & solarSystem, int32_t team, ::llc::SSpaceshipOrbiter & shipPart, int32_t iShip, int32_t iShipPart, ::llc::papod<::llc::n3f2_t> & shipPartDistancesToTargets, double secondsLastFrame)	{
	::llc::SWeapon				& weapon					= solarSystem.ShipState.WeaponManager.Weapons	[shipPart.Weapon];
	::llc::SShots				& shots						= solarSystem.ShipState.WeaponManager.Shots	[shipPart.Weapon];
	for(uint32_t iParticle = 0; iParticle < shots.Particles.Position.size(); ++iParticle)
		shots.Particles.Position[iParticle].x				-= (float)(solarSystem.PlayState.BackgroundSpeed.Current * secondsLastFrame * .2);

	;

	if(weapon.Shot.Type == ::llc::WEAPON_LOAD_Rocket || weapon.Shot.Type == ::llc::WEAPON_LOAD_Missile || weapon.Type == ::llc::WEAPON_TYPE_Cannon) {
		updateDistancesToTargets(solarSystem, team, iShipPart, shipPartDistancesToTargets, shots); 
	}

	if(weapon.Trigger.Update(secondsLastFrame)) {}
	else {
		solarSystem.ShipState.SpaceshipManager.ShipScores[iShip].Shots	+= 1;
		solarSystem.ShipState.SpaceshipManager.ShipScores[iShip].Bullets	+= weapon.Shot.ParticleCount;

		const ::llc::m4f2_t			& shipModuleMatrix			= solarSystem.ShipState.Scene.Transforms[solarSystem.ShipState.EntityVisuals[solarSystem.ShipState.GetShipPartEntity(iShipPart)].Transform];
		::llc::n3f2_t				positionGlobal				= shipModuleMatrix.GetTranslation();
		::llc::n3f2_t				targetDistance				= (shipPartDistancesToTargets && shipPartDistancesToTargets->size()) ? (*shipPartDistancesToTargets)[::llc::noise1DBase32((uint32_t)solarSystem.PlayState.Constants.Seed + iShip * ::ghg::MAX_ORBITER_COUNT + iShipPart) % shipPartDistancesToTargets->size()] : ::llc::n3f2_t{};
		::llc::n3f2_t				targetPosition				= targetDistance + positionGlobal;
		if(weapon.Type == ::llc::WEAPON_TYPE_Cannon) {
			if(1 < targetDistance.LengthSquared()) {
				::llc::n3f2_t			direction					= targetDistance;
				direction.Normalize(); 
				{
					::std::lock_guard		lockUpdate			(solarSystem.LockUpdate);
					if(weapon.Shot.Type == ::llc::WEAPON_LOAD_Rocket)
						weapon.Shoot(shots, positionGlobal, direction, 1);
					else if(weapon.Shot.Type == ::llc::WEAPON_LOAD_Cannonball)
						weapon.Shoot(shots, positionGlobal, direction, 1);
					else if(weapon.Shot.Type == ::llc::WEAPON_LOAD_Missile)
						weapon.Shoot(shots, positionGlobal, direction, 1);
				}

				if(solarSystem.PlayState.TimeStage > 1) {
					solarSystem.ShipState.Engine.Integrator.Forces[solarSystem.ShipState.Engine.GetRigidBody(solarSystem.ShipState.GetShipPartEntity(iShipPart))].Rotation	= {};
					::llc::m4f2_t			inverseTransform			= shipModuleMatrix.GetInverse();
					::llc::SBodyCenter		& shipModuleTransform		= solarSystem.ShipState.GetOrbiterTransform(iShipPart);
					::llc::n3f2_t			up							= {1, 0, 0};
					::llc::n3f2_t			front						= {0, 1, 0};
					//const ::llc::m4f2_t	& shipMatrix				= solarSystem.Scene.Transforms[solarSystem.Entities[solarSystem.Entities[shipPart.Entity].Parent].Transform];
					targetPosition		= inverseTransform.Transform(targetPosition);
					::llc::quat<float>		q;
					q.LookAt(shipModuleTransform.Position, targetPosition, up, front);
					shipModuleTransform.Orientation	= q;
				}
			}
		}
		else if(weapon.Type == ::llc::WEAPON_TYPE_Gun) {
			::llc::n3f2_t				direction				= {team ? -1.0f : 1.0f, 0, 0};
			::std::lock_guard			lockUpdate				(solarSystem.LockUpdate);
			if(weapon.Shot.Type == ::llc::WEAPON_LOAD_Ray)
				weapon.Shoot(shots, positionGlobal, direction, .75f);
			else
				weapon.Shoot(shots, positionGlobal, direction, 5.0f);
		}
		else if(weapon.Type == ::llc::WEAPON_TYPE_Shotgun) {
			::llc::n3f2_t				direction				= {team ? -1.0f : 1.0f, 0, 0};
			::std::lock_guard			lockUpdate				(solarSystem.LockUpdate);
			if(weapon.Shot.Type == ::llc::WEAPON_LOAD_Ray)
				weapon.Shoot(shots, positionGlobal, direction, .75f);
			else
				weapon.Shoot(shots, positionGlobal, direction, 5.0f);
		}
	}
	return 0;
}

//stacxpr const double						UPDATE_STEP_TIME		= 0.012;

static	::llc::error_t	shipsUpdate				(::ghg::SGalaxyHell & solarSystem, double secondsLastFrame)	{
	::updateShots(solarSystem, secondsLastFrame);

	bool						playing					= 0;
	for(uint32_t iShip = 0; iShip < solarSystem.ShipState.SpaceshipManager.ShipCores.size(); ++iShip) {
		::llc::SSpaceshipCore			& ship					= solarSystem.ShipState.SpaceshipManager.ShipCores[iShip];
		if(0 >= ship.Health)
			continue;
		::llc::au1_t					& shipParts				= solarSystem.ShipState.SpaceshipManager.ShipParts[iShip];
		::llc::SBodyCenter			& shipTransform			= solarSystem.ShipState.Engine.Integrator.Centers[solarSystem.ShipState.Engine.GetRigidBody(solarSystem.ShipState.ShipCoreEntity[iShip])];
		if(ship.Team) { 
			playing					= 1;

			shipTransform.Position.z	= (float)(sin(iShip + solarSystem.PlayState.SimulatedTime.Seconds) * (iShip * 5.0) * ((iShip % 2) ? -1 : 1));
			shipTransform.Position.x	= (float)((iShip * 5.0) - solarSystem.PlayState.GlobalState.Stage + 10 - (solarSystem.PlayState.BackgroundSpeed.Current * solarSystem.PlayState.BackgroundSpeed.Current * .0005 * ((solarSystem.PlayState.BackgroundSpeed.Current >= 0) ? 1 : -1)  ));
			double						timeWaveVertical					= .1;

			if(0 == (solarSystem.PlayState.GlobalState.Stage % 7)) {
				if(iShip % 2)
					shipTransform.Position.z	= float(cos(iShip + solarSystem.PlayState.SimulatedTime.Seconds) * ((solarSystem.ShipState.SpaceshipManager.ShipCores.size() - 1 - iShip) * 4.0) * ((iShip % 2) ? -1 : 1));
				else
					shipTransform.Position.z	= float(sin(iShip + solarSystem.PlayState.SimulatedTime.Seconds) * (iShip * 4.0) * ((iShip % 2) ? -1 : 1));
					 if(0 == (iShip % 2)) timeWaveVertical	= .65;
				else if(0 == (iShip % 3)) timeWaveVertical	= .80;
				else if(0 == (iShip % 7)) timeWaveVertical	= .80;
			}
			else if(0 == (solarSystem.PlayState.GlobalState.Stage % 5)) {
					shipTransform.Position.z	= float(cos(iShip + solarSystem.PlayState.SimulatedTime.Seconds) * ((solarSystem.ShipState.SpaceshipManager.ShipCores.size() - 1 - iShip) * 3.0) * ((iShip % 2) ? -1 : 1));
					 if(0 == (iShip % 2)) timeWaveVertical	= .50;
				else if(0 == (iShip % 3)) timeWaveVertical	= .75;
				else if(0 == (iShip % 7)) timeWaveVertical	= .80;
			}
			else if(0 == (solarSystem.PlayState.GlobalState.Stage % 3)) {
					shipTransform.Position.z	= float(cos(iShip + solarSystem.PlayState.SimulatedTime.Seconds) * ((solarSystem.ShipState.SpaceshipManager.ShipCores.size() - 1 - iShip) * 2.0) * ((iShip % 2) ? -1 : 1));
					 if(0 == (iShip % 2)) timeWaveVertical	= .25;
				else if(0 == (iShip % 3)) timeWaveVertical	= .50;
				else if(0 == (iShip % 7)) timeWaveVertical	= .75;
			}
			else {
					 if(0 == (iShip % 2)) timeWaveVertical	= .50;
				else if(0 == (iShip % 3)) timeWaveVertical	= .25;
				else if(0 == (iShip % 7)) timeWaveVertical	= .15;
			}
			shipTransform.Position.x		+= float(sin(solarSystem.PlayState.SimulatedTime.Seconds * timeWaveVertical * ::llc::math_2pi) * ((iShip % 2) ? -1 : 1));
		}
		for(uint32_t iPart = 0; iPart < shipParts.size(); ++iPart) {
			::llc::SSpaceshipOrbiter				& shipPart				= solarSystem.ShipState.SpaceshipManager.Orbiters[shipParts[iPart]];
			if(0 >= shipPart.Health.Value)
				continue;
			::updateShipOrbiter(solarSystem, ship.Team, shipPart, iShip, shipParts[iPart], solarSystem.ShipState.WeaponManager.WeaponTargets[shipParts[iPart]], secondsLastFrame);
		}
	}
	return playing;
}

static	::llc::error_t	processInput			(::ghg::SGalaxyHell & solarSystem, double secondsLastFrame, const ::llc::SInput & input, ::llc::vpobj<::llc::SEventSystem> frameEvents) {
	(void)frameEvents;


	::llc::view<const ::ghg::SShipController>	controllers	= solarSystem.ShipControllers;
	//const bool				key_rotate_left			= input.KeyboardCurrent.KeyState[VK_NUMPAD8	];
	//const bool				key_rotate_right		= input.KeyboardCurrent.KeyState[VK_NUMPAD2	];
	//const bool				key_rotate_front		= input.KeyboardCurrent.KeyState[VK_NUMPAD6	];
	//const bool				key_rotate_back			= input.KeyboardCurrent.KeyState[VK_NUMPAD4	];
	//const bool				key_rotate_reset		= input.KeyboardCurrent.KeyState[VK_NUMPAD5	];
	const bool					key_camera_switch		= input.KeyboardCurrent.KeyState['C'		];
	const bool					key_camera_move_front	= input.KeyboardCurrent.KeyState[VK_HOME	];
	const bool					key_camera_move_back	= input.KeyboardCurrent.KeyState[VK_END		];
	const bool					key_camera_move_up		= input.KeyboardCurrent.KeyState['E'		];
	const bool					key_camera_move_down	= input.KeyboardCurrent.KeyState['Q'		];
	const bool					key_camera_reset		= input.KeyboardCurrent.KeyState['R'		];
	if(key_camera_reset)
		solarSystem.ShipState.Scene.Global.CameraReset();

	constexpr double			speed					= 40;
	if(solarSystem.ShipState.Engine.Entities.size()) {
		for(uint32_t iPlayer = 0; iPlayer < solarSystem.PlayState.Constants.Players; ++iPlayer) {
			const ::ghg::SShipController	& shipController		= controllers[iPlayer];
			::llc::SBodyCenter				& playerBody			= solarSystem.ShipState.Engine.Integrator.Centers[solarSystem.ShipState.Engine.GetRigidBody(solarSystem.ShipState.ShipCoreEntity[iPlayer])];
			::llc::SSpaceshipCore			& ship					= solarSystem.ShipState.SpaceshipManager.ShipCores[iPlayer];
			bool							turbo					= shipController.Turbo;
			double							speedMultiplier			= 1;
			if(ship.Nitro.Value < ship.Nitro.Limit)
				ship.Nitro.Value			+= (float)(secondsLastFrame * .1);
			if(ship.Nitro.Value > 0 && turbo) {
				ship.Nitro.Value			-= (float)secondsLastFrame;
				speedMultiplier				*= 2;
			}

			{
				solarSystem.PlayState.BackgroundSpeed.AccelerationControl	
					= shipController.Forward	? float(+1)
					: shipController.Back		? float(-1)
					: 0
					;

				playerBody.Position.x	+= (float)(secondsLastFrame * speed * speedMultiplier * solarSystem.PlayState.BackgroundSpeed.AccelerationControl);

				if(shipController.Left 	) { playerBody.Position.z += (float)(secondsLastFrame * speed * speedMultiplier); }
				if(shipController.Right	) { playerBody.Position.z -= (float)(secondsLastFrame * speed * speedMultiplier); }
			}

			//if(shipController.PointerDeltas.x) 
			//	playerBody.Orientation.y += (float)(shipController.PointerDeltas.x / 128.f);
			//if(shipController.PointerDeltas.y) 
			//	playerBody.Orientation.z += (float)(shipController.PointerDeltas.y / 128.f);

			playerBody.Orientation.Normalize();
		}
	}
	if(1 == solarSystem.PlayState.Constants.Players && solarSystem.ShipState.SpaceshipManager.ShipCores.size()) {
		::llc::n3f2_t										shipPosition;
		solarSystem.ShipState.GetShipPosition(0, shipPosition);
		if(solarSystem.ShipState.Scene.Global.CameraMode == ::ghg::CAMERA_MODE_FOLLOW) {
			solarSystem.ShipState.Scene.Global.Camera[::ghg::CAMERA_MODE_FOLLOW].Position	= shipPosition + ::llc::n3f2_t{-80.f, 25, 0};
			solarSystem.ShipState.Scene.Global.Camera[::ghg::CAMERA_MODE_FOLLOW].Target		= shipPosition + ::llc::n3f2_t{1000.f, 0, 0};
			//solarSystem.ShipState.Scene.Global.Camera[::ghg::CAMERA_MODE_FOLLOW].Up			= {0, 1, 0};
		}
		else if(solarSystem.ShipState.Scene.Global.CameraMode == ::ghg::CAMERA_MODE_FRONT) {
			solarSystem.ShipState.Scene.Global.Camera[::ghg::CAMERA_MODE_FRONT].Position	= shipPosition + ::llc::n3f2_t{0, 0, 0};
			solarSystem.ShipState.Scene.Global.Camera[::ghg::CAMERA_MODE_FRONT].Target		= shipPosition + ::llc::n3f2_t{1, 0, 0};
			//solarSystem.ShipState.Scene.Global.Camera[::ghg::CAMERA_MODE_FRONT].Up			= {0, 1, 0};
		}
	}

	//------------------------------------------- Handle misc input
	::llc::SCameraPoints		& camera				= solarSystem.ShipState.Scene.Global.Camera[solarSystem.ShipState.Scene.Global.CameraMode];
	if(controllers.size()) {
		if(key_camera_move_up	) camera.Position.y	+= (float)secondsLastFrame * (controllers[0].Turbo ? 20 : 10);
		if(key_camera_move_down	) camera.Position.y	-= (float)secondsLastFrame * (controllers[0].Turbo ? 20 : 10);
	}

	solarSystem.PlayState.GlobalState.CameraSwitchDelay	+= secondsLastFrame;
	if(key_camera_switch && solarSystem.PlayState.GlobalState.CameraSwitchDelay > .2) {
		solarSystem.PlayState.GlobalState.CameraSwitchDelay	= 0;

		solarSystem.ShipState.Scene.Global.CameraMode = (::ghg::CAMERA_MODE)((solarSystem.ShipState.Scene.Global.CameraMode + 1) % ((solarSystem.PlayState.Constants.Players > 1) ? (::ghg::CAMERA_MODE_PERSPECTIVE + 1) : ::ghg::CAMERA_MODE_COUNT));
	}

	if(camera.Position.y > 0.001f)
	if(camera.Position.y > 0.001f) if(key_camera_move_front) camera.Position.RotateZ(::llc::math_pi * secondsLastFrame);
	if(camera.Position.x < 0.001f)
	if(camera.Position.x < 0.001f) if(key_camera_move_back) camera.Position.RotateZ(::llc::math_pi * -secondsLastFrame);

	if(camera.Position.y < 0) camera.Position.y = 0.0001f;
	if(camera.Position.y < 0) camera.Position.y = 0.0001f;
	if(camera.Position.x > 0) camera.Position.x = -0.0001f;
	if(camera.Position.x > 0) camera.Position.x = -0.0001f;

	return 0;
}

stacxpr	const double	UPDATE_STEP_TIME			= 0.012;

::llc::error_t			ghg::solarSystemUpdate		(::ghg::SGalaxyHell & solarSystem, double actualSecondsLastFrame, const ::llc::SInput & input, ::llc::vpobj<::llc::SEventSystem> frameEvents)	{
	if(solarSystem.PlayState.GlobalState.Paused) 
		return 0;

	{
		::std::lock_guard			lockUpdate					(solarSystem.LockUpdate);
		for(uint32_t iShip = 0; iShip < solarSystem.ShipState.SpaceshipManager.ShipOrbiterActionQueue.size(); ++iShip)
			solarSystem.ShipState.SpaceshipManager.ShipOrbiterActionQueue[iShip].clear();
	}
	::llc::n2u1_t				targetMetrics				= {}; 
	targetMetrics			= solarSystem.DrawCache.RenderTargetMetrics;

	double						secondsToProcess			= ::llc::min(actualSecondsLastFrame, 0.15);
	for(uint32_t iShip = 0; iShip < solarSystem.ShipState.SpaceshipManager.ShipCores.size(); ++iShip) {
		::llc::au1_t					& shipParts					= solarSystem.ShipState.SpaceshipManager.ShipParts[iShip];
		const uint32_t				iBody						= solarSystem.ShipState.Engine.GetRigidBody(solarSystem.ShipState.ShipCoreEntity[iShip]);
		solarSystem.ShipState.Engine.Integrator.SetActive (iBody, true);
		solarSystem.ShipState.Engine.Integrator.Flags[iBody].UpdatedTransform	= false;
		for(uint32_t iPart = 0; iPart < shipParts.size(); ++iPart) {
			::llc::SSpaceshipOrbiter	& shipPart					= solarSystem.ShipState.SpaceshipManager.Orbiters[shipParts[iPart]];
			::std::lock_guard			lockUpdate					(solarSystem.LockUpdate);
			memcpy(solarSystem.ShipState.WeaponManager.Shots[shipPart.Weapon].PositionDraw.begin(), solarSystem.ShipState.WeaponManager.Shots[shipPart.Weapon].Particles.Position.begin(), solarSystem.ShipState.WeaponManager.Shots[shipPart.Weapon].Particles.Position.size() * sizeof(::llc::n3f2_t));
			solarSystem.ShipState.Engine.Integrator.SetActive(solarSystem.ShipState.Engine.GetRigidBody(solarSystem.ShipState.ShipPartEntity[shipParts[iPart]]), true);
			solarSystem.ShipState.Engine.Integrator.Flags	 [solarSystem.ShipState.Engine.GetRigidBody(solarSystem.ShipState.ShipPartEntity[shipParts[iPart]])].UpdatedTransform	= false;
		}
	}

	{
		::std::lock_guard			lockUpdate					(solarSystem.LockUpdate);
		::ghg::decoUpdate(solarSystem.DecoState, secondsToProcess, solarSystem.PlayState.BackgroundSpeed.Current, targetMetrics);
	}

	::processInput(solarSystem, secondsToProcess, input, frameEvents);


	solarSystem.PlayState.GlobalState.UserTime.Played	+= actualSecondsLastFrame;
	solarSystem.PlayState.TimeRealStage					+= actualSecondsLastFrame;
	while(secondsToProcess > 0) {
		const double				secondsLastFrameUnscaled	= ::llc::min(UPDATE_STEP_TIME, secondsToProcess);

		const double				secondsLastFrameSimulated	= solarSystem.PlayState.SimulatedTime.Update(secondsLastFrameUnscaled, solarSystem.PlayState.Fasting);
		solarSystem.PlayState.TimeStage	+= secondsLastFrameSimulated;
		solarSystem.PlayState.BackgroundSpeed.Update(secondsLastFrameSimulated);
		solarSystem.ShipState.Engine.Integrator.Integrate(secondsLastFrameSimulated);

		bool						playing				= shipsUpdate(solarSystem, secondsLastFrameSimulated);
		if(false == playing) {
			playing					= false;
			for(uint32_t iExplosion = 0; iExplosion < solarSystem.DecoState.Explosions.size(); ++iExplosion) {
				if(solarSystem.DecoState.Explosions[iExplosion].Slices.size()) {
					playing					= true;
					break;
				}
			}
			if(false == playing)  {	// Set up enemy ships
				::ghg::stageSetup(solarSystem);
				playing					= true;
				break;
			}
		}

		for(uint32_t iEntity = 0; iEntity < solarSystem.ShipState.Engine.Entities.size(); ++iEntity) {
			const ::llc::SEntity		& entity			= solarSystem.ShipState.Engine.Entities[iEntity];
			if(-1 == entity.Parent)	// process root entities
				::updateEntityTransforms(iEntity, solarSystem.ShipState.Engine.Entities, solarSystem.ShipState.Scene, solarSystem.ShipState.Engine.Integrator);
		}
		secondsToProcess		-= secondsLastFrameUnscaled;
	}
	solarSystem.DecoState.AnimationTime	= solarSystem.PlayState.SimulatedTime.Seconds;
	solarSystem.ShipState.Scene.Global.LightVector.Normalize();

	::llc::m4f2_t				& matrixProjection		= solarSystem.ShipState.Scene.Global.MatrixProjection;

	//if(targetMetrics.x >= (double)targetMetrics.y)
		matrixProjection.FieldOfView(::llc::math_pi * .25, targetMetrics.x / (double)targetMetrics.y, 0.1, 5000.0);
	//else
	//	matrixProjection.FieldOfView(::llc::math_pi * .25, targetMetrics.y / (double)targetMetrics.x, 0.1, 5000.0);
	::llc::m4f2_t				matrixViewport			= {};
	matrixViewport.ViewportLH(targetMetrics.u1());
	matrixProjection		*= matrixViewport;

	//solarSystem.Scene.LightVector	= solarSystem.Scene.LightVector.RotateY(secondsLastFrame * 2);
	return 0;
}
