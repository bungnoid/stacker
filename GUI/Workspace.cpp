#include "global.h"
#include "Workspace.h"
#include <QVBoxLayout>
#include <QFileInfo>
#include <QFileDialog>
#include "Stacker/Controller.h"
#include <QTextStream>

Workspace::Workspace(QWidget *parent, Qt::WFlags flags)	: QMainWindow(parent, flags)
{
	activeScene = NULL;
	sceneCount = 0;

	ui.setupUi(this);

	QVBoxLayout * leftLayout = (QVBoxLayout *) ui.leftDockWidget->layout();
	QVBoxLayout * rightLayout = (QVBoxLayout *) ui.rightDockWidget->layout();

	// === Left dock
	// Stacker widget
	sp = new StackerPanel();
	leftLayout->addWidget(sp);


	// === Richt dock
	// Controller widget
	cp = new ControllerPanel();
	rightLayout->addWidget(cp);

	// Group widget
	gp = new GroupPanel();
	rightLayout->addWidget(gp);

	// Transformation widget
	tp = new TransformationPanel();
	rightLayout->addWidget(tp);

	// Deformer widget
	dp = new DeformerPanel();
	rightLayout->addWidget(dp);

	// Voxel Deformer widget
	vdp = new QVoxelDeformerPanel();
	rightLayout->addWidget(vdp);

	// Mesh info widget
	mi = new MeshInfoPanel();
	rightLayout->addWidget(mi);

	// Create mesh document manager
	mDoc = new QMeshDoc(this);

	// Add new scene action
	connect(ui.actionNewScene, SIGNAL(triggered()), SLOT(addNewScene()));

	// Connect to mesh management
	connect(ui.actionImportObject, SIGNAL(triggered()), mDoc, SLOT(importObject()));

	// Connect to mesh browser
	connect(ui.actionMeshBrowser, SIGNAL(triggered()), mDoc, SLOT(importObjectBrowser()));

	// Connect to Process User study
	connect(ui.actionUnserialize, SIGNAL(triggered()), SLOT(processUserStudyResults2()));

	leftLayout->addStretch();
	rightLayout->addStretch();

	// Among panels
	connect(sp, SIGNAL(objectModified()), gp, SLOT(updateWidget()));
	connect(cp, SIGNAL(controllerModified()), sp, SLOT(resetSolutionTree()));
	connect(gp, SIGNAL(groupsModified()), sp, SLOT(resetSolutionTree()));
	connect(cp, SIGNAL(objectModified()), sp, SLOT(updateActiveObject()));
	connect(tp, SIGNAL(objectModified()), sp, SLOT(updateActiveObject()));
}

Workspace::~Workspace()
{
	
}

