#pragma once

#include "Group.h"

class PointJointGroup : public Group{

public:
	PointJointGroup(GroupType newType);

	// Inherited methods
	void process(QVector< Primitive* > segments);
	void regroup();
	void draw();	
	void saveParameters( std::ofstream &outF );
	void loadParameters( std::ifstream &inF, Vec3d translation, double scaleFactor );
	Group* clone();

	// Get
	Point getJointPosOnPrimitive(Primitive* prim);
	Point getJointPos();

	// Sliding
	void slide(QString sliderID);
	void rejoint(QString fixedID);

public:
	Point pos; // Only used once at the very beginning for \process
    QMap<QString, std::vector<double> > jointCoords;
};
