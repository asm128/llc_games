#include "llc_galaxy_hell.h"
#include "llc_geometry.h"

#include "llc_raster_lh.h"
#include "llc_gui_text.h"
#include "llc_font.h"

#include "llc_label.h"
#include "llc_slice.h"

using ::llc::get_value_namep, ::llc::get_enum_namep, ::llc::failed;
LLC_USING_TYPEINT();

stxp	u2_t			MAX_LIGHT_RANGE		= 10;

static ::llc::error_t drawShipTriangle
	( ::llc::g8bgra targetPixels, const ::ghg::SShipGeometry & geometry, int iTriangle
	, const ::llc::m4f2_t & matrixTransform, const ::llc::m4f2_t & matrixTransformVP, const ::llc::n3f2_t & lightVector
	, ::llc::an2s1_t & pixelCoords, ::llc::apod<::llc::trif2_t> & pixelVertexWeights, ::llc::gc8bgra textureImage
	, ::llc::an3f2_t & lightPoints, ::llc::a8bgra & lightColors, ::llc::gu2_t depthBuffer
	, const ::llc::function<::llc::error_t(::llc::g8bgra, const ::llc::n2s1_t &, const ::llc::bgra &)> & funcSetPixel
	) {
	const ::llc::tri3f2_t & triangle = geometry.Triangles[iTriangle];
	const ::llc::tri2f2_t & triangleTexCoords = geometry.TextureCoords[iTriangle];
	const ::llc::n3f2_t & normal = geometry.Normals[iTriangle * 3];
	::llc::drawQuadTriangle(targetPixels.metrics(), triangle, matrixTransformVP, pixelCoords, pixelVertexWeights, depthBuffer);
	const ::llc::n3f2_t xnormal = matrixTransform.TransformDirection(normal).Normalize();
	::llc::tri3f2_t triangleWorld = triangle;
	::llc::transform(triangleWorld, matrixTransform);
	return ::llc::drawPixels(targetPixels, triangleWorld, xnormal, triangleTexCoords, lightVector, pixelCoords, pixelVertexWeights, textureImage, lightPoints, lightColors, funcSetPixel);
}
static	::llc::error_t	drawStars			(const ::ghg::SStars & stars, ::llc::g8bgra targetPixels)	{
	::llc::bgra					colors[]			=
		{ {0xfF, 0xfF, 0xfF, }
		, {0xC0, 0xC0, 0xfF, }
		, {0xfF, 0xC0, 0xC0, }
		, {0xC0, 0xD0, 0xC0, }
		};
	for(uint32_t iStar = 0; iStar < stars.Brightness.size(); ++iStar) {
		::llc::n2f2_t				starPos				= stars.Position[iStar];
		::llc::bgra					starFinalColor		= colors[iStar % ::llc::size(colors)] * stars.Brightness[iStar];
		::llc::setPixel(targetPixels, starPos.s1(), starFinalColor);
		const	int32_t					brightRadius		= 1 + (iStar % 3) + (rand() % 2);
		const	double					brightRadiusSquared	= brightRadius * (double)brightRadius;
		double							brightUnit			= 1.0 / brightRadiusSquared;
		for(int32_t y = -brightRadius; y < brightRadius; ++y)
		for(int32_t x = -brightRadius; x < brightRadius; ++x) {
			::llc::n2f2_t					brightPos			= {(float)x, (float)y};
			const double					brightDistance		= brightPos.LengthSquared();
			if(brightDistance <= brightRadiusSquared) {
				::llc::n2s1_t				pixelPos			= (starPos + brightPos).s1();
				if( pixelPos.y >= 0 && pixelPos.y < (int32_t)targetPixels.metrics().y
				 && pixelPos.x >= 0 && pixelPos.x < (int32_t)targetPixels.metrics().x
 				)
					::llc::setPixel(targetPixels, pixelPos, targetPixels[pixelPos.y][pixelPos.x] + starFinalColor * (1.0-(brightDistance * brightUnit * (1 + (rand() % 3)))));
			}
		}
	}
	return 0;
}

