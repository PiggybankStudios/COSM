/*
File:   map_view.c
Author: Taylor Robbins
Date:   08\23\2025
Description: 
	** Holds code that handles moving a view around a mercator (or other) projection map
	** The API largely serves the map rendering code in converting to and from various
	** coordinate spaces and deciding what needs to get rendered and where on screen.
*/

void InitMapView(MapView* view, MapProjection projection)
{
	view->projection = projection;
	view->mapRec = NewRecd(0, 0, MERCATOR_MAP_ASPECT_RATIO, 1.0);
	view->position = NewV2d(view->mapRec.X + view->mapRec.Width/2.0, view->mapRec.Y + view->mapRec.Height/2.0);
	view->zoom = 0.0; //this will get set to something reasonable after our first UI layout
}

void UpdateMapView(MapView* view, MouseState* mouse, KeyboardState* keyboard)
{
	// +==============================+
	// |  Scroll Wheel Zooms In/Out   |
	// +==============================+
	if (mouse->scrollDelta.Y != 0 && view->zoom != 0.0 && !IsKeyboardKeyDown(keyboard, Key_Control))
	{
		view->zoom *= 1.0 + (mouse->scrollDelta.Y * 0.1);
		if (IsInfiniteOrNanR64(view->zoom)) { view->zoom = view->minZoom; }
		view->zoom = ClampR64(view->zoom, view->minZoom, MAP_MAX_ZOOM);
	}
	
	// +==============================+
	// |       WASD Moves View        |
	// +==============================+
	r64 viewSpeed = IsKeyboardKeyDown(keyboard, Key_Shift) ? 20.0 : 8.0;
	if (IsKeyboardKeyDown(keyboard, Key_W) && view->zoom != 0.0)
	{
		view->position.Y -= viewSpeed / view->zoom;
	}
	if (IsKeyboardKeyDown(keyboard, Key_A) && view->zoom != 0.0)
	{
		view->position.X -= viewSpeed / view->zoom;
	}
	if (IsKeyboardKeyDown(keyboard, Key_S) && view->zoom != 0.0)
	{
		view->position.Y += viewSpeed / view->zoom;
	}
	if (IsKeyboardKeyDown(keyboard, Key_D) && view->zoom != 0.0)
	{
		view->position.X += viewSpeed / view->zoom;
	}
}
