#include "Cuboid.h"
#include "SimpleDraw.h"

#include <Eigen/Geometry>
using namespace Eigen;

#define RADIANS(deg)    ((deg)/180.0 * M_PI)
#define DEGREES(rad)    ((rad)/M_PI * 180.0)

// Rodrigues' rotation
#define ROTATE_VEC(v, theta, axis) (v = v * cos(theta) + cross(axis, v) * sin(theta) + axis * dot(axis, v) * (1 - cos(theta)))

Cuboid::Cuboid(QSurfaceMesh* mesh)
	: Primitive(mesh)
{
	fit();

	selectedPartId = -1;

}

void Cuboid::fit()
{	
	MinOBB3 obb(m_mesh);
	originalBox = currBox = obb.mMinBox;

	// Compute the OBB coordinates for all vertices
	coordinates.clear();
	Surface_mesh::Vertex_property<Point> points = m_mesh->vertex_property<Point>("v:point");
	Surface_mesh::Vertex_iterator vit, vend = m_mesh->vertices_end();

	for (vit = m_mesh->vertices_begin(); vit != vend; ++vit)
	{
		Vector3 coord = getCoordinatesInBox(originalBox, points[vit]);
		coordinates.push_back(coord);
	}
}

void Cuboid::deformMesh()
{
	Surface_mesh::Vertex_property<Point> points = m_mesh->vertex_property<Point>("v:point");
	Surface_mesh::Vertex_iterator vit, vend = m_mesh->vertices_end();

	for (vit = m_mesh->vertices_begin(); vit != vend; ++vit)
	{
		int vidx = ((Surface_mesh::Vertex)vit).idx();
		points[vit] = getPositionInBox(currBox, coordinates[vidx]);
	}
}

Vector3 Cuboid::getCoordinatesInBox( MinOBB3::Box3 &box, Vector3 &p )
{
	Vector3 local_p = p - box.Center;

	return Vector3( dot(local_p, box.Axis[0]) / box.Extent[0],
		dot(local_p, box.Axis[1]) / box.Extent[1],
		dot(local_p, box.Axis[2]) / box.Extent[2]);
}

Vector3 Cuboid::getPositionInBox( MinOBB3::Box3 &box, Vector3 &coord )
{
	Vector3 local_p = box.Extent[0] * coord[0] * box.Axis[0]
	+ box.Extent[1] * coord[1] * box.Axis[1]
	+ box.Extent[2] * coord[2] * box.Axis[2];

	return local_p + box.Center;
}

std::vector<Vector3> Cuboid::getBoxConners( MinOBB3::Box3 box )
{
	std::vector<Vector3> pnts(8);

	// Create right-hand system
	if ( dot(cross(box.Axis[0], box.Axis[1]), box.Axis[2]) < 0 ) 
	{
		box.Axis[2]  = -box.Axis[2];
	}

	std::vector<Vector3> Axis;
	for (int i=0;i<3;i++)
	{
		Axis.push_back( 2 * box.Extent[i] * box.Axis[i]);
	}

	pnts[0] = box.Center - 0.5*Axis[0] - 0.5*Axis[1] + 0.5*Axis[2];
	pnts[1] = pnts[0] + Axis[0];
	pnts[2] = pnts[1] - Axis[2];
	pnts[3] = pnts[2] - Axis[0];

	pnts[4] = pnts[0] + Axis[1];
	pnts[5] = pnts[1] + Axis[1];
	pnts[6] = pnts[2] + Axis[1];
	pnts[7] = pnts[3] + Axis[1];

	return pnts;
}

std::vector< std::vector<Vector3> > Cuboid::getBoxFaces(MinOBB3::Box3 fromBox)
{
	std::vector< std::vector<Vector3> > faces(6);
	std::vector<Vector3> pnts = getBoxConners(fromBox);

	uint ids[6][4] = {1, 2, 6, 5,
					  0, 4, 7, 3,
					  4, 5, 6, 7,
					  0, 3, 2, 1,
					  0, 1, 5, 4,
					  2, 3, 7, 6};

	for (int i = 0; i < 6; i++)	{
		for (int j = 0; j < 4; j++)	{
			faces[i].push_back( pnts[ ids[i][j] ] );
		}
	}

	return faces;
}

void Cuboid::draw()
{
	// Draw center point
	//SimpleDraw::IdentifyPoint(currBox.Center);

	if(isSelected)
		drawCube(5, Vec4d(1,1,0,1));
	else
		drawCube(2, Vec4d(0,0,1,1));
}