static	::llc::error_t	drawDebris
	( ::llc::g8bgra			targetPixels
	, const ::ghg::SDebris					& debris
	, const ::llc::m4f2_t					& matrixVPV
	, ::llc::grid<uint32_t>				depthBuffer
	, const ::llc::view<const ::llc::bgra>	& debrisColors
	)	{

	float prebrightness = 2.5f * (1.0f / debris.Brightness.size());
	for(uint32_t iParticle = 0; iParticle < debris.Brightness.size(); ++iParticle) {
		const ::llc::rgbaf			& colorShot			= debrisColors[iParticle % debrisColors.size()];
		::llc::n3f2_t				starPos				= debris.Particles.Position[iParticle];
		starPos					= matrixVPV.Transform(starPos);
		if(starPos.z > 1 || starPos.z < 0)
			continue;
		const ::llc::n2s2_t			pixelCoord			= {(int32_t)starPos.x, (int32_t)starPos.y};
		if( pixelCoord.y < 0 || pixelCoord.y >= (int32_t)targetPixels.metrics().y
		 || pixelCoord.x < 0 || pixelCoord.x >= (int32_t)targetPixels.metrics().x
		)
			continue;
		uint32_t					depth				= uint32_t(starPos.z * 0xFFFFFFFFU);
		if(depth > depthBuffer[pixelCoord.y][pixelCoord.x])
			continue;
		float						brightness		= debris.Brightness[iParticle];
		::llc::rgbaf				starFinalColor	= colorShot * brightness;
		starFinalColor.g		= ::llc::max(0.0f, starFinalColor.g - (1.0f - ::llc::min(1.0f, brightness * prebrightness * (iParticle * 2))));
		starFinalColor.b		= ::llc::max(0.0f, starFinalColor.b - (1.0f - ::llc::min(1.0f, brightness * prebrightness * (iParticle * 1))));
		//::llc::setPixel(targetPixels, pixelCoord, starFinalColor);
		stacxpr	 double				brightRadius		= 1.5;
		stacxpr	 double				brightRadiusSquared	= brightRadius * brightRadius;
		stacxpr	 double				brightUnit			= 1.0 / brightRadiusSquared;

		for(int32_t y = (int32_t)-brightRadius - 1, yStop = (int32_t)brightRadius + 1; y < yStop; ++y)
		for(int32_t x = (int32_t)-brightRadius - 1; x < yStop; ++x) {
			::llc::n2f2_t				brightPos			= {(float)x, (float)y};
			const double				brightDistance		= brightPos.LengthSquared();
			if(brightDistance <= brightRadiusSquared) {
				::llc::n2s2_t				blendPos			= pixelCoord + (brightPos).s2();
				if( blendPos.y < 0 || blendPos.y >= (int32_t)targetPixels.metrics().y
				 || blendPos.x < 0 || blendPos.x >= (int32_t)targetPixels.metrics().x
				)
					continue;
				uint32_t					& blendVal			= depthBuffer[blendPos.y][blendPos.x];
				if(depth > blendVal)
					continue;
				blendVal				= depth;
				double						finalBrightness		= 1.0-(brightDistance * brightUnit);
				::llc::bgra					& pixelVal			= targetPixels[blendPos.y][blendPos.x];
				pixelVal				= (starFinalColor * finalBrightness + pixelVal).Clamp();
			}
		}
	}
	return 0;
}

static	::llc::error_t	drawScoreParticles
	( ::llc::g8bgra	targetPixels
	, const ::ghg::SScoreParticles	& debris
	, const ::llc::m4f2_t			& matrixVPV
	, ::llc::grid<uint32_t>		depthBuffer
	, const ::llc::SRasterFont		& font
	)	{

	stacxpr	double				brightRadius		= 1.5;
	stacxpr	double				brightRadiusSquared	= brightRadius * brightRadius;
	stacxpr	double				brightUnit			= 1.0 / brightRadiusSquared;
	for(uint32_t iParticle = 0; iParticle < debris.Scores.size(); ++iParticle) {
		::llc::n3f2_t				starPos				= debris.Particles.Position[iParticle];
		starPos					= matrixVPV.Transform(starPos);
		if(starPos.z > 1 || starPos.z < 0)
			continue;
		const ::llc::n2s1_t			pixelCoord			= {(int16_t)starPos.x, (int16_t)starPos.y};
		if( pixelCoord.y < 0 || pixelCoord.y >= (int32_t)targetPixels.metrics().y
		 || pixelCoord.x < 0 || pixelCoord.x >= (int32_t)targetPixels.metrics().x
		)
			continue;
		uint32_t					depth				= uint32_t(starPos.z * 0xFFFFFFFFU);
		if(depth > depthBuffer[pixelCoord.y][pixelCoord.x])
			continue;

		::ghg::SScoreParticle		particle			= debris.Scores[iParticle];
		::llc::rgbaf				starFinalColor		= ::llc::GREEN;
		starFinalColor.r		+= 1.0f - ::llc::min(1.0f, particle.Brightness);
		starFinalColor.g		-= 1.0f - ::llc::min(1.0f, particle.Brightness);
		starFinalColor.b		-= 1.0f - ::llc::min(1.0f, particle.Brightness);
		starFinalColor.Clamp();
		starFinalColor.a		= starFinalColor.g;
		::llc::apod<::llc::n2u1_t>	dstCoords;

		sc_t						textToShow[64]		= {};
		sprintf_s(textToShow, "%i", particle.Score);
		const ::llc::vcs			finalText			= textToShow;
		::llc::rect2s1_t				rectText			= {{}, {int16_t(font.CharSize.x * finalText.size()), font.CharSize.y}};
		rectText.Offset = (pixelCoord - ::llc::n2s1_t{int16_t(rectText.Size.x >> 1), int16_t(rectText.Size.y >> 1)});

		llc_necs(::llc::textLineRaster(targetPixels.metrics(), font.CharSize, rectText, font.Texture, finalText, dstCoords));
		for(uint32_t iCoord = 0; iCoord < dstCoords.size(); ++iCoord) {
			::llc::pixelBlend(targetPixels, dstCoords[iCoord].s1(), starFinalColor);
		}
	}
	return 0;
}

typedef ::llc::error_t (FSetPixel)	(::llc::g8bgra targetPixels, const ::llc::n2s1_t & pixelCoord, const ::llc::bgra & color);

