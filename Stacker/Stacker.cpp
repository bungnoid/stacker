#include "Stacker.h"

Stacker::Stacker()
{
	this->activeScene = NULL;
}

void Stacker::computeOffset()
{
	if(!activeScene) return;

	Offset offset(activeScene);
	offset.run();

	// update colors
	activeScene->updateActiveObject();
}

void Stacker::setScene( Scene * newScene )
{
	this->activeScene = newScene;
}

Scene * Stacker::scene()
{
	return activeScene;
}



