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
	view->mapRec = MakeRecd(0, 0, MERCATOR_MAP_ASPECT_RATIO, 1.0);
	view->position = MakeV2d(view->mapRec.x + view->mapRec.width/2.0, view->mapRec.y + view->mapRec.height/2.0);
	view->zoom = 0.0; //this will get set to something reasonable after our first UI layout
}

void UpdateMapView(MapView* view, bool isMouseOverMainViewport, MouseState* mouse, KeyboardState* keyboard)
{
	// +==============================+
	// |  Scroll Wheel Zooms In/Out   |
	// +==============================+
	if (mouse->scrollDelta.y != 0 && view->zoom != 0.0 && !IsKeyboardKeyDown(keyboard, nullptr, Key_Control) && isMouseOverMainViewport)
	{
		view->zoom *= 1.0 + (mouse->scrollDelta.y * 0.1);
		if (IsInfiniteOrNanR64(view->zoom)) { view->zoom = view->minZoom; }
		view->zoom = ClampR64(view->zoom, view->minZoom, MAP_MAX_ZOOM);
	}
	
	// +==============================+
	// |       WASD Moves View        |
	// +==============================+
	r64 viewSpeed = IsKeyboardKeyDown(keyboard, nullptr, Key_Shift) ? 20.0 : 8.0;
	if (IsKeyboardKeyDown(keyboard, nullptr, Key_W) && view->zoom != 0.0)
	{
		view->position.y -= viewSpeed / view->zoom;
	}
	if (IsKeyboardKeyDown(keyboard, nullptr, Key_A) && view->zoom != 0.0)
	{
		view->position.x -= viewSpeed / view->zoom;
	}
	if (IsKeyboardKeyDown(keyboard, nullptr, Key_S) && view->zoom != 0.0)
	{
		view->position.y += viewSpeed / view->zoom;
	}
	if (IsKeyboardKeyDown(keyboard, nullptr, Key_D) && view->zoom != 0.0)
	{
		view->position.x += viewSpeed / view->zoom;
	}
	
	// +==============================+
	// |    Middle Mouse Drag Pans    |
	// +==============================+
	rec mainViewportRec = GetClayElementDrawRecNt("MainViewport");
	if (mainViewportRec.width > 0 && mainViewportRec.height > 0)
	{
		recd screenMapRec = view->mapRec;
		screenMapRec.topLeft = SubV2d(screenMapRec.topLeft, view->position);
		screenMapRec = ScaleRecd(screenMapRec, view->zoom);
		screenMapRec.topLeft = AddV2d(screenMapRec.topLeft, AddV2d(ToV2dFromf(mainViewportRec.topLeft), ToV2dFromf(ShrinkV2(mainViewportRec.size, 2.0f))));
		
		if (!view->isDragPanning && isMouseOverMainViewport && IsMouseBtnPressed(mouse, nullptr, MouseBtn_Middle))
		{
			view->isDragPanning = true;
			view->dragPanningPos = DivV2d(SubV2d(ToV2dFromf(mouse->position), screenMapRec.topLeft), screenMapRec.size);
		}
		if (view->isDragPanning)
		{
			if (IsMouseBtnDown(mouse, nullptr, MouseBtn_Middle))
			{
				v2 screenCenter = AddV2(mainViewportRec.topLeft, ShrinkV2(mainViewportRec.size, 2.0f));
				view->position.x = (screenCenter.x - (mouse->position.x - view->dragPanningPos.x * screenMapRec.width)) / view->zoom;
				view->position.y = (screenCenter.y - (mouse->position.y - view->dragPanningPos.y * screenMapRec.height)) / view->zoom;
			}
			else { view->isDragPanning = false; }
		}
	}
}
