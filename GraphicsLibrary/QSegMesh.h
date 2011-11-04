#pragma once
#include <QObject>
#include <QString>
#include "QSurfaceMesh.h"
#include <vector>

class QSegMesh : public QObject
{
	Q_OBJECT

public:
	QSegMesh();
	QSegMesh(const QSegMesh& from);
	QSegMesh& operator=(const QSegMesh& rhs);

	// Face, vertex
	uint n_faces();
	std::vector<uint> vertexIndicesAroundFace(uint fid);
	Point getVertexPos( uint vid );
	void setVertexColor( uint vid, const Color& newColor );

	// Get segment
	QSurfaceMesh* operator [] (int i);
	QSurfaceMesh* getSegment(int i);
	int nbSegments();

	// Draw
	void simpleDraw();
	void drawFacesUnique();

	// Load the mesh from file
	void read(QString fileName);

	// Build up the mesh
	void build_up();
	void moveCenterToOrigin();
	void computeBoundingBox();
	void setColorVertices();
	void update_face_normals();
	void update_vertex_normals();

	// Properties
	bool isReady;
	Point bbmin, bbmax, center;
	Scalar radius;

	// Set global unique name for QSegMesh and all its segments
	void setObjectName(const QString &name);

private:
	std::vector<QSurfaceMesh*> segment;
	void global2local_fid(uint fid, int& sid, uint& fid_local);
	void global2local_vid(uint vid, int& sid, uint& vid_local);
};