void Cuboid::drawCube(double lineWidth, Vec4d color, bool isOpaque)
{
	std::vector< std::vector<Vector3> > faces = getBoxFaces(currBox);

	if(selectedPartId >= 0)
	{
		SimpleDraw::DrawSquare(faces[this->selectedPartId], false, 6, Vec4d(0,1,0,1));
		//SimpleDraw::IdentifyPoint(selectedPartPos(), 0,1,0,20);
	}

	for(int i = 0; i < faces.size(); i++)
		SimpleDraw::DrawSquare(faces[i], isOpaque, lineWidth, color);
}

void Cuboid::drawNames(bool isDrawParts)
{
	if(isDrawParts)
	{
		int faceId = 0;

		std::vector< std::vector<Vector3> > faces = getBoxFaces(currBox);

		for(int i = 0; i < faces.size(); i++)
		{
			glPushName(faceId++);
			SimpleDraw::DrawSquare(faces[i]);
			glPopName();
		}

	}
	else
	{
		glPushName(this->id);
		drawCube(1,Vec4d(1,1,1,1), true);
		glPopName();
	}
}

Eigen::Vector3d Cuboid::V2E( Vector3 &vec )
{
	return Eigen::Vector3d(vec[0], vec[1], vec[2]);
}

Vector3 Cuboid::E2V( Eigen::Vector3d &vec )
{
	return Vector3(vec[0], vec[1], vec[2]);
}

void Cuboid::scaleAlongAxis( Vector3 &scales )
{
	currBox.Extent[0] *= scales[0];
	currBox.Extent[1] *= scales[1];
	currBox.Extent[2] *= scales[2];
}

void Cuboid::translate( Vector3 &T )
{
	currBox.Center += T;
}


Eigen::Matrix3d Cuboid::rotationMatrixAroundAxis( int axisId, double theta )
{
	theta = M_PI * theta / 180;
	Vector3 u = currBox.Axis[axisId];
	double x = u[0], y = u[1], z = u[2];

	Eigen::Matrix3d I, cpm, tp, R;

	I = Eigen::Matrix3d::Identity(3,3);

	tp <<	x*x, x*y, x*z,
			x*y, y*y, y*z,
			x*z, y*z, z*z;

	cpm <<  0, -z,  y,
			z,  0, -x,
		   -y,  x,  0;

	R = cos(theta)*I + sin(theta)*cpm + (1-cos(theta))*tp;

	return R;
}


void Cuboid::rotateAroundAxes( Vector3 &angles )
{
	Eigen::Matrix3d Rx = rotationMatrixAroundAxis(0, angles[0]);
	Eigen::Matrix3d Ry = rotationMatrixAroundAxis(1, angles[1]);
	Eigen::Matrix3d Rz = rotationMatrixAroundAxis(2, angles[2]);
	Eigen::Matrix3d R = Rx * Ry * Rz;

	Eigen::Vector3d p0 = R * V2E(currBox.Axis[0]);
	Eigen::Vector3d p1 = R * V2E(currBox.Axis[1]);
	Eigen::Vector3d p2 = R * V2E(currBox.Axis[2]);

	currBox.Axis[0] = E2V(p0);
	currBox.Axis[1] = E2V(p1);
	currBox.Axis[2] = E2V(p2);
}

void Cuboid::deform( PrimitiveParam* params, bool isPermanent /*= false*/ )
{
	CuboidParam* cp = (CuboidParam*) params;

	// Deform the OBB
	translate(cp->getT());
	rotateAroundAxes(cp->getR());
	scaleAlongAxis(cp->getS());

	// Apply the deformation 
	deformMesh();

	// Apply deformation forever...
	if(isPermanent)
		originalBox = currBox;
}

void Cuboid::recoverMesh()
{
	currBox = originalBox;
	deformMesh();
}

double Cuboid::volume()
{
	return 8 * currBox.Extent.x() * currBox.Extent.y() * currBox.Extent.z();
}

std::vector <Vec3d> Cuboid::points()
{
	return getBoxConners(currBox);
}

Vec3d Cuboid::selectedPartPos()
{
	if(selectedPartId < 0) return Vec3d();

	Vec3d partPos(0,0,0);

	std::vector< std::vector<Vector3> > faces = getBoxFaces(currBox);
	std::vector<Vector3> face = faces[selectedPartId];

	for(int i = 0; i < face.size(); i++)
		partPos += face[i];

	return partPos / face.size();
}

