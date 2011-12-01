#include "Cuboid.h"
#include "SimpleDraw.h"

#include <Eigen/Geometry>
using namespace Eigen;

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

std::vector< std::vector<Vector3> > Cuboid::getBoxFaces()
{
	std::vector< std::vector<Vector3> > faces;
	std::vector<Vector3> pnts = getBoxConners(currBox);
	std::vector<Vector3> f0 (4), f1 (4), f2 (4), f3 (4), f4 (4), f5 (4);

	f0[0] = pnts[1]; f0[1] = pnts[0]; f0[2] = pnts[3]; f0[3] = pnts[2];faces.push_back(f0);
	f1[0] = pnts[4]; f1[1] = pnts[5]; f1[2] = pnts[6]; f1[3] = pnts[7];faces.push_back(f1);
	f2[0] = pnts[0]; f2[1] = pnts[1]; f2[2] = pnts[5]; f2[3] = pnts[4];faces.push_back(f2);
	f3[0] = pnts[2]; f3[1] = pnts[3]; f3[2] = pnts[7]; f3[3] = pnts[6];faces.push_back(f3);
	f4[0] = pnts[1]; f4[1] = pnts[2]; f4[2] = pnts[6]; f4[3] = pnts[5];faces.push_back(f4);
	f5[0] = pnts[0]; f5[1] = pnts[4]; f5[2] = pnts[7]; f5[3] = pnts[3];faces.push_back(f5);
	 
	return faces;
}

void Cuboid::draw()
{
	// Draw center point
	SimpleDraw::IdentifyPoint(currBox.Center);

	if(isSelected)
		drawCube(5, Vec4d(1,1,0,1));
	else
		drawCube(2, Vec4d(0,0,1,1));
}

void Cuboid::drawCube(double lineWidth, Vec4d color, bool isOpaque)
{
	std::vector< std::vector<Vector3> > faces = getBoxFaces();

	if(selectedPartId >= 0)
	{
		SimpleDraw::DrawSquare(faces[this->selectedPartId], false, 6, Vec4d(1,0,0,1));
		SimpleDraw::IdentifyPoint(selectedPartPos(), 0,1,0,20);
	}

	for(int i = 0; i < faces.size(); i++)
		SimpleDraw::DrawSquare(faces[i], isOpaque, lineWidth, color);
}

void Cuboid::drawNames(bool isDrawParts)
{
	if(isDrawParts)
	{
		int faceId = 0;

		std::vector< std::vector<Vector3> > faces = getBoxFaces();

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
	theta = 3.1415926 * theta / 180;
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

	std::vector< std::vector<Vector3> > faces = getBoxFaces();
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

	Vec3d currCenter = selectedPartPos(), oppositeCenter(0,0,0);
	
	std::vector< std::vector<Vector3> > faces = getBoxFaces();

	// even / odd
	int j = selectedPartId;
	int k = j + 1;
	if(j % 2 != 0)	k = j - 1;

	// Get old faces vertices
	std::vector<Vector3> oldCurrFace = faces[j];
	std::vector<Vector3> oldOppositeFace = faces[k];

	// Compute opposite face center
	for(int i = 0; i < oldOppositeFace.size(); i++) 
		oppositeCenter += oldOppositeFace[i];
	oppositeCenter = oppositeCenter / 4;

	debugLines.push_back(std::make_pair(currCenter, q));
	debugLines.push_back(std::make_pair(currCenter, oppositeCenter));

	// Compute rotation made
	Vec3d v1 = currCenter - oppositeCenter;
	Vec3d v2 = q - oppositeCenter;
	Vec3d axis = cross(v1,v2).normalized();

	Vec3d delta = oppositeCenter - q;
	double theta = acos(RANGED(-1, dot(v1.normalized(),v2.normalized()), 1));

	std::vector<Vector3> newCurrFace = faces[k];
	std::vector<Vector3> newOppositeFace(4);

	for(int i = 0; i < 4; i++)
	{
		// Rotate
		Vec3d v = ((newCurrFace[i] - currCenter));
		v = v * cos(theta) + cross(axis, v) * sin(theta) + axis * dot(axis, v) * (1 - cos(theta));

		newCurrFace[i] = v + q;
		newOppositeFace[i] = newCurrFace[i] + delta;
	}

	std::vector<Vector3> A,B;
	
	A.insert(A.begin(), oldCurrFace.begin(), oldCurrFace.end());
	A.insert(A.begin(), oldOppositeFace.begin(), oldOppositeFace.end());

	B.insert(B.begin(), newCurrFace.begin(), newCurrFace.end());
	B.insert(B.begin(), newOppositeFace.begin(), newOppositeFace.end());

	MatrixXd matA(3, 8), matB(3, 8);

	for(int i = 0; i < 8; i++){
		matA(0, i) = A[i].x();    
		matA(1, i) = A[i].y();    
		matA(2, i) = A[i].z();

		matB(0, i) = B[i].x();   
		matB(1, i) = B[i].y();   
		matB(2, i) = B[i].z();
	}

	Matrix4d tm = umeyama(matA, matB, true);
	Affine3d T(tm);
	
	Vector3d t = T.translation();

	Quaterniond r;
	AlignedScaling3d s; 
	T.computeScalingRotation(&s, &t);

	std::cout << "\n Trans:\n"<< t << std::endl;
	//std::cout << "\n Rot:\n" << r << std::endl;
	//std::cout << "\n Scale:\n" << s << std::endl;
}