static	::llc::error_t	drawCannonball
	( const ::ghg::SShipManager		& shipState
	, const ::llc::n3f2_t			& position
	, const ::llc::n3f2_t			& prevPosition
	, const ::llc::n3f2_t			& _direction
	, const ::llc::rgbaf			& bulletColor
	, const float					animationTime
	, const ::llc::m4f2_t			& matrixVP
	, ::llc::g8bgra					& targetPixels
	, ::llc::gu2_t					depthBuffer
	, ::ghg::SGalaxyHellDrawCache	& drawCache
	) {
	uint32_t					pixelsDrawn				= 0;
	double						absanim					= fabsf(sinf(animationTime * 2));
	::llc::m4f2_t				matrixTransform				= {};
	matrixTransform.Identity();
	matrixTransform.SetTranslation(position, false);
	::llc::img<::llc::bgra>		image						= {};
	image.resize(::llc::n2u2_t{1, 1286}, bulletColor);
	{
		::llc::m4f2_t				matrixTransformVP			= matrixTransform * matrixVP;
		::ghg::getLightArrays(matrixTransform.GetTranslation(), drawCache.LightPointsWorld, drawCache.LightColorsWorld, drawCache.LightPointsModel, drawCache.LightColorsModel);
		::ghg::SShipGeometry mesh;
		llc_necs(shipState.GetGeometry(shipState.GetModelMesh(::llc::SHIP_GEOMETRY_Sphere), mesh));

		for(uint32_t iTriangle = 0; iTriangle < mesh.Triangles.size(); ++iTriangle) {
			::llc::clear(drawCache.PixelCoords, drawCache.PixelVertexWeights);
			pixelsDrawn += ::drawShipTriangle(targetPixels, mesh, iTriangle, matrixTransform, matrixTransformVP, shipState.Scene.Global.LightVector, drawCache.PixelCoords, drawCache.PixelVertexWeights, image, drawCache.LightPointsModel, drawCache.LightColorsModel, depthBuffer
				, [bulletColor, absanim](::llc::g8bgra targetPixels, const ::llc::n2s1_t & pixelCoord, const ::llc::bgra & color) {
					targetPixels[pixelCoord.y][pixelCoord.x] = ::llc::interpolate_linear(::llc::rgbaf(color), bulletColor, absanim).Clamp(); 
					return 0;
			});
		}
	}
	{
		//image.resize({1, 64}, bulletColor);
		for(uint32_t y = 0; y < image.metrics().y; ++y)
			image[y][0].a	= uint8_t(y * 2);
		::llc::m4f2_t				mS							= {};

		(void)position; (void)prevPosition;
		::llc::n3f2_t				direction					= _direction;//position - prevPosition;
		double						blurLength					= direction.Length();
		mS.Scale({1, (float)blurLength, 1}, true);

		::llc::m4f2_t				mRY							= {};
		::llc::m4f2_t				mRP							= {};
		//direction.AngleWith();
		::llc::quatf2_t				qry							= {}; 
		::llc::quatf2_t				qrz							= {}; 
		direction.Normalize();
		::llc::n2f2_t				vxz							= {_direction.x, _direction.z}; vxz.InPlaceNormalize();
		double						angleY						= vxz.AngleWith({1, 0});
		double						angleZ						= ::llc::math_pi_2;
		if(_direction.z >= 0) 
			angleY *= -1;
		qrz.CreateFromAxisAngle({0, 0, 1}, angleZ); qrz.Normalize();
		qry.CreateFromAxisAngle({0, 1, 0}, angleY); qry.Normalize();
		mRY.SetOrientation(qrz);
		mRP.SetOrientation(qry);
		matrixTransform			= mS * mRY * mRP;
		matrixTransform.SetTranslation(position, false);
		const ::llc::m4f2_t			matrixTransformVP			= matrixTransform * matrixVP;
		::llc::clear(drawCache.LightPointsModel, drawCache.LightColorsModel);
		::ghg::SShipGeometry mesh;
		llc_necs(shipState.GetGeometry(shipState.GetModelMesh(::llc::SHIP_GEOMETRY_Cylinder), mesh));
		for(uint32_t iTriangle = 0; iTriangle < mesh.Triangles.size(); ++iTriangle) {
			::llc::clear(drawCache.PixelCoords, drawCache.PixelVertexWeights);
			pixelsDrawn += ::drawShipTriangle(targetPixels, mesh, iTriangle, matrixTransform, matrixTransformVP, shipState.Scene.Global.LightVector, drawCache.PixelCoords, drawCache.PixelVertexWeights, image, drawCache.LightPointsModel, drawCache.LightColorsModel, depthBuffer
				, [](::llc::g8bgra targetPixels, const ::llc::n2s1_t & pixelCoord, const ::llc::bgra & color) {
					::llc::bgra			& tp						= targetPixels[pixelCoord.y][pixelCoord.x];
					tp				= ::llc::interpolate_linear(color, tp, color.a / 255.0f); 
					return 0;
			});
		}
	}
	return pixelsDrawn;
}

