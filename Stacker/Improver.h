#pragma once

#include <QTime>

#include "HotSpot.h"
#include "ShapeState.h"

class Offset;
class QSegMesh;
class Controller;
class Propagator;

#define IMPROVER_MAGIC_NUMBER -99999

class Improver : public QObject
{
	Q_OBJECT

public:
	Improver(Offset *offset);

	// Parameters
	int NUM_EXPECTED_SOLUTION;
	double BB_TOLERANCE;
	double TARGET_STACKABILITY;
	int LOCAL_RADIUS;

	// Execute improving
	void execute(int level = IMPROVER_MAGIC_NUMBER);

private:
	void setPositionalConstriants( HotSpot& fixedHS );
	bool satisfyBBConstraint();
	bool isUnique( ShapeState state, double threshold );
	void recordSolution(Point handleCenter, Vec3d localMove);

	QVector<Vec3d> getLocalMoves( HotSpot& HS );
	QVector<double> getLocalScales( HotSpot& HS );
	void deformNearPointLineHotspot( int side );
	void deformNearRingHotspot( int side );
	void deformNearHotspot( int side );

public:
	// Best first Searching
	double origStackability;
	Vec3d constraint_bbmin, constraint_bbmax;
	ShapeState currentCandidate;
	PQShapeStateLessEnergy candidateSolutions;
	QVector<ShapeState> usedCandidateSolutions;
	QVector<ShapeState> solutions;

private:
	Offset* activeOffset;
	QSegMesh* activeObject();
	Controller* ctrl();

	QTime timer;

public slots:
	void setTargetStackability(double s);
	void setBBTolerance(double tol);
	void setNumExpectedSolutions(int num);
	void setLocalRadius(int R);

signals:
	void printMessage( QString );
};