void Workspace::addNewScene()
{
	Scene * newScene = new Scene(this);

	ui.sceneArea->addSubWindow(newScene)->show();
	sceneCount++;

	newScene->showMaximized();
	newScene->setWindowTitle("Untitled");

	newScene->sp = sp;

	// == CONNECTIONS ==

	// Focus changes in scene
	connect(newScene, SIGNAL(gotFocus(Scene*)), SLOT(setActiveScene(Scene*)));
	connect(newScene, SIGNAL(gotFocus(Scene*)), tp, SLOT(setActiveScene(Scene*)));
	connect(newScene, SIGNAL(gotFocus(Scene*)), mi, SLOT(setActiveScene(Scene*)));
	connect(newScene, SIGNAL(gotFocus(Scene*)), sp, SLOT(setActiveScene(Scene*)));
	connect(newScene, SIGNAL(gotFocus(Scene*)), cp, SLOT(setActiveScene(Scene*)));
	connect(newScene, SIGNAL(gotFocus(Scene*)), dp, SLOT(setActiveScene(Scene*)));
	connect(newScene, SIGNAL(gotFocus(Scene*)), vdp, SLOT(setActiveScene(Scene*)));
	connect(newScene, SIGNAL(gotFocus(Scene*)), gp, SLOT(setActiveScene(Scene*)));

	connect(newScene, SIGNAL(lostFocus(Scene*)), SLOT(disconnectScene(Scene*)));
	connect(newScene, SIGNAL(sceneClosed(Scene*)), SLOT(sceneClosed(Scene*)));
	connect(newScene, SIGNAL(sceneClosed(Scene*)), sp, SLOT(setActiveScene(Scene*)));

	// Objects changed in scene
	connect(newScene, SIGNAL(objectDiscarded(QString)), mDoc, SLOT(deleteObject(QString)));
	connect(newScene, SIGNAL(objectInserted()), sp, SLOT(setActiveObject()));
	connect(newScene, SIGNAL(objectInserted()), gp, SLOT(updateWidget()));

	// Stack panel
	connect(sp, SIGNAL(printMessage(QString)), newScene, SLOT(print(QString)));
	connect(sp, SIGNAL(objectModified()), newScene, SLOT(updateActiveObject()));

	// Controller
	connect(cp, SIGNAL(objectModified()), newScene, SLOT(updateActiveObject()));

	// Groups
	connect(newScene, SIGNAL(groupsChanged()), gp, SLOT(updateWidget()));
	connect(newScene, SIGNAL(groupsChanged()), sp, SLOT(resetSolutionTree()));

	// Object transformed by transformation panel
	connect(tp, SIGNAL(objectModified()), newScene, SLOT(updateActiveObject()));

	// Deformation
	connect(dp, SIGNAL(deformerCreated(QFFD *)), newScene, SLOT(setActiveDeformer(QFFD *)));
	connect(vdp, SIGNAL(deformerCreated(VoxelDeformer *)), newScene, SLOT(setActiveVoxelDeformer(VoxelDeformer *)));

	// == END ==

	setActiveScene(newScene);
}

void Workspace::setActiveScene(Scene* scene)
{
	QString title = QString("%1").arg(QFileInfo(QApplication::applicationFilePath()).baseName());
	
	activeScene = scene;

	if(activeScene)
	{
		// View operations
		connect(ui.actionCameraProjection, SIGNAL(triggered()), activeScene, SLOT(toggleCameraProjection()), Qt::UniqueConnection);
	
		// Connect mDoc
		activeScene->connect(ui.actionExportObject, SIGNAL(triggered()), SLOT(exportActiveObject()), Qt::UniqueConnection);
		activeScene->connect(mDoc, SIGNAL(objectImported(QSegMesh*)), SLOT(setActiveObject(QSegMesh*)), Qt::UniqueConnection);
		activeScene->connect(mDoc, SIGNAL(printMessage(QString)), SLOT(print(QString)), Qt::UniqueConnection);
		mDoc->connect(activeScene, SIGNAL(exportActiveObject(QSegMesh*)), SLOT(exportObject(QSegMesh*)), Qt::UniqueConnection);

		title += QString(" - %2").arg(scene->windowTitle());
	}

	// Set active scene
	tp->setActiveScene(activeScene);
	mi->setActiveScene(activeScene);
	sp->setActiveScene(activeScene);
	gp->setActiveScene(activeScene);
	cp->setActiveScene(activeScene);

	this->setWindowTitle(title);
}

void Workspace::disconnectScene(Scene* scene)
{
	mDoc->disconnect();
	ui.actionCameraProjection->disconnect();
}

void Workspace::sceneClosed( Scene* scene )
{
	int count = ui.sceneArea->subWindowList().size() - 1;

	if(count == 0)
	{
		setActiveScene(activeScene = NULL);
		printf("No scenes! %d\n", count);
	}
}