void Cuboid::reshapePart( Vec3d q )
{
	if(selectedPartId < 0) return;

	debugPoints.clear();
	debugLines.clear();
	debugPoly.clear();

	std::vector< std::vector<Vector3> > faces = getBoxFaces(originalBox);
	std::vector< std::vector<Vector3> > newFaces = faces;

	// even / odd
	int j = selectedPartId;
	int k = j + 1;
	if(j % 2 != 0)	k = j - 1;

	// Get old faces vertices
	std::vector<Vector3> oldCurrFace = faces[j];
	std::vector<Vector3> oldOppositeFace = faces[k];
	std::vector<Vector3> tempFace(4);
	
	// Compute center of faces
	std::vector<Vec3d> faceCenter(faces.size());

	for(uint i = 0; i < faces.size(); i++){
		for(uint j = 0; j < faces[i].size(); j++)
			faceCenter[i] += faces[i][j];

		faceCenter[i] /= faces[i].size();
	}

	debugLines.push_back(std::make_pair(faceCenter[j], q));
	debugLines.push_back(std::make_pair(faceCenter[j], faceCenter[k]));

	// Compute rotation made
	Vec3d v1 = faceCenter[j] - faceCenter[k];
	Vec3d v2 = q - faceCenter[k];
	Vec3d axis = cross(v1,v2).normalized();

	Vec3d delta = faceCenter[k] - q;
	double theta = acos(RANGED(-1, dot(v1.normalized(),v2.normalized()), 1));

	// Rotate new face
	for(int i = 0; i < 4; i++){
		Vec3d v = ((faces[j][i] - faceCenter[j]));
		//v = v * cos(theta) + cross(axis, v) * sin(theta) + axis * dot(axis, v) * (1 - cos(theta));
		ROTATE_VEC(v, theta, axis);

		newFaces[j][i] = v + q;
		newFaces[k][i] = newFaces[j][i] + delta;

		tempFace[i] = newFaces[k][i] - (delta.normalized() * v1.norm());
	}

	Vec3d v3 = newFaces[j][1] - newFaces[j][0];
	Vec3d v4 = newFaces[j][2] - newFaces[j][1];

	int x = ((j / 2) + 1) % 3;
	int y = (x + 1) % 3;
	int z = (y + 1) % 3;

	// Modify box
	this->currBox.Extent[x] = v2.norm() * 0.5;
	this->currBox.Extent[y] = v3.norm() * 0.5;
	this->currBox.Extent[z] = v4.norm() * 0.5;

	ROTATE_VEC(currBox.Axis[x], theta, axis);
	ROTATE_VEC(currBox.Axis[y], theta, axis);
	ROTATE_VEC(currBox.Axis[z], theta, axis);
	
	this->currBox.Center = (v2 / 2.0) + faceCenter[k];

	this->originalBox = currBox;

	// Selected new part Id
	newFaces = getBoxFaces(originalBox);
	faceCenter.resize(newFaces.size(), Vec3d(0,0,0));
	double minDist = DBL_MAX;

	//printf("Old selected part = %d ", selectedPartId);

	for(uint i = 0; i < newFaces.size(); i++){
		for(uint j = 0; j < newFaces[i].size(); j++)
			faceCenter[i] += newFaces[i][j];

		faceCenter[i] /= faces[i].size();

		if((faceCenter[i] - q).norm() < minDist)
		{
			minDist = (faceCenter[i] - q).norm();
			selectedPartId = i;
		}
	}

	debugLines.push_back(std::make_pair(originalBox.Center, originalBox.Center + this->originalBox.Axis[x] * 0.5));

	//printf(" New selected part = %d \n", selectedPartId);

	//std::cout << this->currBox.Extent << std::endl;
	std::cout << "X = " << this->currBox.Axis[0]  << " Y = " << this->currBox.Axis[1] << " Z = " << this->currBox.Axis[2] << std::endl;


	this->deformMesh();
}

uint Cuboid::detectHotCurve( std::vector< Vec3d > &hotSamples )
{
	if ( dot( originalBox.Axis[2], cross( originalBox.Axis[0],  originalBox.Axis[1] ) ) < 0 )
		std::cout << "The coordinate frame is not right handed!" << std::endl;

	std::vector< std::vector< double > > projections(3);

	Vec3d &center = originalBox.Center;

	for (int i = 0; i < hotSamples.size(); i++)
	{
		Vec3d &vec = hotSamples[i] - center;

		for (int j = 0; j < 3; j++)
		{
			Vec3d &axis = originalBox.Axis[j];
			projections[j].push_back( dot( axis, vec ) );
		}

	}

	std::vector< double > range, mean;
	for (int j = 0; j < 3; j++)
	{
		double maxProj = MaxElement( projections[j] );
		double minProj = MinElement( projections[j] );

		range.push_back( maxProj - minProj );
		mean.push_back( (maxProj + minProj) / 2 );
	}


	uint axis = std::min_element( range.begin(), range.end() ) - range.begin();

	selectedPartId = 2 * axis;
	if (mean[axis] < 0) selectedPartId += 1;

	return selectedPartId;
}
