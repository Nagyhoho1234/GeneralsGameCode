/*
**	Command & Conquer Generals Zero Hour(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

////////////////////////////////////////////////////////////////////////////////
//																																						//
//  (c) 2001-2003 Electronic Arts Inc.																				//
//																																						//
////////////////////////////////////////////////////////////////////////////////

// View.cpp ///////////////////////////////////////////////////////////////////
// A "view", or window, into the World
// Author: Michael S. Booth, February 2001

#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#include "Common/GameEngine.h"
#include "Common/Xfer.h"
#include "GameClient/Display.h"
#include "GameClient/Drawable.h"
#include "GameClient/GameClient.h"
#include "GameClient/View.h"

UnsignedInt View::m_idNext = 1;

// the tactical view singleton
View *TheTacticalView = nullptr;


View::View()
{
	m_userControlLockedUntilFrame = 0u;
	m_isUserControlled = true;
	m_currentHeightAboveGround = 0.0f;
	m_defaultAngle = 0.0f;
	m_defaultPitch = 0.0f;
	m_heightAboveGround = 0.0f;
	m_lockDist = 0.0f;
	m_maxHeightAboveGround = 0.0f;
	m_minHeightAboveGround = 0.0f;
	m_next = nullptr;
	m_okToAdjustHeight = TRUE;
	m_originX = 0;
	m_originY = 0;
	m_snapImmediate = FALSE;
	m_terrainHeightAtPivot = 0.0f;
	m_zoom = 0.0f;
	m_pos.zero();
	m_width = 0;
	m_height = 0;
	m_angle = 0.0f;
	m_pitch = 0.0f;
	m_cameraLock = INVALID_ID;
	m_cameraLockDrawable = nullptr;
	m_zoomLimited = TRUE;

	// create unique view ID
	m_id = m_idNext++;

	// default field of view
	m_FOV = DEG_TO_RADF(50.0f);

	m_mouseLocked = FALSE;

	m_guardBandBias.x = 0.0f;
	m_guardBandBias.y = 0.0f;
}

View::~View()
{
}

void View::init()
{
	m_width = DEFAULT_VIEW_WIDTH;
	m_height = DEFAULT_VIEW_HEIGHT;
	m_originX = DEFAULT_VIEW_ORIGIN_X;
	m_originY = DEFAULT_VIEW_ORIGIN_Y;
	m_pos.zero();
	m_angle = 0.0f;
	m_cameraLock = INVALID_ID;
	m_cameraLockDrawable = nullptr;
	m_zoomLimited = TRUE;

	m_zoom = 1.0f;
	// TheSuperHackers @bugfix ZsoltFeher 18/07/2026 Scale the configured camera heights with the
	// display aspect ratio. See scaleCameraHeightForAspectRatio() and GitHub issue #78.
	m_maxHeightAboveGround = scaleCameraHeightForAspectRatio(TheGlobalData->m_maxCameraHeight);
	m_minHeightAboveGround = scaleCameraHeightForAspectRatio(TheGlobalData->m_minCameraHeight);
	m_okToAdjustHeight = FALSE;

	m_defaultAngle = DEG_TO_RADF(TheGlobalData->m_cameraYaw);
	m_defaultPitch = DEG_TO_RADF(TheGlobalData->m_cameraPitch);
	m_angle = m_defaultAngle;
	m_pitch = m_defaultPitch;
}

// TheSuperHackers @bugfix ZsoltFeher 18/07/2026 Scales a configured camera height with the display
// aspect ratio. See GitHub issue #78: MaxCameraHeight/MinCameraHeight in GameData.ini are tuned
// for 4:3 displays. A wider display shows more horizontal terrain at a given camera height than
// 4:3 did, so unscaled heights bring the camera far too close to the ground on widescreen.
// The formula is GenTool's community-established aspect-ratio scaling, adapted from its original
// memory-patch approach on the retail executable to this source reimplementation:
// - 4:3 (and narrower) is the baseline and stays unscaled.
// - Between 4:3 and 16:9, heights scale up by (aspect - 4/3 + 1), softened by a small "nerf"
//   factor of (1 - (aspect - 4/3) / 12). At 16:9 this yields a factor of ~1.39.
// - The scaling factor is clamped at 16:9 so ultrawide monitors do not get an ever-increasing
//   camera height.
// This is applied at the few places that read the configured heights (view initialization, map
// default view setup, camera boom offset), not per frame in any hot path. Reading the live
// display size on each call keeps the values correct if the display mode changes at runtime.
// Note: GenTool also force-enabled DrawEntireTerrain when raising the camera. That is not needed
// here, because the terrain draw window is already enlarged (see NORMAL_DRAW_WIDTH/HEIGHT in
// WorldHeightMap.h) with plenty of headroom for this modest, at most ~1.39x height increase,
// whereas DrawEntireTerrain would rebuild and render the whole map with a heavy performance cost.
Real View::scaleCameraHeightForAspectRatio( Real height )
{
	if (TheDisplay == nullptr)
		return height;

	const Int screenWidth = (Int)TheDisplay->getWidth();
	const Int screenHeight = (Int)TheDisplay->getHeight();

	// The scaling is designed for resolutions wider than 4:3 and at least 640x480.
	if (screenWidth < 640 || screenHeight < 480)
		return height;

	const Real aspect_4_3 = 4.0f / 3.0f;
	const Real aspect_16_9 = 16.0f / 9.0f;
	Real aspect = (Real)screenWidth / (Real)screenHeight;

	if (aspect <= aspect_4_3)
		return height;

	if (aspect > aspect_16_9)
		aspect = aspect_16_9; // clamp the scaling factor at 16:9, do not scale further for ultrawide

	const Real multi = aspect - aspect_4_3 + 1.0f;
	const Real nerf = 1.0f - (aspect - aspect_4_3) / 12.0f;
	return height * multi * nerf;
}

void View::reset()
{
	// Only fixing the reported bug.  Who knows what side effects resetting the rest could have.
	m_zoomLimited = TRUE;

	m_userControlLockedUntilFrame = 0u;
	m_isUserControlled = true;
}

/**
 * Prepend this view to the given list, return the new list.
 */