static	::llc::error_t	drawShots			(::llc::g8bgra targetPixels
	, const ::ghg::SShipManager		& shipState
	, const ::llc::SShots			& shots
	, const ::llc::m4f2_t			& matrixVPV
	, float							animationTime
	, const ::llc::SSpaceshipCore		& shipCore
	, const ::llc::SWeapon			& weapon
	, ::llc::grid<uint32_t>		depthBuffer
	, ::llc::apod<::llc::n3f2_t>		pixelCoordsCache
	, ::ghg::SGalaxyHellDrawCache	& drawCache
	) {
	::llc::rgbaf				colorShot				= ::llc::WHITE;
	double						brightRadius			= 1;
	double						intensity				= 1;
	bool						line					= true;
	if(::llc::WEAPON_LOAD_Cannonball == weapon.Shot.Type) {
		colorShot				= shipCore.Team ? ::llc::rgbaf{1.0f, 0.25f, 0.75f} : ::llc::TURQUOISE;
		line					= false;
	}
	else {
		if(::llc::WEAPON_LOAD_Ray == weapon.Shot.Type) { 
			colorShot				= ::llc::rgbaf{1.0f, 0.1f, 0.0f}; 
			intensity				=  2; 
		}
		else if(::llc::WEAPON_LOAD_Bullet == weapon.Shot.Type) { 
			colorShot				= ::llc::DARKGRAY; 
			intensity				= .25; 
		}
		else if(::llc::WEAPON_LOAD_Shot == weapon.Shot.Type) { 
			colorShot				= ::llc::GRAY; 
			intensity				= .25; 
		}
		else if(::llc::WEAPON_LOAD_Rocket == weapon.Shot.Type) {
			colorShot				= shipCore.Team ? ::llc::rgbaf{1.0f, 0.125f, 0.25f} : ::llc::LIGHTORANGE;
			//brightRadius			= 2.6;
			line					= false;
		}
		else if(::llc::WEAPON_LOAD_Missile == weapon.Shot.Type) {
			colorShot				= shipCore.Team ? ::llc::rgbaf{1.0f, 0.025f, 0.05f} : ::llc::CYAN;
			//brightRadius			= 2.6;
			line					= false;
		}
	}
	const ::llc::n2s2_t			targetMetrics			= targetPixels.metrics().s2();
	for(uint32_t iShot = 0; iShot < shots.Brightness.size(); ++iShot) {
		float						brightness				= shots.Brightness[iShot];
		pixelCoordsCache.clear();
		const ::llc::n3f2_t			& starPosPrev			= shots.PositionDraw[iShot];
		const ::llc::n3f2_t			& starPos				= shots.Particles.Position[iShot];
		//::llc::line3<float>		raySegment				= {starPosPrev, starPos};

		::llc::line3f2_t				raySegment				= {starPos, starPosPrev}; //raySegmentWorld;
		raySegment.A			= matrixVPV.Transform(raySegment.A);
		raySegment.B			= matrixVPV.Transform(raySegment.B);
		if(raySegment.A.z < 0 || raySegment.A.z > 1) continue;
		if(raySegment.B.z < 0 || raySegment.B.z > 1) continue;
		::llc::drawLine(targetMetrics.u1(), raySegment, pixelCoordsCache, depthBuffer);
		if(line) {
		}
		else {
			//pixelCoordsCache.push_back(raySegment.B);
			::drawCannonball(shipState, starPos, starPosPrev, shots.Particles.Direction[iShot], colorShot, animationTime, matrixVPV, targetPixels, depthBuffer, drawCache);
		}
		const double			pixelCoordUnit		= 1.0 / (pixelCoordsCache.size());
		for(uint32_t iPixelCoord = 0, countPixelCoords = pixelCoordsCache.size(); iPixelCoord < countPixelCoords; ++iPixelCoord) {
			const ::llc::n3f2_t		& pixelCoord		= pixelCoordsCache[iPixelCoord];
			if(pixelCoord.z < 0 || pixelCoord.z > 1)
				continue;
			if( pixelCoord.y < 0 || pixelCoord.y >= targetMetrics.y
			 || pixelCoord.x < 0 || pixelCoord.x >= targetMetrics.x
			)
				continue;
			targetPixels[(uint32_t)pixelCoord.y][(uint32_t)pixelCoord.x]	= colorShot;
			const uint32_t			depth				= uint32_t(pixelCoord.z * 0xFFFFFFFFU);
			const double			brightRadiusSquared	= brightRadius * brightRadius;
			double					brightUnit			= 1.0 / brightRadiusSquared * brightness;
			for(int32_t y = (int32_t)-brightRadius, brightCount = (int32_t)brightRadius; y < brightCount; ++y)
			for(int32_t x = (int32_t)-brightRadius; x < brightCount; ++x) {
				::llc::n2f2_t			brightPos			= {(float)x, (float)y};
				const double			brightDistance		= brightPos.LengthSquared();
				if(brightDistance <= brightRadiusSquared) {
					::llc::n2s2_t			blendPos			= ::llc::n2<int32_t>{(int32_t)pixelCoord.x, (int32_t)pixelCoord.y} + (brightPos).s2();
					if( blendPos.y < 0 || blendPos.y >= targetMetrics.y
					 || blendPos.x < 0 || blendPos.x >= targetMetrics.x
					)
						continue;
					uint32_t				& blendVal			= depthBuffer[blendPos.y][blendPos.x];
					if(depth > blendVal)
						continue;
					blendVal			= depth;
					::llc::bgra				& pixelVal			= targetPixels[blendPos.y][blendPos.x];
					double					finalBrightness
						= line ? brightDistance * brightUnit * pixelCoordUnit * (countPixelCoords - 1 - iPixelCoord)
						: 1.0 - (brightDistance * brightUnit)
						;
					::llc::rgbaf			pixelColor			= ::llc::interpolate_linear(::llc::rgbaf{pixelVal}, colorShot, finalBrightness * intensity);
					pixelVal			= pixelColor.Clamp();
				}
			}
		}

	}
	return 0;
}

