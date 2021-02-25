#include "glew.h"
#include "freeglut.h"
#include "glm.hpp"
#include "ext.hpp"
#include <iostream>
#include <cmath>
#include <vector>

#include <filesystem>
#include <assimp/Importer.hpp>
#include<assimp/scene.h>
#include<assimp/postprocess.h>

#include "Shader_Loader.h"
#include "Render_Utils.h"
#include "Camera.h"
#include "Texture.h"
#include "Physics.h"
#define STB_IMAGE_IMPLEMENTATION
#include "SOIL/stb_image_aug.h"

//------------------------------------------------------------------
// contact pairs filtering function
static PxFilterFlags simulationFilterShader(PxFilterObjectAttributes attributes0,
	PxFilterData filterData0, PxFilterObjectAttributes attributes1, PxFilterData filterData1,
	PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize)
{
	pairFlags =
		PxPairFlag::eCONTACT_DEFAULT | // default contact processing
		PxPairFlag::eNOTIFY_CONTACT_POINTS | // contact points will be available in onContact callback
		PxPairFlag::eNOTIFY_TOUCH_FOUND; // onContact callback will be called for this pair
		//PxPairFlag::eNOTIFY_TOUCH_PERSISTS;

	return physx::PxFilterFlag::eDEFAULT;
}

//------------------------------------------------------------------
// simulation events processor
class SimulationEventCallback : public PxSimulationEventCallback
{
public:
	void onContact(const PxContactPairHeader& pairHeader,
		const PxContactPair* pairs, PxU32 nbPairs)
	{
		const PxU32 bufferSize = 64;
		PxContactPairPoint contacts[bufferSize];
		//std::cout << "number of contact pairs: " << nbPairs;

		for (PxU32 i = 0; i < nbPairs; i++)
		{
			const PxContactPair& cp = pairs[i];
			//std::cout << "pair " << i;


			if (("asteroid" == pairHeader.actors[0]->getName()) ||
				(pairHeader.actors[1]->getName() == "asteroid"))
			{
				PxU32 nbContacts = pairs[i].extractContacts(contacts, bufferSize);
				//std::cout << "number of contacts: " << nbContacts << '\n';


				for (PxU32 j = 0; j < nbContacts; j++)
				{
					PxVec3 point = contacts[j].position;
					std::cout << "asteroid collision" << '\n';
					//std::cout << "x: " << contacts[j].position.x << " y: " << contacts[j].position.y << " z: " << contacts[j].position.z << '\n';

				}
			}
			//if (("ship" == pairHeader.actors[0]->getName()) ||
			//	(pairHeader.actors[1]->getName() == "ship"))
			//{
			//	PxU32 nbContacts = pairs[i].extractContacts(contacts, bufferSize);
			//	//std::cout << "number of contacts: " << nbContacts << '\n';


			//	for (PxU32 j = 0; j < nbContacts; j++)
			//	{
			//		PxVec3 point = contacts[j].position;
			//		std::cout << "ship collision" << '\n';
			//		std::cout << "x: " << contacts[j].position.x << " y: " << contacts[j].position.y << " z: " << contacts[j].position.z << '\n';

			//	}
			//}
			if (("bullet" == pairHeader.actors[0]->getName()) ||
				(pairHeader.actors[1]->getName() == "bullet"))
			{
				PxU32 nbContacts = pairs[i].extractContacts(contacts, bufferSize);
				//std::cout << "number of contacts: " << nbContacts << '\n';


				for (PxU32 j = 0; j < nbContacts; j++)
				{
					PxVec3 point = contacts[j].position;
					std::cout << "bullet collision" << '\n';
					//std::cout << "x: " << contacts[j].position.x << " y: " << contacts[j].position.y << " z: " << contacts[j].position.z << '\n';

				}
			}
		}
	}

	virtual void onConstraintBreak(PxConstraintInfo* constraints, PxU32 count) {}
	virtual void onWake(PxActor** actors, PxU32 count) {}
	virtual void onSleep(PxActor** actors, PxU32 count) {}
	virtual void onTrigger(PxTriggerPair* pairs, PxU32 count) {}
	virtual void onAdvance(const PxRigidBody* const* bodyBuffer, const PxTransform* poseBuffer, const PxU32 count) {}
};

// Initalization of physical scene (PhysX)
SimulationEventCallback simulationEventCallback;
Physics pxScene(0, simulationFilterShader, &simulationEventCallback);