void Workspace::processUserStudyResults()
{
	QStringList xmlFileNames =  QFileDialog::getOpenFileNames(0, "Result file", "", "Results Files (*.xml)"); 

	foreach(QString xmlFileName, xmlFileNames)
	{
		QFileInfo fi(xmlFileName);

		// The current folder
		QString currFolder = fi.absoluteDir().path();

		// Submitter name
		// Clean up name
		QString submitName = fi.baseName();
		if(submitName.contains("_")){
			QStringList sl = submitName.split("_");
			submitName = sl.last();
		}
		QString submitFolder = currFolder + "/" + submitName + "/" ;

		// Find all obj.txt files
		QDir submitDir(submitFolder);
		QStringList filters;filters << "*.obj.txt";		
		foreach (QString itemName, submitDir.entryList(filters))
		{		
			// Create a new scene
			addNewScene();

			// Load the mesh
			QString meshName = itemName;
			meshName.chop(4);
			QString meshFilename = currFolder + "/tasks/" + meshName;
			mDoc->importObject(meshFilename);

			// Unserialize user's result
			QString resultFilename = submitFolder + itemName;
			QFile inF(resultFilename); 
			inF.open(QIODevice::ReadOnly | QIODevice::Text);
			QString content = inF.readAll();
			Controller * ctrl = (Controller * )activeScene->activeMesh->ptr["controller"];
			ctrl->unserialize(content);

			// update
			activeScene->updateActiveObject();
			sp->updateActiveObject();
		}		
	}

}

void Workspace::processUserStudyResults2()
{
	// Meshes
	QStringList meshNames;
	meshNames << "IKEA-cup.obj" << "chair-armrest.obj" << "chair.obj" << "cup-handle.obj" 
		<< "table-skirt.obj" << "table-middlebar.obj" << "stool.obj";

	// Select XML files
	QStringList xmlFileNames =  QFileDialog::getOpenFileNames(0, "Result file", "", "Results Files (*.xml)"); 

	// The folder names
	QFileInfo fi(xmlFileNames.front());
	QString parentFolder = fi.absoluteDir().path();
	QString outputFolder = parentFolder + "/distortion-stackability/";
	QString taskFolder = parentFolder + "/tasks/";
	QDir parentDir(parentFolder);
	parentDir.mkdir("distortion-stackability");

	// Load meshes
	QMap<QString, QSegMesh*> meshes;
	foreach (QString meshName, meshNames)
	{
		QString meshFilename = taskFolder + meshName;
		meshes[meshName] = mDoc->importObject(meshFilename);
	}

	// Open output files
	QMap<QString, QFile*> outFile;
	foreach (QString meshName, meshNames)
	{
		outFile[meshName] =  new QFile(outputFolder + meshName + ".csv");
		outFile[meshName]->open(QIODevice::WriteOnly | QIODevice::Text);
		outFile[meshName]->write("user,volume,BB_extent,stackability\n");
	}

	// For each user
	foreach(QString xmlFileName, xmlFileNames)
	{
		// Read xmlFile
		QFile xmlFile(xmlFileName); xmlFile.open(QIODevice::ReadOnly | QIODevice::Text);
		QString xmlString = xmlFile.readAll(); xmlFile.close();
		xmlString.replace("data/Tasks/","");
		QDomDocument doc; doc.setContent(xmlString);

		// Submitter name
		QFileInfo fi_foo(xmlFileName);
		QString submitName = fi_foo.baseName();
		if(submitName.contains("_")){
			QStringList sl = submitName.split("_");
			submitName = sl.last();
		}

		std::cout << qPrintable(submitName) << ": ";

		// For each shape
		QDomElement e = doc.firstChildElement().firstChildElement();
		while(!e.isNull())
		{
			if(e.tagName().endsWith(".obj"))
			{
				// Raw data
				QDomElement name = e.elementsByTagName("mesh-name").at(0).toElement();
				QDomElement stackability = e.elementsByTagName("stackability").at(0).toElement();
				QDomElement controller = e.elementsByTagName("controller").at(0).toElement();

				// Compute the distortion
				QString meshName = name.text();
				if (outFile.contains(meshName))
				{
					Controller * ctrl = (Controller *)meshes[meshName]->ptr["controller"];
					ctrl->unserialize(controller.text());
					std::vector<double> Ds = ctrl->getDistortions();

					QString line = submitName + QString(",%1,%2,%3\n").arg(Ds[0]).arg(Ds[1]).arg(stackability.text());
					outFile[meshName]->write(qPrintable(line));

					std::cout << ".";		
				}
			}

			e = e.nextSiblingElement();
		}

		std::cout << "Done.\n";

	}

	// Close files, delete meshes
	foreach (QString meshName, meshNames)
	{
		delete meshes[meshName];
		outFile[meshName]->close();
	}

	

}