::llc::error_t			ghg::getLightArraysFromDebris
	( const ::ghg::SDecoState								& decoState
	, ::llc::an3f2_t			& lightPoints
	, ::llc::a8bgra			& lightColors
	, const ::llc::vc8bgra	& debrisColors
	)						{
	for(uint32_t iParticle = 0; iParticle < decoState.Debris.Particles.Position.size(); ++iParticle) {
		lightPoints.push_back(decoState.Debris.Particles.Position[iParticle]);
		::llc::rgbaf										colorShot			= debrisColors[iParticle % debrisColors.size()];
		lightColors.push_back(colorShot * decoState.Debris.Brightness[iParticle]);
	}
	return 0;
}


::llc::error_t			ghg::getLightArraysFromShips
	( const ::ghg::SShipManager	& shipState
	, ::llc::an3f2_t				& lightPoints
	, ::llc::a8bgra				& lightColors
	) {
	constexpr ::llc::bgra		colorLightPlayer		= ::llc::bgra{0xFF, 0x88, 0xFF};
	constexpr ::llc::bgra		colorLightEnemy			= ::llc::bgra{0xFF, 0x88, 0x88};
	for(uint32_t iShip = 0; iShip < shipState.SpaceshipManager.ShipCores.size(); ++iShip) {
		lightPoints.push_back(shipState.GetShipPosition(iShip));
		const ::llc::SSpaceshipCore	& ship					= shipState.SpaceshipManager.ShipCores[iShip];
		lightColors.push_back((0 == ship.Team) ? colorLightPlayer : colorLightEnemy);
		for(uint32_t iPart = 0; iPart < shipState.SpaceshipManager.ShipParts[iShip].size(); ++iPart) {
			const ::llc::SSpaceshipOrbiter									& shipPart				= shipState.SpaceshipManager.Orbiters[shipState.SpaceshipManager.ShipParts[iShip][iPart]];
			const ::llc::rgbaf								colorShot
				= (::llc::WEAPON_LOAD_Ray			== shipState.WeaponManager.Weapons[shipPart.Weapon].Shot.Type) ? ::llc::rgbaf{1.0f, 0.1f, 0.0f}
				: (::llc::WEAPON_LOAD_Cannonball	== shipState.WeaponManager.Weapons[shipPart.Weapon].Shot.Type) ? ship.Team ? ::llc::rgbaf{1.0f, 0.125f, 0.25f} : ::llc::TURQUOISE
				: (::llc::WEAPON_LOAD_Bullet		== shipState.WeaponManager.Weapons[shipPart.Weapon].Shot.Type) ? ::llc::GRAY
				: ::llc::rgbaf{::llc::bgra{0xFF, 0xFF, 0xFF}}
				;
			for(uint32_t iShot = 0; iShot < shipState.WeaponManager.Shots[shipPart.Weapon].Particles.Position.size(); ++iShot) {
				lightPoints.push_back(shipState.WeaponManager.Shots[shipPart.Weapon].Particles.Position[iShot]);
				lightColors.push_back(colorShot);
			}
		}
	}
	return 0;
}

::llc::error_t			ghg::getLightArrays
	( const ::ghg::SShipManager	& shipState
	, const ::ghg::SDecoState	& decoState
	, ::llc::an3f2_t				& lightPoints
	, ::llc::a8bgra				& lightColors
	, const ::llc::vc8bgra		& debrisColors
	) {
	::ghg::getLightArraysFromShips(shipState, lightPoints, lightColors);
	::ghg::getLightArraysFromDebris(decoState, lightPoints, lightColors, debrisColors);
	return 0;
}

::llc::error_t			ghg::getLightArrays
	( const ::llc::n3f2_t		& modelPosition
	, const ::llc::vn3f2_t		& lightPointsWorld
	, const ::llc::a8bgra		& lightColorsWorld
	, ::llc::an3f2_t				& lightPointsModel
	, ::llc::a8bgra				& lightColorsModel
	) {
	::llc::clear(lightPointsModel, lightColorsModel);
	for(uint32_t iLightPoint = 0; iLightPoint < lightPointsWorld.size(); ++iLightPoint) {
		const ::llc::n3f2_t								& lightPoint		=	lightPointsWorld[iLightPoint];
		if((lightPoint - modelPosition).LengthSquared() < (MAX_LIGHT_RANGE * MAX_LIGHT_RANGE)) {
			lightPointsModel.push_back(lightPoint);
			lightColorsModel.push_back(lightColorsWorld[iLightPoint]);
		}
	}
	return 0;
}
//
//static	::llc::error_t						getLightArrays
//	( const ::llc::n3f2_t							& modelPosition
//	, const ::llc::apod<::llc::n3f2_t>			& lightPointsWorld
//	, ::llc::apod<uint16_t>							& indicesPointLights
//	) {
//	indicesPointLights.clear();
//	for(uint32_t iLightPoint = 0; iLightPoint < lightPointsWorld.size(); ++iLightPoint) {
//		const ::llc::n3f2_t								& lightPoint		=	lightPointsWorld[iLightPoint];
//		if((lightPoint - modelPosition).LengthSquared() < (MAX_LIGHT_RANGE * MAX_LIGHT_RANGE))
//			indicesPointLights.push_back((uint16_t)iLightPoint);
//	}
//	return 0;
//}