// fixed timestep for stable and deterministic simulation
const double physicsStepTime = 1.f / 60.f;
double physicsTimeToProcess = 0;

// programs
GLuint programColor;
GLuint programTexture;
GLuint programSkybox;
GLuint programSun;
GLuint programParallax;

Core::Shader_Loader shaderLoader;

// physx
obj::Model planeModel, boxModel, sphereModel, shipModel, bulletModel;
Core::RenderContext planeContext, boxContext, sphereContext, shipContext, bulletContext;
GLuint boxTexture, groundTexture, shipTexture, textureBullet;

// physical objects
PxRigidDynamic* sphereBody;
PxRigidDynamic* shipBody;
PxRigidStatic* asteroidBody;
PxMaterial* shipMaterial = nullptr;
std::vector<PxRigidDynamic*> boxBodies;
PxMaterial* boxMaterial = nullptr;
PxMaterial* sphereMaterial = nullptr;

// renderable objects (description of a single renderable instance)
struct Renderable {
	Core::RenderContext* context;
	glm::mat4 modelMatrix;
	GLuint textureId;
};
std::vector<Renderable*> renderables;

GLuint textureShip;
GLuint textureShipNormal;
GLuint textureShipDepth;

GLuint textureAsteroid;
GLuint textureAsteroidNormal;
static const int NUM_ASTEROIDS = 10;
PxVec3 asteroidPositions[NUM_ASTEROIDS];

GLuint textureSun;
glm::vec3 sunPosition = glm::vec3(0);
glm::mat4 sunModelMatrix;

unsigned int cubemapTexture;
unsigned int skyboxVAO, skyboxVBO;

glm::mat4 shipModelMatrix;
glm::mat4 shipInitialTransformation;
// glm::vec3 lightPos = glm::vec3(0, 0, 0);


// camera + keyboard + mouse
glm::mat4 cameraMatrix, perspectiveMatrix;
float lastX = 300.0f, lastY = 300.0f;
float yaw = 0.0f;
float pitch = 0.0f;
glm::quat rotation = glm::quat(1, 0, 0, 0);
glm::vec3 cameraPos = glm::vec3(-5, 0, 0);
glm::vec3 cameraDir; // Wektor "do przodu" kamery
glm::vec3 cameraSide; // Wektor "w bok" kamery
float cameraAngle = 0;


void initRenderables() {
	// ASTEROIDS
	for (int i = 0; i < NUM_ASTEROIDS; i++)
	{
		Renderable* asteroid = new Renderable();
		asteroid->context = &sphereContext;
		asteroid->textureId = textureAsteroid;
		renderables.emplace_back(asteroid);
	}

	// SUN
	Renderable* sun = new Renderable();
	sun->context = &sphereContext;
	sun->textureId = textureSun;
	renderables.emplace_back(sun);

	// SHIP
	Renderable* ship = new Renderable();
	ship->context = &shipContext;
	ship->textureId = textureShip;
	renderables.emplace_back(ship);
}


void initPhysicsScene()
{
	boxMaterial = pxScene.physics->createMaterial(0.5, 0.5, 0.6);
	int x = 0;

	// ASTEROIDS
	for (int i = 0; i < NUM_ASTEROIDS; i++)
	{
		glm::vec3 a = glm::vec3(glm::ballRand(10.0));
		asteroidPositions[i] = PxVec3(a.x, a.y, a.z);
	}

	for (int i = 0; i < NUM_ASTEROIDS; i++)
	{
		PxShape* sphereShape;
		//sphereBody = pxScene.physics->createRigidDynamic(PxTransform(asteroidPositions[i]));
		asteroidBody = pxScene.physics->createRigidStatic(PxTransform(asteroidPositions[i]));
		sphereShape = pxScene.physics->createShape(PxSphereGeometry(1), *boxMaterial);
		asteroidBody->attachShape(*sphereShape);
		asteroidBody->setName("asteroid");
		sphereShape->release();
		asteroidBody->userData = (void*)renderables[i];
		pxScene.scene->addActor(*asteroidBody);
		x += 1;
	}

	//// SUN 
	//PxShape* sphereShape;
	//sphereBody = pxScene.physics->createRigidDynamic(PxTransform(PxVec3(0)));
	//sphereShape = pxScene.physics->createShape(PxSphereGeometry(1), *boxMaterial);
	//sphereBody->attachShape(*sphereShape);
	//sphereBody->setName("sun");
	//sphereShape->release();
	//sphereBody->userData = (void*)renderables[x];
	//pxScene.scene->addActor(*sphereBody);

	//// SHIP
	//glm::mat4 shipModelMatrix = glm::translate(cameraPos + cameraDir * 0.5f) * glm::mat4_cast(glm::inverse(rotation)) * glm::translate(glm::vec3(0, -0.25f, 0)) * glm::rotate(glm::radians(180.0f), glm::vec3(0, 1, 0) * rotation);
	//PxShape* sphereShape;
	//shipBody = pxScene.physics->createRigidDynamic(PxTransform(PxMat44((float*)&shipModelMatrix)));
	//sphereShape = pxScene.physics->createShape(PxSphereGeometry(1), *boxMaterial);
	//shipBody->attachShape(*sphereShape);
	//shipBody->setName("ship");
	//sphereShape->release();
	//shipBody->userData = (void*)renderables[x];
	//pxScene.scene->addActor(*shipBody);
	//x += 1;

}