View *View::prependViewToList( View *list )
{
	m_next = list;
	return this;
}

void View::zoom( Real height )
{
	setHeightAboveGround(getHeightAboveGround() + height);
}

/**
 * Center the view on the given coordinate.
 */
void View::lookAt( const Coord3D *o )
{
	/// @todo this needs to be changed to be 3D, this is still old 2D stuff
	Coord2D pos = getPosition2D();
	pos.x = o->x - m_width * 0.5f;
	pos.y = o->y - m_height * 0.5f;
	setPosition2D(pos);
}

/**
 * Shift the view by the given delta.
 */
void View::scrollBy( const Coord2D *delta )
{
	// update view's world position
	m_pos.x += delta->x;
	m_pos.y += delta->y;
}

/**
 * Rotate the view around the vertical axis to the given angle.
 */
void View::setAngle( Real radians )
{
	m_angle = WWMath::Normalize_Angle(radians);
}

#define CLAMP_VIEW_PITCH 1
/**
 * Rotate the view around the horizontal (X) axis to the given angle.
 */
void View::setPitch( Real radians )
{
#if CLAMP_VIEW_PITCH
	m_pitch = clamp(DEG_TO_RADF(0.1f), radians, DEG_TO_RADF(89.9f));
#else
	m_pitch = WWMath::Normalize_Angle(radians);
#endif
}

void View::setDefaultPitch( Real radians )
{
#if CLAMP_VIEW_PITCH
	m_defaultPitch = clamp(DEG_TO_RADF(0.1f), radians, DEG_TO_RADF(89.9f));
#else
	m_defaultPitch = WWMath::Normalize_Angle(radians);
#endif
}

/**
 * Set the view angle back to default
 */
void View::setAngleToDefault()
{
	m_angle = m_defaultAngle;
}

/**
 * Set the view pitch back to default
 */
void View::setPitchToDefault()
{
	m_pitch = m_defaultPitch;
}