::llc::error_t			ghg::drawOrbiter
	( const ::ghg::SShipManager			& shipState
	, const uint32_t					iPartEntity
	, const ::llc::rgbaf				& shipColor
	, float								animationTime
	, const ::llc::m4f2_t				& matrixVP
	, ::llc::g8bgra						& targetPixels
	, ::llc::gu2_t						depthBuffer
	, ::ghg::SGalaxyHellDrawCache		& drawCache
	) {
	uint32_t												pixelsDrawn				= 0;
	const ::llc::vceid						  entityChildren		= shipState.Engine.Entities.GetChildren(iPartEntity);
	double													absanim					= fabsf(sinf(animationTime * 3));
	const ::llc::rgbaf								shadedColor				= (absanim < .5) ? ::llc::rgbaf{} : shipColor * (absanim * .5);
	for(uint32_t iEntity = 0; iEntity < entityChildren.size(); ++iEntity) {
		const ::llc::SRenderNode									& entityChild				= shipState.GetNode(entityChildren[iEntity]);
		if(::llc::EID_INVALID == shipState.Engine.Entities[entityChildren[iEntity]].Parent)
			continue;
		if(shipState.Engine.Scene->RenderNodes.Flags[shipState.Engine.GetRenderNode(entityChildren[iEntity])].NoDraw)
			continue;
		const ::llc::m4f2_t							& matrixTransform				= shipState.GetTransform(entityChildren[iEntity]);
		::llc::m4f2_t									matrixTransformVP			= matrixTransform * matrixVP;
		::ghg::getLightArrays(matrixTransform.GetTranslation(), drawCache.LightPointsWorld, drawCache.LightColorsWorld, drawCache.LightPointsModel, drawCache.LightColorsModel);
		::ghg::SShipGeometry mesh;
		llc_necs(shipState.GetGeometry(entityChild.Mesh, mesh));
		::llc::gc8bgra image;
		llc_necs(shipState.GetImage(shipState.Engine.Scene->Graphics->Skins[entityChild.Skin]->Textures[0], image));
		for(uint32_t iTriangle = 0; iTriangle < mesh.Triangles.size(); ++iTriangle) {
			::llc::clear(drawCache.PixelCoords, drawCache.PixelVertexWeights);
			pixelsDrawn += ::drawShipTriangle(targetPixels, mesh, iTriangle, matrixTransform, matrixTransformVP, shipState.Scene.Global.LightVector, drawCache.PixelCoords, drawCache.PixelVertexWeights, image, drawCache.LightPointsModel, drawCache.LightColorsModel, depthBuffer
				, [shadedColor](::llc::g8bgra targetPixels, const ::llc::n2s1_t & pixelCoord, const ::llc::bgra & color) {
					//if( color.r > 64
					// || color.g > 128
					//)
						targetPixels[pixelCoord.y][pixelCoord.x] = (::llc::rgbaf(color) + shadedColor).Clamp();
					//else
					//	targetPixels[pixelCoord.y][pixelCoord.x] = color; 
					return 1;
			});
		}
	}

	return pixelsDrawn;
}

static	::llc::error_t	drawShip
	( const ::ghg::SGalaxyHell		& solarSystem
	, int32_t						iShip
	, const ::llc::m4f2_t			& matrixVP
	, ::llc::g8bgra					& targetPixels
	, ::llc::gu2_t					depthBuffer
	, ::ghg::SGalaxyHellDrawCache	& drawCache
	, const ::llc::SRasterFont		& font
	) {
	const ::llc::SSpaceshipCore				& shipCore			= solarSystem.ShipState.SpaceshipManager.ShipCores[iShip];

	uint32_t							pixelsDrawn			= 0;
	const ::llc::bgra					playerColor			= ((uint32_t)iShip < solarSystem.PlayState.Constants.Players) ? ::llc::bgra(solarSystem.Pilots[iShip].Color) : ::llc::bgra(::llc::RED);
	const ::llc::vcu1_t					shipParts			= solarSystem.ShipState.SpaceshipManager.ShipParts[iShip];
	for(uint32_t iPart = 0; iPart < shipParts.size(); ++iPart) {
		const ::llc::SSpaceshipOrbiter				& shipPart			= solarSystem.ShipState.SpaceshipManager.Orbiters[shipParts[iPart]];
		if(shipPart.Health.Value <= 0)
			continue;
		pixelsDrawn += ::ghg::drawOrbiter(solarSystem.ShipState, solarSystem.ShipState.ShipPartEntity[shipParts[iPart]], playerColor, (float)solarSystem.DecoState.AnimationTime, matrixVP, targetPixels, depthBuffer, drawCache);
	}

	if(iShip >= (int32_t)solarSystem.PlayState.Constants.Players || shipCore.Team)
		return 0;

	const ::llc::m4f2_t					& matrixTransform	= solarSystem.ShipState.GetTransform(solarSystem.ShipState.ShipCoreEntity[iShip]);
	::llc::n3f2_t						starPos				= matrixTransform.GetTranslation() + ::llc::n3f2_t{0.0f, 8.0f, 0.0f};
	starPos							= matrixVP.Transform(starPos);
	starPos.x = ::llc::clamped(starPos.x, 20.0f, targetPixels.metrics().x - 20.0f);
	starPos.y = ::llc::clamped(starPos.y, 20.0f, targetPixels.metrics().y - 20.0f);
	const ::llc::n2<int32_t>			pixelCoord			= {(int32_t)starPos.x, (int32_t)starPos.y};

	const ::llc::vcs					finalText			= solarSystem.Pilots[iShip].Name;
	::llc::rect2s1_t						rectText			= {{}, {int16_t(font.CharSize.x * finalText.size()), font.CharSize.y}};
	rectText.Offset = (pixelCoord - ::llc::n2s2_t{(rectText.Size.x >> 1), (rectText.Size.y >> 1)}).s1();

	::llc::an2u1_t						dstCoords;
	llc_necs(::llc::textLineRaster(targetPixels.metrics(), font.CharSize, rectText, font.Texture, finalText, dstCoords));
	for(uint32_t iCoord = 0; iCoord < dstCoords.size(); ++iCoord) {
		const ::llc::n2u1_t					dstCoord			= dstCoords[iCoord];
		if(::llc::in_range(dstCoord, {{}, targetPixels.metrics().u1()})) {
			::llc::setPixel(targetPixels, dstCoord.s1(), (shipCore.Health > 0) ? playerColor : ::llc::bgra(::llc::interpolate_linear(::llc::rgbaf(playerColor), ::llc::rgbaf(targetPixels[dstCoord.y][dstCoord.x]), .8f)));
		}
	}
	return pixelsDrawn;
}