void initBullets()
{
	for (int i = 0; i < 2; i++) {
		Renderable* sphere = new Renderable();
		sphere->context = &bulletContext;
		sphere->textureId = textureBullet;
		renderables.emplace_back(sphere);
	}
}

void initRightBulletPhysicsScene(GLuint i) {
	boxMaterial = pxScene.physics->createMaterial(0.5, 0.5, 0.6);
	//glm::mat4 shipModelMatrix = glm::translate(cameraPos + cameraDir * 0.5f) * glm::mat4_cast(glm::inverse(rotation)) * glm::translate(glm::vec3(0, -0.25f, 0))  * glm::rotate(glm::radians(180.0f), glm::vec3(0, 1, 0) * rotation);
	glm::mat4 rightBulletModelMatrix = glm::translate(cameraPos + cameraDir * 0.5f) * glm::mat4_cast(glm::inverse(rotation)) * glm::translate(glm::vec3(0.15f, -0.25f, -0.1f)) * glm::rotate(glm::radians(180.0f), glm::vec3(0, 1, 0) * rotation);
	PxShape* sphereShape;
	sphereBody = pxScene.physics->createRigidDynamic(PxTransform(PxMat44((float*)&rightBulletModelMatrix)));
	sphereShape = pxScene.physics->createShape(PxSphereGeometry(0.05), *boxMaterial);
	sphereBody->setLinearVelocity(PxVec3(15 * cameraDir.x, 15 * cameraDir.y, 15 * cameraDir.z));
	sphereBody->attachShape(*sphereShape);
	sphereBody->setName("bullet");
	sphereShape->release();
	sphereBody->userData = (void*)renderables[i];
	pxScene.scene->addActor(*sphereBody);
}

void initLeftBulletPhysicsScene(GLuint i) {
	boxMaterial = pxScene.physics->createMaterial(0.5, 0.5, 0.6);
	glm::mat4 leftBulletModelMatrix = glm::translate(cameraPos + cameraDir * 0.5f) * glm::mat4_cast(glm::inverse(rotation)) * glm::translate(glm::vec3(-0.15f, -0.25f, -0.1f)) * glm::rotate(glm::radians(180.0f), glm::vec3(0, 1, 0) * rotation);
	PxShape* sphereShape;
	sphereBody = pxScene.physics->createRigidDynamic(PxTransform(PxMat44((float*)&leftBulletModelMatrix)));
	sphereShape = pxScene.physics->createShape(PxSphereGeometry(0.05), *boxMaterial);
	sphereBody->setLinearVelocity(PxVec3(15 * cameraDir.x, 15 * cameraDir.y, 15 * cameraDir.z));
	sphereBody->attachShape(*sphereShape);
	sphereBody->setName("bullet");
	sphereShape->release();
	sphereBody->userData = (void*)renderables[i];
	pxScene.scene->addActor(*sphereBody);
}

void shoot() {
	initBullets();
	int count = renderables.size();
	initRightBulletPhysicsScene(count - 1);
	initLeftBulletPhysicsScene(count - 2);
}