void View::setHeightAboveGround(Real z)
{
	// if our zoom is limited, we will stay within a predefined distance from the terrain
	if( m_zoomLimited )
	{
		m_heightAboveGround = clamp(m_minHeightAboveGround, z, m_maxHeightAboveGround);
	}
	else
	{
		m_heightAboveGround = z;
	}
}

/**
 * write the view's current location in to the view location object
 */
void View::getLocation( ViewLocation *location )
{
	location->init( getPosition(), getAngle(), getPitch(), getZoom() );
}


/**
 * set the view's current location from to the view location object
 */
void View::setLocation( const ViewLocation *location )
{
	if ( location->isValid() )
	{
		setPosition(location->getPosition());
		setAngle(location->getAngle());
		setPitch(location->getPitch());
		setZoom(location->getZoom());
	}

}

Bool View::isUserControlLocked() const
{
	return m_userControlLockedUntilFrame > TheGameClient->getFrame();
}

//-------------------------------------------------------------------------------------------------
/** project the 4 corners of this view into the world and return each point as a parameter,
		the world points are at the requested Z */
//-------------------------------------------------------------------------------------------------
PlaneClass::IntersectionResType View::getScreenCornerWorldPointsAtZ( Coord3D *topLeft, Coord3D *topRight,
																					Coord3D *bottomRight, Coord3D *bottomLeft,
																					Real z, ViewportClass viewPort )
{
	if( topLeft == nullptr || topRight == nullptr || bottomRight == nullptr || bottomLeft == nullptr)
		return PlaneClass::NO_INTERSECTION;

	ICoord2D screenTopLeft;
	ICoord2D screenTopRight;
	ICoord2D screenBottomRight;
	ICoord2D screenBottomLeft;
	ICoord2D origin;
	const Int viewWidth = getWidth();
	const Int viewHeight = getHeight();

	// setup the screen coords for the 4 corners of the viewable display
	getOrigin( &origin.x, &origin.y );

	screenTopLeft.x = origin.x + viewWidth * viewPort.Min.X;
	screenTopLeft.y = origin.y + viewHeight * viewPort.Min.Y;
	screenTopRight.x = origin.x + viewWidth * viewPort.Max.X;
	screenTopRight.y = origin.y + viewHeight * viewPort.Min.Y;
	screenBottomRight.x = origin.x + viewWidth * viewPort.Max.X;
	screenBottomRight.y = origin.y + viewHeight * viewPort.Max.Y;
	screenBottomLeft.x = origin.x + viewWidth * viewPort.Min.X;
	screenBottomLeft.y = origin.y + viewHeight * viewPort.Max.Y;

	PlaneClass::IntersectionResType combinedResult = PlaneClass::INSIDE_SEGMENT;
	PlaneClass::IntersectionResType individualResults[4];
	individualResults[0] = screenToWorldAtZ( &screenTopLeft, topLeft, z );
	individualResults[1] = screenToWorldAtZ( &screenTopRight, topRight, z );
	individualResults[2] = screenToWorldAtZ( &screenBottomRight, bottomRight, z );
	individualResults[3] = screenToWorldAtZ( &screenBottomLeft, bottomLeft, z );

	for( Int i = 0; i < 4; ++i )
	{
		if( individualResults[i] == PlaneClass::NO_INTERSECTION )
		{
			combinedResult = PlaneClass::NO_INTERSECTION;
			break;
		}
		if( individualResults[i] == PlaneClass::OUTSIDE_LINE )
		{
			combinedResult = PlaneClass::OUTSIDE_LINE;
		}
	}

	return combinedResult;
}

// ------------------------------------------------------------------------------------------------
/** Xfer method for a view */
// ------------------------------------------------------------------------------------------------
void View::xfer( Xfer *xfer )
{

	// version
	XferVersion currentVersion = 1;
	XferVersion version = currentVersion;
	xfer->xferVersion( &version, currentVersion );

	// camera angle
	Real angle = getAngle();
	xfer->xferReal( &angle );
	setAngle( angle );

	// view position
	Coord3D viewPos = getPosition();
	xfer->xferReal( &viewPos.x );
	xfer->xferReal( &viewPos.y );
	xfer->xferReal( &viewPos.z );
	lookAt( &viewPos );

}