static	::llc::error_t	drawExplosion
	( const ::ghg::SGalaxyHell		& solarSystem
	, const ::ghg::SExplosion		& explosion
	, const ::llc::m4f2_t			& matrixView
	, ::llc::g8bgra					& targetPixels
	, ::llc::gu2_t					depthBuffer
	, ::ghg::SGalaxyHellDrawCache	& drawCache
	) {
	::llc::gc8bgra image;
	llc_necs(solarSystem.ShipState.GetImage(solarSystem.ShipState.Engine.Scene->Graphics->Skins[explosion.IndexImage]->Textures[0], image));
	::ghg::SShipGeometry mesh;
	llc_necs(solarSystem.ShipState.GetGeometry(explosion.IndexMesh, mesh));
	for(uint32_t iExplosionPart = 0; iExplosionPart < explosion.Particles.Position.size(); ++iExplosionPart) {
		const ::llc::rangeu1_t				& sliceMesh			= explosion.Slices[iExplosionPart];
		::llc::m4f2_t						matrixPart			= {};
		matrixPart.Identity();
		if(iExplosionPart % 5) matrixPart.RotationY(solarSystem.DecoState.AnimationTime * 2);
		if(iExplosionPart % 3) matrixPart.RotationX(solarSystem.DecoState.AnimationTime * 2);
		if(iExplosionPart % 2) matrixPart.RotationZ(solarSystem.DecoState.AnimationTime * 2);
		matrixPart.SetTranslation(explosion.Particles.Position[iExplosionPart], false);
		::llc::m4f2_t						matrixTransformView	= matrixPart * matrixView;
		::ghg::getLightArrays(matrixPart.GetTranslation(), drawCache.LightPointsWorld, drawCache.LightColorsWorld, drawCache.LightPointsModel, drawCache.LightColorsModel);
		for(uint32_t iTriangle = 0, countTriangles = sliceMesh.Count; iTriangle < countTriangles; ++iTriangle) {
			drawCache.PixelCoords			.clear();
			drawCache.PixelVertexWeights	.clear();
			const uint32_t						iActualTriangle		= sliceMesh.Offset + iTriangle;
			::llc::tri3f2_t						triangleWorld		= mesh.Triangles	[iActualTriangle];
			::llc::n3f2_t						normal				= mesh.Normals		[iActualTriangle * 3];
			::llc::tri2f2_t						triangleTexCoords	= mesh.TextureCoords[iActualTriangle];
			::llc::tri3f2_t						triangleScreen		= triangleWorld;
			::llc::transform(triangleScreen, matrixTransformView);
			if(triangleScreen.ClipZ())
				continue;
			if(triangleScreen.A.z <= 0 || triangleScreen.B.z <= 0 || triangleScreen.C.z <= 0)
				continue;

			::llc::transform(triangleWorld, matrixPart);
			normal							= matrixPart.TransformDirection(normal).Normalize();
			::llc::drawQuadTriangle(targetPixels.metrics(), triangleScreen, drawCache.PixelCoords, drawCache.PixelVertexWeights, depthBuffer);
 			::llc::drawPixels(targetPixels, triangleWorld, normal, triangleTexCoords, solarSystem.ShipState.Scene.Global.LightVector, drawCache.PixelCoords, drawCache.PixelVertexWeights, image, drawCache.LightPointsModel, drawCache.LightColorsModel);
			drawCache.PixelCoords			.clear();
			drawCache.PixelVertexWeights	.clear();
			triangleScreen				= {triangleScreen.A, triangleScreen.C, triangleScreen.B};
			triangleWorld					= {triangleWorld.A, triangleWorld.C, triangleWorld.B};
			triangleTexCoords				= {triangleTexCoords.A, triangleTexCoords.C, triangleTexCoords.B};
			normal							*= -1;
			::llc::drawQuadTriangle(targetPixels.metrics(), triangleScreen, drawCache.PixelCoords, drawCache.PixelVertexWeights, depthBuffer);
  			::llc::drawPixels(targetPixels, triangleWorld, normal, triangleTexCoords, solarSystem.ShipState.Scene.Global.LightVector, drawCache.PixelCoords, drawCache.PixelVertexWeights, image, drawCache.LightPointsModel, drawCache.LightColorsModel);
		}
	}
	return 0;
}