void updateTransforms(glm::mat4 shipMatrix)
{
	// Here we retrieve the current transforms of the objects from the physical simulation.
	auto actorFlags = PxActorTypeFlag::eRIGID_DYNAMIC | PxActorTypeFlag::eRIGID_STATIC;
	PxU32 nbActors = pxScene.scene->getNbActors(actorFlags);
	if (nbActors)
	{
		std::vector<PxRigidActor*> actors(nbActors);
		pxScene.scene->getActors(actorFlags, (PxActor**)&actors[0], nbActors);
		for (auto actor : actors)
		{

			// We use the userData of the objects to set up the model matrices
			// of proper renderables.
			if (!actor->userData) continue;
			Renderable* renderable = (Renderable*)actor->userData;
			PxMat44 transform;

			if (renderable->textureId == textureShip) {
				transform = PxMat44((float*)&shipMatrix);
			}

			else {
				// get world matrix of the object (actor)
				transform = actor->getGlobalPose();
			}
			auto& c0 = transform.column0;
			auto& c1 = transform.column1;
			auto& c2 = transform.column2;
			auto& c3 = transform.column3;

			// set up the model matrix used for the rendering
			renderable->modelMatrix = glm::mat4(
				c0.x, c0.y, c0.z, c0.w,
				c1.x, c1.y, c1.z, c1.w,
				c2.x, c2.y, c2.z, c2.w,
				c3.x, c3.y, c3.z, c3.w);
			
		}
	}
}

void keyboard(unsigned char key, int x, int y)
{
	float angleSpeed = 0.1f;
	float moveSpeed = 0.1f;
	switch (key)
	{
	case 'z': rotation = glm::angleAxis(-angleSpeed, glm::vec3(0, 0, 1)) * rotation; break;
	case 'x': rotation = glm::angleAxis(angleSpeed, glm::vec3(0, 0, 1)) * rotation; break;
	case 'w': cameraPos += cameraDir * moveSpeed; break;
	case 's': cameraPos -= cameraDir * moveSpeed; break;
	case 'd': cameraPos += cameraSide * moveSpeed; break;
	case 'a': cameraPos -= cameraSide * moveSpeed; break;
	case 'p': shoot(); break;
	}
}

void mouse(int x, int y)
{
	float xoffset = x - lastX;
	float yoffset = y - lastY;
	lastX = (float)x;
	lastY = (float)y;

	const float sen = 0.00001f;
	xoffset *= sen;
	yoffset *= sen;

	yaw += xoffset;
	pitch += yoffset;
}


glm::mat4 createCameraMatrix()
{
	glm::quat rotationAboutX = glm::angleAxis(pitch, glm::vec3(1, 0, 0));
	glm::quat rotationAboutY = glm::angleAxis(yaw, glm::vec3(0, 1, 0));
	glm::quat rotationChange = rotationAboutX * rotationAboutY;
	rotation = glm::normalize(rotationChange * rotation);

	cameraDir = glm::inverse(rotation) * glm::vec3(0, 0, -1);
	glm::vec3 up = glm::vec3(0, 1, 0);
	cameraSide = glm::cross(cameraDir, up);
	return Core::createViewMatrixQuat(cameraPos, rotation);
}

void drawObjectTexture(Core::RenderContext* context, glm::mat4 modelMatrix, GLuint textureId, glm::vec3 lightDir, GLuint normalmapId)
{
	GLuint program = programTexture;

	glUseProgram(program);

	glUniform3f(glGetUniformLocation(program, "lightDir"), lightDir.x, lightDir.y, lightDir.z);
	glUniform3f(glGetUniformLocation(program, "cameraPos"), cameraPos.x, cameraPos.y, cameraPos.z);

	Core::SetActiveTexture(textureId, "textureSampler", program, 1);
	Core::SetActiveTexture(normalmapId, "normalSampler", program, 0);

	//glUniform3f(glGetUniformLocation(program, "lightPos"), lightPos.x, lightPos.y, lightPos.z);

	glm::mat4 transformation = perspectiveMatrix * cameraMatrix * modelMatrix;
	glUniformMatrix4fv(glGetUniformLocation(program, "modelViewProjectionMatrix"), 1, GL_FALSE, (float*)&transformation);
	glUniformMatrix4fv(glGetUniformLocation(program, "modelMatrix"), 1, GL_FALSE, (float*)&modelMatrix);

	Core::DrawContext(*context);

	glUseProgram(0);
}

void drawObjectParallax(Core::RenderContext* context, glm::mat4 modelMatrix, GLuint textureId, glm::vec3 lightDir, GLuint normalmapId, GLuint depthmapId)
{
	GLuint program = programParallax;

	glUseProgram(program);

	glUniform3f(glGetUniformLocation(program, "lightDir"), lightDir.x, lightDir.y, lightDir.z);
	glUniform3f(glGetUniformLocation(program, "cameraPos"), cameraPos.x, cameraPos.y, cameraPos.z);

	Core::SetActiveTexture(textureId, "textureSampler", program, 0);
	Core::SetActiveTexture(normalmapId, "normalSampler", program, 1);
	Core::SetActiveTexture(depthmapId, "depthSampler", program, 2);
	//glUniform3f(glGetUniformLocation(program, "lightPos"), lightPos.x, lightPos.y, lightPos.z);

	glm::mat4 transformation = perspectiveMatrix * cameraMatrix * modelMatrix;
	glUniformMatrix4fv(glGetUniformLocation(program, "modelViewProjectionMatrix"), 1, GL_FALSE, (float*)&transformation);
	glUniformMatrix4fv(glGetUniformLocation(program, "modelMatrix"), 1, GL_FALSE, (float*)&modelMatrix);

	Core::DrawContext(*context);

	glUseProgram(0);
}

void drawSun(Core::RenderContext* context, glm::mat4 modelMatrix, GLuint textureId, glm::vec3 lightDir)
{
	GLuint program = programSun;

	glUseProgram(program);

	Core::SetActiveTexture(textureId, "textureSampler", program, 0);
	glUniform3f(glGetUniformLocation(program, "cameraPos"), cameraPos.x, cameraPos.y, cameraPos.z);

	glUniform3f(glGetUniformLocation(program, "lightDir"), lightDir.x, lightDir.y, lightDir.z);

	glm::mat4 transformation = perspectiveMatrix * cameraMatrix * modelMatrix;
	glUniformMatrix4fv(glGetUniformLocation(program, "transformation"), 1, GL_FALSE, (float*)&transformation);
	glUniformMatrix4fv(glGetUniformLocation(program, "modelMatrix"), 1, GL_FALSE, (float*)&modelMatrix);

	Core::DrawContext(*context);

	glUseProgram(0);
}

void drawSkybox(unsigned int cubemapTexture)
{

	//skybox
	glUseProgram(programSkybox);
	glDepthFunc(GL_LEQUAL);
	glm::mat4 transformation = perspectiveMatrix * glm::mat4(glm::mat3(cameraMatrix));
	glUniformMatrix4fv(glGetUniformLocation(programSkybox, "modelViewProjectionMatrix"), 1, GL_FALSE, (float*)&transformation);
	// skybox cube
	glBindVertexArray(skyboxVAO);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
	glDepthFunc(GL_LESS); // set depth function back to default

	glClear(GL_DEPTH_BUFFER_BIT);

	glUseProgram(0);
}


void renderScene()
{
	double time = glutGet(GLUT_ELAPSED_TIME) / 1000.0;
	static double prevTime = time;
	double dtime = time - prevTime;
	prevTime = time;

	if (dtime < 1.f) {
		physicsTimeToProcess += dtime;
		while (physicsTimeToProcess > 0) {
			pxScene.step(physicsStepTime);
			physicsTimeToProcess -= physicsStepTime;
		}
	}

	cameraMatrix = createCameraMatrix();
	perspectiveMatrix = Core::createPerspectiveMatrix();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glClearColor(0.0f, 0.1f, 0.3f, 1.0f);

	drawSkybox(cubemapTexture);

	glm::vec3 lightPos = glm::vec3(0, 0, 0);
	glm::vec3 lightDir = glm::normalize(cameraPos - lightPos);

	shipInitialTransformation = glm::translate(glm::vec3(0, -0.25f, 0)) * glm::rotate(glm::radians(180.0f), glm::vec3(0, 1, 0)) * glm::scale(glm::vec3(0.05f));
	shipModelMatrix = glm::translate(cameraPos + cameraDir * 0.5f) * glm::mat4_cast(glm::inverse(rotation)) * shipInitialTransformation;


	updateTransforms(shipModelMatrix);

	for (Renderable* renderable : renderables) {
		if (renderable->textureId == textureAsteroid) {
			drawObjectTexture(renderable->context, renderable->modelMatrix, renderable->textureId, lightDir, textureAsteroidNormal);
		}
		else if (renderable->textureId == textureBullet) {
			drawSun(renderable->context, renderable->modelMatrix, renderable->textureId, lightDir);
		}
		else if (renderable->textureId == textureSun) {
			drawSun(renderable->context, renderable->modelMatrix, renderable->textureId, lightDir);
		}
		else if (renderable->textureId == textureShip) {
			drawObjectParallax(renderable->context, shipModelMatrix, renderable->textureId, lightDir, textureShipNormal, textureShipDepth);
		}
	}

	glutSwapBuffers();
}