::llc::error_t			ghg::solarSystemDraw		(const ::ghg::SGalaxyHell & solarSystem, ::ghg::SGalaxyHellDrawCache & drawCache, ::std::mutex & mutexUpdate)	{
	drawCache.RenderTarget->resize(drawCache.RenderTargetMetrics);
	::llc::pobj<::ghg::TRenderTarget>	renderTarget				= drawCache.RenderTarget;
	::llc::g8bgra			targetPixels				= renderTarget->Color			; 
	::llc::grid<uint32_t>				depthBuffer					= renderTarget->DepthStencil	;

	// ------------------------------------------- Transform and Draw
	if(0 == targetPixels.size())
		return 1;

	memset(targetPixels.begin(), 0, targetPixels.byte_count());
	memset(depthBuffer.begin(), -1, depthBuffer.byte_count());
	{
		::std::lock_guard			lockUpdate					(mutexUpdate);
		::drawStars(solarSystem.DecoState.Stars, targetPixels);
	}

	::llc::m4f2_t				matrixView					= {};
	const ::llc::SCameraPoints	& camera					= solarSystem.ShipState.Scene.Global.Camera[solarSystem.ShipState.Scene.Global.CameraMode];
	::llc::n3f2_t				cameraUp					= {0, 1, 0};
	matrixView.LookAt(camera.Position, camera.Target, cameraUp);
	matrixView				*= solarSystem.ShipState.Scene.Global.MatrixProjection;
	drawCache.LightPointsWorld.clear();
	drawCache.LightColorsWorld.clear();
	{
		::std::lock_guard		lockUpdate					(mutexUpdate);
		::ghg::getLightArrays(solarSystem.ShipState, solarSystem.DecoState, drawCache.LightPointsWorld, drawCache.LightColorsWorld, ::ghg::DEBRIS_COLORS);
	}
	drawCache.LightPointsModel.reserve(drawCache.LightPointsWorld.size());
	drawCache.LightColorsModel.reserve(drawCache.LightColorsWorld.size());
	{
		::std::lock_guard		lockUpdate					(mutexUpdate);
		for(uint32_t iShip = 0; iShip < solarSystem.ShipState.SpaceshipManager.ShipCores.size(); ++iShip) {
			const ::llc::SSpaceshipCore	& ship						= solarSystem.ShipState.SpaceshipManager.ShipCores[iShip];
			if(ship.Health <= 0 && ship.Team)
				continue;

			::drawShip(solarSystem, iShip, matrixView, targetPixels, depthBuffer, drawCache, *solarSystem.DecoState.FontManager.Fonts[10]);
		}
	}

	{
		::std::lock_guard		lockUpdate					(mutexUpdate);
		for(uint32_t iExplosion = 0; iExplosion < solarSystem.DecoState.Explosions.size(); ++iExplosion) {
			const ::ghg::SExplosion	& explosion					= solarSystem.DecoState.Explosions[iExplosion];
			if(0 == explosion.Slices.size())
				continue;
			::drawExplosion(solarSystem, explosion, matrixView, targetPixels, depthBuffer, drawCache);
		}
	}
#pragma pack(push, 1)
	struct SRenderNode	{
		uint32_t			Mesh				;
		int32_t				Image				;
		::llc::sliceu2_t	Slice				;
		::llc::au2_t		IndicesPointLight	;
	};
#pragma pack(pop)
	{
		::std::lock_guard		lockUpdate			(mutexUpdate);
		for(uint32_t iShip = 0; iShip < solarSystem.ShipState.SpaceshipManager.ShipCores.size(); ++iShip) {
			const ::llc::SSpaceshipCore	& shipCore			= solarSystem.ShipState.SpaceshipManager.ShipCores[iShip];
			const ::llc::au1_t		& shipParts			= solarSystem.ShipState.SpaceshipManager.ShipParts[iShip];
			for(uint32_t iPart = 0; iPart < shipParts.size(); ++iPart) {
				const ::llc::SSpaceshipOrbiter	& orbiter			= solarSystem.ShipState.SpaceshipManager.Orbiters[shipParts[iPart]];
				const ::llc::SWeapon	& weapon			= solarSystem.ShipState.WeaponManager.Weapons[orbiter.Weapon];
				::drawShots(targetPixels, solarSystem.ShipState, solarSystem.ShipState.WeaponManager.Shots[orbiter.Weapon], matrixView, (float)solarSystem.DecoState.AnimationTime, shipCore, weapon, depthBuffer, drawCache.LightPointsModel, drawCache);

			}
		}
	}
	{
		::std::lock_guard		lockUpdate			(mutexUpdate);
		::drawDebris(targetPixels, solarSystem.DecoState.Debris, matrixView, depthBuffer, ::ghg::DEBRIS_COLORS);
	}
	{
		::std::lock_guard		lockUpdate			(mutexUpdate);
		::drawScoreParticles(targetPixels, solarSystem.DecoState.ScoreParticles, matrixView, depthBuffer, *solarSystem.DecoState.FontManager.Fonts[8]);
	}
	return 0;
}