void initSkybox() {

	GLuint program = programSkybox;
	float skyboxVertices[] = {
		// positions
		-1.0f,  1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		-1.0f,  1.0f, -1.0f,
		 1.0f,  1.0f, -1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		 1.0f, -1.0f,  1.0f
	};

	Core::SetActiveTexture(cubemapTexture, "cubemap", program, 0);

	// skybox VAO
	glGenVertexArrays(1, &skyboxVAO);
	glBindVertexArray(skyboxVAO);
	glGenBuffers(1, &skyboxVBO);
	glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
}

void init()
{
	srand(time(0));
	glEnable(GL_DEPTH_TEST);
	programColor = shaderLoader.CreateProgram("shaders/shader_color.vert", "shaders/shader_color.frag");
	programTexture = shaderLoader.CreateProgram("shaders/shader_tex.vert", "shaders/shader_tex.frag");
	programSkybox = shaderLoader.CreateProgram("shaders/shader_sky.vert", "shaders/shader_sky.frag");
	programSun = shaderLoader.CreateProgram("shaders/shader_4_sun.vert", "shaders/shader_4_sun.frag");
	programParallax = shaderLoader.CreateProgram("shaders/shader_tex_parallax.vert", "shaders/shader_tex_parallax.frag");

	sphereModel = obj::loadModelFromFile("models/sphere.obj");
	shipModel = obj::loadModelFromFile("models/spaceship.obj");
	bulletModel = obj::loadModelFromFile("models/bullet.obj");

	
	sphereContext.initFromOBJ(sphereModel);
	shipContext.initFromOBJ(shipModel);
	bulletContext.initFromOBJ(bulletModel);

	textureAsteroid = Core::LoadTexture("textures/asteroid.png");
	textureShip = Core::LoadTexture("textures/ship.png");
	textureSun = Core::LoadTexture("textures/sun.png");
	textureBullet = Core::LoadTexture("textures/green.jpg");

	textureShipNormal = Core::LoadTexture("textures/ship_normals.png");
	textureShipDepth = Core::LoadTexture("textures/ship_deplacement.png");

	textureAsteroidNormal = Core::LoadTexture("textures/asteroid_normals.png");

	textureSun = Core::LoadTexture("textures/sun.png");

	initRenderables();
	initPhysicsScene();


	std::vector<std::string> faces
	{
		"textures/skybox/right.png",
		"textures/skybox/left.png",
		"textures/skybox/top.png",
		"textures/skybox/bottom.png",
		"textures/skybox/front.png",
		"textures/skybox/back.png",
	};
	//loadCubemap
	unsigned int id;
	glGenTextures(1, &id);
	glBindTexture(GL_TEXTURE_CUBE_MAP, id);

	int width, height, nrChannels;
	unsigned char* data;
	for (unsigned int i = 0; i < faces.size(); i++)
	{
		unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
		if (data)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
			stbi_image_free(data);
		}
		else
		{
			std::cout << "Cubemap tex failed to load at path: " << faces[i] << std::endl;
			stbi_image_free(data);
		}
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	cubemapTexture = id;
	//std::cout << cubemapTexture << '\n';


	initSkybox();
}

void shutdown()
{
	shaderLoader.DeleteProgram(programColor);
	shaderLoader.DeleteProgram(programTexture);
	shaderLoader.DeleteProgram(programSkybox);
	shaderLoader.DeleteProgram(programSun);
	shaderLoader.DeleteProgram(programParallax);
}

void idle()
{
	glutPostRedisplay();
}

int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(200, 200);
	glutInitWindowSize(600, 600);
	glutCreateWindow("S P A C E   C L A S H");
	glewInit();

	init();
	glutKeyboardFunc(keyboard);
	glutPassiveMotionFunc(mouse);
	glutDisplayFunc(renderScene);
	glutIdleFunc(idle);

	glutMainLoop();

	shutdown();

	return 0;
}
